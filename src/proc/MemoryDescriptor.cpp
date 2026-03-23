#include "MemoryDescriptor.h"
#include "Kernel.h"
#include "PageManager.h"
#include "Machine.h"
#include "PageDirectory.h"
#include "Video.h"

void MemoryDescriptor::Initialize()
{
	KernelPageManager& kernelPageManager = Kernel::Instance().GetKernelPageManager();
	
	/* m_UserPageTableArray��Ҫ��AllocMemory()���ص������ڴ��ַ + 0xC0000000 */
	this->m_UserPageTableArray = (PageTable*)(kernelPageManager.AllocMemory(sizeof(PageTable) * USER_SPACE_PAGE_TABLE_CNT) + Machine::KERNEL_SPACE_START_ADDRESS);
}

void MemoryDescriptor::Release()
{
	KernelPageManager& kernelPageManager = Kernel::Instance().GetKernelPageManager();
	if ( this->m_UserPageTableArray )
	{
		kernelPageManager.FreeMemory(sizeof(PageTable) * USER_SPACE_PAGE_TABLE_CNT, (unsigned long)this->m_UserPageTableArray - Machine::KERNEL_SPACE_START_ADDRESS);
		this->m_UserPageTableArray = NULL;
	}
}

unsigned int MemoryDescriptor::MapEntry(unsigned long virtualAddress, unsigned int size, unsigned long phyPageIdx, bool isReadWrite)
{
	unsigned long address = virtualAddress - USER_SPACE_START_ADDRESS;

	unsigned long startIdx = address >> 12;
	unsigned long cnt = ( size + (PageManager::PAGE_SIZE - 1) )/ PageManager::PAGE_SIZE;

	PageTableEntry* entrys = (PageTableEntry*)this->m_UserPageTableArray;
	for ( unsigned int i = startIdx; i < startIdx + cnt; i++, phyPageIdx++ )
	{
		entrys[i].m_Present = 0x1;
		entrys[i].m_ReadWriter = isReadWrite;
		entrys[i].m_UserSupervisor = 1;		/* U bit: 用户态可访问 */
		entrys[i].m_PageBaseAddress = phyPageIdx;
	}
	return phyPageIdx;
}

void MemoryDescriptor::MapTextEntrys(unsigned long textStartAddress, unsigned long textSize, unsigned long textPageIdx)
{
	this->MapEntry(textStartAddress, textSize, textPageIdx, false);
}
void MemoryDescriptor::MapDataEntrys(unsigned long dataStartAddress, unsigned long dataSize, unsigned long dataPageIdx)
{
	this->MapEntry(dataStartAddress, dataSize, dataPageIdx, true);
}

void MemoryDescriptor::MapStackEntrys(unsigned long stackSize, unsigned long stackPageIdx)
{
	unsigned long stackStartAddress = (USER_SPACE_START_ADDRESS + USER_SPACE_SIZE - stackSize) & 0xFFFFF000;
	this->MapEntry(stackStartAddress, stackSize, stackPageIdx, true);
}

PageTable* MemoryDescriptor::GetUserPageTableArray()
{
	return this->m_UserPageTableArray;
}
unsigned long MemoryDescriptor::GetTextStartAddress()
{
	return this->m_TextStartAddress;
}
unsigned long MemoryDescriptor::GetTextSize()
{
	return this->m_TextSize;
}
unsigned long MemoryDescriptor::GetDataStartAddress()
{
	return this->m_DataStartAddress;
}
unsigned long MemoryDescriptor::GetDataSize()
{
	return this->m_DataSize;
}
unsigned long MemoryDescriptor::GetStackSize()
{
	return this->m_StackSize;
}

bool MemoryDescriptor::EstablishUserPageTable( unsigned long textVirtualAddress, unsigned long textSize, unsigned long dataVirtualAddress, unsigned long dataSize, unsigned long stackSize )
{
	User& u = Kernel::Instance().GetUser();

	/* ��������������û��������8M�ĵ�ַ�ռ����� */
	if ( textSize + dataSize + stackSize  + PageManager::PAGE_SIZE > USER_SPACE_SIZE - textVirtualAddress)
	{
		u.u_error = User::ENOMEM;
		Diagnose::Write("u.u_error = %d\n",u.u_error);
		return false;
	}

	this->ClearUserPageTable();

	/*
	 * 离散化：各段直接使用绝对物理帧号存入页表，不再存储相对偏移量。
	 * MapToPageTable() 只需将这些帧号原样拷贝到系统页表，无需再加基址偏移。
	 */

	/* 文本段：帧号 = x_caddr / PAGE_SIZE，即文本在物理内存中的实际起始帧 */
	unsigned int textBaseFrame = 0;
	if ( u.u_procp->p_textp != NULL )
	{
		textBaseFrame = u.u_procp->p_textp->x_caddr >> 12;
	}
	this->MapEntry(textVirtualAddress, textSize, textBaseFrame, false);

	/*
	 * 数据段：帧号从 (p_addr >> 12) + 1 开始。
	 * p_addr 的第 0 帧是 PPDA（用户结构体），不映射到用户虚拟地址空间；
	 * 数据页从第 1 帧起依次向后排列。
	 */
	unsigned int dataBaseFrame = (u.u_procp->p_addr >> 12) + 1;
	unsigned int nextFrame = this->MapEntry(dataVirtualAddress, dataSize, dataBaseFrame, true);

	/* 栈段：紧接数据页之后，映射到用户虚拟地址空间高端（靠近 0x800000） */
	unsigned long stackStartAddress = (USER_SPACE_START_ADDRESS + USER_SPACE_SIZE - stackSize) & 0xFFFFF000;
	this->MapEntry(stackStartAddress, stackSize, nextFrame, true);

	/* 将绝对帧号写入系统页表，使新建地址映射关系立即生效 */
	this->MapToPageTable();
	return true;
}

void MemoryDescriptor::ClearUserPageTable()
{
	User& u = Kernel::Instance().GetUser();
	PageTable* pUserPageTable = u.u_MemoryDescriptor.m_UserPageTableArray;

	unsigned int i ;
	unsigned int j ;

	for (i = 0; i < Machine::USER_PAGE_TABLE_CNT; i++)
	{
		for (j = 0; j < PageTable::ENTRY_CNT_PER_PAGETABLE; j++ )
		{
			pUserPageTable[i].m_Entrys[j].m_Present = 0;
			pUserPageTable[i].m_Entrys[j].m_ReadWriter = 0;
			pUserPageTable[i].m_Entrys[j].m_UserSupervisor = 1;
			pUserPageTable[i].m_Entrys[j].m_PageBaseAddress = 0;
		}
	}

}

void MemoryDescriptor::RebaseDataFrames(long delta)
{
	if ( this->m_UserPageTableArray == NULL || delta == 0 )
		return;

	/*
	 * 离散化辅助：进程的 p_addr 发生变化（Expand 重新分配了更大的物理块）时，
	 * 将页表中所有可读写（数据段/栈段）页的帧号整体平移 delta。
	 * 只调整 RW 页（数据/栈），RO 页（文本段，帧号来自共享 x_caddr）不受影响。
	 */
	PageTableEntry* entrys = (PageTableEntry*)this->m_UserPageTableArray;
	unsigned int total = Machine::USER_PAGE_TABLE_CNT * PageTable::ENTRY_CNT_PER_PAGETABLE;
	for ( unsigned int i = 0; i < total; i++ )
	{
		if ( entrys[i].m_Present && entrys[i].m_ReadWriter )
		{
			entrys[i].m_PageBaseAddress = (unsigned int)((long)entrys[i].m_PageBaseAddress + delta);
		}
	}
}

void MemoryDescriptor::DisplayPageTable()
{
	unsigned int i,j;

	Diagnose::Write("Process PT:");
	for (i = 0; i < Machine::USER_PAGE_TABLE_CNT; i++)
		for ( j = 0; j < PageTable::ENTRY_CNT_PER_PAGETABLE; j++)
			if ( 1 == this->m_UserPageTableArray[i].m_Entrys[j].m_Present )
				Diagnose::Write("<%d,%x>  ",i*1024+j,this->m_UserPageTableArray[i].m_Entrys[j].m_PageBaseAddress);
	Diagnose::Write("\n");

	Diagnose::Write("<PPDA,%x>  ",Machine::Instance().GetKernelPageTable().m_Entrys[1023].m_PageBaseAddress);

	PageTable* pUserPageTable = Machine::Instance().GetUserPageTableArray();
	Diagnose::Write("System PT:");
	for (i = 0; i < Machine::USER_PAGE_TABLE_CNT; i++)
		for ( j = 0; j < PageTable::ENTRY_CNT_PER_PAGETABLE; j++)
			if ( 1 == pUserPageTable[i].m_Entrys[j].m_Present )
				Diagnose::Write("<%d,%x>  ",i*1024+j,pUserPageTable[i].m_Entrys[j].m_PageBaseAddress);
	Diagnose::Write("\n");
}

void MemoryDescriptor::MapToPageTable()
{
	User& u = Kernel::Instance().GetUser();

	if(u.u_MemoryDescriptor.m_UserPageTableArray == NULL)
		return;

	/*
	 * 离散化后，m_UserPageTableArray 中已存储绝对物理帧号，
	 * 直接拷贝到系统页表即可，不再需要加 textPF / pAddrPF 偏移。
	 * 同时为用户态页设置 U bit（m_UserSupervisor=1），
	 * 确保用户态代码能访问自身的虚拟地址空间。
	 */
	PageTable* pUserPageTable = Machine::Instance().GetUserPageTableArray();

	for (unsigned int i = 0; i < Machine::USER_PAGE_TABLE_CNT; i++)
	{
		for ( unsigned int j = 0; j < PageTable::ENTRY_CNT_PER_PAGETABLE; j++ )
		{
			pUserPageTable[i].m_Entrys[j].m_Present = 0;

			if ( 1 == this->m_UserPageTableArray[i].m_Entrys[j].m_Present )
			{
				/* 将进程页表中的绝对帧号原样映射到系统页表，并置 U bit */
				pUserPageTable[i].m_Entrys[j].m_Present       = 1;
				pUserPageTable[i].m_Entrys[j].m_ReadWriter     = this->m_UserPageTableArray[i].m_Entrys[j].m_ReadWriter;
				pUserPageTable[i].m_Entrys[j].m_UserSupervisor = 1;   /* U bit: 允许用户态访问 */
				pUserPageTable[i].m_Entrys[j].m_PageBaseAddress = this->m_UserPageTableArray[i].m_Entrys[j].m_PageBaseAddress;
			}
		}
	}

	/* 虚拟地址 0x0 映射到物理帧 0，供运行时/信号处理存根使用 */
	pUserPageTable[0].m_Entrys[0].m_Present        = 1;
	pUserPageTable[0].m_Entrys[0].m_ReadWriter      = 1;
	pUserPageTable[0].m_Entrys[0].m_UserSupervisor  = 1;
	pUserPageTable[0].m_Entrys[0].m_PageBaseAddress = 0;

	/* 修改 PTE 后必须刷新 TLB，使新映射关系立即生效 */
	FlushPageDirectory();
}


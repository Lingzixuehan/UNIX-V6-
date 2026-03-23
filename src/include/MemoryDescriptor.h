#ifndef MEMORY_DESCRIPTOR_H
#define MEMORY_DESCRIPTOR_H

#include "PageTable.h"

class MemoryDescriptor
{
public:
	/* �û��ռ��С 8M 0x0 - 0x800000 2 PageTable */
	static const unsigned int USER_SPACE_SIZE	= 0x800000; 
	static const unsigned int USER_SPACE_PAGE_TABLE_CNT = 0x2;
	static const unsigned long USER_SPACE_START_ADDRESS		= 0x0;



public:
	MemoryDescriptor();
	~MemoryDescriptor();

public:
	/* ���벢��ʼ��PageDirectory������Map����ǰʹ�� */
	void Initialize();
	/* ���ͷŽ���ʱ����Ҫ���øò����ͷű�ռ�õ�ҳ�� */
	void Release();

	/* ���º����û���ɶ�user�ṹ��ҳ��Entry����䣬��ҳ���ڽ����л�ʱ������е�ҳ�� */
	void MapTextEntrys(unsigned long textStartAddress, unsigned long textSize, unsigned long textPageIdxInPhyMemory);
	void MapDataEntrys(unsigned long dataStartAddress, unsigned long dataSize, unsigned long dataPageIdxInPhyMemory);
	void MapStackEntrys(unsigned long stackSize, unsigned long stackPageIdxInPhyMemory);

	/* @comment ԭunixv6��sureg()����.ԭ�������ڽ�����u���е�uisa��uisd�������е��ڴ�ҳӳ������ӳ�䵽UISA��UISD
	 * �Ĵ�����.������ϵ�ṹ�Ĺ�ϵ��ʹ��MapToPageTable()������MemoryDescriptor�е�ҳ��copy������ϵͳ��ʹ�õ�
	 * PageTable�У�Ȼ��ʹ��FlushPageDirectory()�������ҳ��ӳ�䣬����̨���̵��û�������ӳ����� */
	void MapToPageTable();
	void DisplayPageTable();

	/*
	 * @comment 离散化辅助函数：当进程物理内存基地址从 oldAddr 移动到 newAddr 时，
	 * 将 m_UserPageTableArray 中所有可读写(RW)页表项的物理帧号加上 delta，
	 * 以反映数据/栈页在新位置的帧号。文本(RO)页不受影响（文本段共享，帧号不变）。
	 * delta = (newAddr - oldAddr) / PAGE_SIZE
	 */
	void RebaseDataFrames(long delta);

	/* 
	 * @comment ԭunix v6��estabur()���������ڽ����û�̬��ַ�ռ����Ե�ַӳ�����Ȼ�����
	 * MapToPageTable()��������Ե�ַӳ������ص��û�̬ҳ���С�
	 */
	bool EstablishUserPageTable(unsigned long textVirtualAddress, unsigned long textSize, unsigned long dataVirtualAddress, unsigned long dataSize, unsigned long stackSize);
	void ClearUserPageTable();
	PageTable* GetUserPageTableArray();
	unsigned long GetTextStartAddress();
	unsigned long GetTextSize();
	unsigned long GetDataStartAddress();
	unsigned long GetDataSize();
	unsigned long GetStackSize();

private:
	/* @comment����ҳ��Ŀ¼��
	 * @param
	 * unsigned long virtualAddress:	�����ַ(���ֽ�Ϊ��λ) 
	 * unsigned int size:				��Ҫӳ��������ַ��С(���ֽ�Ϊ��λ) 
	 * unsigned long phyPageIdx:		��ʵ����ҳ������(ҳΪ��λ)		
	 * bool isReadWrite:				ҳ���ԣ�trueΪ�ɶ���дҳ
	 */
	unsigned int MemoryDescriptor::MapEntry(unsigned long virtualAddress, unsigned int size, unsigned long phyPageIdx, bool isReadWrite);
	
public:
	PageTable*		m_UserPageTableArray;
	/* �������ݶ������Ե�ַ */
	unsigned long	m_TextStartAddress;	/* �������ʼ��ַ */
	unsigned long	m_TextSize;			/* ����γ��� */

	unsigned long	m_DataStartAddress; /* ���ݶ���ʼ��ַ */
	unsigned long	m_DataSize;			/* ���ݶγ��� */

	unsigned long	m_StackSize;		/* ջ�γ��� */
	//unsigned long	m_HeapSize;			/* �Ѷγ��� */
};

#endif


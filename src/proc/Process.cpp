#include "Process.h"
#include "ProcessManager.h"
#include "Kernel.h"
#include "Utility.h"
#include "Machine.h"
#include "Video.h"


Process::Process()
{
	/* ��ʶ����p_statΪSNULL����ʶ�ý��������ʹ�� */
	this->p_stat = SNULL;
	/* ����0#������Wait()ʱ���������process����0#����Ϊ������ */
	this->p_ppid = -1;
}

Process::~Process()
{
}


void Process::SetRun()
{
	ProcessManager& procMgr = Kernel::Instance().GetProcessManager();

	/* ���˯��ԭ��תΪ����״̬ */
	this->p_wchan = 0;
	this->p_stat = Process::SRUN;
	if ( this->p_pri < procMgr.CurPri )
	{
		procMgr.RunRun++;
	}
	if ( 0 != procMgr.RunOut && (this->p_flag & Process::SLOAD) == 0 )
	{
		procMgr.RunOut = 0;
		procMgr.WakeUpAll((unsigned long)&procMgr.RunOut);
	}
}

void Process::SetPri()
{
	int priority;
	ProcessManager& procMgr = Kernel::Instance().GetProcessManager();

	priority = this->p_cpu / 16;
	priority += ProcessManager::PUSER + this->p_nice;

	if ( priority > 255 )
	{
		priority = 255;
	}
	if ( priority > procMgr.CurPri )
	{
		procMgr.RunRun++;
	}
	this->p_pri = priority;
}

bool Process::IsSleepOn(unsigned long chan)
{
	/* ��鵱ǰ����˯��ԭ���Ƿ�Ϊchan */
	if( this->p_wchan == chan 
		&& (this->p_stat == Process::SWAIT || this->p_stat == Process::SSLEEP) )
	{
		return true;
	}
	return false;
}

void Process::Sleep(unsigned long chan, int pri)
{
	User& u = Kernel::Instance().GetUser();
	ProcessManager& procMgr = Kernel::Instance().GetProcessManager();

	if ( pri > 0 )
	{
		/* 
		 * �����ڽ��������Ȩ˯��֮ǰ���Լ�������֮��������յ����ɺ���
		 * ���źţ���ִֹͣ��Sleep()��ͨ��aRetU()ֱ����ת��Trap1()����
		 */
		if ( this->IsSig() )
		{
			/* returnȷ��aRetU()���ص�SystemCall::Trap1()֮������ִ��ret����ָ�� */
			aRetU(u.u_qsav);
			return;
		}
		/* 
		* �˴����жϽ����ٽ�������֤����������˯��ԭ��chan��
		* �Ľ���״̬ΪSSLEEP֮�䲻�ᷢ���л���
		*/
		X86Assembly::CLI();
		this->p_wchan = chan;
		/* ����˯�����ȼ�priȷ�����̽���ߡ�������Ȩ˯�� */
		this->p_stat = Process::SWAIT;
		this->p_pri = pri;
		X86Assembly::STI();

		if ( procMgr.RunIn != 0 )
		{
			procMgr.RunIn = 0;
			procMgr.WakeUpAll((unsigned long)&procMgr.RunIn);
		}
		/* ��ǰ���̷���CPU���л�����������̨ */
		//Diagnose::Write("Process %d Start Sleep!\n", this->p_pid);
		Kernel::Instance().GetProcessManager().Swtch();
		//Diagnose::Write("Process %d End Sleep!\n", this->p_pid);
		/* ������֮���ٴμ���ź� */
		if ( this->IsSig() )
		{
			/* returnȷ��aRetU()���ص�SystemCall::Trap1()֮������ִ��ret����ָ�� */
			aRetU(u.u_qsav);
			return;
		}
	}
	else
	{
		X86Assembly::CLI();
		this->p_wchan = chan;
		/* ����˯�����ȼ�priȷ�����̽���ߡ�������Ȩ˯�� */
		this->p_stat = Process::SSLEEP;
		this->p_pri = pri;
		X86Assembly::STI();

		/* ��ǰ���̷���CPU���л�����������̨ */
		//Diagnose::Write("Process %d Start Sleep!\n", this->p_pid);
		Kernel::Instance().GetProcessManager().Swtch();
		//Diagnose::Write("Process %d End Sleep!\n", this->p_pid);
	}
}

void Process::Expand(unsigned int newSize)
{
	UserPageManager& userPgMgr = Kernel::Instance().GetUserPageManager();
	ProcessManager& procMgr = Kernel::Instance().GetProcessManager();
	User& u = Kernel::Instance().GetUser();
	Process* pProcess = u.u_procp;

	unsigned int oldSize = pProcess->p_size;
	p_size = newSize;
	unsigned long oldAddress = pProcess->p_addr;
	unsigned long newAddress;

	/* �������ͼ����С�����ͷŶ�����ڴ� */
	if ( oldSize >= newSize )
	{
		if(oldSize > newSize)
			userPgMgr.FreeMemory(oldSize - newSize, oldAddress + newSize);
		return;
	}

	/* ����ͼ��������ҪѰ��һ���СnewSize�������ڴ��� */
	SaveU(u.u_rsav);
	newAddress = userPgMgr.AllocMemory(newSize);
	/* �����ڴ�ʧ�ܣ���������ʱ�������������� */
	if ( NULL == newAddress )
	{
		SaveU(u.u_ssav);
		procMgr.XSwap(pProcess, true, oldSize);
		pProcess->p_flag |= Process::SSWAP;
		procMgr.Swtch();
		/* no return */
	}
	/* 分配成功：将进程镜像拷贝到新内存，然后切换到新内存，并更新页表映射 */
	pProcess->p_addr = newAddress;

	/*
	 * 离散化：p_addr 已指向新物理块，先更新进程页表中数据/栈页的帧号，
	 * 再执行 CopySeg，最后 MapToPageTable 时帧号已是正确的新位置。
	 */
	long frameDelta = ((long)newAddress - (long)oldAddress) >> 12;
	u.u_MemoryDescriptor.RebaseDataFrames(frameDelta);

	for ( unsigned int i = 0; i < oldSize; i++ )
	{
		Utility::CopySeg(oldAddress + i, newAddress + i);
	}

	/* 释放原来占用的内存块 */
	userPgMgr.FreeMemory(oldSize, oldAddress);

	X86Assembly::CLI();
	SwtchUStruct(pProcess);
	RetU();
	X86Assembly::STI();

	u.u_MemoryDescriptor.MapToPageTable();
}

void Process::Exit()
{
	int i;
	User& u = Kernel::Instance().GetUser();
	ProcessManager& procMgr = Kernel::Instance().GetProcessManager();
	OpenFileTable& fileTable = *Kernel::Instance().GetFileManager().m_OpenFileTable;
	InodeTable& inodeTable = *Kernel::Instance().GetFileManager().m_InodeTable;

	Diagnose::Write("Process %d is exiting\n",u.u_procp->p_pid);
	/* Reset Tracing flag */
	u.u_procp->p_flag &= (~Process::STRC);

	/* ������̵��źŴ�������������Ϊ1��ʾ���Ը��ź����κδ��� */
	for ( i = 0; i < User::NSIG; i++ )
	{
		u.u_signal[i] = 1;
	}

	/* �رս��̴��ļ� */
	for ( i = 0; i < OpenFiles::NOFILES; i++ )
	{
		File* pFile = NULL;
		if ( (pFile = u.u_ofiles.GetF(i)) != NULL )
		{
			fileTable.CloseF(pFile);
			u.u_ofiles.SetF(i, NULL);
		}
	}
	/*  ���ʲ����ڵ�fd�����error code�����u.u_error����Ӱ���������ִ������ */
	u.u_error = User::NOERROR;

	/* �ݼ���ǰĿ¼�����ü��� */
	inodeTable.IPut(u.u_cdir);

	/* �ͷŸý��̶Թ������Ķε����� */
	if ( u.u_procp->p_textp != NULL )
	{
		u.u_procp->p_textp->XFree();
		u.u_procp->p_textp = NULL;
	}

	/* ��u��д�뽻�������ȴ����������ƺ��� */
	SwapperManager& swapperMgr = Kernel::Instance().GetSwapperManager();
	BufferManager& bufMgr = Kernel::Instance().GetBufferManager();
	/* u���Ĵ�С���ᳬ��512�ֽڣ�����ֻд��ppda����ǰ512�ֽڣ�������u�ṹ��ȫ����Ϣ */
	int blkno = swapperMgr.AllocSwap(BufferManager::BUFFER_SIZE);
	if ( NULL == blkno )
	{
		Utility::Panic("Out of Swapper Space");
	}
	Buf* pBuf = bufMgr.GetBlk(DeviceManager::ROOTDEV, blkno);
	Utility::DWordCopy((int *)&u, (int *)pBuf->b_addr, BufferManager::BUFFER_SIZE / sizeof(int));
	bufMgr.Bwrite(pBuf);

	/* �ͷ��ڴ���Դ */
	u.u_MemoryDescriptor.Release();
	Process* current = u.u_procp;
	UserPageManager& userPageMgr = Kernel::Instance().GetUserPageManager();
	userPageMgr.FreeMemory(current->p_size, current->p_addr);
	current->p_addr = blkno;
	current->p_stat = Process::SZOMB;

	/* ���Ѹ����̽����ƺ��� */
	for ( i = 0; i < ProcessManager::NPROC; i++ )
	{
		if ( procMgr.process[i].p_pid == current->p_ppid )
		{
			procMgr.WakeUpAll((unsigned long)&procMgr.process[i]);
			break;
		}
	}
	/* û�ҵ������� */
	if ( ProcessManager::NPROC == i )
	{
		current->p_ppid = 1;
		procMgr.WakeUpAll((unsigned long)&procMgr.process[1]);
	}

	/* ���Լ����ӽ��̴����Լ��ĸ����� */
	for ( i = 0; i < ProcessManager::NPROC; i++ )
	{
		if ( current->p_pid == procMgr.process[i].p_ppid )
		{
			Diagnose::Write("My:%d 's child %d passed to 1#process",current->p_pid,procMgr.process[i].p_pid);
			procMgr.process[i].p_ppid = 1;
			if ( procMgr.process[i].p_stat == Process::SSTOP )
			{
				procMgr.process[i].SetRun();
			}
		}
	}

	procMgr.Swtch();
}

void Process::Clone(Process& proc)
{
	User& u = Kernel::Instance().GetUser();

	/* ����������Process�ṹ�еĴ󲿷����� */
	proc.p_size = this->p_size;
	proc.p_stat = Process::SRUN;
	proc.p_flag = Process::SLOAD;
	proc.p_uid = this->p_uid;
	proc.p_ttyp = this->p_ttyp;
	proc.p_nice = this->p_nice;
	proc.p_textp = this->p_textp;
	
	/* �������ӹ�ϵ */
	proc.p_pid = ProcessManager::NextUniquePid();
	proc.p_ppid = this->p_pid;
	
	/* ��ʼ�����̵�����س�Ա */
	proc.p_pri = 0;		/* ȷ��child����������С��������������ȸ��л���ռ��CPU */
	proc.p_time = 0;
	

	/* ���ļ����ƿ�File�ṹ���ü���+1 */
	for ( int i = 0; i < OpenFiles::NOFILES; i++ )
	{
		File* pFile;
		if ( (pFile = u.u_ofiles.GetF(i)) != NULL )
		{
			pFile->f_count++;
		}
	}
	/* 
	 * GetF()����u.u_ofiles�еĿ��������������룬
	 * �粻��������½��̴���(fork)ϵͳ����ʧ�ܡ�
	 */
	u.u_error = User::NOERROR;

	/* ���ӶԹ������Ķε����ü��� */
	if ( proc.p_textp != 0 )
	{
		proc.p_textp->x_count++;
		proc.p_textp->x_ccount++;
	}

	/* ���ӶԵ�ǰ����Ŀ¼�����ü��� */
	u.u_cdir->i_count++;
}

//���ڶ�ջ���ʱ���Զ���չ��ջ
void Process::SStack()
{
	User& u = Kernel::Instance().GetUser();
	MemoryDescriptor& md = u.u_MemoryDescriptor;
	unsigned int change = 4096;
	//unsigned int change = 0;
	md.m_StackSize += change;
	unsigned int newSize = ProcessManager::USIZE + md.m_DataSize + md.m_StackSize;

	if ( false == u.u_MemoryDescriptor.EstablishUserPageTable(md.m_TextStartAddress,
						md.m_TextSize, md.m_DataStartAddress, md.m_DataSize, md.m_StackSize) )
	{
		u.u_error = User::ENOMEM;
		return;
	}

	this->Expand(newSize);
	int dst = u.u_procp->p_addr + newSize;
	unsigned int count = md.m_StackSize - change;
	while(count--)
	{
		dst--;
		Utility::CopySeg(dst - change, dst);
	}

	u.u_MemoryDescriptor.MapToPageTable();
}


void Process::SBreak()
{
	User& u = Kernel::Instance().GetUser();
	unsigned int newEnd = u.u_arg[0];
	MemoryDescriptor& md = u.u_MemoryDescriptor;
	unsigned int newSize = newEnd - md.m_DataStartAddress;

	if (newEnd == 0)
	{
		u.u_ar0[User::EAX] = md.m_DataStartAddress + md.m_DataSize;
		return;
	}

	if ( false == u.u_MemoryDescriptor.EstablishUserPageTable(md.m_TextStartAddress, 
						md.m_TextSize, md.m_DataStartAddress, newSize, md.m_StackSize) )
	{
		//ϵͳ���ó���ʱ�������������ַ�ʽ���ء�ִ������·���ᵼ�� u.u_intflg == 1��u.u_error�������޸�ΪEINTR��4�������ۺιʵ���ϵͳ����ʧ�ܡ�
		//aRetU(u.u_qsav);
		return;
	}

	int change = newSize - md.m_DataSize;
	md.m_DataSize = newSize;
	newSize += ProcessManager::USIZE + md.m_StackSize;

	/* ���ݶ���С */
	if ( change < 0 )
	{
		int dst = u.u_procp->p_addr + newSize - md.m_StackSize;
		int count = md.m_StackSize;
		while(count--)
		{
			Utility::CopySeg(dst - change, dst);
			dst++;
		}
		this->Expand(newSize);
	}
	/* ���ݶ����� */
	else if ( change > 0 )
	{
		this->Expand(newSize);
		int dst = u.u_procp->p_addr + newSize;
		int count = md.m_StackSize;
		while(count--)
		{
			dst--;
			Utility::CopySeg(dst - change, dst);
		}
	}
	u.u_ar0[User::EAX] = md.m_DataStartAddress + md.m_DataSize;
}

void Process::PSignal( int signal )
{
	if ( signal >= User::NSIG )
	{
		return;
	}

	/* ����Ѿ����յ�SIGKILL�źţ�����Ժ����ź� */
	if ( this->p_sig != User::SIGKILL )
	{
		this->p_sig = signal;
	}
	/* �����̵�����������PUSER(100)����������ΪPUSER */
	if ( this->p_pri > ProcessManager::PUSER )
	{
		this->p_pri	= ProcessManager::PUSER;
	}
	/* �����̵Ĵ��ڵ�����Ȩ˯�ߣ����份�� */
	if ( this->p_stat == Process::SWAIT )
	{
		this->SetRun();
	}
}

int Process::IsSig()
{
	User& u = Kernel::Instance().GetUser();

	/* δ���ܵ��ź� */
	if ( this->p_sig == 0 )
	{
		return 0;
	}
	/* u.u_signal[n]Ϊż���ű�ʾ���źŽ��̴��� */
	else if ( (u.u_signal[this->p_sig] & 1) == 0 )
	{
		return this->p_sig;
	}
	return 0;
}

/*
extern "C" void runtime();
extern "C" void SignalHandler();
*/

void Process::PSig(struct pt_context* pContext)
{
	User& u = Kernel::Instance().GetUser();
	int signal = this->p_sig;
	/* ����ѽ��봦�����̵��ź� */
	this->p_sig = 0;

	if ( u.u_signal[signal] != 0 )
	{
		/* ����������յ��ź�֮ǰִ��ϵͳ�����ڼ���ܲ�����ErrCode */
		u.u_error = User::NOERROR;

		unsigned int old_eip = pContext->eip;

		/* ����̬����ֵΪԤ�����û�����SignalHandler()���׵�ַ */
		/*pContext->eip = ((unsigned long)SignalHandler - (unsigned long)runtime);
		pContext->esp -= 8;
		int* pInt = (int *)pContext->esp;
		*pInt = u.u_signal[signal];
		*(pInt + 1) = old_eip;*/
		pContext->eip = u.u_signal[signal];
		pContext->esp -= 4;
		int* pInt = (int *)pContext->esp;
		*pInt = old_eip;

		/* 
		 * ��ǰ�źŴ�����������Ӧ�걾���ź�֮����Ҫ����ΪĬ��
		 * ���źŴ�����������Ϊ0��ʾ���źŵĴ�����ʽΪ��ֹ�����̡�
		 */
		u.u_signal[signal] = 0;
		return;
	}

	/* u.u_signal[n]Ϊ0������źŵĴ�����ʽ����ֹ������ */
	u.u_procp->Exit();
}

void Process::Nice()
{
	User& u = Kernel::Instance().GetUser();
	int niceValue = u.u_arg[0];

	if (niceValue > 20)
	{
		niceValue = 20;
	}
	if (niceValue < 0 && !u.SUser())
	{
		/* ��ϵͳ�����û�����Ϊ��������С��0�Ľ�������������ƫ��ֵ */
		niceValue = 0;
	}
	this->p_nice = niceValue;
}

void Process::Ssig()
{
	User& u = Kernel::Instance().GetUser();

	int signalIndex = u.u_arg[0];
	unsigned long func = u.u_arg[1];

	/* �⼸���źŲ������� */
	if ( signalIndex <= 0 || signalIndex >= User::NSIG || signalIndex == User::SIGKILL )
	{
		u.u_error = User::EINVAL;
		return;
	}
	/* ���ú�����ַ���źŴ����������� */
	u.u_ar0[User::EAX] = u.u_signal[signalIndex];
	u.u_signal[signalIndex] = func;
	/* �嵱ǰ�ź� */
	if ( u.u_procp->p_sig == signalIndex )
	{
		u.u_procp->p_sig = 0;
	}
}






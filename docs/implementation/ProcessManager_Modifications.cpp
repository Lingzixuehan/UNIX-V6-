/*
 * ====================================================================
 * ProcessManager修改示例
 * ====================================================================
 *
 * 本文件展示如何修改ProcessManager以支持ESP定位User结构
 * 主要修改点：
 * 1. 进程创建时分配8KB对齐的内核栈区域
 * 2. 初始化User结构和栈指针
 * 3. 进程切换时无需修改页表
 */

#include "ProcessManager.h"
#include "Kernel.h"
#include "Utility.h"

/*
 * ====================================================================
 * 修改1：内存分配器增加对齐分配功能
 * ====================================================================
 */

/*
 * 分配指定大小和对齐要求的内存
 *
 * @param size: 需要分配的内存大小（字节）
 * @param alignment: 对齐要求（必须是2的幂，如0x2000表示8KB对齐）
 * @return: 分配的内存起始地址（对齐后），失败返回0
 *
 * 算法说明：
 * 1. 分配比请求更大的内存（size + alignment）
 * 2. 在分配的内存中找到满足对齐要求的地址
 * 3. 释放不需要的头部和尾部内存
 */
unsigned long UserPageManager::AllocateAlignedMemory(unsigned long size, unsigned long alignment)
{
    // 验证对齐参数（必须是2的幂）
    if (alignment == 0 || (alignment & (alignment - 1)) != 0)
    {
        Diagnose::Write("ERROR: Invalid alignment parameter: ");
        Diagnose::WriteHex(alignment);
        Diagnose::Write("\n");
        return 0;
    }

    // 计算需要分配的总大小（考虑对齐）
    // 最坏情况：需要额外的alignment-PAGE_SIZE字节来满足对齐
    unsigned long total_size = size + alignment - PageManager::PAGE_SIZE;
    unsigned long num_pages = (total_size + PageManager::PAGE_SIZE - 1) / PageManager::PAGE_SIZE;

    // 分配内存
    unsigned long addr = this->AllocMemory(num_pages);
    if (addr == 0)
    {
        // 内存分配失败
        return 0;
    }

    // 计算对齐后的地址
    // 公式：(addr + alignment - 1) & ~(alignment - 1)
    unsigned long aligned_addr = (addr + alignment - 1) & ~(alignment - 1);

    // 释放对齐地址之前的未使用页面
    if (aligned_addr > addr)
    {
        unsigned long unused_front = aligned_addr - addr;
        unsigned long unused_pages = unused_front / PageManager::PAGE_SIZE;

        if (unused_pages > 0)
        {
            this->FreeMemory(addr, unused_pages);
        }
    }

    // 释放对齐区域之后的未使用页面
    unsigned long aligned_end = aligned_addr + size;
    unsigned long total_end = addr + num_pages * PageManager::PAGE_SIZE;

    if (total_end > aligned_end)
    {
        unsigned long unused_back = total_end - aligned_end;
        unsigned long unused_pages = unused_back / PageManager::PAGE_SIZE;

        if (unused_pages > 0)
        {
            this->FreeMemory(aligned_end, unused_pages);
        }
    }

    // 验证对齐
    if ((aligned_addr & (alignment - 1)) != 0)
    {
        Diagnose::Write("ERROR: Alignment failed! Address: ");
        Diagnose::WriteHex(aligned_addr);
        Diagnose::Write("\n");
        Panic("AllocateAlignedMemory: alignment check failed");
    }

    return aligned_addr;
}

/*
 * ====================================================================
 * 修改2：NewProc()函数 - 使用8KB对齐分配
 * ====================================================================
 */

/*
 * 创建新进程（ESP定位版本）
 *
 * 关键修改：
 * 1. 使用AllocateAlignedMemory分配8KB对齐的内核栈区域
 * 2. User结构位于栈区域起始位置
 * 3. 初始化ESP指向栈底（栈区域的高地址）
 */
int ProcessManager::NewProc_ESP()
{
    User& u = Kernel::Instance().GetUser();
    Process* parent = u.u_procp;

    // 1. 查找空闲的Process表项
    Process* child = nullptr;
    for (int i = 0; i < ProcessManager::NPROC; i++)
    {
        if (process[i].p_stat == Process::SNULL)
        {
            child = &process[i];
            break;
        }
    }

    if (child == nullptr)
    {
        // 进程表已满
        u.u_error = User::EAGAIN;
        return -1;
    }

    // 2. 克隆父进程信息到子进程
    parent->Clone(*child);

    // 3. 保存当前进程的栈指针（用于fork返回）
    SaveU(u.u_rsav);

    // 4. 为子进程分配8KB对齐的内核栈区域
    unsigned long kernel_stack = userPageManager.AllocateAlignedMemory(
        Kernel::KERNEL_STACK_SIZE,  // 8KB
        Kernel::KERNEL_STACK_SIZE   // 8KB对齐
    );

    if (kernel_stack == 0)
    {
        // 内存分配失败，需要交换
        Diagnose::Write("NewProc: Memory allocation failed, swapping...\n");

        // 将子进程标记为等待交换
        parent->p_stat = Process::SIDL;
        child->p_addr = parent->p_addr;  // 临时使用父进程地址
        SaveU(u.u_ssav);

        // 执行交换
        this->XSwap(child, false, 0);
        child->p_flag |= Process::SSWAP;

        parent->p_stat = Process::SRUN;
    }
    else
    {
        // 内存分配成功

        // 5. 设置子进程的内核栈区域地址
        child->p_addr = kernel_stack;

        // 6. 初始化子进程的User结构（位于栈区域起始位置）
        User* child_user = (User*)kernel_stack;

        // 复制父进程的User结构到子进程
        Utility::MemCopy((char*)&u, (char*)child_user, sizeof(User));

        // 7. 初始化子进程的栈指针
        // ESP指向栈底（栈区域的高地址），栈向低地址增长
        unsigned long initial_esp = kernel_stack + Kernel::KERNEL_STACK_SIZE;
        unsigned long initial_ebp = initial_esp;

        child_user->u_rsav[0] = initial_esp;
        child_user->u_rsav[1] = initial_ebp;
        child_user->u_ssav[0] = initial_esp;
        child_user->u_ssav[1] = initial_ebp;
        child_user->u_qsav[0] = initial_esp;
        child_user->u_qsav[1] = initial_ebp;

        // 8. 设置子进程指针
        child_user->u_procp = child;

        // 9. 分配并复制用户态内存
        unsigned long user_memory = userPageManager.AllocMemory(parent->p_size);
        if (user_memory == 0)
        {
            // 用户内存分配失败，释放内核栈
            userPageManager.FreeMemory(kernel_stack,
                Kernel::KERNEL_STACK_SIZE / PageManager::PAGE_SIZE);

            child->p_stat = Process::SNULL;
            u.u_error = User::EAGAIN;
            return -1;
        }

        // 复制用户态内存
        unsigned long src = parent->p_MemoryDescriptor.GetUserBase();
        unsigned long dst = user_memory;
        int pages = parent->p_size;

        while (pages-- > 0)
        {
            Utility::CopySeg(src, dst);
            src += PageManager::PAGE_SIZE;
            dst += PageManager::PAGE_SIZE;
        }

        // 10. 设置子进程的用户态内存基址
        child->p_MemoryDescriptor.SetUserBase(user_memory);
    }

    // 11. 恢复父进程的User结构指针
    u.u_procp = parent;

    return 0;  // 返回0表示这是子进程
               // 父进程通过Swtch()返回1
}

/*
 * ====================================================================
 * 修改3：Swtch()函数 - 移除页表切换
 * ====================================================================
 */

/*
 * 进程切换（ESP定位版本）
 *
 * 关键修改：
 * 1. 移除SwtchUStruct()调用（不再需要修改页表）
 * 2. 简化进程切换流程
 * 3. 性能提升：每次切换节省约100+时钟周期
 */
int ProcessManager::Swtch_ESP()
{
    User& u = Kernel::Instance().GetUser();
    Process* current = u.u_procp;

    // 1. 保存当前进程的栈指针
    SaveU(u.u_rsav);

    // 2. 切换到进程0（idle进程）
    Process* proc_zero = &process[0];

    X86Assembly::CLI();  // 禁中断

    // 注意：移除了SwtchUStruct(proc_zero)调用！
    // 因为ESP定位方式不需要修改页表

    // 恢复进程0的栈指针
    aRetU(proc_zero->u_rsav);  // 注意：需要直接访问进程0的User结构

    X86Assembly::STI();  // 开中断

    // 3. 在进程0中选择下一个待运行进程
    Process* next = Select();

    // 4. 切换到选中的进程
    X86Assembly::CLI();

    // 注意：同样移除了SwtchUStruct(next)调用！
    // ESP定位方式下，只需要恢复栈指针

    // 恢复新进程的栈指针
    aRetU(next->u_rsav);  // 注意：需要直接访问新进程的User结构

    X86Assembly::STI();

    // 5. 获取新进程的User结构（通过ESP自动定位）
    User& new_u = Kernel::Instance().GetUser();

    // 6. 建立新进程的用户态页表映射
    new_u.u_MemoryDescriptor.MapToPageTable();

    // 7. 处理交换标志
    if (new_u.u_procp->p_flag & Process::SSWAP)
    {
        new_u.u_procp->p_flag &= ~Process::SSWAP;
        aRetU(new_u.u_ssav);
    }

    return 1;  // 返回1表示这是父进程（fork后）
}

/*
 * ====================================================================
 * 问题：如何在Swtch_ESP中访问其他进程的User结构？
 * ====================================================================
 *
 * 在ESP定位方式下，GetUser()只能获取当前进程的User结构。
 * 但在进程切换时，需要访问其他进程的User结构来恢复栈指针。
 *
 * 解决方案1：通过Process结构中的p_addr访问
 *
 * User* GetUserByProcess(Process* p)
 * {
 *     return (User*)p->p_addr;
 * }
 *
 * 解决方案2：在User结构中缓存栈指针到Process结构
 *
 * 在SaveU时：
 * parent->p_saved_esp = u.u_rsav[0];
 * parent->p_saved_ebp = u.u_rsav[1];
 *
 * 在恢复时：
 * __asm__ __volatile__(
 *     "movl %0, %%esp\n\t"
 *     "movl %1, %%ebp\n\t"
 *     : : "r"(next->p_saved_esp), "r"(next->p_saved_ebp)
 * );
 */

/*
 * ====================================================================
 * 改进的Swtch()实现 - 使用p_addr访问User结构
 * ====================================================================
 */
int ProcessManager::Swtch_ESP_Improved()
{
    User& u = Kernel::Instance().GetUser();
    Process* current = u.u_procp;

    // 1. 保存当前进程的栈指针到User结构
    SaveU(u.u_rsav);

    // 2. 切换到进程0
    Process* proc_zero = &process[0];
    User* user_zero = (User*)proc_zero->p_addr;

    X86Assembly::CLI();

    // 恢复进程0的栈指针
    __asm__ __volatile__(
        "movl %0, %%esp\n\t"
        "movl %1, %%ebp\n\t"
        : : "r"(user_zero->u_rsav[0]), "r"(user_zero->u_rsav[1])
    );

    X86Assembly::STI();

    // 3. 选择下一个进程
    Process* next = Select();

    // 4. 切换到新进程
    User* user_next = (User*)next->p_addr;

    X86Assembly::CLI();

    // 恢复新进程的栈指针
    __asm__ __volatile__(
        "movl %0, %%esp\n\t"
        "movl %1, %%ebp\n\t"
        : : "r"(user_next->u_rsav[0]), "r"(user_next->u_rsav[1])
    );

    X86Assembly::STI();

    // 5. 现在ESP已指向新进程的栈，GetUser()会自动返回正确的User结构
    User& new_u = Kernel::Instance().GetUser();

    // 6. 建立新进程的用户态页表映射
    new_u.u_MemoryDescriptor.MapToPageTable();

    // 7. 处理交换标志
    if (new_u.u_procp->p_flag & Process::SSWAP)
    {
        new_u.u_procp->p_flag &= ~Process::SSWAP;
        __asm__ __volatile__(
            "movl %0, %%esp\n\t"
            "movl %1, %%ebp\n\t"
            : : "r"(new_u.u_ssav[0]), "r"(new_u.u_ssav[1])
        );
    }

    return 1;
}

/*
 * ====================================================================
 * 辅助宏：直接访问指定进程的User结构
 * ====================================================================
 */
#define GetUserByProcess(p) ((User*)((p)->p_addr))

/*
 * ====================================================================
 * 修改4：进程0（idle进程）的初始化
 * ====================================================================
 */

/*
 * 初始化进程0
 *
 * 进程0是特殊的idle进程，需要特殊处理其内核栈
 */
void ProcessManager::InitProcess0_ESP()
{
    Process* proc0 = &process[0];

    // 为进程0分配8KB对齐的内核栈区域
    unsigned long stack_base = userPageManager.AllocateAlignedMemory(
        Kernel::KERNEL_STACK_SIZE,
        Kernel::KERNEL_STACK_SIZE
    );

    if (stack_base == 0)
    {
        Panic("InitProcess0: Cannot allocate kernel stack for process 0");
    }

    // 设置进程0的地址
    proc0->p_addr = stack_base;

    // 初始化User结构
    User* user0 = (User*)stack_base;
    Utility::MemSet((char*)user0, 0, sizeof(User));

    // 设置初始栈指针
    unsigned long initial_esp = stack_base + Kernel::KERNEL_STACK_SIZE;
    user0->u_rsav[0] = initial_esp;
    user0->u_rsav[1] = initial_esp;

    // 设置进程指针
    user0->u_procp = proc0;

    // 初始化进程0的其他字段
    proc0->p_pid = 0;
    proc0->p_stat = Process::SRUN;
    proc0->p_flag = Process::SLOAD | Process::SSYS;
    proc0->p_nice = 0;
    proc0->p_pri = 0;
}

/*
 * ====================================================================
 * 性能分析：进程切换时间对比
 * ====================================================================
 *
 * 旧实现（虚拟地址映射）：
 * 1. SaveU(u.u_rsav)                    - 保存栈指针（2条mov指令）
 * 2. SwtchUStruct(next)                 - 修改页表项（2-3条指令）
 * 3. FlushPageDirectory()               - 刷新TLB（mov cr3, cr3）约100+周期
 * 4. RetU()                             - 恢复栈指针（3条mov指令）
 * 总计：约110-120时钟周期
 *
 * 新实现（ESP定位）：
 * 1. SaveU(u.u_rsav)                    - 保存栈指针（2条mov指令）
 * 2. 直接恢复新进程栈指针                - 恢复栈指针（2条mov指令）
 * 总计：约4-6时钟周期
 *
 * 性能提升：每次进程切换节省约100+时钟周期（约95%）
 *
 * 在高并发系统中，假设每秒1000次进程切换：
 * - 旧实现：约110,000时钟周期/秒
 * - 新实现：约5,000时钟周期/秒
 * - 节省：约105,000时钟周期/秒
 *
 * 在3GHz CPU上，相当于每秒节省约35微秒
 * ====================================================================
 */

/*
 * ====================================================================
 * 调试辅助函数
 * ====================================================================
 */

void ProcessManager::DumpProcessStackInfo(Process* p)
{
    if (p == nullptr || p->p_stat == Process::SNULL)
    {
        Diagnose::Write("Invalid process\n");
        return;
    }

    User* user = (User*)p->p_addr;

    Diagnose::Write("\n=== Process Stack Info ===\n");
    Diagnose::Write("Process PID: ");
    Diagnose::WriteInt(p->p_pid);
    Diagnose::Write("\nStack base: ");
    Diagnose::WriteHex(p->p_addr);
    Diagnose::Write("\nUser struct: ");
    Diagnose::WriteHex((unsigned long)user);
    Diagnose::Write("\nStack top: ");
    Diagnose::WriteHex(p->p_addr + Kernel::KERNEL_STACK_SIZE);
    Diagnose::Write("\nSaved ESP: ");
    Diagnose::WriteHex(user->u_rsav[0]);
    Diagnose::Write("\nSaved EBP: ");
    Diagnose::WriteHex(user->u_rsav[1]);
    Diagnose::Write("\n===========================\n");
}

void ProcessManager::ValidateAllProcessStacks()
{
    for (int i = 0; i < NPROC; i++)
    {
        Process* p = &process[i];

        if (p->p_stat == Process::SNULL)
            continue;

        // 验证栈地址是否8KB对齐
        if ((p->p_addr & (Kernel::KERNEL_STACK_SIZE - 1)) != 0)
        {
            Diagnose::Write("ERROR: Process ");
            Diagnose::WriteInt(p->p_pid);
            Diagnose::Write(" stack not aligned! Address: ");
            Diagnose::WriteHex(p->p_addr);
            Diagnose::Write("\n");
            Panic("Stack alignment check failed");
        }

        // 验证ESP在合法范围内
        User* user = (User*)p->p_addr;
        unsigned long esp = user->u_rsav[0];
        unsigned long stack_limit = p->p_addr + Kernel::STACK_OFFSET;
        unsigned long stack_top = p->p_addr + Kernel::KERNEL_STACK_SIZE;

        if (esp < stack_limit || esp > stack_top)
        {
            Diagnose::Write("ERROR: Process ");
            Diagnose::WriteInt(p->p_pid);
            Diagnose::Write(" ESP out of range! ESP: ");
            Diagnose::WriteHex(esp);
            Diagnose::Write("\n");
            Panic("Stack pointer check failed");
        }
    }

    Diagnose::Write("All process stacks validated successfully.\n");
}

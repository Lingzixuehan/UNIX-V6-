/*
 * ====================================================================
 * GetUser()函数的多种实现方式
 * ====================================================================
 *
 * 本文件展示了GetUser()函数的几种不同实现方式，用于教学和对比
 */

#include "Kernel.h"

/*
 * ====================================================================
 * 方案1：原始实现（虚拟地址映射）
 * ====================================================================
 *
 * 优点：
 * - 实现简单
 * - 所有进程使用统一虚拟地址
 *
 * 缺点：
 * - 需要在进程切换时修改页表项
 * - 需要刷新TLB（约100+时钟周期开销）
 * - 依赖分页机制
 */
User& Kernel::GetUser_Original()
{
    return *(User*)USER_ADDRESS;  // USER_ADDRESS = 0xC03FF000
}

/*
 * ====================================================================
 * 方案2：ESP位运算定位（推荐实现）
 * ====================================================================
 *
 * 前提条件：
 * - 每个进程的内核栈区域大小为8KB（0x2000）
 * - 内核栈区域按8KB对齐（地址低13位为0）
 * - User结构位于栈区域起始位置
 *
 * 优点：
 * - 性能优越：仅需2条指令（mov esp + and）
 * - 无TLB开销：不需要修改页表和刷新TLB
 * - 硬件无关：不依赖分页机制
 * - 原子操作：无需禁中断
 *
 * 工作原理：
 * 1. 读取ESP寄存器 -> 假设ESP = 0x00401A34
 * 2. ESP & 0xFFFFE000 -> 0x00401A34 & 0xFFFFE000 = 0x00400000
 * 3. 0x00400000就是User结构的起始地址
 */
User& Kernel::GetUser_ESP()
{
    unsigned long esp;

    // 使用内联汇编读取ESP寄存器
    __asm__ __volatile__("movl %%esp, %0" : "=r"(esp));

    // 位运算：清除ESP的低13位，得到8KB对齐的边界地址
    unsigned long user_base = esp & KERNEL_STACK_MASK;

    return *(User*)user_base;
}

/*
 * ====================================================================
 * 方案3：使用GCC内建函数
 * ====================================================================
 *
 * 使用__builtin_frame_address()获取栈帧地址
 * 注意：某些编译器可能优化此函数
 */
User& Kernel::GetUser_Builtin()
{
    // __builtin_frame_address(0)返回当前栈帧的基址指针（EBP）
    // 在大多数情况下，EBP和ESP都在同一个8KB区域内
    unsigned long frame_addr = (unsigned long)__builtin_frame_address(0);

    return *(User*)(frame_addr & KERNEL_STACK_MASK);
}

/*
 * ====================================================================
 * 方案4：宏定义方式（内联展开，性能最优）
 * ====================================================================
 *
 * 优点：
 * - 完全内联，无函数调用开销
 * - 编译器可以更好地优化
 *
 * 缺点：
 * - 可读性稍差
 * - 调试较困难
 */
#define GET_CURRENT_USER() \
    ({ \
        unsigned long __esp; \
        __asm__ __volatile__("movl %%esp, %0" : "=r"(__esp)); \
        *(User*)(__esp & Kernel::KERNEL_STACK_MASK); \
    })

/*
 * ====================================================================
 * 方案5：汇编优化版本
 * ====================================================================
 *
 * 直接使用汇编实现，减少中间变量
 * 适合对性能要求极高的场景
 */
User& Kernel::GetUser_Assembly()
{
    User* pUser;

    __asm__ __volatile__(
        "movl %%esp, %%eax\n\t"           // 读取ESP到EAX
        "andl $0xFFFFE000, %%eax\n\t"     // 清除低13位
        "movl %%eax, %0\n\t"              // 结果存入pUser
        : "=r"(pUser)                      // 输出操作数
        :                                  // 无输入操作数
        : "%eax"                           // 破坏的寄存器
    );

    return *pUser;
}

/*
 * ====================================================================
 * 辅助函数：验证GetUser()实现的正确性
 * ====================================================================
 *
 * 在过渡期间，可以同时运行新旧实现，验证结果一致性
 */
void Kernel::ValidateGetUserImplementation()
{
    // 获取旧实现的结果
    User* user_legacy = &GetUser_Legacy();

    // 获取新实现的结果
    User* user_esp = &GetUser_ESP();

    // 验证两者是否指向同一个User结构
    if (user_legacy != user_esp)
    {
        // 实现不一致，输出错误信息
        Diagnose::Write("ERROR: GetUser() implementations mismatch!\n");
        Diagnose::Write("Legacy address: ");
        Diagnose::WriteHex((unsigned long)user_legacy);
        Diagnose::Write("\nESP-based address: ");
        Diagnose::WriteHex((unsigned long)user_esp);
        Diagnose::Write("\n");

        // 触发内核恐慌
        Panic("GetUser() validation failed");
    }
}

/*
 * ====================================================================
 * 辅助函数：栈溢出检测
 * ====================================================================
 *
 * ESP定位方式使栈溢出检测变得简单
 */
bool Kernel::IsStackOverflow()
{
    unsigned long esp;
    __asm__ __volatile__("movl %%esp, %0" : "=r"(esp));

    // 计算栈区域起始地址
    unsigned long stack_base = esp & KERNEL_STACK_MASK;

    // 计算栈的最低有效地址（User结构结束位置）
    unsigned long stack_limit = stack_base + STACK_OFFSET;

    // 如果ESP低于栈限制，说明栈溢出
    if (esp < stack_limit)
    {
        Diagnose::Write("KERNEL PANIC: Stack overflow detected!\n");
        Diagnose::Write("ESP: ");
        Diagnose::WriteHex(esp);
        Diagnose::Write("\nStack limit: ");
        Diagnose::WriteHex(stack_limit);
        Diagnose::Write("\n");

        return true;
    }

    return false;
}

/*
 * ====================================================================
 * 辅助函数：调试信息输出
 * ====================================================================
 */
void Kernel::DumpCurrentProcessInfo()
{
    unsigned long esp, ebp;

    __asm__ __volatile__(
        "movl %%esp, %0\n\t"
        "movl %%ebp, %1\n\t"
        : "=r"(esp), "=r"(ebp)
    );

    unsigned long stack_base = esp & KERNEL_STACK_MASK;
    User* pUser = (User*)stack_base;

    Diagnose::Write("\n=== Current Process Info ===\n");
    Diagnose::Write("ESP: ");
    Diagnose::WriteHex(esp);
    Diagnose::Write("\nEBP: ");
    Diagnose::WriteHex(ebp);
    Diagnose::Write("\nStack base: ");
    Diagnose::WriteHex(stack_base);
    Diagnose::Write("\nUser struct: ");
    Diagnose::WriteHex((unsigned long)pUser);
    Diagnose::Write("\nProcess PID: ");
    Diagnose::WriteInt(pUser->u_procp->p_pid);
    Diagnose::Write("\n============================\n");
}

/*
 * ====================================================================
 * 性能测试函数
 * ====================================================================
 *
 * 对比不同实现的性能
 */
void Kernel::BenchmarkGetUser()
{
    const int ITERATIONS = 1000000;
    unsigned long start, end;

    // 测试旧实现
    __asm__ __volatile__("rdtsc" : "=A"(start));
    for (int i = 0; i < ITERATIONS; i++)
    {
        volatile User* u = &GetUser_Legacy();
    }
    __asm__ __volatile__("rdtsc" : "=A"(end));

    Diagnose::Write("Legacy implementation: ");
    Diagnose::WriteInt((int)(end - start));
    Diagnose::Write(" cycles\n");

    // 测试ESP实现
    __asm__ __volatile__("rdtsc" : "=A"(start));
    for (int i = 0; i < ITERATIONS; i++)
    {
        volatile User* u = &GetUser_ESP();
    }
    __asm__ __volatile__("rdtsc" : "=A"(end));

    Diagnose::Write("ESP-based implementation: ");
    Diagnose::WriteInt((int)(end - start));
    Diagnose::Write(" cycles\n");
}

/*
 * ====================================================================
 * 汇编级别的性能对比
 * ====================================================================
 *
 * 编译后的汇编代码对比：
 *
 * 旧实现（GetUser_Legacy）：
 * mov eax, 0xC03FF000      ; 加载固定地址
 * ret
 *
 * 新实现（GetUser_ESP）：
 * mov eax, esp             ; 读取ESP
 * and eax, 0xFFFFE000      ; 位运算
 * ret
 *
 * 指令数量：相同（2条指令 + ret）
 *
 * 但在进程切换时的差异：
 *
 * 旧实现需要：
 * mov eax, [process_addr]
 * shr eax, 12                    ; 计算页号
 * mov [page_table + 1023*4], eax ; 更新页表项
 * mov cr3, cr3                   ; 刷新TLB（约100+周期）
 *
 * 新实现：
 * （无需任何操作，自动通过ESP定位）
 *
 * 每次进程切换节省：约100+时钟周期
 * ====================================================================
 */

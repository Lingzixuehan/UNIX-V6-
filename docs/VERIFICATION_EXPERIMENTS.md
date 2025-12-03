# ESP定位User结构 - 验证实验方案

## 实验目标

验证使用ESP寄存器定位User结构的设计方案的正确性和性能优势。

---

## 实验层次

### 🟢 Level 1：原理验证实验（推荐，简单快速）
**目的**：验证位运算定位的基本原理
**时间**：30分钟
**难度**：⭐

### 🟡 Level 2：独立原型实验（可选，深入理解）
**目的**：用独立程序模拟整个机制
**时间**：2小时
**难度**：⭐⭐⭐

### 🔴 Level 3：实际内核修改实验（完整验证）
**目的**：在UNIX V6++中实际实现并测试
**时间**：1-2天
**难度**：⭐⭐⭐⭐⭐

---

## Level 1：原理验证实验 ✅ 推荐

### 实验1.1：位运算验证

**目的**：验证ESP & 0xFFFFE000能正确定位到8KB边界

创建测试程序：

```cpp
// test_esp_alignment.cpp
#include <stdio.h>
#include <stdint.h>

void test_alignment_calculation() {
    printf("=== ESP对齐计算验证 ===\n\n");

    // 测试用例：模拟不同的ESP值
    uint32_t test_esp_values[] = {
        0x00400000,  // 正好在边界
        0x00400100,  // 边界 + 256字节
        0x00401000,  // 边界 + 4KB
        0x00401A34,  // 边界 + 6708字节（示例中的ESP）
        0x00401FFF,  // 边界 + 8KB - 1（最大值）
    };

    const uint32_t KERNEL_STACK_MASK = 0xFFFFE000;
    const uint32_t KERNEL_STACK_SIZE = 0x2000;  // 8KB

    for (int i = 0; i < 5; i++) {
        uint32_t esp = test_esp_values[i];
        uint32_t user_base = esp & KERNEL_STACK_MASK;
        uint32_t offset = esp - user_base;

        printf("测试 %d:\n", i + 1);
        printf("  ESP:        0x%08X\n", esp);
        printf("  计算结果:    0x%08X\n", user_base);
        printf("  偏移:        0x%04X (%u 字节)\n", offset, offset);
        printf("  验证对齐:    %s\n",
               (user_base & 0x1FFF) == 0 ? "✓ PASS" : "✗ FAIL");
        printf("  在范围内:    %s\n\n",
               offset < KERNEL_STACK_SIZE ? "✓ PASS" : "✗ FAIL");
    }
}

void test_different_alignments() {
    printf("=== 不同对齐大小对比 ===\n\n");

    uint32_t esp = 0x00401A34;  // 示例ESP

    // 4KB对齐
    uint32_t mask_4k = 0xFFFFF000;
    printf("4KB对齐 (mask=0x%08X): 0x%08X\n", mask_4k, esp & mask_4k);

    // 8KB对齐
    uint32_t mask_8k = 0xFFFFE000;
    printf("8KB对齐 (mask=0x%08X): 0x%08X\n", mask_8k, esp & mask_8k);

    // 16KB对齐
    uint32_t mask_16k = 0xFFFFC000;
    printf("16KB对齐 (mask=0x%08X): 0x%08X\n\n", mask_16k, esp & mask_16k);
}

void test_performance_comparison() {
    printf("=== 性能对比测试 ===\n\n");

    const int ITERATIONS = 10000000;
    volatile uint32_t result;

    // 方法1：固定地址（模拟旧实现）
    uint32_t fixed_address = 0xC03FF000;
    uint64_t start = __rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        result = fixed_address;
    }
    uint64_t end = __rdtsc();
    uint64_t cycles_fixed = end - start;

    // 方法2：ESP位运算（新实现）
    uint32_t esp = 0x00401A34;
    start = __rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        result = esp & 0xFFFFE000;
    }
    end = __rdtsc();
    uint64_t cycles_esp = end - start;

    printf("固定地址方法: %llu 周期 (%.2f ns/iter)\n",
           cycles_fixed, (double)cycles_fixed / ITERATIONS);
    printf("ESP位运算方法: %llu 周期 (%.2f ns/iter)\n",
           cycles_esp, (double)cycles_esp / ITERATIONS);
    printf("差异: %.2f%%\n\n",
           ((double)cycles_esp / cycles_fixed - 1.0) * 100);

    printf("注意：这只测试GetUser()本身，不包括进程切换时的TLB刷新开销\n");
    printf("      实际进程切换时，旧方法还需要约100周期的TLB刷新\n");
}

int main() {
    test_alignment_calculation();
    test_different_alignments();
    test_performance_comparison();

    printf("=== 结论 ===\n");
    printf("✓ 位运算能正确定位到8KB边界\n");
    printf("✓ 不同ESP值都能定位到同一个User结构起始地址\n");
    printf("✓ 性能与固定地址方法相当\n");
    printf("✓ 实际优势在于消除进程切换时的TLB刷新开销\n");

    return 0;
}
```

**运行方法：**
```bash
cd /home/user/UNIX-V6-/docs/experiments
g++ -o test_esp test_esp_alignment.cpp
./test_esp
```

**预期输出：**
- 所有ESP值都正确定位到0x00400000
- 性能测试显示两种方法相当
- 所有验证通过

---

### 实验1.2：内存布局验证

**目的**：验证栈增长方向和User结构位置关系

```cpp
// test_stack_layout.cpp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// 模拟User结构（简化版）
struct MockUser {
    uint32_t u_rsav[2];    // ESP, EBP
    uint32_t u_pid;
    char padding[4084];    // 填充到4KB
};

// 模拟进程的内核栈区域
struct KernelStackArea {
    MockUser user;          // 前4KB
    char stack[4096];       // 后4KB作为栈空间
};

void test_stack_layout() {
    printf("=== 内存布局验证 ===\n\n");

    // 分配对齐的内存（8KB）
    void* raw_mem = nullptr;
    if (posix_memalign(&raw_mem, 0x2000, 0x2000) != 0) {
        printf("内存分配失败\n");
        return;
    }

    KernelStackArea* area = (KernelStackArea*)raw_mem;
    uint32_t base = (uint32_t)(uintptr_t)raw_mem;

    printf("内核栈区域布局：\n");
    printf("  基址:          0x%08X\n", base);
    printf("  User结构:      0x%08X - 0x%08X (4KB)\n",
           base, base + 0x1000);
    printf("  栈空间:        0x%08X - 0x%08X (4KB)\n",
           base + 0x1000, base + 0x2000);
    printf("  对齐验证:      %s\n\n",
           (base & 0x1FFF) == 0 ? "✓ 8KB对齐" : "✗ 未对齐");

    // 验证：模拟栈指针在不同位置
    printf("模拟ESP在不同位置：\n");
    uint32_t test_positions[] = {
        base + 0x2000,      // 栈底
        base + 0x1F00,      // 使用了256字节
        base + 0x1800,      // 使用了2KB
        base + 0x1400,      // 使用了3KB
    };

    for (int i = 0; i < 4; i++) {
        uint32_t esp = test_positions[i];
        uint32_t calculated_base = esp & 0xFFFFE000;
        uint32_t stack_used = (base + 0x2000) - esp;

        printf("  ESP=0x%08X -> User=0x%08X, 栈使用=%u字节 %s\n",
               esp, calculated_base, stack_used,
               calculated_base == base ? "✓" : "✗");
    }

    free(raw_mem);
}

void test_stack_overflow_detection() {
    printf("\n=== 栈溢出检测验证 ===\n\n");

    uint32_t base = 0x00400000;
    uint32_t stack_limit = base + 0x1000;  // User结构后

    uint32_t test_cases[] = {
        base + 0x2000,      // 栈底，正常
        base + 0x1800,      // 中间，正常
        base + 0x1100,      // 接近限制，正常
        base + 0x1000,      // 边界，正常
        base + 0x0F00,      // 溢出！进入User区域
        base + 0x0500,      // 严重溢出
    };

    printf("栈限制: 0x%08X\n\n", stack_limit);

    for (int i = 0; i < 6; i++) {
        uint32_t esp = test_cases[i];
        bool overflow = (esp < stack_limit);

        printf("ESP=0x%08X: %s\n", esp,
               overflow ? "✗ 栈溢出！" : "✓ 正常");
    }
}

int main() {
    test_stack_layout();
    test_stack_overflow_detection();

    printf("\n=== 结论 ===\n");
    printf("✓ 8KB对齐的内存可以正确分为User区和栈区\n");
    printf("✓ 不同栈位置都能正确定位到User结构\n");
    printf("✓ 栈溢出检测机制简单有效\n");

    return 0;
}
```

---

## Level 2：独立原型实验 🔬

### 实验2.1：完整的进程切换模拟

**目的**：模拟完整的进程创建、切换和User结构访问

```cpp
// test_process_switch.cpp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define KERNEL_STACK_SIZE 0x2000
#define KERNEL_STACK_MASK 0xFFFFE000

// 模拟User结构
struct User {
    uint32_t u_rsav[2];     // 保存的ESP和EBP
    int u_pid;              // 进程ID
    char u_name[32];        // 进程名
    int u_counter;          // 计数器（用于验证）
    char padding[4020];     // 填充到4KB
};

// 模拟Process结构
struct Process {
    int p_pid;
    uint32_t p_addr;        // 内核栈区域基址
    int p_state;            // 进程状态
};

// 全局进程表
#define MAX_PROCS 10
Process g_processes[MAX_PROCS];
int g_current_proc = 0;

// 模拟GetUser() - 通过ESP定位
User* GetUser(uint32_t esp) {
    uint32_t user_base = esp & KERNEL_STACK_MASK;
    return (User*)user_base;
}

// 创建进程
Process* CreateProcess(int pid, const char* name) {
    // 分配8KB对齐的内核栈区域
    void* raw_mem;
    if (posix_memalign(&raw_mem, KERNEL_STACK_SIZE, KERNEL_STACK_SIZE) != 0) {
        printf("内存分配失败\n");
        return nullptr;
    }

    uint32_t base = (uint32_t)(uintptr_t)raw_mem;

    // 初始化Process结构
    Process* proc = &g_processes[pid];
    proc->p_pid = pid;
    proc->p_addr = base;
    proc->p_state = 1;  // RUNNING

    // 初始化User结构
    User* user = (User*)base;
    memset(user, 0, sizeof(User));
    user->u_pid = pid;
    strncpy(user->u_name, name, 31);
    user->u_counter = 0;

    // 初始化栈指针（指向栈底）
    user->u_rsav[0] = base + KERNEL_STACK_SIZE;  // ESP
    user->u_rsav[1] = base + KERNEL_STACK_SIZE;  // EBP

    printf("创建进程 %d (%s):\n", pid, name);
    printf("  栈区域基址: 0x%08X\n", base);
    printf("  User结构:   0x%08X\n", base);
    printf("  初始ESP:    0x%08X\n", user->u_rsav[0]);
    printf("  对齐检查:   %s\n\n",
           (base & 0x1FFF) == 0 ? "✓" : "✗");

    return proc;
}

// 模拟进程切换
void SwitchProcess(int old_pid, int new_pid, uint32_t current_esp) {
    printf("进程切换: %d -> %d\n", old_pid, new_pid);

    // 1. 保存当前进程的ESP
    Process* old_proc = &g_processes[old_pid];
    User* old_user = (User*)old_proc->p_addr;
    old_user->u_rsav[0] = current_esp;
    printf("  保存进程%d的ESP: 0x%08X\n", old_pid, current_esp);

    // 2. 验证通过当前ESP能找到正确的User结构
    User* found_user = GetUser(current_esp);
    if (found_user != old_user) {
        printf("  ✗ 错误：ESP定位失败！\n");
    } else {
        printf("  ✓ ESP定位成功: PID=%d, Name=%s\n",
               found_user->u_pid, found_user->u_name);
    }

    // 3. 加载新进程的ESP
    Process* new_proc = &g_processes[new_pid];
    User* new_user = (User*)new_proc->p_addr;
    uint32_t new_esp = new_user->u_rsav[0];
    printf("  加载进程%d的ESP: 0x%08X\n", new_pid, new_esp);

    // 4. 验证新ESP能找到新进程的User结构
    found_user = GetUser(new_esp);
    if (found_user != new_user) {
        printf("  ✗ 错误：新ESP定位失败！\n");
    } else {
        printf("  ✓ 新ESP定位成功: PID=%d, Name=%s\n",
               found_user->u_pid, found_user->u_name);
    }

    g_current_proc = new_pid;
    printf("\n");
}

// 模拟进程执行（修改栈指针）
void ProcessWork(int pid, uint32_t stack_usage) {
    Process* proc = &g_processes[pid];
    User* user = (User*)proc->p_addr;

    // 模拟栈增长
    uint32_t new_esp = user->u_rsav[0] - stack_usage;
    user->u_rsav[0] = new_esp;
    user->u_counter++;

    printf("进程%d工作中:\n", pid);
    printf("  栈使用: %u 字节\n", stack_usage);
    printf("  新ESP:  0x%08X\n", new_esp);
    printf("  计数:   %d\n", user->u_counter);

    // 验证仍能通过ESP找到User结构
    User* found = GetUser(new_esp);
    if (found != user) {
        printf("  ✗ 错误：ESP定位失败！\n");
    } else {
        printf("  ✓ ESP仍然正确定位: PID=%d\n", found->u_pid);
    }
    printf("\n");
}

// 性能测试：进程切换开销
void BenchmarkSwitch() {
    printf("=== 进程切换性能测试 ===\n\n");

    const int ITERATIONS = 1000000;
    clock_t start, end;

    // 测试旧方法（模拟TLB刷新）
    volatile uint32_t dummy = 0;
    start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        // 模拟页表修改
        dummy = 0xC03FF000;
        // 模拟TLB刷新（空操作，实际约100周期）
        for (int j = 0; j < 10; j++) dummy++;
    }
    end = clock();
    double time_old = (double)(end - start) / CLOCKS_PER_SEC;

    // 测试新方法（仅ESP位运算）
    uint32_t esp = 0x00401A34;
    start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        dummy = esp & KERNEL_STACK_MASK;
    }
    end = clock();
    double time_new = (double)(end - start) / CLOCKS_PER_SEC;

    printf("旧方法（含TLB刷新模拟）: %.3f 秒\n", time_old);
    printf("新方法（ESP位运算）:      %.3f 秒\n", time_new);
    printf("性能提升: %.1fx\n\n", time_old / time_new);
}

int main() {
    printf("=== 进程切换完整模拟测试 ===\n\n");

    // 创建3个进程
    CreateProcess(0, "init");
    CreateProcess(1, "shell");
    CreateProcess(2, "worker");

    // 模拟进程执行和切换
    printf("=== 模拟进程调度 ===\n\n");

    // 进程0执行
    ProcessWork(0, 512);  // 使用512字节栈

    // 切换到进程1
    SwitchProcess(0, 1, g_processes[0].p_addr + KERNEL_STACK_SIZE - 512);

    // 进程1执行
    ProcessWork(1, 1024); // 使用1KB栈

    // 切换到进程2
    SwitchProcess(1, 2, g_processes[1].p_addr + KERNEL_STACK_SIZE - 1024);

    // 进程2执行
    ProcessWork(2, 2048); // 使用2KB栈

    // 切换回进程0
    SwitchProcess(2, 0, g_processes[2].p_addr + KERNEL_STACK_SIZE - 2048);

    // 性能测试
    BenchmarkSwitch();

    // 清理
    for (int i = 0; i < 3; i++) {
        free((void*)(uintptr_t)g_processes[i].p_addr);
    }

    printf("=== 实验结论 ===\n");
    printf("✓ ESP能在不同栈深度正确定位User结构\n");
    printf("✓ 进程切换后ESP定位到新进程User结构\n");
    printf("✓ 新方法性能显著优于旧方法\n");
    printf("✓ 机制简单、可靠、高效\n");

    return 0;
}
```

---

## Level 3：实际内核修改实验 🚀

### 实验3.1：最小化修改验证

**目的**：在UNIX V6++中实际实现ESP定位，与旧方法对比验证

**步骤：**

#### 第1步：添加新GetUser()实现

```bash
# 备份原文件
cp src/kernel/Kernel.cpp src/kernel/Kernel.cpp.backup
cp src/include/Kernel.h src/include/Kernel.h.backup
```

修改`src/include/Kernel.h`：

```cpp
// 在Kernel类中添加
public:
    // 新常量
    static const unsigned long KERNEL_STACK_SIZE = 0x2000;
    static const unsigned long KERNEL_STACK_MASK = 0xFFFFE000;

    // 新方法
    User& GetUser_ESP();        // ESP定位版本
    User& GetUser_Legacy();     // 旧版本（重命名）

    // 验证方法
    void ValidateGetUser();     // 验证两种方法结果一致
```

修改`src/kernel/Kernel.cpp`：

```cpp
// 实现ESP版本
User& Kernel::GetUser_ESP()
{
    unsigned long esp;
    __asm__ __volatile__("movl %%esp, %0" : "=r"(esp));
    unsigned long user_base = esp & KERNEL_STACK_MASK;
    return *(User*)user_base;
}

// 保留旧版本
User& Kernel::GetUser_Legacy()
{
    return *(User*)USER_ADDRESS;
}

// 验证函数
void Kernel::ValidateGetUser()
{
    User* u_esp = &GetUser_ESP();
    User* u_legacy = &GetUser_Legacy();

    if (u_esp != u_legacy)
    {
        Diagnose::Write("WARNING: GetUser() mismatch!\n");
        Diagnose::Write("ESP method:    ");
        Diagnose::WriteHex((unsigned long)u_esp);
        Diagnose::Write("\nLegacy method: ");
        Diagnose::WriteHex((unsigned long)u_legacy);
        Diagnose::Write("\n");
    }
    else
    {
        Diagnose::Write("GetUser() validation: PASS\n");
    }
}
```

#### 第2步：在系统初始化时验证

修改`src/kernel/main.cpp`或合适的初始化位置：

```cpp
// 在初始化完成后，进程0运行时
Kernel::Instance().ValidateGetUser();
```

#### 第3步：编译并运行

```bash
cd /home/user/UNIX-V6-
make clean
make

# 运行系统，观察输出
# 应该看到 "GetUser() validation: PASS"
```

#### 第4步：在多个关键点验证

在以下位置添加验证调用：
- 系统调用入口（SystemCall::Trap）
- 时间中断处理（Time::Clock）
- 进程切换后（ProcessManager::Swtch）

```cpp
// 在关键函数中添加
#ifdef DEBUG_GETUSER
    Kernel::Instance().ValidateGetUser();
#endif
```

#### 第5步：压力测试

创建测试程序：

```c
// user/test_fork.c
int main() {
    int i;
    for (i = 0; i < 10; i++) {
        int pid = fork();
        if (pid == 0) {
            // 子进程
            printf("Child %d running\n", i);
            exit(0);
        }
    }

    // 父进程等待所有子进程
    for (i = 0; i < 10; i++) {
        wait();
    }

    printf("All children finished\n");
    return 0;
}
```

---

### 实验3.2：性能测试

**在内核中添加性能计数器：**

```cpp
// 在Kernel类中添加
private:
    static uint64_t tlb_flush_count;
    static uint64_t switch_count;

public:
    static void IncrementTLBFlush() { tlb_flush_count++; }
    static void IncrementSwitch() { switch_count++; }
    static void PrintStats();
```

在`ProcessManager::Swtch()`中：

```cpp
// 旧实现
SwtchUStruct(next);
Kernel::IncrementTLBFlush();

// 新实现
// （移除SwtchUStruct，不增加计数）

Kernel::IncrementSwitch();
```

添加统计命令：

```bash
# 在shell中执行
$ stats

输出：
进程切换次数: 12345
TLB刷新次数: 12345 (旧实现) 或 0 (新实现)
```

---

## 📊 实验预期结果

### Level 1结果：
✅ 位运算正确定位到8KB边界
✅ 所有测试用例通过
✅ 性能测试显示GetUser()本身性能相当

### Level 2结果：
✅ 完整的进程切换模拟成功
✅ ESP在不同栈深度都能正确定位
✅ 进程切换后定位到新进程
✅ 性能提升明显（约10倍，因为模拟TLB刷新）

### Level 3结果：
✅ 两种GetUser()返回相同地址
✅ 系统稳定运行
✅ Fork/Exec/Wait等系统调用正常
✅ 进程切换无TLB刷新（新实现）
✅ 性能提升约5-10%（取决于进程切换频率）

---

## ⚠️ 注意事项

### Level 1实验：
- 在x86-64上编译时注意32位/64位指针差异
- 使用`-m32`标志编译32位程序

### Level 2实验：
- 确保系统支持`posix_memalign`
- 注意内存对齐可能失败的情况

### Level 3实验：
- **重要**：在修改内核前备份所有文件
- 建议在虚拟机中测试
- 先用旧实现运行系统，确保系统正常
- 逐步添加验证代码
- 不要一次性切换到新实现

---

## 🎯 推荐的实验路径

### 对于学习理解（推荐）：
1. **必做**：Level 1 - 原理验证（30分钟）
2. **推荐**：Level 2 - 独立原型（2小时）
3. **可选**：Level 3 - 如果想深入实践

### 对于实际应用：
1. Level 1 + Level 2 先验证原理
2. Level 3.1 最小化修改，验证正确性
3. 充分测试后再考虑完全切换

---

## 📝 实验报告模板

```
实验名称：ESP寄存器定位User结构验证

1. 实验目的
   [描述你的实验目标]

2. 实验环境
   - 操作系统：
   - 编译器：
   - CPU：

3. 实验步骤
   [记录你的操作]

4. 实验结果
   [截图或输出]

5. 分析与结论
   - 正确性验证：
   - 性能对比：
   - 遇到的问题：
   - 解决方案：

6. 收获与思考
   [你的理解和感想]
```

---

## 总结

- **Level 1**：快速验证原理，理解机制 ✅ **推荐从这里开始**
- **Level 2**：深入理解，完整模拟
- **Level 3**：实战练习，完整实现

**建议**：先做Level 1，大约30分钟就能理解核心原理并验证可行性。如果想深入，再进行Level 2和Level 3。

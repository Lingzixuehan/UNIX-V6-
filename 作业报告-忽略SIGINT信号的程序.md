# 作业报告：实现 Ctrl+C 杀不死的应用程序

## 一、仓库说明

本仓库是 **UNIX V6++** 操作系统的实现代码，这是一个基于经典 UNIX Version 6 的教学操作系统项目。系统包含完整的内核实现，包括：

- **进程管理**：进程调度、进程间通信
- **内存管理**：分页机制、内存交换
- **文件系统**：inode、目录管理
- **信号机制**：支持 32 种标准 UNIX 信号
- **设备驱动**：键盘、鼠标、磁盘等

## 二、实现原理

### 2.1 信号机制基础

在 UNIX 系统中，按下 **Ctrl+C** 会向前台进程发送 **SIGINT (信号 2)** 信号，默认行为是终止进程。但该信号可以被捕获和处理，程序可以选择忽略或自定义处理。

### 2.2 关键技术点

**可捕获的信号：**
- SIGINT (Ctrl+C)
- SIGTERM (终止请求)
- SIGUSR1/SIGUSR2 (用户自定义信号)

**不可捕获的信号：**
- SIGKILL (kill -9) - 强制终止
- SIGSTOP - 强制暂停

## 三、程序实现

### 3.1 核心代码

文件位置：`src/program/immortal.c`

```c
#include <stdio.h>
#include <sys.h>

// SIGINT 信号处理函数 - 必须接受 int 参数
void sigint_handler(int signo)
{
    if (signo == SIGINT)
    {
        printf("\n[!] Caught SIGINT (Ctrl+C), but I refuse to die!\n");
        // 重要：在 UNIX V6 中，signal() 是一次性的
        // 必须在处理函数中重新注册，否则第二次 Ctrl+C 会终止进程
        signal(SIGINT, sigint_handler);
    }
}

int main1(int argc, char* argv[])
{
    // 注册信号处理函数，捕获 SIGINT 但不退出
    if (signal(SIGINT, sigint_handler) == -1)
    {
        printf("Signal registration failed!\n");
        return -1;
    }

    printf("Immortal Process (PID: %d)\n", getpid());
    printf("Press Ctrl+C to test - I won't die!\n");
    printf("Use 'kill -9 %d' to kill me.\n", getpid());
    printf("Starting...\n\n");

    // 主循环：持续运行
    while(1)
    {
        sleep(10);
        printf("Still alive (PID: %d)\n", getpid());
    }

    return 0;
}
```

### 3.2 关键要点（重要！）

**⚠️ 要点 1：信号处理函数签名**

在 UNIX V6++ 系统中，信号处理函数**必须接受一个 int 参数**：

```c
// ✅ 正确
void sigint_handler(int signo)

// ❌ 错误（会导致信号无法正确捕获）
void sigint_handler()
```

这个参数表示接收到的信号编号，即使不使用也必须声明。

**⚠️ 要点 2：signal() 是一次性的（最关键！）**

UNIX V6 的 `signal()` 系统调用是**一次性的**：

```c
// 在 src/proc/Process.cpp 的 PSig() 函数中（第 481 行）：
u.u_signal[signal] = 0;  // 信号处理后自动重置为 0（默认行为）
```

**后果：**
- 第 1 次按 Ctrl+C：✅ 调用信号处理函数
- 第 2 次按 Ctrl+C：❌ 进程被终止（因为信号处理已重置为默认）

**解决方案：**
必须在信号处理函数中**重新注册**信号处理：

```c
void sigint_handler(int signo)
{
    printf("Caught signal!\n");
    signal(SIGINT, sigint_handler);  // ← 必须重新注册！
}
```

这是经典 UNIX 系统（V6/V7）的标准行为，现代 UNIX 使用 `sigaction()` 来避免这个问题。

### 3.3 实现方式

本程序提供了两种实现方式：

**方式 1：自定义处理函数（已采用）**
```c
signal(SIGINT, sigint_handler);
```
- 捕获 SIGINT 信号
- 执行自定义处理函数
- 显示提示信息但不退出

**方式 2：完全忽略信号**
```c
signal(SIGINT, SIG_IGN);
```
- 直接忽略 SIGINT 信号
- 按 Ctrl+C 没有任何反应

## 四、构建系统集成

### 4.1 修改 Makefile

在 `src/program/Makefile` 中添加编译规则：

```makefile
# 1. 添加到目标列表
SHELL_OBJS = ... \
             $(TARGET)\immortal.exe

# 2. 添加编译规则
$(TARGET)\immortal.exe : immortal.c
    $(CC) $(CFLAGS) -I"$(INCLUDE)" -I"$(LIB_INCLUDE)" \
          $< -e _main1 $(V6++LIB) -o $@
    copy $(TARGET)\immortal.exe $(MAKEIMAGEPATH)\$(BIN)\immortal
```

### 4.2 编译参数说明

- `-nostdlib -nostartfiles -nostdinc`：裸机编译，不依赖标准库
- `-e _main1`：指定入口点为 `main1` 函数（V6++ 规范）
- `$(V6++LIB)`：链接 V6++ 系统库，提供 signal、printf 等函数

## 五、运行效果

```
Immortal Process (PID: 42)
Press Ctrl+C to test - I won't die!
Use 'kill -9 42' to kill me.
Starting...

Still alive (PID: 42)
Still alive (PID: 42)
^C
[!] Caught SIGINT (Ctrl+C), but I refuse to die!
Still alive (PID: 42)
Still alive (PID: 42)
```

**测试结果：**
- ✅ 按 Ctrl+C：程序捕获信号，显示提示，继续运行
- ✅ 使用 `kill -9 42`：程序被强制终止

**调试经验：**

1. **问题 1：信号处理函数签名错误**
   - 现象：按 Ctrl+C 后程序立即退出
   - 原因：函数定义为 `void handler()` 而非 `void handler(int signo)`
   - 解决：参考 sigTest.c 添加 int 参数

2. **问题 2：第二次 Ctrl+C 导致退出（最难排查）**
   - 现象：第一次 Ctrl+C 被捕获，第二次直接退出
   - 原因：UNIX V6 的 signal() 是一次性的（Process.cpp:481）
   - 解决：在信号处理函数中调用 `signal(SIGINT, sigint_handler)` 重新注册
   - 教训：经典 UNIX 与现代 UNIX 的信号机制差异

3. **问题 3：输出混乱**
   - 现象：终端显示大量调试信息和进程状态
   - 原因：输出过于频繁触发系统调试
   - 解决：降低输出频率（1秒 → 10秒）

## 六、技术总结

### 6.1 关键知识点

1. **信号处理机制**：`signal(int sig, void (*handler)())` 系统调用
2. **信号类型**：可捕获信号 vs 不可捕获信号
3. **经典 UNIX 特性**：signal() 的一次性行为（需要重新注册）
4. **进程控制**：getpid()、sleep()、while 循环
5. **裸机编程**：无标准库环境下的系统编程
6. **内核源码分析**：通过阅读 Process.cpp 理解信号处理的内部实现

### 6.2 实际应用场景

- **服务器守护进程**：忽略 SIGINT，避免误操作关闭
- **数据库系统**：捕获信号，优雅关闭（保存数据后退出）
- **长时间任务**：防止用户误按 Ctrl+C 中断任务
- **调试工具**：捕获所有信号用于调试分析

## 七、补充说明

本次作业还顺带修复了 Makefile 中 `kill_child.exe` 缺失的编译规则，确保所有程序都能正确编译。

---

**开发环境：** UNIX V6++ 操作系统
**编译工具：** gcc (裸机模式)
**测试状态：** ✅ 通过编译，已集成到构建系统

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

// SIGINT 信号处理函数
void sigint_handler()
{
    printf("\n[!] 捕获到 SIGINT 信号 (Ctrl+C)，但我拒绝退出！\n");
    printf("[*] 提示：使用 'kill -9 %d' 可以强制终止我\n", getpid());
}

int main1(int argc, char* argv[])
{
    int count = 0;

    // 注册信号处理函数，捕获 SIGINT 但不退出
    signal(SIGINT, sigint_handler);

    printf("========================================\n");
    printf("  不死程序 (Immortal Process)\n");
    printf("========================================\n");
    printf("[*] 进程 ID: %d\n", getpid());
    printf("[*] 按 Ctrl+C 试试看，我不会退出！\n");
    printf("[*] 要终止我，请使用: kill -9 %d\n", getpid());
    printf("========================================\n\n");

    // 主循环：持续运行
    while(1)
    {
        count++;
        printf("[%d] 我还活着... (运行中: %d 秒)\n", getpid(), count);
        sleep(1);
    }

    return 0;
}
```

### 3.2 实现方式

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
========================================
  不死程序 (Immortal Process)
========================================
[*] 进程 ID: 42
[*] 按 Ctrl+C 试试看，我不会退出！
[*] 要终止我，请使用: kill -9 42
========================================

[42] 我还活着... (运行中: 1 秒)
[42] 我还活着... (运行中: 2 秒)
^C
[!] 捕获到 SIGINT 信号 (Ctrl+C)，但我拒绝退出！
[*] 提示：使用 'kill -9 42' 可以强制终止我
[42] 我还活着... (运行中: 3 秒)
```

**测试结果：**
- ✅ 按 Ctrl+C：程序捕获信号，显示提示，继续运行
- ✅ 使用 `kill -9 42`：程序被强制终止

## 六、技术总结

### 6.1 关键知识点

1. **信号处理机制**：`signal(int sig, void (*handler)())` 系统调用
2. **信号类型**：可捕获信号 vs 不可捕获信号
3. **进程控制**：getpid()、sleep()、while 循环
4. **裸机编程**：无标准库环境下的系统编程

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

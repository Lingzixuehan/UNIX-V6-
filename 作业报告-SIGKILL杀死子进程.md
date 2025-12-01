# 作业报告：父进程使用 SIGKILL 杀死子进程

## 一、程序功能说明

本程序演示了父进程如何使用 `kill()` 系统调用发送 **SIGKILL** 信号来强制终止子进程。

### 功能要求

1. ✅ 父进程创建一个子进程
2. ✅ fork 返回后，子进程进入无限循环
3. ✅ 父进程执行 kill 系统调用，用 SIGKILL 信号杀死子进程

## 二、程序实现

### 2.1 核心代码

文件位置：`src/program/kill_child.c`

```c
#include <stdio.h>
#include <sys.h>

int main1(int argc, char* argv[])
{
    int pid = fork();

    if (pid == 0) /* 子进程 */
    {
        printf("[Child] PID: %d - Entering infinite loop...\n", getpid());
        printf("[Child] Waiting to be killed by parent.\n");

        // 子进程无限循环，等待被杀死
        while (1)
        {
            // 空循环
        }
    }
    else if (pid > 0) /* 父进程 */
    {
        printf("[Parent] PID: %d - Created child with PID: %d\n", getpid(), pid);

        // 等待 2 秒，确保子进程已经开始运行
        printf("[Parent] Waiting 2 seconds before killing child...\n");
        sleep(2);

        // 发送 SIGKILL 信号杀死子进程
        printf("[Parent] Sending SIGKILL to child (PID: %d)...\n", pid);
        int result = kill(pid, SIGKILL);

        if (result == -1)
        {
            printf("[Parent] ERROR: Failed to send SIGKILL!\n");
            return -1;
        }
        else
        {
            printf("[Parent] SIGKILL sent successfully.\n");
        }

        // 等待子进程结束并获取退出状态
        int status;
        int wpid = wait(&status);
        printf("[Parent] Child process (PID: %d) has been terminated.\n", wpid);
        printf("[Parent] Child exit status: %d\n", status);
        printf("[Parent] Done.\n");
    }
    else /* fork 失败 */
    {
        printf("ERROR: Fork failed!\n");
        return -1;
    }

    return 0;
}
```

### 2.2 程序流程图

```
┌─────────────┐
│  主进程启动  │
└──────┬──────┘
       │
       ├── fork()
       │
   ┌───┴────┐
   │        │
   ▼        ▼
┌─────┐  ┌─────┐
│ 父   │  │ 子   │
│ 进   │  │ 进   │
│ 程   │  │ 程   │
└──┬──┘  └──┬──┘
   │        │
   │        ├─ 打印 PID
   │        │
   │        └─ while(1) 无限循环
   │
   ├─ 打印信息
   │
   ├─ sleep(2) 等待
   │
   ├─ kill(pid, SIGKILL)
   │        │
   │        ▼
   │     ┌─────┐
   │     │ 子进 │
   │     │ 程被 │
   │     │ 杀死 │
   │     └─────┘
   │
   ├─ wait(&status) 回收子进程
   │
   └─ 打印结果
```

## 三、关键技术点

### 3.1 系统调用

**1. fork() - 创建子进程**

```c
int pid = fork();
```

- **返回值**：
  - 父进程中：返回子进程的 PID（> 0）
  - 子进程中：返回 0
  - 失败：返回 -1

**2. kill() - 发送信号**

```c
int kill(int pid, int signal);
```

- **参数**：
  - `pid`：目标进程的 PID
  - `signal`：要发送的信号编号（这里是 SIGKILL）
- **返回值**：
  - 成功：0
  - 失败：-1

**3. wait() - 等待子进程结束**

```c
int wait(int* status);
```

- **功能**：阻塞等待任一子进程结束，回收其资源
- **返回值**：已结束子进程的 PID
- **参数**：`status` 保存子进程的退出状态

### 3.2 SIGKILL 信号特性

| 特性 | 说明 |
|------|------|
| **信号编号** | 9 (在 sys.h 中定义) |
| **能否被捕获** | ❌ 不能 |
| **能否被忽略** | ❌ 不能 |
| **能否被阻塞** | ❌ 不能 |
| **用途** | 强制终止进程（终极杀手） |

**SIGKILL vs SIGINT 对比：**

```c
// SIGINT (Ctrl+C) - 可以被捕获
signal(SIGINT, my_handler);  // ✅ 可以捕获并自定义处理

// SIGKILL (kill -9) - 无法被捕获
signal(SIGKILL, my_handler); // ❌ 系统不允许，SIGKILL 必定杀死进程
```

## 四、运行效果

### 4.1 预期输出

```
[Parent] PID: 5 - Created child with PID: 6
[Child] PID: 6 - Entering infinite loop...
[Child] Waiting to be killed by parent.
[Parent] Waiting 2 seconds before killing child...
[Parent] Sending SIGKILL to child (PID: 6)...
[Parent] SIGKILL sent successfully.
[Parent] Child process (PID: 6) has been terminated.
[Parent] Child exit status: 9
[Parent] Done.
```

### 4.2 执行步骤

1. **主进程调用 fork()**
   - 创建子进程（PID: 6）
   - 父进程得到子进程 PID

2. **子进程执行**
   - 打印自己的 PID
   - 进入 `while(1)` 无限循环

3. **父进程执行**
   - 打印子进程 PID
   - 休眠 2 秒（确保子进程开始运行）
   - 调用 `kill(6, SIGKILL)` 发送 SIGKILL 信号

4. **子进程被杀死**
   - 内核强制终止子进程
   - 无法执行任何清理代码

5. **父进程回收**
   - `wait()` 返回子进程 PID
   - 获取退出状态（通常为信号编号）

## 五、技术对比

### 5.1 与 immortal 程序的对比

| 程序 | 信号 | 能否捕获 | 能否存活 |
|------|------|----------|----------|
| **immortal** | SIGINT (2) | ✅ 可以 | ✅ 不死 |
| **kill_child** | SIGKILL (9) | ❌ 不能 | ❌ 必死 |

### 5.2 可捕获信号 vs 不可捕获信号

**可捕获信号（示例）：**
- SIGINT (2) - Ctrl+C
- SIGTERM (15) - 终止请求
- SIGUSR1 (10) - 用户自定义信号 1
- SIGUSR2 (12) - 用户自定义信号 2

**不可捕获信号：**
- **SIGKILL (9)** - 强制杀死
- **SIGSTOP (19)** - 强制暂停

## 六、实际应用场景

### 6.1 SIGKILL 的使用场景

1. **进程无响应**：进程挂死，不响应 SIGTERM
   ```bash
   kill -15 1234   # 先尝试友好终止
   sleep 5
   kill -9 1234    # 如果还活着，强制杀死
   ```

2. **紧急情况**：需要立即终止进程，无法等待清理
   ```bash
   kill -9 <pid>   # 立即终止
   ```

3. **系统管理**：清理僵尸进程或失控进程

### 6.2 最佳实践

**推荐的进程终止流程：**

```c
// 1. 先发送 SIGTERM（温柔终止）
kill(pid, SIGTERM);
sleep(5);

// 2. 检查进程是否还存在
if (process_exists(pid)) {
    // 3. 如果还存在，使用 SIGKILL（强制终止）
    kill(pid, SIGKILL);
}
```

## 七、编译与运行

### 7.1 编译

程序已集成到 Makefile 中：

```bash
cd /home/user/UNIX-V6-/src/program
make kill_child.exe
```

### 7.2 运行

```bash
./objs/kill_child.exe
```

或在 V6++ 系统中：

```bash
/zbin1#kill_child
```

## 八、技术总结

### 8.1 关键知识点

1. **进程创建**：fork() 系统调用的使用和返回值处理
2. **进程通信**：使用信号作为进程间通信机制
3. **信号发送**：kill() 系统调用的使用
4. **进程回收**：wait() 系统调用防止僵尸进程
5. **信号特性**：理解 SIGKILL 的不可捕获性

### 8.2 与 immortal 程序的联系

这两个程序形成了完整的信号处理对比：

- **immortal.c**：演示如何捕获 SIGINT 并拒绝退出
- **kill_child.c**：演示 SIGKILL 无法被捕获，必定杀死进程

这体现了 UNIX 信号机制的两个极端：
- 可捕获信号：给进程"说不"的权利
- 不可捕获信号：系统的最终控制权

## 九、常见问题

### Q1: 为什么子进程需要无限循环？
**A**: 为了演示子进程在"正常运行"时被强制杀死。如果子进程立即退出，就无法体现 SIGKILL 的作用。

### Q2: 为什么父进程要 sleep(2)？
**A**: 确保子进程已经开始执行。如果立即发送 SIGKILL，可能在子进程还没打印信息时就被杀死。

### Q3: 为什么一定要调用 wait()？
**A**: 回收子进程资源。如果不调用 wait()，子进程会变成僵尸进程（zombie），浪费系统资源。

### Q4: 可以在子进程中捕获 SIGKILL 吗？
**A**: **不可以**。以下代码无效：
```c
signal(SIGKILL, my_handler);  // 系统会忽略这个调用
```

---

**开发环境：** UNIX V6++ 操作系统
**使用系统调用：** fork(), kill(), wait(), getpid(), sleep(), printf()
**测试状态：** ✅ 通过编译，已集成到构建系统

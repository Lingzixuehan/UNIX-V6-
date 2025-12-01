# 选做实验：使用 kill -9 命令杀死进程

## 一、实验目的

1. 理解 `kill` 命令的使用方法
2. 掌握进程 PID 的获取方式
3. 观察 SIGKILL 信号的强制终止效果
4. 对比 SIGINT 和 SIGKILL 的区别

## 二、实验原理

### 2.1 kill 命令语法

```bash
kill [-signal] pid
```

**常用信号：**
- `kill pid` 或 `kill -15 pid` - 发送 SIGTERM（友好终止）
- `kill -9 pid` - 发送 SIGKILL（强制终止）
- `kill -2 pid` - 发送 SIGINT（等同于 Ctrl+C）

### 2.2 SIGKILL 特性

- **信号编号**：9
- **不可捕获**：进程无法捕获或忽略此信号
- **立即终止**：内核强制终止进程
- **无清理机会**：进程无法执行任何清理代码

## 三、实验步骤

### 实验 1：杀死普通进程

#### 步骤 1：启动测试进程

运行 immortal 程序（不死进程）：

```bash
cd /home/user/UNIX-V6-/src/program
./objs/immortal.exe &
```

程序输出：
```
Immortal Process (PID: 42)
Press Ctrl+C to test - I won't die!
Use 'kill -9 42' to kill me.
Starting...
```

记录进程 PID：**42**（示例）

#### 步骤 2：尝试使用 Ctrl+C（失败）

如果程序在前台运行，按 `Ctrl+C`：

**预期结果：**
```
^C
[!] Caught SIGINT (Ctrl+C), but I refuse to die!
Still alive (PID: 42)
```

**结论**：进程捕获了 SIGINT，拒绝退出 ❌

#### 步骤 3：使用 kill -9 强制终止

在另一个终端或将程序后台运行后，执行：

```bash
kill -9 42
```

**预期结果：**
```
Killed
```

进程被立即终止 ✅

#### 步骤 4：验证进程已终止

```bash
ps aux | grep immortal
```

或

```bash
ps -p 42
```

**预期结果：**
```
(无输出或显示进程不存在)
```

### 实验 2：对比不同信号

创建测试脚本 `test_signals.sh`：

```bash
#!/bin/bash

echo "=== 信号测试实验 ==="
echo ""

# 启动测试进程
echo "[1] 启动 immortal 进程..."
./objs/immortal.exe &
PID=$!
echo "进程 PID: $PID"
sleep 2

# 测试 SIGTERM (15)
echo ""
echo "[2] 发送 SIGTERM (kill -15)..."
kill -15 $PID
sleep 2

# 检查进程是否还存在
if ps -p $PID > /dev/null 2>&1; then
    echo "✓ 进程还活着（SIGTERM 可以被捕获）"
else
    echo "✗ 进程已终止"
    exit 0
fi

# 测试 SIGINT (2)
echo ""
echo "[3] 发送 SIGINT (kill -2)..."
kill -2 $PID
sleep 2

# 检查进程是否还存在
if ps -p $PID > /dev/null 2>&1; then
    echo "✓ 进程还活着（SIGINT 被捕获）"
else
    echo "✗ 进程已终止"
    exit 0
fi

# 测试 SIGKILL (9)
echo ""
echo "[4] 发送 SIGKILL (kill -9)..."
kill -9 $PID
sleep 1

# 检查进程是否还存在
if ps -p $PID > /dev/null 2>&1; then
    echo "✗ 进程还活着（不应该发生！）"
else
    echo "✓ 进程已被杀死（SIGKILL 无法被捕获）"
fi

echo ""
echo "=== 实验完成 ==="
```

运行脚本：

```bash
chmod +x test_signals.sh
./test_signals.sh
```

**预期输出：**
```
=== 信号测试实验 ===

[1] 启动 immortal 进程...
进程 PID: 123

[2] 发送 SIGTERM (kill -15)...
✓ 进程还活着（SIGTERM 可以被捕获）

[3] 发送 SIGINT (kill -2)...
✓ 进程还活着（SIGINT 被捕获）

[4] 发送 SIGKILL (kill -9)...
✓ 进程已被杀死（SIGKILL 无法被捕获）

=== 实验完成 ===
```

### 实验 3：交互式实验

创建一个简单的交互式测试程序 `target.c`：

```c
#include <stdio.h>
#include <sys.h>

void sigint_handler(int signo)
{
    printf("\n[Process %d] Caught signal %d, ignoring...\n",
           getpid(), signo);
    signal(signo, sigint_handler);
}

int main1(int argc, char* argv[])
{
    // 捕获多种信号
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);
    signal(SIGUSR1, sigint_handler);

    printf("===================================\n");
    printf("Target Process for Kill Experiment\n");
    printf("===================================\n");
    printf("PID: %d\n", getpid());
    printf("\nThis process ignores:\n");
    printf("  - SIGINT  (2)  - kill -2 %d\n", getpid());
    printf("  - SIGTERM (15) - kill -15 %d\n", getpid());
    printf("  - SIGUSR1 (10) - kill -10 %d\n", getpid());
    printf("\nBut CANNOT ignore:\n");
    printf("  - SIGKILL (9)  - kill -9 %d  <-- Use this!\n", getpid());
    printf("\nWaiting for signals...\n\n");

    while(1)
    {
        sleep(5);
        printf("[PID %d] Still running...\n", getpid());
    }

    return 0;
}
```

## 四、实验记录表

### 实验记录模板

| 实验序号 | 目标进程 | 进程 PID | 使用命令 | 信号类型 | 进程状态 | 备注 |
|---------|---------|---------|---------|---------|---------|------|
| 1 | immortal | 42 | kill -2 42 | SIGINT | 存活 | 信号被捕获 |
| 2 | immortal | 42 | kill -15 42 | SIGTERM | 存活 | 信号被捕获 |
| 3 | immortal | 42 | kill -9 42 | SIGKILL | 终止 | 强制杀死 ✓ |
| 4 | kill_child | 55 | kill -9 55 | SIGKILL | 终止 | 立即终止 ✓ |

### 实际实验记录

**实验时间：** _______________

**操作系统：** UNIX V6++

| 实验序号 | 目标进程 | 进程 PID | 使用命令 | 观察结果 |
|---------|---------|---------|---------|---------|
| 1 | | | | |
| 2 | | | | |
| 3 | | | | |
| 4 | | | | |

## 五、实验现象分析

### 5.1 可捕获信号的表现

测试 SIGINT (Ctrl+C 或 kill -2)：

```bash
# 启动 immortal
./objs/immortal.exe

# 在另一个终端
kill -2 <PID>
```

**观察到的现象：**
- 进程输出捕获信号的提示信息
- 进程继续运行，未终止
- 可以多次发送，进程始终存活

**原因分析：**
- 程序使用 `signal(SIGINT, handler)` 注册了处理函数
- 信号处理函数中重新注册了信号（UNIX V6 特性）
- 进程有权拒绝响应该信号

### 5.2 不可捕获信号的表现

测试 SIGKILL (kill -9)：

```bash
kill -9 <PID>
```

**观察到的现象：**
- 进程立即终止
- 没有任何输出或清理动作
- 无法被捕获或阻止

**原因分析：**
- SIGKILL 由内核直接处理
- 进程没有机会执行任何代码
- 这是操作系统的最终控制权

## 六、对比实验

### 实验对照表

| 特性 | SIGINT (kill -2) | SIGTERM (kill -15) | SIGKILL (kill -9) |
|------|------------------|-------------------|-------------------|
| 能否被捕获 | ✓ 可以 | ✓ 可以 | ✗ 不能 |
| 能否被忽略 | ✓ 可以 | ✓ 可以 | ✗ 不能 |
| 能否被阻塞 | ✓ 可以 | ✓ 可以 | ✗ 不能 |
| 进程可清理 | ✓ 可以 | ✓ 可以 | ✗ 不能 |
| 终止保证 | ✗ 不保证 | ✗ 不保证 | ✓ 必定终止 |
| 使用场景 | 交互式中断 | 友好终止 | 强制终止 |

### 杀死 immortal 进程的方法对比

```bash
# 方法 1：Ctrl+C (失败)
^C
结果：进程捕获信号，拒绝退出 ❌

# 方法 2：kill -2 (SIGINT，失败)
kill -2 <PID>
结果：同 Ctrl+C，被捕获 ❌

# 方法 3：kill -15 (SIGTERM，失败)
kill -15 <PID>
结果：可以被捕获，进程仍存活 ❌

# 方法 4：kill -9 (SIGKILL，成功)
kill -9 <PID>
结果：强制终止 ✅
```

## 七、实验扩展

### 扩展 1：查看进程的信号掩码

```bash
# 查看进程的信号状态
cat /proc/<PID>/status | grep Sig
```

**输出示例：**
```
SigQ:    0/31787
SigPnd:  0000000000000000
SigBlk:  0000000000000000
SigIgn:  0000000000000000
SigCgt:  0000000000000004
```

### 扩展 2：批量杀死进程

```bash
# 杀死所有 immortal 进程
killall -9 immortal

# 或使用 pkill
pkill -9 -f immortal
```

### 扩展 3：编写自动化测试

创建 Python 测试脚本：

```python
#!/usr/bin/env python3
import subprocess
import time
import signal
import os

def test_kill_signals():
    print("=== 自动化信号测试 ===\n")

    # 启动测试进程
    print("[1] 启动测试进程...")
    proc = subprocess.Popen(['./objs/immortal.exe'])
    pid = proc.pid
    print(f"进程 PID: {pid}\n")
    time.sleep(2)

    # 测试 SIGINT
    print(f"[2] 发送 SIGINT 到进程 {pid}...")
    os.kill(pid, signal.SIGINT)
    time.sleep(1)

    if proc.poll() is None:
        print("✓ 进程存活（SIGINT 被捕获）\n")
    else:
        print("✗ 进程已终止\n")
        return

    # 测试 SIGKILL
    print(f"[3] 发送 SIGKILL 到进程 {pid}...")
    os.kill(pid, signal.SIGKILL)
    time.sleep(1)

    if proc.poll() is None:
        print("✗ 进程仍存活（不应该！）\n")
    else:
        print("✓ 进程已终止（SIGKILL 生效）\n")

    print("=== 测试完成 ===")

if __name__ == '__main__':
    test_kill_signals()
```

## 八、实验问题思考

### 问题 1
**Q:** 为什么 immortal 进程可以忽略 Ctrl+C 但无法忽略 kill -9？

**A:** _______________________________________________

### 问题 2
**Q:** 如果进程正在写入文件，使用 kill -9 会有什么后果？

**A:** _______________________________________________

### 问题 3
**Q:** 在什么情况下应该使用 kill -9？什么情况下应该使用 kill -15？

**A:** _______________________________________________

### 问题 4
**Q:** 为什么操作系统要保留 SIGKILL 这种不可捕获的信号？

**A:** _______________________________________________

## 九、实验总结

### 关键发现

1. **SIGKILL 的绝对性**
   - 任何进程都无法拒绝 SIGKILL
   - 这是操作系统保留的最终控制权

2. **信号的层次性**
   - SIGINT/SIGTERM：友好请求（可拒绝）
   - SIGKILL/SIGSTOP：强制命令（不可拒绝）

3. **实际应用建议**
   - 优先使用 SIGTERM (kill -15)
   - 等待 5-10 秒后再使用 SIGKILL (kill -9)
   - 给予进程清理资源的机会

### 实验结论

通过本实验验证了：
- ✓ kill -9 可以强制终止任何进程
- ✓ SIGKILL 无法被捕获或忽略
- ✓ immortal 程序可以抵御 SIGINT 但无法抵御 SIGKILL
- ✓ 不同信号有不同的终止强度

---

**实验人员：** _______________

**实验日期：** _______________

**实验评分：** _______________

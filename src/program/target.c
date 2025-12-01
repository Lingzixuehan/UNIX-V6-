#include <stdio.h>
#include <sys.h>

/*
 * target - kill 实验的目标进程
 * 功能：捕获多种信号，用于演示 kill 命令的效果
 *
 * 捕获的信号：SIGINT, SIGTERM, SIGUSR1
 * 无法捕获：SIGKILL (9)
 */

void signal_handler(int signo)
{
    // 根据不同的信号显示不同的消息
    if (signo == SIGINT)
    {
        printf("\n[PID %d] Caught SIGINT (2) - Ctrl+C or kill -2\n", getpid());
        printf("[PID %d] Ignoring and re-registering handler...\n", getpid());
    }
    else if (signo == SIGTERM)
    {
        printf("\n[PID %d] Caught SIGTERM (15) - kill -15\n", getpid());
        printf("[PID %d] Ignoring and re-registering handler...\n", getpid());
    }
    else if (signo == SIGUSR1)
    {
        printf("\n[PID %d] Caught SIGUSR1 (10) - kill -10\n", getpid());
        printf("[PID %d] Ignoring and re-registering handler...\n", getpid());
    }
    else
    {
        printf("\n[PID %d] Caught signal %d\n", getpid(), signo);
    }

    // 重新注册信号处理（UNIX V6 特性）
    signal(signo, signal_handler);
}

int main1(int argc, char* argv[])
{
    int pid = getpid();

    // 注册多个信号处理函数
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGUSR1, signal_handler);

    printf("================================================\n");
    printf("  Target Process for Kill Command Experiment\n");
    printf("================================================\n");
    printf("Process ID (PID): %d\n\n", pid);

    printf("This process can CATCH and IGNORE:\n");
    printf("  [✓] SIGINT  (2)  - Try: kill -2 %d\n", pid);
    printf("  [✓] SIGTERM (15) - Try: kill -15 %d\n", pid);
    printf("  [✓] SIGUSR1 (10) - Try: kill -10 %d\n\n", pid);

    printf("This process CANNOT catch or ignore:\n");
    printf("  [✗] SIGKILL (9)  - Use: kill -9 %d\n", pid);
    printf("  [✗] SIGSTOP (19) - Use: kill -19 %d\n\n", pid);

    printf("================================================\n");
    printf("Process is running...\n");
    printf("Press Ctrl+C or use kill commands to test.\n");
    printf("Use 'kill -9 %d' to force terminate.\n", pid);
    printf("================================================\n\n");

    int counter = 0;
    while(1)
    {
        sleep(5);
        counter++;
        printf("[PID %d] Still alive... (uptime: %d seconds)\n",
               pid, counter * 5);
    }

    return 0;
}

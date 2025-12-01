#include <stdio.h>
#include <sys.h>

/*
 * kill_child - 父进程使用 SIGKILL 杀死子进程的演示程序
 * 功能：
 *   1. 父进程创建一个子进程
 *   2. 子进程进入无限循环
 *   3. 父进程使用 kill() 系统调用发送 SIGKILL 信号杀死子进程
 *
 * 关键点：SIGKILL 信号无法被捕获或忽略，必定杀死进程
 */

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
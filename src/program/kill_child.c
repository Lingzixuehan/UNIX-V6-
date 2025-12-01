#include <stdio.h>
#include <sys.h>

int main1(int argc, char* argv[])
{
    int pid = fork();

    if (pid == 0) /* 子进程 */
    {
        printf("Child process (PID: %d) started, entering infinite loop.\n", getpid());
        while (1)
        {
            // 子进程无限循环，等待被杀死
        }
    }
    else if (pid > 0) /* 父进程 */
    {
        printf("Parent process (PID: %d) created child with PID: %d.\n", getpid(), pid);
        sleep(2); // 等待2秒，确保子进程已经运行
        printf("Parent process sending SIGKILL to child (PID: %d).\n", pid);
        int ans = kill(pid, SIGKILL);
        if (ans == -1)
        {
            printf("Failed to send SIGKILL to child.\n");
        }
        else
        {
            printf("SIGKILL sent successfully.\n");
        }

        int status;
        wait(&status);
        printf("Parent: Child process (PID: %d) exited with status [%d].\n", pid, status);
    }
    else /* fork 失败 */
    {
        printf("Fork failed!\n");
        return -1;
    }

    return 0;
}
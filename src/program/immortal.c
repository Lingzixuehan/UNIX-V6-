#include <stdio.h>
#include <sys.h>

/*
 * immortal - 一个忽略 SIGINT 信号的程序
 * 功能：演示信号处理，按 Ctrl+C 无法杀死此进程
 * 使用方法：
 *   - 运行程序后，按 Ctrl+C 将看到提示信息，但程序不会退出
 *   - 需要使用 kill -9 <pid> 或 SIGKILL 信号才能强制终止
 */

// SIGINT 信号处理函数 - 必须接受 int 参数
void sigint_handler(int signo)
{
	if (signo == SIGINT)
	{
		printf("\n[!] Caught SIGINT (Ctrl+C), but I refuse to die!\n");
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

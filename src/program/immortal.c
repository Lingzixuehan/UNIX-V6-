#include <stdio.h>
#include <sys.h>

/*
 * immortal - 一个忽略 SIGINT 信号的程序
 * 功能：演示信号处理，按 Ctrl+C 无法杀死此进程
 * 使用方法：
 *   - 运行程序后，按 Ctrl+C 将看到提示信息，但程序不会退出
 *   - 需要使用 kill -9 <pid> 或 SIGKILL 信号才能强制终止
 */

// SIGINT 信号处理函数
void sigint_handler()
{
	printf("\n[!] 捕获到 SIGINT 信号 (Ctrl+C)，但我拒绝退出！\n");
	printf("[*] 提示：使用 'kill -9 %d' 可以强制终止我\n", getpid());
}

int main1(int argc, char* argv[])
{
	int count = 0;

	// 方式1：注册信号处理函数，捕获但不退出
	signal(SIGINT, sigint_handler);

	// 方式2（可选）：完全忽略 SIGINT 信号
	// signal(SIGINT, SIG_IGN);

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

		// 每10秒提示一次
		if (count % 10 == 0)
		{
			printf("\n[*] 提示：尝试按 Ctrl+C，你会发现我不会死掉！\n\n");
		}
	}

	return 0;
}

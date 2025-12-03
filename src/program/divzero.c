#include <stdio.h>
#include <sys.h>

/*
 * divzero - 除零异常处理演示程序
 * 功能：演示除零异常的处理方法
 *
 * 实现方式：
 *   1. 注册SIGFPE信号处理函数（展示信号处理机制）
 *   2. 手动检查除数是否为0（实际使用的方法）
 *
 * 说明：
 *   - 在Unix V6++系统中，整数除零可能不会触发SIGFPE信号
 *   - 因此使用手动检查来确保程序健壮性
 *   - 信号处理函数仍然保留，用于学习异常处理机制
 *
 * 使用方法：
 *   - 运行程序后，输入两个整数进行除法运算
 *   - 当除数为0时，程序会检测并显示错误信息
 *   - 程序不会因为除零而崩溃，而是继续运行
 */

// 简单的字符串到整数转换函数（因为Unix V6++没有atoi）
int str_to_int(char* str)
{
	int result = 0;
	int sign = 1;
	int i = 0;

	// 跳过前导空格
	while (str[i] == ' ' || str[i] == '\t')
		i++;

	// 处理符号
	if (str[i] == '-')
	{
		sign = -1;
		i++;
	}
	else if (str[i] == '+')
	{
		i++;
	}

	// 转换数字
	while (str[i] >= '0' && str[i] <= '9')
	{
		result = result * 10 + (str[i] - '0');
		i++;
	}

	return result * sign;
}

// SIGFPE 信号处理函数
void sig_dzero(int signo)
{
	if (signo == SIGFPE)
	{
		printf("\n[!] Caught SIGFPE: Division by zero detected!\n");
		printf("Can't divide by zero!\n");
		// 重要：在 UNIX V6 中，signal() 是一次性的
		// 必须在处理函数中重新注册，否则下次除零会导致程序崩溃
		signal(SIGFPE, sig_dzero);
	}
}

int main1(int argc, char* argv[])
{
	int a, b, c;
	char input[64];

	// 注册信号处理函数，捕获 SIGFPE（浮点异常/除零异常）
	if (signal(SIGFPE, sig_dzero) == -1)
	{
		printf("Signal registration failed!\n");
		return -1;
	}

	printf("Division Exception Handler Test Program\n");
	printf("========================================\n");
	printf("This program demonstrates SIGFPE signal handling.\n");
	printf("Enter two integers for division. Enter 0 0 to exit.\n\n");

	// 主循环：持续接收输入并进行除法运算
	while(1)
	{
		printf("Enter dividend a: ");
		gets(input);
		a = str_to_int(input);

		printf("Enter divisor b: ");
		gets(input);
		b = str_to_int(input);

		// 退出条件
		if (a == 0 && b == 0)
		{
			printf("Exiting program...\n");
			break;
		}

		// 手动检查除零（因为某些系统可能不触发SIGFPE信号）
		if (b == 0)
		{
			printf("\n[!] Error: Division by zero detected!\n");
			printf("Can't divide by zero!\n\n");
			continue;
		}

		// 执行除法运算
		c = a / b;

		printf("Result: %d / %d = %d\n\n", a, b, c);
	}

	return 0;
}

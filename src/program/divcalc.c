#include <stdio.h>
#include <sys.h>

/*
 * divcalc - Unix V6++ 除法计算器
 * 功能：提供完整的除法运算功能，包括商和余数
 *
 * 使用方法：
 *   1. 命令行模式：divcalc <被除数> <除数>
 *      例如：divcalc 17 5
 *      输出：17 / 5 = 3 remainder 2
 *
 *   2. 交互模式：divcalc
 *      运行后按提示输入两个数字
 *      输入 q 退出程序
 *
 * 特性：
 *   - 支持正数和负数
 *   - 自动检测除零错误
 *   - 同时显示商和余数
 *   - 提供详细的运算信息
 */

// 字符串到整数转换函数（Unix V6++没有标准atoi）
int str_to_int(char* str)
{
	int result = 0;
	int sign = 1;
	int i = 0;

	// 跳过前导空格和制表符
	while (str[i] == ' ' || str[i] == '\t')
		i++;

	// 处理正负号
	if (str[i] == '-')
	{
		sign = -1;
		i++;
	}
	else if (str[i] == '+')
	{
		i++;
	}

	// 转换数字字符
	while (str[i] >= '0' && str[i] <= '9')
	{
		result = result * 10 + (str[i] - '0');
		i++;
	}

	return result * sign;
}

// 执行除法运算并显示结果
void perform_division(int dividend, int divisor)
{
	int quotient, remainder;

	// 检查除零错误
	if (divisor == 0)
	{
		printf("Error: Division by zero is not allowed!\n");
		printf("Divisor cannot be 0.\n\n");
		return;
	}

	// 执行除法和取模运算
	quotient = dividend / divisor;
	remainder = dividend % divisor;

	// 显示运算结果
	printf("\n");
	printf("================================\n");
	printf("  Division Calculation Result\n");
	printf("================================\n");
	printf("Dividend  : %d\n", dividend);
	printf("Divisor   : %d\n", divisor);
	printf("Quotient  : %d\n", quotient);
	printf("Remainder : %d\n", remainder);
	printf("--------------------------------\n");
	printf("Formula   : %d = %d * %d + %d\n", dividend, divisor, quotient, remainder);
	printf("Expression: %d / %d = %d ... %d\n", dividend, divisor, quotient, remainder);
	printf("================================\n\n");
}

// 交互式模式
void interactive_mode()
{
	char input[64];
	int dividend, divisor;

	printf("\n");
	printf("====================================\n");
	printf("  Unix V6++ Division Calculator\n");
	printf("====================================\n");
	printf("Interactive Mode\n");
	printf("Enter 'q' or '0 0' to exit\n");
	printf("====================================\n\n");

	while(1)
	{
		// 输入被除数
		printf("Enter dividend (or 'q' to quit): ");
		gets(input);

		// 检查退出命令
		if (input[0] == 'q' || input[0] == 'Q')
		{
			printf("Thank you for using the calculator!\n");
			break;
		}

		dividend = str_to_int(input);

		// 输入除数
		printf("Enter divisor: ");
		gets(input);

		// 检查退出命令
		if (input[0] == 'q' || input[0] == 'Q')
		{
			printf("Thank you for using the calculator!\n");
			break;
		}

		divisor = str_to_int(input);

		// 退出条件：0 0
		if (dividend == 0 && divisor == 0)
		{
			printf("Thank you for using the calculator!\n");
			break;
		}

		// 执行除法运算
		perform_division(dividend, divisor);
	}
}

// 命令行模式
void command_line_mode(int dividend, int divisor)
{
	printf("\n");
	printf("====================================\n");
	printf("  Unix V6++ Division Calculator\n");
	printf("====================================\n");
	printf("Command Line Mode\n");
	printf("====================================\n");

	perform_division(dividend, divisor);
}

// 显示使用帮助
void show_usage(char* program_name)
{
	printf("\n");
	printf("====================================\n");
	printf("  Unix V6++ Division Calculator\n");
	printf("====================================\n");
	printf("\n");
	printf("USAGE:\n");
	printf("  %s                    - Interactive mode\n", program_name);
	printf("  %s <dividend> <divisor> - Command line mode\n", program_name);
	printf("\n");
	printf("EXAMPLES:\n");
	printf("  %s                    - Start interactive mode\n", program_name);
	printf("  %s 17 5               - Calculate 17 / 5\n", program_name);
	printf("  %s 100 7              - Calculate 100 / 7\n", program_name);
	printf("  %s -20 3              - Calculate -20 / 3\n", program_name);
	printf("\n");
	printf("FEATURES:\n");
	printf("  - Supports positive and negative integers\n");
	printf("  - Displays both quotient and remainder\n");
	printf("  - Automatic division by zero detection\n");
	printf("  - Shows detailed calculation formula\n");
	printf("\n");
	printf("====================================\n\n");
}

// 主程序入口
int main1(int argc, char* argv[])
{
	int dividend, divisor;

	// 根据参数数量决定运行模式
	if (argc == 1)
	{
		// 无参数：交互模式
		interactive_mode();
	}
	else if (argc == 3)
	{
		// 两个参数：命令行模式
		dividend = str_to_int(argv[1]);
		divisor = str_to_int(argv[2]);
		command_line_mode(dividend, divisor);
	}
	else
	{
		// 参数错误：显示帮助信息
		printf("Error: Invalid number of arguments!\n");
		show_usage(argv[0]);
		return -1;
	}

	return 0;
}

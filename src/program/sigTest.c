#include <stdio.h>
#include <sys.h>

void sig_handler(int signo)
{
	if (signo == SIGINT)
	{
		printf("received SIGINT\n");
	}
}

/*test for ctrl-c*/
int main1(int argc, char* argv[])
{
		if (signal(SIGINT, sig_handler) == -1)
		{
			printf("signal error\n");
			return -1;
		}
		printf("Getting into sleep.\n");

		while(1)
		{
			sleep(50);
			printf("Wakeup.\n");
		}

		return 0; /* Should not reach here */
}

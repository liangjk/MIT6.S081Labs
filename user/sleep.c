#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
	int ticks;

	if (argc <= 1)
	{
		fprintf(2, "sleep: no parameter found\n");
		exit(0);
	}

	ticks = atoi(argv[1]);

	sleep(ticks);
	exit(0);
}
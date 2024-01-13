#include "kernel/types.h"
#include "user/user.h"

void primes(int numbers[], int n)
{
	int pipefd[2], status, pid, i, ccode;
	status = pipe(pipefd);
	if (status < 0)
	{
		pid = getpid();
		fprintf(2, "primes: process %d failed to create pipe\n", pid);
		exit(-1);
	}
	pid = fork();
	if (pid == 0)
	{
		close(pipefd[1]);
		n = 0;
		pid = getpid();
		while (1)
		{
			status = read(pipefd[0], numbers + n, 4);
			if (status < 0)
			{
				fprintf(2, "primes: process %d failed to read from pipe\n", pid);
				exit(-1);
			}
			else if (status < 4)
				break;
			++n;
		}
		close(pipefd[0]);
		if (n)
			primes(numbers, n);
	}
	else
	{
		close(pipefd[0]);
		printf("prime %d\n", numbers[0]);
		for (i = 1; i < n; ++i)
		{
			if (numbers[i] % numbers[0] == 0)
				continue;
			status = write(pipefd[1], numbers + i, 4);
			if (status < 0)
			{
				fprintf(2, "primes: process %d failed to write to pipe\n", pid);
				exit(-1);
			}
		}
		close(pipefd[1]);
		status = wait(&ccode);
		if (status < 0 || ccode < 0)
		{
			fprintf(2, "primes: process %d's child had something wrong\n", pid);
			exit(-1);
		}
		exit(0);
	}
}

int main(int argc, char *argv[])
{
	int numbers[35], n, i;
	n = 0;
	for (i = 2; i <= 35; ++i)
		numbers[n++] = i;
	primes(numbers, n);
	return 0;
}
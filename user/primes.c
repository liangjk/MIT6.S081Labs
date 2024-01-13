#include "kernel/types.h"
#include "user/user.h"

void primes(int pipein)
{
	int pipefd[2], status, number, bytes, pid, pm;
	status = 0;
	pm = 1;
	pid = getpid();
	for (;;)
	{
		bytes = read(pipein, &number, 4);
		if (bytes < 0)
		{
			fprintf(2, "primes: process %d failed to read from pipe\n", pid);
			exit(-2);
		}
		else if (bytes < 4)
			break;
		switch (status)
		{
		case 0:
			pm = number;
			printf("prime %d\n", pm);
			++status;
			break;
		case 1:
			if (pipe(pipefd) < 0)
			{
				fprintf(2, "primes: process %d failed to create pipe\n", pid);
				exit(-1);
			}
			pid = fork();
			if (pid == 0)
			{
				close(pipefd[1]);
				primes(pipefd[0]);
				exit(0);
			}
			else
			{
				close(pipefd[0]);
				pid = getpid();
			}
			++status;
		case 2:
			if (number % pm == 0)
				break;
			if (write(pipefd[1], &number, 4) < 0)
				fprintf(2, "primes: process %d failed to write to pipe\n", pid);
		}
	}
	close(pipein);
	close(pipefd[1]);
	if (status > 1 && (wait(&status) < 0 || status < 0))
	{
		fprintf(2, "primes: process %d's child had something wrong\n", pid);
		exit(-3);
	}
}

int main(int argc, char *argv[])
{
	int i, pipefd[2], pid, status;
	if (pipe(pipefd) < 0)
	{
		pid = getpid();
		fprintf(2, "primes: process %d failed to create pipe\n", pid);
		exit(-1);
	}
	pid = fork();
	if (pid == 0)
	{
		close(pipefd[1]);
		primes(pipefd[0]);
		exit(0);
	}
	else
	{
		close(pipefd[0]);
		pid = getpid();
		for (i = 2; i <= 35; ++i)
			if (write(pipefd[1], &i, 4) < 0)
			{
				fprintf(2, "primes: process %d failed to write to pipe\n", pid);
				break;
			}
		close(pipefd[1]);
		if (wait(&status) < 0 || status < 0)
		{
			fprintf(2, "primes: process %d's child had something wrong\n", pid);
			exit(-3);
		}
		exit(0);
	}
}
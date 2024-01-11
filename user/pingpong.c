#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
	int pipefd[2], pid, status, content, ccode;

	status = pipe(pipefd);
	if (status < 0)
	{
		fprintf(2, "pingpong: failed to create pipe\n");
		exit(-1);
	}

	pid = fork();
	if (pid == 0)
	{
		status = read(pipefd[0], &content, 4);
		if (status < 4)
		{
			fprintf(2, "pingpong: child failed to read from pipe\n");
			exit(-2);
		}
		pid = getpid();
		printf("%d: received ping\n", pid);
		// printf("%d: received %d\n", pid, content);
		status = write(pipefd[1], &pid, 4);
		if (status < 0)
		{
			fprintf(2, "pingpong: child failed to write to pipe\n");
			exit(-2);
		}
		exit(0);
	}
	else
	{
		pid = getpid();
		status = write(pipefd[1], &pid, 4);
		if (status < 0)
		{
			fprintf(2, "pingpong: parent failed to write to pipe\n");
			exit(-2);
		}
		status = wait(&ccode);
		if (status < 0 || ccode < 0)
		{
			fprintf(2, "pingpong: child had something wrong\n");
			exit(-3);
		}
		status = read(pipefd[0], &content, 4);
		if (status < 0)
		{
			fprintf(2, "pingpong: parent failed to read from pipe\n");
			exit(-2);
		}
		printf("%d: received pong\n", pid);
		// printf("%d: received %d\n", pid, content);
		exit(0);
	}
}
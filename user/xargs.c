#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
	char buf[512];
	char *cargv[MAXARG + 1];
	int len, i, pid, status;
	if (argc < 2)
	{
		fprintf(2, "xargs: too few arguments\n");
		exit(-1);
	}
	for (i = 1; i < argc; ++i)
		cargv[i - 1] = argv[i];
	cargv[argc - 1] = buf;
	cargv[argc] = 0;
	while (1)
	{
		gets(buf, 512);
		len = strlen(buf);
		if (len == 0)
			break;
		if (buf[len - 1] != '\n' && buf[len - 1] != '\r')
		{
			fprintf(2, "xargs: cannot read whole line\n");
			exit(-2);
		}
		buf[len - 1] = 0;
		pid = fork();
		if (pid == 0)
			exec(cargv[0], cargv);
		else
			wait(&status);
	}
	exit(0);
}
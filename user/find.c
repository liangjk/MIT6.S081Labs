#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

char *
fmtname(char *path)
{
	char *p;
	for (p = path + strlen(path); p >= path && *p != '/'; p--)
		;
	p++;
	return p;
}

void find(char path[], const char *filename, uint pathLimit)
{
	char *p;
	int fd;
	struct dirent de;
	struct stat st;
	if ((fd = open(path, O_RDONLY)) < 0)
	{
		fprintf(2, "find: cannot open %s\n", path);
		return;
	}
	if (fstat(fd, &st) < 0)
	{
		fprintf(2, "find: cannot stat %s\n", path);
		close(fd);
		return;
	}
	switch (st.type)
	{
	case T_DEVICE:
	case T_FILE:
		if (strcmp(filename, fmtname(path)) == 0)
			printf("%s\n", path);
		break;
	case T_DIR:
		if (strlen(path) + 1 + DIRSIZ + 1 > pathLimit)
		{
			printf("find: path too long\n");
			break;
		}
		p = path + strlen(path);
		*p++ = '/';
		while (read(fd, &de, sizeof(de)) == sizeof(de))
		{
			if (de.inum == 0)
				continue;
			if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
				continue;
			memmove(p, de.name, DIRSIZ);
			p[DIRSIZ] = 0;
			find(path, filename, pathLimit);
		}
		break;
	}
	close(fd);
}

int main(int argc, char *argv[])
{
	char buf[512];
	if (argc < 3)
	{
		fprintf(2, "find: too few arguments\n");
		exit(-1);
	}
	strcpy(buf, argv[1]);
	find(buf, argv[2], 512);
	exit(0);
}
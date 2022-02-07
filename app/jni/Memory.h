#ifndef MEMORY_H
#define MEMORY_H

#include <dirent.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#define MEM_MAPPED 262144
#define MEM_PRIVATE 131072

using namespace std;

typedef struct
{
	unsigned long long baseAddress;
	void *next_ptr;
	int moduleSize;
	char *moduleName;

} ModuleListEntry, *PModuleListEntry;

//获取进程pid
pid_t find_pid(const char *process_name)
{
	int id;
	pid_t pid = -1;
	DIR *dir;
	FILE *fp;
	char filename[32];
	char cmdline[256];

	struct dirent *entry;
	if (process_name == NULL)
	{
		return -1;
	}
	dir = opendir("/proc");
	if (dir == NULL)
	{
		return -1;
	}
	while ((entry = readdir(dir)) != NULL)
	{
		id = atoi(entry->d_name);
		if (id != 0)
		{
			sprintf(filename, "/proc/%d/cmdline", id);
			fp = fopen(filename, "r");
			if (fp)
			{
				fgets(cmdline, sizeof(cmdline), fp);
				fclose(fp);

				if (strcmp(process_name, cmdline) == 0)
				{
					/* process found */
					pid = id;
					break;
				}
			}
		}
	}

	closedir(dir);
	return pid;
}

//获取mmap内存,在Android5.1可以当作共享内存通信
char *get_mmap_memory(const char *file, int size)
{
	int MAP_SIZE = size;
	//打开文件，fd文件句柄
	int fd = open(file, O_RDWR | O_CREAT, 0644);

	if (fd < 0)
	{
		printf("Can't open %s\n", file);
		return nullptr;
	}
	ftruncate(fd, MAP_SIZE);

	char *mapped = (char *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (mapped == MAP_FAILED)
	{
		printf("File mmap failed\n");
		return nullptr;
	}

	//映射结束，关闭文件
	close(fd);

	return mapped;
}

//取消mmap内存映射
void close_mmap_memory(char *mapped, int size)
{
	munmap(mapped, size);
}

//根据pid获取模块基址
unsigned int get_module_base(int target_pid, const char *module_name)
{
	FILE *fp;
	unsigned int addr = 0;
	char filename[32], buffer[1024];
	snprintf(filename, sizeof(filename), "/proc/%d/maps", target_pid);
	fp = fopen(filename, "rt");
	if (fp != nullptr)
	{
		while (fgets(buffer, sizeof(buffer), fp))
		{
			if (strstr(buffer, module_name))
			{
				addr = (uintptr_t)strtoul(buffer, nullptr, 16);
				break;
			}
		}
		fclose(fp);
	}
	return addr;
}

//mem方式读取内存
//pid 目标进程pid
//lpAddress 目标进程内存地址
//buffer 读入数据的缓冲区
//size 读入数据大小
int ReadProcessMemory(int pid, void *lpAddress, void *buffer, int size)
{
	char processpath[100];
	sprintf(processpath, "/proc/%d/mem", pid);
	int fd = open(processpath, O_RDONLY);
	lseek64(fd, (uintptr_t)lpAddress, SEEK_SET);

	int bread = read(fd, buffer, size);

	if (bread == -1)
	{
		// printf("ReadProcessMemory fail\n");
		bread = 0;
	}
	close(fd);
	return bread;
}

/* Write NLONG 4 byte words from BUF into PID starting
   at address POS.  Calling process must be attached to PID. */
//参数和上面读内存一样
int WriteProcessMemory(int pid, void *lpAddress, void *buffer, int size)
{
	int result = ptrace(PTRACE_ATTACH, pid, NULL, NULL);
	if (result != 0)
	{
		printf("ptrace error(%d)!\n", result);
		return -1;
	}
	int status;
	pid = wait(&status);

	unsigned long *p;
	int i;

	for (p = (unsigned long *)buffer, i = 0; i < size; p++, i++)
	{
		if (0 > ptrace(PTRACE_POKETEXT, pid, (void *)((unsigned long)lpAddress + (i * 4)), (void *)*p))
		{
			result = -1;
			break;
		}
	}
	result = ptrace(PTRACE_DETACH, pid, 0, 0);
	return result;
}

//获取进程所有模块信息
//这里部分来源ce-server
PModuleListEntry get_process_map(int pid)
{
	int max = 64;
	char mapfile[255];
	FILE *f = NULL;
	snprintf(mapfile, 255, "/proc/%d/maps", pid);

	f = fopen(mapfile, "r");

	if (f)
	{
		char s[512];
		memset(s, 0, 512);
		PModuleListEntry _ml = nullptr;
		PModuleListEntry ret = nullptr;
		while (fgets(s, 511, f)) //read a line into s
		{
			char *currentModule;
			unsigned long long start, stop;
			char memoryrange[64], protectionstring[32], modulepath[511];
			uint32_t magic;

			modulepath[0] = '\0';
			memset(modulepath, 0, 511);

			sscanf(s, "%llx-%llx %s %*s %*s %*s %[^\t\n]\n", &start, &stop, protectionstring, modulepath);

			if (strchr(protectionstring, 's'))
				continue;
			int i;
			if (strcmp(modulepath, "[heap]") == 0) //not static enough to mark as a 'module'
				continue;

			// printf("%s\n", modulepath);

			if (strcmp(modulepath, "[vdso]") != 0) //temporary patch as to not rename vdso, because it is treated differently by the ce symbol loader
			{
				for (i = 0; modulepath[i]; i++) //strip square brackets from the name (conflicts with pointer notations)
				{
					if ((modulepath[i] == '[') || (modulepath[i] == ']'))
						modulepath[i] = '_';
				}
			}

			i = ReadProcessMemory(pid, (void *)start, &magic, 4);
			if (i == 0)
			{
				// printf("%s is unreadable(%llx)\n", modulepath, start);
				continue; //unreadable
			}

			PModuleListEntry ml = (PModuleListEntry)malloc(sizeof(ModuleListEntry));
			if (ret == nullptr)
			{
				ret = ml;
			}
			else
			{
				_ml->next_ptr = ml;
				_ml = ml;
			}
			ml->baseAddress = start;
			ml->moduleSize = stop - start;
			ml->moduleName = strdup(modulepath);
			ml->next_ptr = nullptr;

			_ml = ml;
		}
		return ret;
	}
	return nullptr;
}

#endif //MEMORY_H

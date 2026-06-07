#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

void* mmap(void* addr, size_t len, int prot, int flags, int fildes, off_t off)
{
	sys_mmap_t args {
		.addr = addr,
		.len = len,
		.prot = prot,
		.flags = flags,
		.fildes = fildes,
		.off = off
	};
	long ret = syscall(SYS_MMAP, &args);
	if (ret == -1)
		return MAP_FAILED;
	return (void*)ret;
}

int munmap(void* addr, size_t len)
{
	return syscall(SYS_MUNMAP, addr, len);
}

int mprotect(void* addr, size_t len, int prot)
{
	return syscall(SYS_MPROTECT, addr, len, prot);
}

int msync(void* addr, size_t len, int flags)
{
	pthread_testcancel();
	return syscall(SYS_MSYNC, addr, len, flags);
}

int posix_madvise(void* addr, size_t len, int advice)
{
	(void)addr;
	(void)len;
	(void)advice;
	return 0;
}

int mlock(const void* addr, size_t len)
{
	(void)addr;
	(void)len;
	return 0;
}

int munlock(const void* addr, size_t len)
{
	(void)addr;
	(void)len;
	return 0;
}

int shm_open(const char* name, int oflag, mode_t mode)
{
	if (mkdir("/tmp/shm", 0777) == -1 && errno != EEXIST)
		return -1;
	char path[PATH_MAX];
	sprintf(path, "/tmp/shm%s", name);
	return open(path, oflag | O_CLOEXEC, mode);
}

int shm_unlink(const char* name)
{
	char path[PATH_MAX];
	sprintf(path, "/tmp/shm%s", name);
	return unlink(path);
}

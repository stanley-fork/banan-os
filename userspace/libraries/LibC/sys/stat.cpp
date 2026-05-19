#include <BAN/Assert.h>
#include <BAN/Debug.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

mode_t __umask = 0;

int chmod(const char* path, mode_t mode)
{
	return fchmodat(AT_FDCWD, path, mode, 0);
}

int fchmod(int fildes, mode_t mode)
{
	return fchmodat(fildes, nullptr, mode, 0);
}

int fchmodat(int fildes, const char* path, mode_t mode, int flag)
{
	return syscall(SYS_FCHMODAT, fildes, path, mode, flag);
}

int fstat(int fildes, struct stat* buf)
{
	return fstatat(fildes, nullptr, buf, 0);
}

int fstatat(int fd, const char* __restrict path, struct stat* __restrict buf, int flag)
{
	return syscall(SYS_FSTATAT, fd, path, buf, flag);
}

int lstat(const char* __restrict path, struct stat* __restrict buf)
{
	return fstatat(AT_FDCWD, path, buf, AT_SYMLINK_NOFOLLOW);
}

int stat(const char* __restrict path, struct stat* __restrict buf)
{
	return fstatat(AT_FDCWD, path, buf, 0);
}

mode_t umask(mode_t cmask)
{
	mode_t old = __umask;
	__umask = cmask;
	return old;
}

int mkdir(const char* path, mode_t mode)
{
	return mkdirat(AT_FDCWD, path, mode);
}

int mkdirat(int fd, const char* path, mode_t mode)
{
	return syscall(SYS_MKDIRAT, fd, path, __UMASKED_MODE(mode));
}

int mkfifo(const char* path, mode_t mode)
{
	return mkfifoat(AT_FDCWD, path, mode);
}

int mkfifoat(int fd, const char* path, mode_t mode)
{
	return syscall(SYS_MKFIFOAT, fd, path, __UMASKED_MODE(mode));
}

int mknod(const char* path, mode_t mode, dev_t dev)
{
	return mknodat(AT_FDCWD, path, mode, dev);
}

int mknodat(int fd, const char* path, mode_t mode, dev_t dev)
{
	(void)fd; (void)path; (void)mode; (void)dev;
	dwarnln("TODO: mknodat");
	return -1;
}

int futimens(int fd, const struct timespec times[2])
{
	return utimensat(fd, nullptr, times, 0);
}

int utimensat(int fd, const char* path, const struct timespec times[2], int flag)
{
	return syscall(SYS_UTIMENSAT, fd, path, times, flag);
}

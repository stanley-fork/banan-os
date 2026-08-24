#include <fcntl.h>
#include <sys/random.h>

ssize_t getrandom(void* buf, size_t size, unsigned int flags)
{
	const unsigned int valid_flags = GRND_RANDOM | GRND_NONBLOCK;
	if ((flags & valid_flags) != flags)
	{
		errno = EINVAL;
		return -1;
	}

	const char* path = (flags & GRND_RANDOM) ? "/dev/random" : "/dev/urandom";

	int oflag = O_RDONLY;
	if (flags & GRND_NONBLOCK)
		oflag |= O_NONBLOCK;

	int fd = open(path, oflag);
	if (fd == -1)
		return -1;

	uint8_t* ubuf = static_cast<uint8_t*>(buf);

	size_t total_read = 0;
	while (total_read < size)
	{
		const ssize_t nread = read(fd, ubuf + total_read, size - total_read);
		if (nread <= 0)
			break;
		total_read += nread;
	}

	close(fd);

	return total_read;
}

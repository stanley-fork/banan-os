#ifndef _SYS_RANDOM_H
#define _SYS_RANDOM_H 1

#include <sys/cdefs.h>

__BEGIN_DECLS

#define __need_size_t
#define __need_ssize_t
#include <sys/types.h>

#define GRND_RANDOM   0x01
#define GRND_NONBLOCK 0x02

ssize_t getrandom(void* buf, size_t size, unsigned int flags);

__END_DECLS

#endif

#ifndef UTIL_H_
#define UTIL_H_

#include <stddef.h>
#include <sys/types.h>

#define CLAMP(val, min, max) \
	((val) < (min) ? (min) : (val) > (max) ? (max) : (val))

#define STRINGIFY(x) STRINGIFY_(x)
#define STRINGIFY_(x) #x

ssize_t read_timeout(int fd, void *buf, size_t size, int timeout);

#endif /* UTIL_H_ */

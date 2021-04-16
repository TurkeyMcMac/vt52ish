#ifndef UTIL_H_
#define UTIL_H_

#include <stddef.h>
#include <sys/types.h>

// Clamps the value between the min and max, evaluating things more than once.
#define CLAMP(val, min, max) \
	((val) < (min) ? (min) : (val) > (max) ? (max) : (val))

// Expands a preprocessor symbol and stringifies the result.
#define STRINGIFY(x) STRINGIFY_(x)
#define STRINGIFY_(x) #x

// Tries to read(), but waits at most timeout milliseconds.
ssize_t read_timeout(int fd, void *buf, size_t size, int timeout);

// Tries to write(), but waits at most timeout milliseconds.
ssize_t write_timeout(int fd, const void *buf, size_t size, int timeout);

#endif /* UTIL_H_ */

#include "util.h"
#include <errno.h>
#include <poll.h>
#include <unistd.h>

ssize_t read_timeout(int fd, void *buf, size_t size, int timeout)
{
	struct pollfd pollfd = {
		.fd = fd,
		.events = POLLIN,
	};
	int n_events;
	while ((n_events = poll(&pollfd, 1, timeout)) < 0 && errno == EINTR)
		;
	if (n_events == 1) {
		if (pollfd.revents & POLLIN) return read(fd, buf, size);
		if (pollfd.revents & POLLHUP) return 0;
		if (pollfd.revents & (POLLERR | POLLNVAL)) return -1;
		// I guess it's EAGAIN if this point is reached?
	} else if (n_events < 0) {
		return -1;
	}
	errno = EAGAIN;
	return -1;
}

ssize_t write_timeout(int fd, const void *buf, size_t size, int timeout)
{
	struct pollfd pollfd = {
		.fd = fd,
		.events = POLLOUT,
	};
	int n_events;
	while ((n_events = poll(&pollfd, 1, timeout)) < 0 && errno == EINTR)
		;
	if (n_events == 1) {
		if (pollfd.revents & POLLOUT) return write(fd, buf, size);
		if (pollfd.revents & POLLHUP) return 0;
		if (pollfd.revents & (POLLERR | POLLNVAL)) return -1;
		// I guess it's EAGAIN if this point is reached?
	} else if (n_events < 0) {
		return -1;
	}
	errno = EAGAIN;
	return -1;
}

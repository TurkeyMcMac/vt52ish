#include "pty.h"
#include "config.h"
#include "util.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void do_slave(int pt_master_fd, char *argv[])
{
	if (grantpt(pt_master_fd) < 0 || unlockpt(pt_master_fd) < 0) goto error;
	char *pt_slave_path = ptsname(pt_master_fd);
	(void)close(pt_master_fd);
	int pt_slave_fd = open(pt_slave_path, O_RDWR);
	if (pt_slave_fd < 0) goto error;
	if (dup2(pt_slave_fd, STDIN_FILENO) < 0
	 || dup2(pt_slave_fd, STDOUT_FILENO) < 0
	 || dup2(pt_slave_fd, STDERR_FILENO) < 0)
		goto error;
	(void)close(pt_master_fd);
	if (setenv("TERM", "vt52", 1) < 0
	 || setenv("LINES", STRINGIFY(N_LINES), 1) < 0
	 || setenv("COLUMNS", STRINGIFY(N_COLUMNS), 1) < 0)
		goto error;
	execvp(argv[0], argv);
error:
	{
		char *error_str = strerror(errno);
		(void)write(STDERR_FILENO, error_str, strlen(error_str));
	}
	exit(EXIT_FAILURE);
}

int pty_start(char *slave_argv[])
{
	int pt_master_fd = posix_openpt(O_RDWR);
	if (pt_master_fd < 0) goto error;
	switch (fork()) {
	case -1:
		goto error;
	default:
		return pt_master_fd;
	case 0:
		do_slave(pt_master_fd, slave_argv);
		// do_slave never returns.
		break;
	}

error:
	if (pt_master_fd >= 0) {
		int errno_save = errno;
		(void)close(pt_master_fd);
		errno = errno_save;
	}
	return -1;
}

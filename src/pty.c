#include "pty.h"
#include "config.h"
#include "util.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

// Give the fd the right tty settings.
static int set_up_pty(int pt_slave_fd)
{
	struct termios t;
	if (tcgetattr(pt_slave_fd, &t) < 0) return -1;
	// XXX I'm not sure what I'm doing here. I copied most of this from an
	// existing tty configuration.
	cfsetispeed(&t, B9600);
	cfsetospeed(&t, B9600);
	t.c_iflag = ICRNL;
	t.c_oflag = OPOST | ONLCR | NL0 | CR0 | TAB0 | BS0 | VT0 | FF0;
	t.c_cflag = CREAD | CS8;
	t.c_lflag = ISIG | ICANON | IEXTEN | ECHO | ECHOE | ECHOK;
	// ECHOKE and ECHOCTL are non-standard.
#ifdef ECHOKE
	t.c_lflag |= ECHOKE;
#endif
#ifdef ECHOCTL
	t.c_lflag |= ECHOCTL;
#endif
	t.c_cc[VERASE] = '\b';
	if (tcsetattr(pt_slave_fd, TCSANOW, &t) < 0) return -1;
	// Set the terminal size.
	struct winsize ws = { .ws_row = N_LINES, .ws_col = N_COLS };
	if (ioctl(pt_slave_fd, TIOCSWINSZ, &ws) < 0) return -1;
	return 0;
}

static void do_slave(int pt_master_fd, char *argv[])
{
	// Start a new session.
	if (setsid() == (pid_t)-1) goto error;
	// Get the slave end of the pty.
	if (grantpt(pt_master_fd) < 0 || unlockpt(pt_master_fd) < 0) goto error;
	char *pt_slave_path = ptsname(pt_master_fd);
	(void)close(pt_master_fd);
	int pt_slave_fd = open(pt_slave_path, O_RDWR);
	if (pt_slave_fd < 0) goto error;
	// Connect the standard streams to the slave end.
	if (dup2(pt_slave_fd, STDIN_FILENO) < 0
	 || dup2(pt_slave_fd, STDOUT_FILENO) < 0
	 || dup2(pt_slave_fd, STDERR_FILENO) < 0)
		goto error;
	// Give the tty the right settings.
	if (set_up_pty(pt_slave_fd) < 0) goto error;
	// Use it as the controlling tty.
	if (ioctl(pt_slave_fd, TIOCSCTTY, 0) < 0) goto error;
	// Set up the environment.
	if (setenv("TERM", "vt52", 1) < 0
	 || setenv("LINES", STRINGIFY(N_LINES), 1) < 0
	 || setenv("COLUMNS", STRINGIFY(N_COLS), 1) < 0)
		goto error;
	(void)close(pt_slave_fd);
	// Run the slave program.
	execvp(argv[0], argv);
error:
	{
		char *str = "Slave error: ";
		(void)write(STDERR_FILENO, str, strlen(str));
		str = strerror(errno);
		(void)write(STDERR_FILENO, str, strlen(str));
		(void)write(STDERR_FILENO, "\n", 1);
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

#ifndef PTY_H_
#define PTY_H_

// Start the pseudo-terminal connection. On success, the slave process is
// started with the given argv as a session connected to the pty (with the
// correct tty settings,) and the fd of the master end of the pty is returned.
// On failure, -1 is returned.
int pty_start(char *slave_argv[]);

#endif /* PTY_H_ */

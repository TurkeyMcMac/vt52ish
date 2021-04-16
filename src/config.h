#ifndef CONFIG_H_
#define CONFIG_H_

// The character cell dimensions of the terminal. These are part of the protocol
// and should not be changed lightly.
#define N_LINES 24
#define N_COLS 80

// The input and output baud rate used when setting up the tty device. This must
// be one of the baud constants defined in <termios.h>.
#define BAUD_RATE B9600

// The maximum timeout for reading slave output in milliseconds. A higher
// timeout will make keyboard input less responsive.
#define READ_TIMEOUT 6

// The maximum timeout for writing keyboard input to the slave. A higher
// timeout will make displaying characters less responsive.
#define WRITE_TIMEOUT 1

#endif /* CONFIG_H_ */

#include "buffer.h"
#include "config.h"
#include "key.h"
#include "parser.h"
#include "pty.h"
#include "util.h"
#include <curses.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define VERSION_FMT "%s version " VERSION "\n"

#define USAGE_FMT "Usage: %s {-h | -v | [--] command [args...]}\n"

#define HELP_FMT \
	USAGE_FMT \
	"\n" \
	"Runs the command in a TUI VT52-based terminal emulator.\n" \
	"\n" \
	"Options:\n" \
	"  -h  Print this help and exit.\n" \
	"  -v  Print the program version and exit.\n"


int main(int argc, char *argv[])
{
	const char *progname = argv[0] ? argv[0] : "";
	bool posixly_correct_was_set = getenv("POSIXLY_CORRECT");
	// POSIXLY_CORRECT prevents the slave's options from being detected.
	if (!posixly_correct_was_set) setenv("POSIXLY_CORRECT", "", 1);
	for (int opt; (opt = getopt(argc, argv, "hv")) >= 0; ) {
		switch (opt) {
		case 'h':
			printf(HELP_FMT, progname);
			return EXIT_SUCCESS;
		case 'v':
			printf(VERSION_FMT, progname);
			return EXIT_SUCCESS;
		case '?':
			fprintf(stderr, USAGE_FMT, progname);
			return EXIT_FAILURE;
		}
	}
	if (!argv[optind]) {
		fprintf(stderr, USAGE_FMT, progname);
		return EXIT_FAILURE;
	}

	// Avoid messing with the slave's environment.
	if (!posixly_correct_was_set) unsetenv("POSIXLY_CORRECT");
	// The leftover arguments are passed to the slave.
	int pt_master_fd = pty_start(argv + optind);
	if (pt_master_fd < 0) {
		fprintf(stderr, "%s: Unable to initialize pseudo-terminal "
			"master end: %s\n", progname, strerror(errno));
		return EXIT_FAILURE;
	}

	initscr();
	raw();
	noecho();
	timeout(0);
	keypad(stdscr, TRUE);

	struct parser p;
	if (parser_init(&p, newwin(N_LINES, N_COLS, 0, 0)) < 0) {
		endwin();
		fprintf(stderr, "%s: Unable to initialize parser\n", progname);
		return EXIT_FAILURE;
	}

	struct buffer send;
	if (buffer_init(&send, 8) < 0) {
		endwin();
		fprintf(stderr,
			"%s: Unable to initialize input buffer\n", progname);
		return EXIT_FAILURE;
	}

	for (;;) {
		uint8_t byte;
		// Read is on a timeout to keep keyboard input responsive.
		ssize_t r = read_timeout(pt_master_fd, &byte, 1, READ_TIMEOUT);
		if (r > 0) {
			// A byte was received.
			parser_process_byte(&p, byte);
			wrefresh(p.win);
			// Send any requested identifier sequences.
			while (p.idents_pending > 0) {
				// This string identifies the VT52.
				buffer_append_str(&send, "\033/K");
				--p.idents_pending;
			}
			// Play any requested bells.
			while (p.bells_pending > 0) {
				beep();
				--p.bells_pending;
			}
		} else if (r == 0) {
			// The slave has closed.
			break;
		}
		// Prepare all the pending keys for sending.
		int key;
		while ((key = getch()) != ERR) {
			key_translate(key, &send);
		}
		if (send.len > 0) {
			// This strategy of nonblocking is used instead of
			// O_NONBLOCK since the latter doesn't work on Mac OS.
			ssize_t w = write_timeout(pt_master_fd,
				send.data, send.len, 0);
			if ((size_t)w == send.len) {
				// All the data was sent.
				send.len = 0;
			} else if (w > 0 && (size_t)w < send.len) {
				// The unsent data is next up.
				memmove(send.data, send.data + w, send.len - w);
				send.len -= w;
			}
		}
	}

	endwin();
	return EXIT_SUCCESS;
}

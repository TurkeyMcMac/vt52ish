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

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;

	if (argc < 2) {
		fprintf(stderr,
			"Usage: %s cmd [arg...]\n", argv[0] ? argv[0] : "");
		goto end;
	}
	// The non-initial arguments are passed to the slave.
	int pt_master_fd = pty_start(argv + 1);
	if (pt_master_fd < 0) {
		fprintf(stderr, "%s: Unable to initialize pseudo-terminal "
			"master end: %s\n", argv[0], strerror(errno));
		goto end;
	}

	initscr();
	raw();
	noecho();
	timeout(0);
	keypad(stdscr, TRUE);

	struct parser p;
	if (parser_init(&p, newwin(N_LINES, N_COLS, 0, 0)) < 0) {
		fprintf(stderr, "%s: Unable to initialize parser\n", argv[0]);
		goto end_curses;
	}
	struct buffer send;
	if (buffer_init(&send, 8) < 0) {
		fprintf(stderr,
			"%s: Unable to initialize input buffer\n", argv[0]);
		goto end_curses;
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
			// The write is on a timeout so the emulator can keep
			// responding to slave output even if the input buffer
			// is full. This is done without O_NONBLOCK since that
			// doesn't work on Mac OS.
			ssize_t w = write_timeout(pt_master_fd,
				send.data, send.len, WRITE_TIMEOUT);
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
	ret = EXIT_SUCCESS;

end_curses:
	endwin();
end:
	return ret;
}

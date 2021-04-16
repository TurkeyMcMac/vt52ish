#include "buffer.h"
#include "config.h"
#include "key.h"
#include "parser.h"
#include "pty.h"
#include "util.h"
#include <curses.h>
#include <errno.h>
#include <fcntl.h>
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
	int pt_master_fd = argc > 1 ? pty_start(argv + 1) : -1;
	int flags = fcntl(pt_master_fd, F_GETFL);
	int set = fcntl(pt_master_fd, F_SETFL, flags | O_NONBLOCK);
	if (pt_master_fd < 0 || flags < 0 || set < 0) {
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
	buffer_init(&send, 8);
	if (parser_init(&p, newwin(N_LINES, N_COLS, 0, 0)) < 0) {
		fprintf(stderr,
			"%s: Unable to initialize input buffer\n", argv[0]);
		goto end_curses;
	}

	for (;;) {
		uint8_t byte;
		ssize_t r = read_timeout(pt_master_fd, &byte, 1, 6);
		if (r > 0) {
			parser_process_byte(&p, byte);
			wrefresh(p.win);
			while (p.idents_pending > 0) {
				buffer_append_str(&send, "\033/K");
				--p.idents_pending;
			}
			while (p.bells_pending > 0) {
				beep();
				--p.bells_pending;
			}
		} else if (r == 0) {
			break;
		}
		int key;
		while ((key = getch()) != ERR) {
			key_translate(key, &send);
		}
		if (send.len > 0) {
			ssize_t w = write(pt_master_fd, send.data, send.len);
			if ((size_t)w == send.len) {
				send.len = 0;
			} else if (w > 0 && (size_t)w < send.len) {
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

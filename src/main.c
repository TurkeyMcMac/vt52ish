#include "config.h"
#include "parser.h"
#include "pty.h"
#include "util.h"
#include <curses.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>
int main(int argc, char *argv[])
{
	int pt_master_fd = argc > 1 ? pty_start(argv + 1) : -1;
	if (pt_master_fd < 0/* || fcntl(pt_master_fd, O_NONBLOCK) < 0*/)
		return EXIT_FAILURE;

	initscr();
	raw();
	noecho();
	timeout(0);

	struct parser p;
	parser_init(&p, newwin(N_LINES, N_COLS, 0, 0));
	for (;;) {
		uint8_t byte;
		ssize_t r = read_timeout(pt_master_fd, &byte, 1, 6);
		if (r > 0) {
			parser_process_byte(&p, byte);
			wrefresh(p.win);
		}
		int key;
		while ((key = getch()) != ERR) {
			if (key >= 31 && key < 127) {
				uint8_t key_byte = key;
				(void)write(pt_master_fd, &key_byte, 1);
			}
		}
	}

	endwin();
}

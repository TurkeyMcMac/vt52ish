#include "parser.h"
#include "util.h"
#include <stdlib.h>

int parser_init(struct parser *p, WINDOW *win)
{
	if (!win || scrollok(win, TRUE) != OK) return -1;
	p->idents_pending = 0;
	p->alt_keypad = false;
	p->win = win;
	p->alt_charset = false;
	p->cmd_state = CS_NONE;
	p->cup_row = 0;
	return 0;
}

void parser_process_byte(struct parser *p, int byte)
{
	// These computations are done up here for simplicity.
	int n_lines, n_cols, y, x;
	getmaxyx(p->win, n_lines, n_cols);
	getyx(p->win, y, x);
	switch (p->cmd_state) {
	case CS_NONE:
		switch (byte) {
		case ESC:
			p->cmd_state = CS_HAS_START;
			break;
		case '\n':
			// Should this clear to eol?
			if (y < n_lines - 1) {
				wmove(p->win, y + 1, x);
			} else {
				wscrl(p->win, 1);
			}
			break;
		case '\r':
			wmove(p->win, y, 0);
			break;
		default:
			if (y == n_lines - 1 && x == n_cols - 1)
				wscrl(p->win, 1);
			waddch(p->win, byte);
			break;
		}
		break;
	case CS_HAS_START:
		p->cmd_state = CS_NONE;
		switch (byte) {
		case 'A':
			if (y > 0) wmove(p->win, y - 1, x);
			break;
		case 'B':
			if (y < n_lines - 1) wmove(p->win, y + 1, x);
			break;
		case 'C':
			if (x < n_cols - 1) wmove(p->win, y, x + 1);
			break;
		case 'D':
			if (x > 0) wmove(p->win, y, x - 1);
			break;
		case 'F':
			p->alt_charset = true;
			break;
		case 'G':
			p->alt_charset = false;
			break;
		case 'H':
			wmove(p->win, 0, 0);
			break;
		case 'I':
			if (y > 0) {
				// Is insertion needed here?
				wmove(p->win, y - 1, x);
			} else {
				wscrl(p->win, -1);
			}
			break;
		case 'J':
			wclrtobot(p->win);
			break;
		case 'K':
			wclrtoeol(p->win);
			break;
		case 'L':
			for (int new_y = n_lines - 1; new_y > y; --new_y) {
				for (int cx = 0; cx < n_cols; ++cx) {
					mvwaddch(p->win, new_y, cx,
						mvwinch(p->win, new_y - 1, cx));
				}
			}
			wmove(p->win, y, 0);
			wclrtoeol(p->win);
			break;
		case 'M':
			for (int new_y = y; new_y < n_lines - 1; ++new_y) {
				for (int cx = 0; cx < n_cols; ++cx) {
					mvwaddch(p->win, new_y, cx,
						mvwinch(p->win, new_y + 1, cx));
				}
			}
			wmove(p->win, n_lines - 1, 0);
			wclrtoeol(p->win);
			wmove(p->win, y, 0);
			break;
		case 'Y':
			p->cmd_state = CS_HAS_CUP_START;
			break;
		case 'Z':
			++p->idents_pending;
			break;
		case '=':
			p->alt_keypad = true;
			break;
		case '>':
			p->alt_keypad = false;
			break;
		}
		break;
	case CS_HAS_CUP_START:
		p->cup_row = byte - 31;
		p->cmd_state = CS_HAS_CUP_ROW;
		break;
	case CS_HAS_CUP_ROW:
		y = CLAMP(p->cup_row, 1, n_lines) - 1;
		x = CLAMP(byte - 31, 1, n_cols) - 1;
		wmove(p->win, y, x);
		p->cmd_state = CS_NONE;
		break;
	}
}

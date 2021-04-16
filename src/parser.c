#include "parser.h"
#include "util.h"
#include <stdlib.h>

int parser_init(struct parser *p, WINDOW *win)
{
	if (!win || scrollok(win, FALSE) != OK) return -1;
	p->idents_pending = 0;
	p->bells_pending = 0;
	p->win = win;
	p->alt_charset = false;
	p->line_full = false;
	p->cmd_state = CS_NONE;
	p->cup_row = 0;
	return 0;
}

static void do_scroll(WINDOW *win, int amount)
{
	scrollok(win, TRUE);
	wscrl(win, amount);
	scrollok(win, FALSE);
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
		case '\033':
			p->cmd_state = CS_HAS_START;
			break;
		case '\n':
		case '\v':
		case '\f':
			// IDK if a linefeed should clear to eol.
			if (y < n_lines - 1) {
				wmove(p->win, y + 1, x);
			} else {
				do_scroll(p->win, 1);
			}
			p->line_full = false;
			break;
		case '\r':
			wmove(p->win, y, 0);
			p->line_full = false;
			break;
		case '\t':
			if (p->line_full) {
				if (y == n_lines - 1) {
					do_scroll(p->win, 1);
					wmove(p->win, y, 0);
				} else {
					wmove(p->win, y + 1, 0);
				}
				p->line_full = false;
			} else {
				p->line_full = n_cols % TABSIZE == 0
					&& x == n_cols - TABSIZE;
			}
			waddch(p->win, '\t');
			if (p->line_full) wmove(p->win, y, n_cols - 1);
			break;
		case '\b':
			if (x > 0) {
				if (p->line_full) {
					waddch(p->win, ' ');
					wmove(p->win, y, x);
				} else {
					waddstr(p->win, "\b \b");
				}
				p->line_full = false;
			} else if (y > 0) {
				mvwaddch(p->win, y - 1, n_cols - 1, ' ');
				wmove(p->win, y - 1, n_cols - 1);
			}
			break;
		case '\a':
			++p->bells_pending;
			break;
		default:
			if (byte > 31 && byte < 127) {
				if (p->line_full) {
					if (y == n_lines - 1) {
						do_scroll(p->win, 1);
						wmove(p->win, y, 0);
					} else {
						wmove(p->win, y + 1, 0);
					}
					p->line_full = false;
				} else {
					p->line_full = x == n_cols - 1;
				}
				chtype ch = byte;
				if (p->alt_charset) {
					// Taken from `infocmp vt52`.
					switch (ch) {
					case 'h': ch = ACS_RARROW; break;
					case 'k': ch = ACS_DARROW; break;
					case 'a': ch = ACS_BLOCK; break;
					case 'f': ch = ACS_DEGREE; break;
					case 'g': ch = ACS_PLMINUS; break;
					case 'l': ch = ACS_S1; break;
					case 'n':
#ifdef ACS_S3
						  ch = ACS_S3;
#else
						  ch = ACS_HLINE;
#endif
						  break;
					case 'p': ch = ACS_HLINE; break;
					case 'r':
#ifdef ACS_S7
						  ch = ACS_S7;
#else
						  ch = ACS_HLINE;
#endif
						  break;
					case 's': ch = ACS_S9; break;
					}
				}
				waddch(p->win, ch);
				if (p->line_full) wmove(p->win, y, x);
			}
			break;
		}
		break;
	case CS_HAS_START:
		p->cmd_state = CS_NONE;
		switch (byte) {
		case 'A':
			if (y > 0) {
				wmove(p->win, y - 1, x);
				p->line_full = false;
			}
			break;
		case 'B':
			if (y < n_lines - 1) {
				wmove(p->win, y + 1, x);
				p->line_full = false;
			}
			break;
		case 'C':
			if (x < n_cols - 1) {
				wmove(p->win, y, x + 1);
				p->line_full = false;
			}
			break;
		case 'D':
			if (x > 0) {
				wmove(p->win, y, x - 1);
				p->line_full = false;
			}
			break;
		case 'F':
			p->alt_charset = true;
			break;
		case 'G':
			p->alt_charset = false;
			break;
		case 'H':
			wmove(p->win, 0, 0);
			p->line_full = false;
			break;
		case 'I':
			if (y > 0) {
				// Is insertion needed here?
				wmove(p->win, y - 1, x);
			} else {
				do_scroll(p->win, -1);
			}
			p->line_full = false;
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
			p->line_full = false;
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
			p->line_full = false;
			break;
		case 'Y':
			p->cmd_state = CS_HAS_CUP_START;
			break;
		case 'Z':
			++p->idents_pending;
			break;
		case '=':
		case '>':
			// XXX Alternate keypad is unimplemented.
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
		p->line_full = false;
		break;
	}
}

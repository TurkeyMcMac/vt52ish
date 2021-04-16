#include "parser.h"
#include "util.h"
#include <stdlib.h>

int parser_init(struct parser *p, WINDOW *win)
{
	if (!win || scrollok(win, FALSE) != OK || leaveok(win, FALSE) != OK)
		return -1;
	p->idents_pending = 0;
	p->bells_pending = 0;
	p->win = win;
	p->alt_charset = false;
	p->line_full = false;
	p->cmd_state = CS_NONE;
	p->cup_row = 0;
	return 0;
}

// Enable scrolling for one call to wscrl(win, amount).
static void do_scroll(WINDOW *win, int amount)
{
	scrollok(win, TRUE);
	wscrl(win, amount);
	scrollok(win, FALSE);
}

void parser_process_byte(struct parser *p, int byte)
{
	// These computations are done up here for simplicity. y and x generally
	// represent the cursor position at the beginning of this call.
	int n_lines, n_cols, y, x;
	getmaxyx(p->win, n_lines, n_cols);
	getyx(p->win, y, x);
	switch (p->cmd_state) {
	case CS_NONE:
		switch (byte) {
		case '\033': // ESC character
			p->cmd_state = CS_HAS_START;
			break;
		case '\n': // The linefeed and others move down.
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
		case '\r': // The carriage return moves to the left.
			wmove(p->win, y, 0);
			p->line_full = false;
			break;
		case '\t':
			if (p->line_full) {
				// Move to the next line if this one's full.
				if (y == n_lines - 1) {
					// Scroll at the bottom of the screen.
					do_scroll(p->win, 1);
					wmove(p->win, y, 0);
				} else {
					wmove(p->win, y + 1, 0);
				}
				p->line_full = false;
			} else {
				// The line is full if this tab fills it up
				// exactly. There are tab stops every 8 cells.
				p->line_full = n_cols % 8 == 0
					&& x == n_cols - 8;
			}
			waddch(p->win, '\t');
			// Move back onto the same line if necessary.
			if (p->line_full) wmove(p->win, y, n_cols - 1);
			break;
		case '\b': // Backspace
			if (x > 0) {
				if (p->line_full) {
					// If the line is full, the character
					// below the cursor (the last character
					// on the line) is erased.
					waddch(p->win, ' ');
					wmove(p->win, y, x);
				} else {
					// Otherwise, the character before is
					// erased.
					waddstr(p->win, "\b \b");
				}
			} else if (y > 0) {
				// Backspace wraps around to the line above.
				mvwaddch(p->win, y - 1, n_cols - 1, ' ');
				wmove(p->win, y - 1, n_cols - 1);
			}
			p->line_full = false;
			break;
		case '\a': // Bell
			++p->bells_pending;
			break;
		default:
			// Only consider the printing characters here.
			if (byte > 31 && byte < 127) {
				if (p->line_full) {
					// Move to the next line if this one's
					// full.
					if (y == n_lines - 1) {
						// Scroll at the bottom of the
						// screen.
						do_scroll(p->win, 1);
						wmove(p->win, y, 0);
					} else {
						wmove(p->win, y + 1, 0);
					}
					p->line_full = false;
				} else {
					// This character may fill the line.
					p->line_full = x == n_cols - 1;
				}
				chtype ch = byte;
				// Translate the alt. char. set if necessary.
				if (p->alt_charset) {
					// Taken from `infocmp vt52`.
					switch (ch) {
					case 'a': ch = ACS_BLOCK; break;
					case 'f': ch = ACS_DEGREE; break;
					case 'g': ch = ACS_PLMINUS; break;
					case 'h': ch = ACS_RARROW; break;
					case 'k': ch = ACS_DARROW; break;
					case 'l': ch = ACS_S1; break;
					case 'm': ch = ACS_S1; break;
					case 'n':
					case 'o':
#ifdef ACS_S3
						  ch = ACS_S3;
#else
						  ch = ACS_HLINE;
#endif
						  break;
					case 'p': ch = ACS_HLINE; break;
					case 'q': ch = ACS_HLINE; break;
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
				// Move back to this line if necessary.
				if (p->line_full) wmove(p->win, y, x);
			}
			break;
		}
		break;
	case CS_HAS_START:
		// Most commands will be done with after this byte.
		p->cmd_state = CS_NONE;
		switch (byte) {
		case 'A': // Move up.
			if (y > 0) {
				wmove(p->win, y - 1, x);
				p->line_full = false;
			}
			break;
		case 'B': // Move down.
			if (y < n_lines - 1) {
				wmove(p->win, y + 1, x);
				p->line_full = false;
			}
			break;
		case 'C': // Move right.
			if (x < n_cols - 1) {
				wmove(p->win, y, x + 1);
				p->line_full = false;
			}
			break;
		case 'D': // Move left.
			if (x > 0) {
				wmove(p->win, y, x - 1);
				p->line_full = false;
			}
			break;
		case 'F': // Set the alt. char. set on.
			p->alt_charset = true;
			break;
		case 'G': // Turn it off.
			p->alt_charset = false;
			break;
		case 'H': // Return to home position.
			wmove(p->win, 0, 0);
			p->line_full = false;
			break;
		case 'I': // Reverse linefeed? (Move up, maybe scroll.)
			if (y > 0) {
				// Is insertion needed here?
				wmove(p->win, y - 1, x);
			} else {
				do_scroll(p->win, -1);
			}
			p->line_full = false;
			break;
		case 'J': // Clear to end of screen.
			wclrtobot(p->win);
			break;
		case 'K': // Clear to end of line.
			wclrtoeol(p->win);
			break;
		case 'L': // Insert line and shift those below down?
			// Scroll the following lines.
			for (int new_y = n_lines - 1; new_y > y; --new_y) {
				for (int cx = 0; cx < n_cols; ++cx) {
					// Move lines downward.
					mvwaddch(p->win, new_y, cx,
						mvwinch(p->win, new_y - 1, cx));
				}
			}
			// Move to the beginning and clear the line.
			wmove(p->win, y, 0);
			wclrtoeol(p->win);
			p->line_full = false;
			break;
		case 'M': // Delete this line and move those below up?
			// Scroll the following lines.
			for (int new_y = y; new_y < n_lines - 1; ++new_y) {
				for (int cx = 0; cx < n_cols; ++cx) {
					// Move lines upward.
					mvwaddch(p->win, new_y, cx,
						mvwinch(p->win, new_y + 1, cx));
				}
			}
			// The new final line is clear.
			wmove(p->win, n_lines - 1, 0);
			wclrtoeol(p->win);
			// Move to the beginning of the original line.
			wmove(p->win, y, 0);
			p->line_full = false;
			break;
		case 'Y': // Start a CUP sequence.
			p->cmd_state = CS_HAS_CUP_START;
			break;
		case 'Z': // Send an identifier to the slave.
			++p->idents_pending;
			break;
		case '=': // These toggle the alt. keypad. What does it do???
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

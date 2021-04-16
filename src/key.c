#include "key.h"
#include <curses.h>

int key_translate(int curses_key, struct buffer *buf)
{
	// XXX This keyboard handling is very limited/incomplete!
	switch (curses_key) {
	case KEY_ENTER:
		return buffer_append_byte(buf, '\r');
	case KEY_BACKSPACE:
		return buffer_append_byte(buf, '\b');
	case KEY_DC:
		return buffer_append_byte(buf, '\177');
	case KEY_UP:
		return buffer_append_str(buf, "\033A");
	case KEY_DOWN:
		return buffer_append_str(buf, "\033B");
	case KEY_RIGHT:
		return buffer_append_str(buf, "\033C");
	case KEY_LEFT:
		return buffer_append_str(buf, "\033D");
	default: 
		// This seems to include characters with the CTRL modifier.
		if (curses_key >= 0 && curses_key < 128)
			return buffer_append_byte(buf, curses_key);
		break;
	}
	return 0;
}

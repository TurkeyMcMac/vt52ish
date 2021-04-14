#ifndef PARSER_H_
#define PARSER_H_

#include <curses.h>

struct parser {
	int idents_pending;
	bool alt_keypad;
	bool alt_charset;
	WINDOW *win;
	enum cmd_state {
		CS_NONE,
		CS_HAS_START,
		CS_HAS_CUP_START,
		CS_HAS_CUP_ROW,
	} cmd_state;
	int cup_row;
};

int parser_init(struct parser *p, WINDOW *win);

void parser_process_byte(struct parser *p, int byte);

#endif /* PARSER_H_ */

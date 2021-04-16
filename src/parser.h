#ifndef PARSER_H_
#define PARSER_H_

#include <curses.h>

struct parser {
	/* Public fields */
	// The number of identifier strings the slave is waiting for. The caller
	// should send these and decrement the number.
	int idents_pending;
	// Similarly, the number bells the slave is wants played for the user.
	int bells_pending;
	// The window the characters are drawn on. The caller should only be
	// reading from this window, not modifying it. Note that the parser
	// uses the window to remember its cursor positon.
	WINDOW *win;

	/* Private fields */
	// Whether the terminal is in alternate character set mode.
	bool alt_charset;
	// Whether the current line of input is full. I'm not sure if an actual
	// VT52 does this, but this program should be able to hold 80 characters
	// followed by CRLF on a single line. This means that when the 80th
	// character is written, the cursor shouldn't advance. When the 80th
	// column is written, line_full is set to true. When the next character
	// is written, line_full is detected and the character is put on the
	// next line. line_full is then falsified. line_full is also unset when
	// the cursor moves between writing the two characters.
	bool line_full;
	// The progress into the command sequence currently being parsed.
	enum cmd_state {
		// No sequence is currently being parsed.
		CS_NONE,
		// The initial escape character has been received.
		CS_HAS_START,
		// The sequence is a cursor position sequence and the row and
		// column have yet to be received.
		CS_HAS_CUP_START,
		// The row has been received.
		CS_HAS_CUP_ROW,
	} cmd_state;
	// This holds the row as a CUP sequence is being received.
	int cup_row;
};

// Initialize the parser with the window win. A negative is returned only if the
// window could not be used (including if it was NULL.) The caller can free the
// window after the parser is destroyed.
int parser_init(struct parser *p, WINDOW *win);

// Process a byte, modifying the parser's state. This pretty much implements the
// VT52 protocol, I think, mostly.
void parser_process_byte(struct parser *p, int byte);

#endif /* PARSER_H_ */

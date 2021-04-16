#ifndef KEY_H_
#define KEY_H_

#include "buffer.h"

// Translate the curses key to zero or more bytes and append them to the buffer.
// A negative is returned only on allocation failure, in which case nothing is
// appended to the buffer. If a key is ignored, calling this function with it
// will do nothing to the buffer.
int key_translate(int curses_key, struct buffer *buf);

#endif /* KEY_H_ */

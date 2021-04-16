#ifndef BUFFER_H_
#define BUFFER_H_

#include <stddef.h>
#include <stdint.h>

struct buffer {
	uint8_t *data;
	size_t len;
	size_t cap;
};

int buffer_init(struct buffer *b, size_t cap);

int buffer_append_byte(struct buffer *b, int ch);

int buffer_append_str(struct buffer *b, const char *str);

void buffer_destroy(struct buffer *b);

#endif /* BUFFER_H_ */

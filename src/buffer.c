#include "buffer.h"
#include <stdlib.h>
#include <string.h>

int buffer_init(struct buffer *b, size_t cap)
{
	b->data = malloc(cap);
	if (!b->data && cap > 0) return -1;
	b->len = 0;
	b->cap = cap;
	return 0;
}

static int grow(struct buffer *b, size_t amount)
{
	size_t new_len = b->len + amount;
	if (new_len > b->cap) {
		size_t new_cap = new_len + new_len / 2;
		uint8_t *new_data = realloc(b->data, new_cap);
		if (!new_data) return -1;
		b->data = new_data;
		b->cap = new_cap;
	}
	b->len = new_len;
	return 0;
}

int buffer_append_byte(struct buffer *b, int byte)
{
	if (grow(b, 1) < 0) return -1;
	b->data[b->len - 1] = byte;
	return 0;
}

int buffer_append_str(struct buffer *b, const char *str)
{
	size_t len = strlen(str);
	if (grow(b, len) < 0) return -1;
	memcpy(b->data + b->len - len, str, len);
	return 0;
}

void buffer_destroy(struct buffer *b)
{
	free(b->data);
}

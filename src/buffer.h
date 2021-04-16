#ifndef BUFFER_H_
#define BUFFER_H_

#include <stddef.h>
#include <stdint.h>

// A buffer is for holding a dynamic amount of ASCII text.
struct buffer {
	// The allocated data pointer, possibly NULL.
	uint8_t *data;
	// The length of the stored data.
	size_t len;
	// The size of the allocation.
	size_t cap;
};

// Initialize the buffer with the capacity. A negative is returned only when an
// allocation failure occurs.
int buffer_init(struct buffer *b, size_t cap);

// Append a byte to the end of the data. A negative is returned only on
// allocation failure.
int buffer_append_byte(struct buffer *b, int ch);

// Much like the above, but appending a NUL-terminated series of bytes instead.
int buffer_append_str(struct buffer *b, const char *str);

// Deallocate the buffer's memory.
void buffer_destroy(struct buffer *b);

#endif /* BUFFER_H_ */

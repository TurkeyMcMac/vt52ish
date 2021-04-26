exe = vt52ish
version = 0.0.6
CPPFLAGS = -DVERSION='"$(version)"' -D_XOPEN_SOURCE=600 -D_DEFAULT_SOURCE
CFLAGS = -std=c99 -Wall -Wextra -pedantic
LDLIBS = -lcurses

CC ?= cc
RM ?= rm -f

$(exe): src/* Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ src/*.c $(LDLIBS)

.PHONY: clean
clean:
	$(RM) $(exe)

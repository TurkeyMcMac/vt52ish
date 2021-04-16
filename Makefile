exe = vt52emulator
CPPFLAGS = -D_XOPEN_SOURCE=600 -D_DEFAULT_SOURCE
CFLAGS = -std=c99 -Wall -Wextra -pedantic
LDLIBS = -lcurses

CC ?= cc
RM ?= rm -f

$(exe): src/*
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ src/*.c $(LDLIBS)

.PHONY: clean
clean:
	$(RM) $(exe)

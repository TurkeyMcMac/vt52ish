# VT52ish

This is a pretty inaccurate emulator of a VT52. I'm sure it's missing several
features. I mostly wanted to make TUI programs work using `TERM=vt52`. I got my
information about VT52s from the terminfo database,
[Wikipedia](https://en.wikipedia.org/wiki/VT52), and
[this page](http://toshyp.atari.org/en/VT_52_terminal.html). The emulator runs
inside another terminal emulator using (N)Curses.

## Building

This project requires a Curses library, a C99 compiler, and a POSIX environment.
I couldn't make it completely POSIX-compliant due to a few necessary tty ioctls,
but it probably works on many Unices. If you have the right environment, you can
build it with `make`. The executable is called `vt52ish`.

## Running

To run an interactive Bash shell, something like this will work:

    ./vt52ish env PS1='$ ' bash --norc --noprofile

You can nest the emulator an arbitrary number of times:

    ./vt52ish ./vt52ish ./vt52ish env PS1='$ ' bash --norc --noprofile

Most simple ncurses programs that don't require colors will probably work to
some extent in this terminal.

## Motivation

I wanted to make this mostly to try implementing a terminal emulator. I decided
to emulate the VT52 because of the simplicity of its protocol.

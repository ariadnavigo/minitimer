# See LICENSE file for copyright and license details.

.POSIX:

include config.mk

TARG = minitimer

SRC = minitimer.c
OBJ = $(SRC:%.c=%.o)

all: $(TARG)

$(TARG): config.mk $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

.c.o:
	$(CC) -c $^ $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)

install:
	install -Dm 755 $(TARG) $(PREFIX)/bin/$(TARG)

clean:
	rm -f $(TARG) $(OBJ)

.PHONY: dbg install clean

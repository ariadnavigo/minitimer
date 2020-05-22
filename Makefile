# See LICENSE file for copyright and license details.

.POSIX:

include config.mk

TARGET = minitimer

SRC = minitimer.c
OBJ = $(SRC:%.c=%.o)

all: $(TARGET)

$(TARGET): config.mk $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

.c.o:
	$(CC) -c $^ $(CFLAGS) $(CPPFLAGS)

install:
	install -Dm 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all install clean

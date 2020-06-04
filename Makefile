# See LICENSE file for copyright and license details.

.POSIX:

include config.mk

SRC = minitimer.c
OBJ = ${SRC:%.c=%.o}

all: options minitimer

options:
	@echo Build options:
	@echo "CFLAGS 	= ${CFLAGS}"
	@echo "LDFLAGS	= ${LDFLAGS}"
	@echo "CC	= ${CC}"


.c.o:
	${CC} -c ${CFLAGS} ${CPPFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	cp config.def.h $@

minitimer: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f minitimer ${OBJ}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f minitimer ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/minitimer

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/minitimer

.PHONY: all options clean install uninstall

# See LICENSE file for copyright and license details.

.POSIX:

include config.mk

SRC = minitimer.c
OBJ = ${SRC:.c=.o}

all: options minitimer

options:
	@echo Build options:
	@echo "CPPFLAGS = ${CPPFLAGS}"
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"
	@echo

config.h:
	cp config.def.h $@

minitimer.o: config.h

${OBJ}: config.mk

minitimer: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f minitimer ${OBJ}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f minitimer ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/minitimer
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" minitimer.1 \
	    > ${DESTDIR}${MANPREFIX}/man1/minitimer.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/minitimer.1

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/minitimer \
	   ${DESTDIR}${MANPREFIX}/man1/minitimer.1

.PHONY: all options clean install uninstall

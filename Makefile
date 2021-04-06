
PREFIX = /usr/local
INSTALL_DEST = ${DESTDIR}${PREFIX}/bin/duplist

CC = gcc -std=gnu99

default: debug

debug:
	${CC} duplist.c -g -o db_duplist

release: r_duplist

r_duplist:
	${CC} duplist.c -O2 -o r_duplist

clean:
	rm db_duplist r_duplist

install: r_duplist
	cp -f r_duplist ${INSTALL_DEST}
	chmod 755 ${INSTALL_DEST}

uninstall:
	rm -f ${INSTALL_DEST}


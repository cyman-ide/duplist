
CC = gcc -std=gnu99

default: debug

debug:
	${CC} duplist.c -g -o db_duplist

release:
	${CC} duplist.c -O2 -o r_duplist


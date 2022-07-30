LDFLAGS = -static
CFLAGS = -g -pedantic -std=c99 -Wall -Wextra -Wno-unused-function

V = v0.0

all: index.cgi tmp db/category db/item db/image

tmp db/category db/item db/image:
	mkdir -p -m 700 $@
	chown www:www $@

index.cgi: index.c libgcgi.h
	${CC} ${LDFLAGS} ${CFLAGS} -o $@ index.c

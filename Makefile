LDFLAGS = -static
CFLAGS = -g -pedantic -std=c99 -Wall -Wextra -Wno-unused-function

all: index.cgi

clean:
	rm -f *.o index.cgi

index.cgi: index.c libgcgi.h
	${CC} ${LDFLAGS} ${CFLAGS} -o $@ index.c

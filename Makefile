LDFLAGS = -static
CFLAGS = -g -pedantic -std=c99 -Wall -Wextra

all: index.cgi

clean:
	rm -f *.o index.cgi

index.cgi: index.c libgcgi.c libgcgi.h
	${CC} ${LDFLAGS} ${CFLAGS} -o $@ index.c libgcgi.c

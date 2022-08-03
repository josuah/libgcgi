LDFLAGS =
#LIBS = -lseccomp #<- uncomment on Linux
CFLAGS = -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE -g -pedantic -std=c99 -Wall -Wextra

all: index.cgi

clean:
	rm -f *.o index.cgi

README: libgcgi.3
	mandoc -Tutf8 libgcgi.3 | col -b | sed '1h; $$g' >$@

index.cgi: index.c libgcgi.c libgcgi.h
	${CC} ${LDFLAGS} ${CFLAGS} -o $@ index.c libgcgi.c ${LIBS}

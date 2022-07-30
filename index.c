#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "libgcgi.h"

static struct gcgi_handler handlers[] = {
//	{ "*",				error_404 },
	{ NULL,				NULL },
};

int
main(int argc, char **argv)
{
	/* restrict allowed paths */
	unveil("gph", "r");
	unveil("tmp", "rwc");
	unveil("db", "rwc");

	/* restrict allowed system calls */
	pledge("stdio rpath wpath cpath", NULL);

	/* handle the request with the handlers */
	gcgi_handle_request(handlers, argv, argc);
	return 0;
}

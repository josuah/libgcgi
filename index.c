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

static void
error_404(char **matches)
{
	struct gcgi_var_list vars = {0};
	char *var;

	gcgi_read_var_list(&vars, "db/vars");

	printf("sorry, I could not find %s\n", matches[0]);
	if ((var = gcgi_get_var(&gcgi_gopher_query, "var")) != NULL)
		printf("I got the $var though! -> '%s'\n", var);

	gcgi_template("gph/404.gph", &vars);
}

static struct gcgi_handler handlers[] = {
	{ "*",				error_404 },
	{ NULL,				NULL },
};

int
main(int argc, char **argv)
{
	/* restrict allowed paths */
	unveil("gph", "r");
	unveil("db", "rwc");

	/* restrict allowed system calls */
	pledge("stdio rpath wpath cpath", NULL);

	/* handle the request with the handlers */
	gcgi_handle_request(handlers, argv, argc);
	return 0;
}

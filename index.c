#include <stddef.h>
#include <unistd.h>
#include <stdio.h>

#include "libgcgi.h"

#ifndef __OpenBSD__
#define pledge(p1,p2) 0
#define unveil(p1,p2) 0
#endif

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
	if (unveil("gph", "r") == -1 || unveil("db", "rwc") == -1)
		gcgi_fatal("unveil failed");

	/* restrict allowed system calls */
	if (pledge("stdio rpath wpath cpath", NULL) == -1)
		gcgi_fatal("pledge failed");

	/* handle the request with the handlers */
	gcgi_handle_request(handlers, argv, argc);
	return 0;
}

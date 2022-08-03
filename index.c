#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef __linux__
#include <seccomp.h>
#endif

#include "libgcgi.h"

static void
page_not_found(char **matches)
{
	struct gcgi_var_list vars = {0};

	gcgi_read_var_list(&vars, "db/vars");
	gcgi_set_var(&vars, "page", matches[0]);
	gcgi_template("gph/page_not_found.gph", &vars);
}

static struct gcgi_handler handlers[] = {
	{ "*",		page_not_found },
	{ NULL,		NULL },
};

int
main(int argc, char **argv)
{

#if defined(__OpenBSD__)
	if (unveil("gph", "r") == -1 || unveil("db", "rwc") == -1)
		gcgi_fatal("unveil failed: %s", strerror(errno));
	if (pledge("stdio rpath wpath cpath", NULL) == -1)
		gcgi_fatal("pledge failed: %s", strerror(errno));
#else
#warning "no syscall restriction enabled"
#endif

	/* handle the request with the handlers */
	gcgi_handle_request(handlers, argv, argc);
	return 0;
}

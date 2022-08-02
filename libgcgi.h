#ifndef LIBGCGI_H
#define LIBGCGI_H

/*
 * Gopher CGI library to use in CGI scripts
 *
 * See libgcgi(3) or the README for usage and examples.
 */

/* maps glob pattern */
struct gcgi_handler {
	char const *glob;
	void (*fn)(char **matches);
};

/* storage for key-value pair */
struct gcgi_var_list {
	struct gcgi_var {
		char *key, *val;
	} *list;
	size_t len;
	char *buf;
};

/* main loop executing h->fn() if h->glob is matching */
void gcgi_handle_request(struct gcgi_handler h[], char **argv, int argc);

/* abort the program with an error message sent to the client */
void gcgi_fatal(char *fmt, ...);

/* print a template with every "{{name}}" looked up in `vars` */
void gcgi_template(char const *path, struct gcgi_var_list *vars);

/* manage a `key`-`val` pair storage `vars`, as used with gcgi_template */
void gcgi_set_var(struct gcgi_var_list *vars, char *key, char *val);
char *gcgi_get_var(struct gcgi_var_list *vars, char *key);
void gcgi_free_var_list(struct gcgi_var_list *vars);

/* store and read a list of variables onto a simple RFC822-like format */
void gcgi_read_var_list(struct gcgi_var_list *vars, char *path);
int gcgi_write_var_list(struct gcgi_var_list *vars, char *path);

/* components of the gopher request */
extern char *gcgi_gopher_search;
extern char *gcgi_gopher_path;
extern struct gcgi_var_list gcgi_gopher_query;
extern char *gcgi_gopher_host;
extern char *gcgi_gopher_port;

/* need to be provided if not present in libc, which is rare */
char *strsep(char **s, const char *delims);

#endif

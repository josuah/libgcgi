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

#define GCGI_MATCH_NUM 5

char *gcgi_gopher_search;
char *gcgi_gopher_path;
char *gcgi_gopher_host;
char *gcgi_gopher_port;
struct gcgi_var_list gcgi_gopher_query;

void
gcgi_fatal(char *fmt, ...)
{
	va_list va;
	char msg[1024];

	va_start(va, fmt);
	vsnprintf(msg, sizeof msg, fmt, va);
	printf("error: %s\n", msg);
	exit(1);
}

static char *
gcgi_fopenread(char *path)
{
	FILE *fp;
	char *buf;
	ssize_t ssz;
	size_t sz;

	if ((fp = fopen(path, "r")) == NULL)
		return NULL;
	if (fseek(fp, 0, SEEK_END) == -1)
		return NULL;
	if ((ssz = ftell(fp)) == -1)
		return NULL;
	sz = ssz;
	if (fseek(fp, 0, SEEK_SET) == -1)
		return NULL;
	if ((buf = malloc(sz + 1)) == NULL)
		return NULL;
	if (fread(buf, sz, 1, fp) == sz) {
		errno = EFBIG;
		goto error_free;
	}
	if (ferror(fp))
		goto error_free;
	fclose(fp);
	buf[sz] = '\0';
	return buf;
error_free:
	free(buf);
	return NULL;
}

static int
gcgi_cmp_var(const void *v1, const void *v2)
{
	return strcasecmp(((struct gcgi_var *)v1)->key, ((struct gcgi_var *)v2)->key);
}

void
gcgi_add_var(struct gcgi_var_list *vars, char *key, char *val)
{
	void *mem;

	vars->len++;
	if ((mem = realloc(vars->list, vars->len * sizeof *vars->list)) == NULL)
		 gcgi_fatal("realloc");
	vars->list = mem;
	vars->list[vars->len-1].key = key;
	vars->list[vars->len-1].val = val;
}

void
gcgi_sort_var_list(struct gcgi_var_list *vars)
{
	qsort(vars->list, vars->len, sizeof *vars->list, gcgi_cmp_var);
}

char *
gcgi_get_var(struct gcgi_var_list *vars, char *key)
{
	struct gcgi_var *v, q = { .key = key };

	v = bsearch(&q, vars->list, vars->len, sizeof *vars->list, gcgi_cmp_var);
	return (v == NULL) ? NULL : v->val;
}

void
gcgi_set_var(struct gcgi_var_list *vars, char *key, char *val)
{
	struct gcgi_var *v, q;

	q.key = key;
	v = bsearch(&q, vars->list, vars->len, sizeof *vars->list, gcgi_cmp_var);
	if (v != NULL) {
		v->val = val;
		return;
	}
	gcgi_add_var(vars, key, val);
	gcgi_sort_var_list(vars);
}

void
gcgi_read_var_list(struct gcgi_var_list *vars, char *path)
{
	char *line, *tail, *key, *s;

	line = NULL;

	if ((tail = vars->buf = gcgi_fopenread(path)) == NULL)
		gcgi_fatal("opening %s: %s", path, strerror(errno));
	while ((line = strsep(&tail, "\n")) != NULL) {
		if (line[0] == '\0')
			break;
		key = strsep(&line, ":");
		if (line == NULL || *line++ != ' ')
			gcgi_fatal("%s: missing ': ' separator", path);
		gcgi_add_var(vars, key, line);
	}
	gcgi_set_var(vars, "text", tail ? tail : "");
	gcgi_set_var(vars, "file", (s = strrchr(path, '/')) ? s + 1 : path);
	gcgi_sort_var_list(vars);
}

void
gcgi_free_var_list(struct gcgi_var_list *vars)
{
	if (vars->buf != NULL)
		free(vars->buf);
	free(vars->list);
}

int
gcgi_write_var_list(struct gcgi_var_list *vars, char *dst)
{
	FILE *fp;
	struct gcgi_var *v;
	size_t n;
	char path[1024];
	char *text;

	text = NULL;

	snprintf(path, sizeof path, "%s.tmp", dst);
	if ((fp = fopen(path, "w")) == NULL)
		gcgi_fatal("opening '%s' for writing", path);

	for (v = vars->list, n = vars->len; n > 0; v++, n--) {
		if (strcasecmp(v->key, "Text") == 0) {
			text = text ? text : v->val;
			continue;
		}
		assert(strchr(v->key, '\n') == NULL);
		assert(strchr(v->val, '\n') == NULL);
		fprintf(fp, "%s: %s\n", v->key, v->val);
	}
	fprintf(fp, "\n%s", text ? text : "");

	fclose(fp);
	if (rename(path, dst) == -1)
		gcgi_fatal( "renaming '%s' to '%s'", path, dst);
	return 0;
}

static int
gcgi_match(char const *glob, char *path, char **matches, size_t m)
{
	if (m >= GCGI_MATCH_NUM)
		gcgi_fatal("too many wildcards in glob");
	matches[m] = NULL;
	while (*glob != '*' && *path != '\0' && *glob == *path)
		glob++, path++;
	if (glob[0] == '*') {
		if (*glob != '\0' && gcgi_match(glob + 1, path, matches, m + 1)) {
			if (matches[m] == NULL)
				matches[m] = path;
			*path = '\0';
			return 1;
		} else if (*path != '\0' && gcgi_match(glob, path + 1, matches, m)) {
			matches[m] = (char *)path;
			return 1;
		}
	}
	return *glob == '\0' && *path == '\0';
}

static void
gcgi_decode_url(struct gcgi_var_list *vars, char *s)
{
	char *tok, *eq;

	while ((tok = strsep(&s, "&"))) {
		//gcgi_decode_hex(tok);
		if ((eq = strchr(tok, '=')) == NULL)
			continue;
		*eq = '\0';
		gcgi_add_var(vars, tok, eq + 1);
	}
	gcgi_sort_var_list(vars);
}

void
gcgi_handle_request(struct gcgi_handler h[], char **argv, int argc)
{
	char *query_string;

	if (argc != 5)
		gcgi_fatal("wrong number of arguments: %c", argc);
	assert(argv[0] && argv[1] && argv[2] && argv[3]);

	/* executable.[d]cgi $search $arguments $host $port */
	gcgi_gopher_search = argv[1];
	gcgi_gopher_path = argv[2];
	gcgi_gopher_host = argv[3];
	gcgi_gopher_port = argv[4];
	query_string = strchr(gcgi_gopher_path, '?');
	if (query_string != NULL) {
		*query_string++ = '\0';
		gcgi_decode_url(&gcgi_gopher_query, query_string);
	}

	for (; h->glob != NULL; h++) {
		char *matches[GCGI_MATCH_NUM + 1];
		if (!gcgi_match(h->glob, gcgi_gopher_path, matches, 0))
			continue;
		h->fn(matches);
		return;
	}
	gcgi_fatal("no handler for '%s'", gcgi_gopher_path);
}

static char*
gcgi_next_var(char *head, char **tail)
{
	char *beg, *end;

	if ((beg = strstr(head, "{{")) == NULL
	  || (end = strstr(beg, "}}")) == NULL)
		return NULL;
	*beg = *end = '\0';
	*tail = end + strlen("}}");
	return beg + strlen("{{");
}

void
gcgi_template(char const *path, struct gcgi_var_list *vars)
{
	FILE *fp;
	size_t sz;
	char *line, *head, *tail, *key, *val;

	if ((fp = fopen(path, "r")) == NULL)
		gcgi_fatal("opening template %s", path);
	sz = 0;
	line = NULL;
	while (getline(&line, &sz, fp) > 0) {
		head = tail = line;
		for (; (key = gcgi_next_var(head, &tail)); head = tail) {
			fputs(head, stdout);
			if ((val = gcgi_get_var(vars, key)))
				fputs(val, stdout);
			else
				fprintf(stdout, "{{error:%s}}", key);
		}
		fputs(tail, stdout);
	}
	if (ferror(fp))
		gcgi_fatal("reading from template: %s", strerror(errno));
	fclose(fp);
}

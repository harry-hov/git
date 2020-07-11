#include "diff.h"
#include "log-tree.h"
#include "color.h"
#include "format-support.h"
#include "trailer.h"
#include "config.h"

static int istitlechar(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9') || c == '.' || c == '_';
}

void format_sanitized_subject(struct strbuf *sb, const char *msg)
{
	size_t trimlen;
	size_t start_len = sb->len;
	int space = 2;

	for (; *msg && *msg != '\n'; msg++) {
		if (istitlechar(*msg)) {
			if (space == 1)
				strbuf_addch(sb, '-');
			space = 0;
			strbuf_addch(sb, *msg);
			if (*msg == '.')
				while (*(msg+1) == '.')
					msg++;
		} else
			space |= 1;
	}

	/* trim any trailing '.' or '-' characters */
	trimlen = 0;
	while (sb->len - trimlen > start_len &&
		(sb->buf[sb->len - 1 - trimlen] == '.'
		|| sb->buf[sb->len - 1 - trimlen] == '-'))
		trimlen++;
	strbuf_remove(sb, sb->len - trimlen, trimlen);
}

static int match_placeholder_arg_value(const char *to_parse, const char *candidate,
				       const char **end, const char **valuestart,
				       size_t *valuelen)
{
	const char *p;

	if (!(skip_prefix(to_parse, candidate, &p)))
		return 0;
	if (valuestart) {
		if (*p == '=') {
			*valuestart = p + 1;
			*valuelen = strcspn(*valuestart, ",)");
			p = *valuestart + *valuelen;
		} else {
			if (*p != ',' && *p != ')')
				return 0;
			*valuestart = NULL;
			*valuelen = 0;
		}
	}
	if (*p == ',') {
		*end = p + 1;
		return 1;
	}
	if (*p == ')') {
		*end = p;
		return 1;
	}
	return 0;
}

static int match_placeholder_bool_arg(const char *to_parse, const char *candidate,
				      const char **end, int *val)
{
	const char *argval;
	char *strval;
	size_t arglen;
	int v;

	if (!match_placeholder_arg_value(to_parse, candidate, end, &argval, &arglen))
		return 0;

	if (!argval) {
		*val = 1;
		return 1;
	}

	strval = xstrndup(argval, arglen);
	v = git_parse_maybe_bool(strval);
	free(strval);

	if (v == -1)
		return 0;

	*val = v;

	return 1;
}

static int format_trailer_match_cb(const struct strbuf *key, void *ud)
{
	const struct string_list *list = ud;
	const struct string_list_item *item;

	for_each_string_list_item (item, list) {
		if (key->len == (uintptr_t)item->util &&
		    !strncasecmp(item->string, key->buf, key->len))
			return 1;
	}
	return 0;
}

const char *format_set_trailers_options(struct process_trailer_options *t_opts,
					struct string_list *filter_list, struct strbuf *sepbuf,
					const char *arg)
{
	for (;;) {
		const char *argval;
		size_t arglen;

		if (match_placeholder_arg_value(arg, "key", &arg, &argval, &arglen)) {
			uintptr_t len = arglen;

			if (!argval)
				goto trailer_out;

			if (len && argval[len - 1] == ':')
				len--;
			string_list_append(filter_list, argval)->util = (char *)len;

			t_opts->filter = format_trailer_match_cb;
			t_opts->filter_data = filter_list;
			t_opts->only_trailers = 1;
		} else if (match_placeholder_arg_value(arg, "separator", &arg, &argval, &arglen)) {
			char *fmt;

			fmt = xstrndup(argval, arglen);
			strbuf_expand(sepbuf, fmt, strbuf_expand_literal_cb, NULL);
			free(fmt);
			t_opts->separator = sepbuf;
		} else if (!match_placeholder_bool_arg(arg, "only", &arg, &t_opts->only_trailers) &&
				!match_placeholder_bool_arg(arg, "unfold", &arg, &t_opts->unfold) &&
				!match_placeholder_bool_arg(arg, "valueonly", &arg, &t_opts->value_only))
			break;
	}
	trailer_out:
		return arg;
}

size_t format_commit_color(struct strbuf *sb, const char *start,
					struct format_commit_context *c)
{
	if (starts_with(start + 1, "(auto)")) {
		c->auto_color = want_color(c->pretty_ctx->color);
		if (c->auto_color && sb->len)
			strbuf_addstr(sb, GIT_COLOR_RESET);
		return 7; /* consumed 7 bytes, "C(auto)" */
	} else {
		int ret = pretty_parse_color(sb, start, c);
		if (ret)
			c->auto_color = 0;
		/*
		* Otherwise, we decided to treat %C<unknown>
		* as a literal string, and the previous
		* %C(auto) is still valid.
		*/
		return ret;
	}
}

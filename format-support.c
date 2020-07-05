#include "diff.h"
#include "log-tree.h"
#include "color.h"
#include "format-support.h"

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

#include "diff.h"
#include "log-tree.h"
#include "color.h"
#include "format-support.h"
#include "reflog-walk.h"
#include "mailmap.h"

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

size_t pretty_parse_color(struct strbuf *sb, /* in UTF-8 */
			  const char *placeholder,
			  struct format_commit_context *c)
{
	const char *rest = placeholder;
	const char *basic_color = NULL;

	if (placeholder[1] == '(') {
		const char *begin = placeholder + 2;
		const char *end = strchr(begin, ')');
		char color[COLOR_MAXLEN];

		if (!end)
			return 0;

		if (skip_prefix(begin, "auto,", &begin)) {
			if (!want_color(c->pretty_ctx->color))
				return end - placeholder + 1;
		} else if (skip_prefix(begin, "always,", &begin)) {
			/* nothing to do; we do not respect want_color at all */
		} else {
			/* the default is the same as "auto" */
			if (!want_color(c->pretty_ctx->color))
				return end - placeholder + 1;
		}

		if (color_parse_mem(begin, end - begin, color) < 0)
			die(_("unable to parse --pretty format"));
		strbuf_addstr(sb, color);
		return end - placeholder + 1;
	}

	/*
	 * We handle things like "%C(red)" above; for historical reasons, there
	 * are a few colors that can be specified without parentheses (and
	 * they cannot support things like "auto" or "always" at all).
	 */
	if (skip_prefix(placeholder + 1, "red", &rest))
		basic_color = GIT_COLOR_RED;
	else if (skip_prefix(placeholder + 1, "green", &rest))
		basic_color = GIT_COLOR_GREEN;
	else if (skip_prefix(placeholder + 1, "blue", &rest))
		basic_color = GIT_COLOR_BLUE;
	else if (skip_prefix(placeholder + 1, "reset", &rest))
		basic_color = GIT_COLOR_RESET;

	if (basic_color && want_color(c->pretty_ctx->color))
		strbuf_addstr(sb, basic_color);

	return rest - placeholder;
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

static int mailmap_name(const char **email, size_t *email_len,
			const char **name, size_t *name_len)
{
	static struct string_list *mail_map;
	if (!mail_map) {
		mail_map = xcalloc(1, sizeof(*mail_map));
		read_mailmap(mail_map, NULL);
	}
	return mail_map->nr && map_user(mail_map, email, email_len, name, name_len);
}

size_t format_person_part(struct strbuf *sb, char part,
			  const char *msg, int len,
			  const struct date_mode *dmode)
{
	/* currently all placeholders have same length */
	const int placeholder_len = 2;
	struct ident_split s;
	const char *name, *mail;
	size_t maillen, namelen;

	if (split_ident_line(&s, msg, len) < 0)
		goto skip;

	name = s.name_begin;
	namelen = s.name_end - s.name_begin;
	mail = s.mail_begin;
	maillen = s.mail_end - s.mail_begin;

	if (part == 'N' || part == 'E' || part == 'L') /* mailmap lookup */
		mailmap_name(&mail, &maillen, &name, &namelen);
	if (part == 'n' || part == 'N') {	/* name */
		strbuf_add(sb, name, namelen);
		return placeholder_len;
	}
	if (part == 'e' || part == 'E') {	/* email */
		strbuf_add(sb, mail, maillen);
		return placeholder_len;
	}
	if (part == 'l' || part == 'L') {	/* local-part */
		const char *at = memchr(mail, '@', maillen);
		if (at)
			maillen = at - mail;
		strbuf_add(sb, mail, maillen);
		return placeholder_len;
	}

	if (!s.date_begin)
		goto skip;

	if (part == 't') {	/* date, UNIX timestamp */
		strbuf_add(sb, s.date_begin, s.date_end - s.date_begin);
		return placeholder_len;
	}

	switch (part) {
	case 'd':	/* date */
		strbuf_addstr(sb, show_ident_date(&s, dmode));
		return placeholder_len;
	case 'D':	/* date, RFC2822 style */
		strbuf_addstr(sb, show_ident_date(&s, DATE_MODE(RFC2822)));
		return placeholder_len;
	case 'r':	/* date, relative */
		strbuf_addstr(sb, show_ident_date(&s, DATE_MODE(RELATIVE)));
		return placeholder_len;
	case 'i':	/* date, ISO 8601-like */
		strbuf_addstr(sb, show_ident_date(&s, DATE_MODE(ISO8601)));
		return placeholder_len;
	case 'I':	/* date, ISO 8601 strict */
		strbuf_addstr(sb, show_ident_date(&s, DATE_MODE(ISO8601_STRICT)));
		return placeholder_len;
	case 's':
		strbuf_addstr(sb, show_ident_date(&s, DATE_MODE(SHORT)));
		return placeholder_len;
	}

skip:
	/*
	 * reading from either a bogus commit, or a reflog entry with
	 * %gn, %ge, etc.; 'sb' cannot be updated, but we still need
	 * to compute a valid return value.
	 */
	if (part == 'n' || part == 'e' || part == 't' || part == 'd'
	    || part == 'D' || part == 'r' || part == 'i')
		return placeholder_len;

	return 0; /* unknown placeholder */
}

static int format_reflog_person(struct strbuf *sb,
				char part,
				struct reflog_walk_info *log,
				const struct date_mode *dmode)
{
	const char *ident;

	if (!log)
		return 2;

	ident = get_reflog_ident(log);
	if (!ident)
		return 2;

	return format_person_part(sb, part, ident, strlen(ident), dmode);
}

int pretty_print_reflog(struct format_commit_context *c, struct strbuf *sb,
			const char *placeholder)
{
	switch (placeholder[1]) {
	case 'd':	/* reflog selector */
	case 'D':
		if (c->pretty_ctx->reflog_info)
			get_reflog_selector(sb,
					   c->pretty_ctx->reflog_info,
					   &c->pretty_ctx->date_mode,
					   c->pretty_ctx->date_mode_explicit,
					   (placeholder[1] == 'd'));
		return 2;
	case 's':	/* reflog message */
		if (c->pretty_ctx->reflog_info)
			get_reflog_message(sb, c->pretty_ctx->reflog_info);
		return 2;
	case 'n':
	case 'N':
	case 'e':
	case 'E':
		return format_reflog_person(sb,
					    placeholder[1],
					    c->pretty_ctx->reflog_info,
					    &c->pretty_ctx->date_mode);
	}
	return 0;
}

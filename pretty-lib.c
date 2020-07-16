#include "commit.h"
#include "ref-filter.h"
#include "pretty-lib.h"
#include "color.h"
#include "diff.h"
#include "log-tree.h"

static size_t convert_format(struct strbuf *sb, const char *start, void *data)
{
	struct format_commit_context *c = data;
	size_t res;
	char **slot;

	/*
	 * These are independent of the commit.
	 *
	 * For options like %n, %x00, %x2C... that expnads to
	 * string literals.
	 */
	res = strbuf_expand_literal_cb(sb, start, NULL);
	if (res)
		return res;

	/* TODO - Add support for more formatting options */
	switch (*start) {
	case 'C':
		return format_commit_color(sb, start, c);
	case 'H':
		strbuf_addstr(sb, diff_get_color(c->auto_color, DIFF_COMMIT));
		strbuf_addstr(sb, "%(objectname)");
		strbuf_addstr(sb, diff_get_color(c->auto_color, DIFF_RESET));
		return 1;
	case 'h':
		strbuf_addstr(sb, diff_get_color(c->auto_color, DIFF_COMMIT));
		strbuf_addstr(sb, "%(objectname:short)");
		strbuf_addstr(sb, diff_get_color(c->auto_color, DIFF_RESET));
		return 1;
	case 'T':
		strbuf_addstr(sb, "%(tree)");
		return 1;
	case 't':
		strbuf_addstr(sb, "%(tree:short)");
		return 1;
	case 'P':
		strbuf_addstr(sb, "%(parent)");
		return 1;
	case 'p':
		strbuf_addstr(sb, "%(parent:short)");
		return 1;
	case 'm':		/* left/right/bottom */
		strbuf_addstr(sb, get_revision_mark(NULL, c->commit));
		return 1;
	case 'd':
		format_decorations(sb, c->commit, c->auto_color);
		return 1;
	case 'D':
		format_decorations_extended(sb, c->commit, c->auto_color, "", ", ", "");
		return 1;
	case 'a':
		if (start[1] == 'n')
			strbuf_addstr(sb, "%(authorname)");
		else if (start[1] == 'e')
			strbuf_addstr(sb, "%(authoremail:trim)");
		else if (start[1] == 'l')
			strbuf_addstr(sb, "%(authoremail:localpart)");
		else if (start[1] == 'd')
			strbuf_addstr(sb, "%(authordate)");
		else if (start[1] == 'D')
			strbuf_addstr(sb, "%(authordate:rfc)");
		else if (start[1] == 'r')
			strbuf_addstr(sb, "%(authordate:relative)");
		else if (start[1] == 't')
			strbuf_addstr(sb, "%(authordate:unix)");
		else if (start[1] == 'i')
			strbuf_addstr(sb, "%(authordate:iso)");
		else if (start[1] == 'I')
			strbuf_addstr(sb, "%(authordate:iso-strict)");
		else if (start[1] == 's')
			strbuf_addstr(sb, "%(authordate:short)");
		else
			die(_("invalid formatting option '%c%c'"), start[0], start[1]);
		return 2;
	case 'c':
		if (start[1] == 'n')
			strbuf_addstr(sb, "%(committername)");
		else if (start[1] == 'e')
			strbuf_addstr(sb, "%(committeremail:trim)");
		else if (start[1] == 'l')
			strbuf_addstr(sb, "%(committeremail:localpart)");
		else if (start[1] == 'd')
			strbuf_addstr(sb, "%(committerdate)");
		else if (start[1] == 'D')
			strbuf_addstr(sb, "%(committerdate:rfc)");
		else if (start[1] == 'r')
			strbuf_addstr(sb, "%(committerdate:relative)");
		else if (start[1] == 't')
			strbuf_addstr(sb, "%(committerdate:unix)");
		else if (start[1] == 'i')
			strbuf_addstr(sb, "%(committerdate:iso)");
		else if (start[1] == 'I')
			strbuf_addstr(sb, "%(committerdate:iso-strict)");
		else if (start[1] == 's')
			strbuf_addstr(sb, "%(committerdate:short)");
		else
			die(_("invalid formatting option '%c%c'"), start[0], start[1]);
		return 2;
	case 's':
		strbuf_addstr(sb, "%(subject)");
		return 1;
	case 'f':
		strbuf_addstr(sb, "%(subject:sanitize)");
		return 1;
	case 'b':
		strbuf_addstr(sb, "%(body)");
		return 1;
	case 'B':
		strbuf_addstr(sb, "%(subject)\n\n%(body)");
		return 1;
	case 'g':		/* reflog info */
		return pretty_print_reflog(c, sb, start);
	case 'N':
		if (c->pretty_ctx->notes_message) {
			strbuf_addstr(sb, c->pretty_ctx->notes_message);
			return 1;
		}
		return 0;
	case 'e':	/* encoding */
		if (c->commit_encoding)
			strbuf_addstr(sb, c->commit_encoding);
		return 1;
	case 'S':		/* tag/branch like --source */
		if (!(c->pretty_ctx->rev && c->pretty_ctx->rev->sources))
			return 0;
		slot = revision_sources_at(c->pretty_ctx->rev->sources, c->commit);
		if (!(slot && *slot))
			return 0;
		strbuf_addstr(sb, *slot);
		return 1;
	default:
		die(_("invalid formatting option '%c'"), *start);
	}
}

void ref_pretty_print_commit(struct pretty_print_context *pp,
			 const struct commit *commit,
			 struct strbuf *sb)
{
	struct ref_format format = REF_FORMAT_INIT;
	struct strbuf sb_fmt = STRBUF_INIT;
	struct format_commit_context fmt_ctx = {
		.commit = commit,
		.pretty_ctx = pp,
		.wrap_start = sb->len
	};
	const char *name = "refs";
	const char *usr_fmt = get_user_format();

	if (pp->fmt == CMIT_FMT_USERFORMAT) {
		strbuf_expand(&sb_fmt, usr_fmt, convert_format, &fmt_ctx);
		format.format = sb_fmt.buf;
	} else if (pp->fmt == CMIT_FMT_DEFAULT || pp->fmt == CMIT_FMT_MEDIUM) {
		format.format = "Author: %(authorname) %(authoremail)\nDate:   %(authordate)\n\n%(subject)\n%(contents:body)";
	} else if (pp->fmt == CMIT_FMT_ONELINE) {
		format.format = "%(subject)";
	} else if (pp->fmt == CMIT_FMT_SHORT) {
		format.format = "Author: %(authorname) %(authoremail)\n\n%(subject)";
	} else if (pp->fmt == CMIT_FMT_FULL) {
		format.format = "Author: %(authorname) %(authoremail)\nCommit: %(committername) %(committeremail)\n\n%(subject)\n%(contents:body)";
	} else if (pp->fmt == CMIT_FMT_FULLER) {
		format.format = "Author:     %(authorname) %(authoremail)\nAuthorDate: %(authordate)\nCommit:     %(committername) %(committeremail)\nCommitDate: %(committerdate)\n\n%(subject)\n%(contents:body)";
	}

	format.need_newline_at_eol = 0;

	verify_ref_format(&format);
	pretty_print_ref(name, &commit->object.oid, &format);
	strbuf_release(&sb_fmt);
}

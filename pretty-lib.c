#include "commit.h"
#include "ref-filter.h"
#include "pretty-lib.h"

static size_t convert_format(struct strbuf *sb, const char *start, void *data)
{
	/* TODO - Add support for more formatting options */
	switch (*start) {
	case 'H':
		strbuf_addstr(sb, "%(objectname)");
		return 1;
	case 'h':
		strbuf_addstr(sb, "%(objectname:short)");
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
	case 'a':
		if (start[1] == 'n')
			strbuf_addstr(sb, "%(authorname)");
		else if (start[1] == 'e')
			strbuf_addstr(sb, "%(authoremail:trim)");
		else if (start[1] == 'l')
			strbuf_addstr(sb, "%(authoremail:localpart)");
		else if (start[1] == 'd')
			strbuf_addstr(sb, "%(authordate)");
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
		else
			die(_("invalid formatting option '%c%c'"), start[0], start[1]);
		return 2;
	case 's':
		strbuf_addstr(sb, "%(subject)");
		return 1;
	case 'b':
		strbuf_addstr(sb, "%(body)");
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
	const char *name = "refs";
	const char *usr_fmt = get_user_format();

	if (pp->fmt == CMIT_FMT_USERFORMAT) {
		strbuf_expand(&sb_fmt, usr_fmt, convert_format, NULL);
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

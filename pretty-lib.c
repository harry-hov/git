#include "commit.h"
#include "ref-filter.h"
#include "pretty-lib.h"
#include "color.h"
#include "pretty.h"
#include "diff.h"
#include "log-tree.h"

static size_t convert_format(struct strbuf *sb, const char *start, void *data)
{
	struct format_commit_context *c = data;
	const struct commit *commit = c->commit;

	if (start[0] == 'G') {
		if (!c->signature_check.result)
			check_commit_signature(c->commit, &(c->signature_check));
		switch (start[1]) {
		case 'G':
			strbuf_addstr(sb, "%(contents:signature)");
			/*
			if (c->signature_check.gpg_output)
				strbuf_addstr(sb, c->signature_check.gpg_output);
			*/
			break;
		case '?':
			switch (c->signature_check.result) {
			case 'G':
				switch (c->signature_check.trust_level) {
				case TRUST_UNDEFINED:
				case TRUST_NEVER:
					strbuf_addch(sb, 'U');
					break;
				default:
					strbuf_addch(sb, 'G');
					break;
				}
				break;
			case 'B':
			case 'E':
			case 'N':
			case 'X':
			case 'Y':
			case 'R':
				strbuf_addch(sb, c->signature_check.result);
			}
			break;
		case 'S':
			if (c->signature_check.signer)
				strbuf_addstr(sb, c->signature_check.signer);
			break;
		case 'K':
			if (c->signature_check.key)
				strbuf_addstr(sb, c->signature_check.key);
			break;
		case 'F':
			if (c->signature_check.fingerprint)
				strbuf_addstr(sb, c->signature_check.fingerprint);
			break;
		case 'P':
			if (c->signature_check.primary_key_fingerprint)
				strbuf_addstr(sb, c->signature_check.primary_key_fingerprint);
			break;
		case 'T':
			switch (c->signature_check.trust_level) {
			case TRUST_UNDEFINED:
				strbuf_addstr(sb, "undefined");
				break;
			case TRUST_NEVER:
				strbuf_addstr(sb, "never");
				break;
			case TRUST_MARGINAL:
				strbuf_addstr(sb, "marginal");
				break;
			case TRUST_FULLY:
				strbuf_addstr(sb, "fully");
				break;
			case TRUST_ULTIMATE:
				strbuf_addstr(sb, "ultimate");
				break;
			}
			break;
		default:
			die(_("invalid formatting option '%c%c'"), start[0], start[1]);
		}
		return 2;
	}

	/* TODO - Add support for more formatting options */
	switch (*start) {
	case 'C':
		if (starts_with(start + 1, "(auto)")) {
			c->auto_color = want_color(c->pretty_ctx->color);
			if (c->auto_color && sb->len)
				strbuf_addstr(sb, GIT_COLOR_RESET);
			return 7; /* consumed 7 bytes, "C(auto)" */
		} else {
			int ret = parse_color(sb, start, c);
			if (ret)
				c->auto_color = 0;
			/*
			 * Otherwise, we decided to treat %C<unknown>
			 * as a literal string, and the previous
			 * %C(auto) is still valid.
			 */
			return ret;
		}
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
		strbuf_addstr(sb, get_revision_mark(NULL, commit));
		return 1;
	case 'd':
		format_decorations(sb, commit, c->auto_color);
		return 1;
	case 'D':
		format_decorations_extended(sb, commit, c->auto_color, "", ", ", "");
		return 1;
	case 'a':
		if (start[1] == 'n')
			strbuf_addstr(sb, "%(authorname)");
		else if (start[1] == 'e')
			strbuf_addstr(sb, "%(authoremail)");
		else if (start[1] == 'd')
			strbuf_addstr(sb, "%(authordate)");
		else
			die(_("invalid formatting option '%c%c'"), start[0], start[1]);
		return 2;
	case 'c':
		if (start[1] == 'n')
			strbuf_addstr(sb, "%(committername)");
		else if (start[1] == 'e')
			strbuf_addstr(sb, "%(committeremail)");
		else if (start[1] == 'd')
			strbuf_addstr(sb, "%(committerdate)");
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
	case 'n':
		strbuf_addstr(sb, "\n");
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
	const char *name = "refs/heads/";
	const char *usr_fmt = get_user_format();

	if (pp->fmt == CMIT_FMT_USERFORMAT) {
		strbuf_expand(&sb_fmt, usr_fmt, convert_format, &fmt_ctx);
		format.format = sb_fmt.buf;
	} else if (pp->fmt == CMIT_FMT_DEFAULT || pp->fmt == CMIT_FMT_MEDIUM) {
		format.format = "Author: %(authorname) %(authoremail)\nDate:\t%(authordate)\n\n%(subject)\n\n%(body)";
	} else if (pp->fmt == CMIT_FMT_ONELINE) {
		format.format = "%(subject)";
	} else if (pp->fmt == CMIT_FMT_SHORT) {
		format.format = "Author: %(authorname) %(authoremail)\n\n\t%(subject)\n";
	} else if (pp->fmt == CMIT_FMT_FULL) {
		format.format = "Author: %(authorname) %(authoremail)\nCommit: %(committername) %(committeremail)\n\n%(subject)\n\n%(body)";
	} else if (pp->fmt == CMIT_FMT_FULLER) {
		format.format = "Author:\t\t%(authorname) %(authoremail)\nAuthorDate:\t%(authordate)\nCommit:\t\t%(committername) %(committeremail)\nCommitDate:\t%(committerdate)\n\n%(subject)\n\n%(body)";
	}

	format.need_newline_at_eol = 0;

	verify_ref_format(&format);
	pretty_print_ref(name, &commit->object.oid, &format);
	strbuf_release(&sb_fmt);
}

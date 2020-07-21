#ifndef FORMAT_SUPPORT_H
#define FORMAT_SUPPORT_H

struct pretty_chunk {
	size_t off;
	size_t len;
};

enum pp_flush_type {
	pp_no_flush,
	pp_flush_right,
	pp_flush_left,
	pp_flush_left_and_steal,
	pp_flush_both
};

enum pp_trunc_type {
	pp_trunc_none,
	pp_trunc_left,
	pp_trunc_middle,
	pp_trunc_right
};

struct format_commit_context {
	const struct commit *commit;
	const struct pretty_print_context *pretty_ctx;
	unsigned commit_header_parsed:1;
	unsigned commit_message_parsed:1;
	unsigned respect_mailmap:1;
	struct signature_check signature_check;
	enum pp_flush_type flush_type;
	enum pp_trunc_type truncate;
	const char *message;
	char *commit_encoding;
	size_t width, indent1, indent2;
	int auto_color;
	int padding;

	/* These offsets are relative to the start of the commit message. */
	struct pretty_chunk author;
	struct pretty_chunk committer;
	size_t message_off;
	size_t subject_off;
	size_t body_off;

	/* The following ones are relative to the result struct strbuf. */
	size_t wrap_start;
};

void format_sanitized_subject(struct strbuf *sb, const char *msg);

/* Parse color */
size_t pretty_parse_color(struct strbuf *sb, /* in UTF-8 */
			  const char *placeholder,
			  struct format_commit_context *c);

size_t format_commit_color(struct strbuf *sb, const char *start,
			   struct format_commit_context *c);

size_t format_person_part(struct strbuf *sb, char part,
			  const char *msg, int len,
			  const struct date_mode *dmode);

int pretty_print_reflog(struct format_commit_context *c,
			struct strbuf *sb, const char *placeholder);

size_t parse_padding_placeholder(const char *placeholder,
				 struct format_commit_context *c);

int pretty_switch_line_wrapping(struct strbuf *sb, const char *placeholder,
				struct format_commit_context *c);

void rewrap_message_tail(struct strbuf *sb,
			 struct format_commit_context *c,
			 size_t new_width, size_t new_indent1,
			 size_t new_indent2);

int pretty_mailmap_name(const char **email, size_t *email_len,
		 	const char **name, size_t *name_len);

int is_blank_line(const char *line, int *len_p);

int get_one_line(const char *msg);

#endif /* FORMAT_SUPPORT_H */

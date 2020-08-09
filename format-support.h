#ifndef FORMAT_SUPPORT_H
#define FORMAT_SUPPORT_H

struct process_trailer_options;
struct string_list;

void format_sanitized_subject(struct strbuf *sb, const char *msg, size_t len);

int format_set_trailers_options(struct process_trailer_options *opts,
				struct string_list *filter_list,
				struct strbuf *sepbuf,
				const char **arg,
				const char **err);

#endif /* FORMAT_SUPPORT_H */

#ifndef FORMAT_SUPPORT_H
#define FORMAT_SUPPORT_H

#include "pretty.h"
#include "trailer.h"

void format_sanitized_subject(struct strbuf *sb, const char *msg);

size_t format_commit_color(struct strbuf *sb, const char *start,
					struct format_commit_context *c);

const char *format_set_trailers_options(struct process_trailer_options *t_opts,
					struct string_list *filter_list, struct strbuf *sepbuf,
					const char *arg);

#endif /* FORMAT_SUPPORT_H */

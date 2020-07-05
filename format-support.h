#ifndef FORMAT_SUPPORT_H
#define FORMAT_SUPPORT_H

#include "pretty.h"

void format_sanitized_subject(struct strbuf *sb, const char *msg);

size_t format_commit_color(struct strbuf *sb, const char *start,
					struct format_commit_context *c);

#endif /* FORMAT_SUPPORT_H */

#ifndef __HTML_GREP_H__
#define __HTML_GREP_H__

#include <stddef.h>

void htmlStanitizeText(char *buf, char *outbuf, size_t size);
void htmlQuikSanitize(char *in, char *out);
void htmlGrepword(char *buf, char *word, int len);
void htmlGrepFromUntil(char *buf, char *from, int fromlen, char *until,
        int untilLen, char *outbuf);

#endif

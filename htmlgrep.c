#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "htmlgrep.h"

enum htmlstate {
    COLLECT,
    WALK,
    IN_TAG
};

struct map {
    int idx;
    char *name;
} state_map[] = {
    { COLLECT, "collect" },
    { WALK, "walk" },
    { IN_TAG, "in_tag" },
};

/* Anything that starts with whitespace is not something we are interested in
 *
 * AND '
 * as ' will kill the SQL insert statement
 */
#define iscollectable(c) ((c) >= 33 && (c) <= 126 && (c) != 39)

typedef struct grepClosure {
    char *until;
    char **out;
    int len;
} grepClosure;

static inline enum htmlstate htmlwalk(char *buf, enum htmlstate state) {
    switch (state) {
        case WALK: {
            if (*buf == '<' && *(buf + 1) == '/') return WALK;
            if (*buf == '<' && *(buf + 1) != '/') return IN_TAG;
            if (*buf == '>' && state == WALK) return COLLECT;

            return WALK;
        }

        case IN_TAG: {
            if (*buf == '>' && state == WALK) return COLLECT;
            return WALK;
        }

        case COLLECT: {
            if (*buf == '<') return WALK;
            return COLLECT;
        }

        default:
            return WALK;
    }
}

static void htmlGrepUntil(char **buf, void *closure) {
    grepClosure *gc = closure;
    enum htmlstate state;
    char tmp[65355] = {'\0'};
    char **ptr, *tmpptr;
    int i;
    
    ptr = buf;
    tmpptr = tmp;
    state = WALK;
    (*ptr)--;

    while (**ptr != '\0') {
        if (**ptr == gc->until[0] && state != COLLECT) {
            for (i = 0; i < gc->len; ++i) {
                if (**ptr == gc->until[i]) {
                    (*ptr)++;
                    continue;
                }
                break;
            }
            if (i == gc->len) {
                *tmpptr = '\n';
                *(tmpptr + 1) = '\0';
                strcat(*gc->out, tmp);
                (*ptr)++;
                return;
            }
        }
        if (state == COLLECT) {
            while (**ptr != '<' && **ptr != '\0') {
                if (iscollectable(**ptr) || **ptr == ' ')
                    *tmpptr++ = **ptr;
                (*ptr)++;
            }
            state = WALK;
            continue;
        }
        state = htmlwalk(*ptr, state);
        (*ptr)++;
    }
}

void htmlGrepAndExec(char *buf, char *word, int len, void *data,
        void (*process)(char **, void *))
{
    char *ptr;
    int i;

    ptr = buf;

    while (*ptr != 0) {
        if (*ptr == word[0]) {
            for (i = 0; i < len; ++i) {
                if (word[i] == *ptr) {
                    ptr++;
                    continue;
                }
                break;
            }
            if (i == len) {
                ptr--;
                if (process != NULL) process(&ptr, data);
            }
        }
        ptr++;
    }
}

void htmlGrepFromUntil(char *buf, char *from, int fromlen, char *until,
        int untilLen, char *outbuf)
{
    grepClosure gc;
    
    gc.len = untilLen;
    gc.out = &outbuf;
    gc.until = until;

    htmlGrepAndExec(buf, from, fromlen, &gc, htmlGrepUntil);
}

void htmlGrepword(char *buf, char *word, int len) {
    htmlGrepAndExec(buf, word, len, NULL, NULL);
}

static void htmlwalkCollect(char **buf, char **outbuf) {
    while (**buf != '<' && **buf != '\0') {
        if (outbuf) *(*outbuf)++ = **buf;
        (*buf)++;
    }
}

void htmlStanitizeText(char *buf, char *outbuf, size_t size) {
    char tmp[2000];
    char *ptr;
    size_t i;

    ptr = tmp;
    i = 0;

    while (!iscollectable(*buf)) buf++;
    while (*buf != '\0') {
        if (*buf == '\n') {
            *ptr++ = '\n';
            i++;
            if (i == size) break;
            while (!iscollectable(*buf)) {
                buf++;
                if (*buf == '\0') break;
            }
        }
        *ptr++ = *buf;
        i++;
        if (i == size) break;
        buf++;
    }
    *(ptr - 1) = '\0';
    if (outbuf) strcpy(outbuf, tmp);
}

void htmlExtractText(char *buf, char *outbuf, size_t size) {
    enum htmlstate state;
    char out[2000];
    char *ptr, *outptr;
    
    ptr = buf;
    outptr = out;
    state = WALK;

    while (*ptr != '\0') {
        if (state == COLLECT) htmlwalkCollect(&ptr, &outptr);
        state = htmlwalk(ptr, state);
        ptr++;
    }

    *outptr = '\0';
    htmlStanitizeText(out, outbuf, size);
}

void htmlQuikSanitize(char *in, char *out) {
    char *ptr;
    ptr = in;
    
    while (*ptr != '\0') {
        if (isalnum(*ptr) || *ptr == ' ' || *ptr == '\n') {
            *out = *ptr;
        }
        ptr++;
    }
}

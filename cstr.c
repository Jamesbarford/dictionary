/**
 * Make c strings slightly less painful to work with
 */
#include <sys/types.h>

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cstr.h"

#if defined(linux) || defined(__linux__)
#include <endian.h>
#elif defined(__APPLE__) && defined(MAC_OS_X_VERSION_10_6)
#include <machine/endian.h>
#endif

#define CSTR_PAD (sizeof(unsigned int) + 1)
#define CSTR_CAPACITY (0)
#define CSTR_LEN (1)

static void
revU32(void *ptr)
{
#if (BYTE_ORDER == BIG_ENDIAN)
    unsigned char *str = ptr;
    unsigned char tmp;

    tmp = str[0];
    str[0] = str[3];
    str[3] = tmp;
    tmp = str[1];
    str[1] = str[2];
    str[2] = tmp;
#endif
    (void)ptr;
}

void
cstrRelease(void *str)
{
    if (str) {
        cstr *s = (cstr *)str;
        s -= (CSTR_PAD * 2);
        s[9] = 'p';
        free(s);
    }
}

/* Set either the length of the string or the buffer capacity */
static void
cstrSetNum(cstr *str, unsigned int num, int type, char terminator)
{
    unsigned char *_str = (unsigned char *)str;
    int offset = 0;

    if (type == CSTR_CAPACITY) {
        offset = (CSTR_PAD * 2);
    } else if (type == CSTR_LEN) {
        offset = CSTR_PAD;
    } else {
        return;
    }

    _str -= offset;
    _str[0] = (num >> 24) & 0xFF;
    _str[1] = (num >> 16) & 0xFF;
    _str[2] = (num >> 8) & 0xFF;
    _str[3] = num & 0xFF;
    (_str)[4] = terminator;
    revU32(_str);
    _str += offset;
}

/* Get number out of cstr predicated on type */
static unsigned int
cstrGetNum(cstr *str, int type)
{
    unsigned char *_str = (unsigned char *)str;
    unsigned int len = 0;

    if (type == CSTR_CAPACITY) {
        _str -= (CSTR_PAD * 2);
    } else if (type == CSTR_LEN) {
        _str -= CSTR_PAD;
    } else {
        return len;
    }

    revU32(_str);
    return ((unsigned int)_str[0] << 24) | ((unsigned int)_str[1] << 16) |
            ((unsigned int)_str[2] << 8) | (unsigned int)_str[3];
}

/* Set unsigned int for length of string and NULL terminate */
void
cstrSetLen(cstr *str, unsigned int len)
{
    cstrSetNum(str, len, CSTR_LEN, '\0');
    str[len] = '\0';
}

/* Set unsigned int for capacity of string buffer */
static void
cstrSetCapacity(cstr *str, unsigned int capacity)
{
    cstrSetNum(str, capacity, CSTR_CAPACITY, '\n');
}

/* Get the size of the buffer */
unsigned int
cstrGetCapacity(cstr *str)
{
    return cstrGetNum(str, CSTR_CAPACITY);
}

/* Get the integer out of the string */
unsigned int
cstrlen(cstr *str)
{
    return cstrGetNum(str, CSTR_LEN);
}

/* Grow the capacity of the string buffer by `additional` space */
static cstr *
cstrExtendBuffer(cstr *str, unsigned int additional)
{
    unsigned int current_capacity = cstrGetCapacity(str);
    unsigned int new_capacity = current_capacity + additional;

    if (new_capacity <= cstrGetCapacity(str)) {
        return str;
    }

    unsigned char *_str = (unsigned char *)str;
    _str -= (CSTR_PAD * 2);
    unsigned char *tmp = (unsigned char *)realloc(_str, new_capacity);

    if (tmp == NULL) {
        _str += (CSTR_PAD * 2);
        return str;
    }

    _str = tmp;
    _str += (CSTR_PAD * 2);
    cstrSetCapacity(_str, new_capacity);

    return (cstr *)_str;
}

/* Only extend the buffer if the additional space required would overspill the
 * current allocated capacity of the buffer */
cstr *
cstrExtendBufferIfNeeded(cstr *str, unsigned int additional)
{
    unsigned int len = cstrlen(str);
    unsigned int capacity = cstrGetCapacity(str);

    if ((len + 1 + additional) >= capacity) {
        return cstrExtendBuffer(str, additional > 256 ? additional : 256);
    }

    return str;
}

/* Allocate a new string `capacity` in size  */
cstr *
cstrAlloc(unsigned int capacity)
{
    cstr *out;

    if ((out = (unsigned char *)malloc(sizeof(unsigned char) * (capacity + 2) +
                 (CSTR_PAD * 2))) == NULL) {
        return NULL;
    }

    out += (CSTR_PAD * 2);
    cstrSetLen(out, 0);
    cstrSetCapacity(out, capacity);

    return out;
}

/* Create a new string buffer with a capacity of 2 ^ 8 */
cstr *
cstrnew(void)
{
    unsigned int capacity = 1 << 8;
    return cstrAlloc(capacity);
}

/* Lowercase a cstr */
void
cstrToLowerCase(cstr *c)
{
    unsigned int len = cstrlen(c);
    for (unsigned int i = 0; i < len; ++i) {
        c[i] = tolower(c[i]);
    }
}

/* Uppercase a cstr */
void
cstrToUpperCase(cstr *c)
{
    unsigned int len = cstrlen(c);
    for (unsigned int i = 0; i < len; ++i) {
        c[i] = toupper(c[i]);
    }
}

/* Append 1 character to the end of the string and increment the length, try
 * avoid using this with really big strings */
cstr *
cstrPutChar(cstr *str, char ch)
{
    unsigned int curlen = cstrlen(str);

    /* 100 is arbitary but theoretically if we're putting one character at time
     * and the string is small this should limit how many allocations we are
     * making */
    str = cstrExtendBufferIfNeeded(str, 100);
    str[curlen] = ch;
    str[curlen + 1] = '\0';
    cstrSetLen(str, curlen + 1);
    return str;
}

/* Remove the last character from the buffer and return it, decrements the
 * length */
unsigned char
cstrUnPutChar(cstr *str)
{
    unsigned len = cstrlen(str);
    unsigned char ch = '\0';

    if (len == 0) {
        return ch;
    }

    ch = str[len - 1];
    cstrSetLen(str, len - 1);
    return ch;
}

/* Compare size bytes of s1 and s2 */
int
cstrnCmp(cstr *s1, cstr *s2, unsigned int size)
{
    return memcmp(s1, s2, size);
}

int
cstrCmp(cstr *s1, cstr *s2)
{
    unsigned int l1 = cstrlen(s1);
    unsigned int l2 = cstrlen(s2);
    /* Lets not exceed the '\0' for either strings! */
    unsigned int minlen = l1 < l2 ? l1 : l2;

    return memcmp(s1, s2, minlen);
}

/* Slice the current string, mutating it  */
cstr *
cstrMutableSlice(cstr *s, unsigned int from, unsigned int to, unsigned int size)
{
    memmove(s + to, s + from, size);
    cstrSetLen(s, size);
    return s;
}

/* Write to buffer len bytes */
cstr *
cstrWrite(cstr *s, char *s2, unsigned int len)
{
    if (cstrlen(s) < len) {
        s = cstrExtendBuffer(s, len + 1);
    }
    memcpy(s, s2, len);
    cstrSetLen(s, len);
    return s;
}

cstr *
cstrDupRaw(char *s, unsigned int len, unsigned int capacity)
{
    cstr *dupe = cstrAlloc(capacity);
    memcpy(dupe, s, len);
    cstrSetLen(dupe, len);
    cstrSetCapacity(dupe, capacity);
    return dupe;
}

/* Duplicate the string */
cstr *
cstrDup(cstr *s)
{
    return cstrDupRaw((char *)s, cstrlen(s), cstrGetCapacity(s));
}

/* Concatinate len bytes of buf to the string */
cstr *
cstrCatLen(cstr *str, const void *buf, unsigned int len)
{
    unsigned int curlen = cstrlen(str);

    str = cstrExtendBufferIfNeeded(str, len);
    if (str == NULL) {
        return NULL;
    }
    memcpy(str + curlen, buf, len);
    cstrSetLen(str, curlen + len);
    return str;
}

/* Concatinate s to the end of str */
cstr *
cstrCat(cstr *str, char *s)
{
    return cstrCatLen(str, s, (unsigned int)strlen(s));
}

/**
 * Limits, `strlen(fmt) * 3` will not work for massive strings but should
 * suffice for our use case
 */
cstr *
cstrCatPrintf(cstr *s, const char *fmt, ...)
{
    va_list ap, copy;
    va_start(ap, fmt);

    /* Probably big enough */
    int min_len = 512;
    int bufferlen = strlen(fmt) * 3;
    bufferlen = bufferlen > min_len ? bufferlen : min_len;
    char *buf = (char *)malloc(sizeof(char) * bufferlen);

    while (1) {
        buf[bufferlen - 2] = '\0';
        va_copy(copy, ap);
        vsnprintf(buf, bufferlen, fmt, ap);
        va_end(copy);
        if (buf[bufferlen - 2] != '\0') {
            free(buf);
            bufferlen *= 2;
            buf = malloc(bufferlen);
            if (buf == NULL) {
                return NULL;
            }
            continue;
        }
        break;
    }

    cstr *newstr = cstrCatLen(s, buf, strlen(buf));

    free(buf);
    va_end(ap);
    return newstr;
}

cstr *
cstrEscapeString(char *buf)
{
    cstr *outstr = cstrnew();
    unsigned char *ptr = (unsigned char *)buf;

    if (buf == NULL) {
        return NULL;
    }

    while (*ptr) {
        if (*ptr > 31 && *ptr != '\"' && *ptr != '\\') {
            outstr = cstrPutChar(outstr, *ptr);
        } else {
            outstr = cstrPutChar(outstr, '\\');
            switch (*ptr) {
            case '\\':
                outstr = cstrPutChar(outstr, '\\');
                break;
            case '\"':
                outstr = cstrPutChar(outstr, '\"');
                break;
            case '\b':
                outstr = cstrPutChar(outstr, 'b');
                break;
            case '\n':
                outstr = cstrPutChar(outstr, 'n');
                break;
            case '\t':
                outstr = cstrPutChar(outstr, 't');
                break;
            case '\v':
                outstr = cstrPutChar(outstr, 'v');
                break;
            case '\f':
                outstr = cstrPutChar(outstr, 'f');
                break;
            case '\r':
                outstr = cstrPutChar(outstr, 'r');
                break;
            default:
                outstr = cstrCatPrintf(outstr, "u%04x", (unsigned int)*ptr);
                break;
            }
        }
        ++ptr;
    }
    return outstr;
}

static int *
cstrComputePrefix(char *pattern, unsigned int patternlen)
{
    int *table = (int *)malloc(sizeof(int) * patternlen);
    table[0] = 0;
    int k = 0;

    for (unsigned int i = 1; i < patternlen; ++i) {
        while (k > 0 && pattern[k] != pattern[i]) {
            k = table[k - 1];
        }
        if (pattern[k] == pattern[i]) {
            k++;
        }
        table[i] = k;
    }
    return table;
}

/* Implementation of KMP Matcher algorithm from introduction to algorithms */
int
cstrContainsPattern(void *buf, unsigned int buflen, char *pattern,
        unsigned int patternlen)
{
    char *_buf = buf;
    int retval = -1;
    if (patternlen == 0) {
        return -1;
    }

    int *table = cstrComputePrefix(pattern, patternlen);
    unsigned int q = 0;
    for (unsigned int i = 0; i < buflen; ++i) {
        if (_buf[i] == '\0') {
            break;
        }
        while (q > 0 && pattern[q] != _buf[i]) {
            q = table[q - 1];
        }
        if (pattern[q] == _buf[i]) {
            q++;
        }

        if (q == patternlen) {
            retval = i;
            break;
        }
    }

    free(table);
    return retval;
}

void
cstrArrayRelease(cstr **arr, int count)
{
    if (arr) {
        for (int i = 0; i < count; ++i)
            cstrRelease(arr[i]);
        free(arr);
    }
}

/**
 * Split into cstrings on delimiter
 */
cstr **
cstrSplit(char *to_split, char delimiter, int *count)
{
    cstr **outArr, **tmp;
    char *ptr;
    int arrsize, memslot;
    long start, end;

    if (*to_split == delimiter) {
        to_split++;
    }

    memslot = 5;
    ptr = to_split;
    *count = 0;
    start = end = 0;
    arrsize = 0;

    if ((outArr = malloc(sizeof(char) * memslot)) == NULL)
        return NULL;

    while (*ptr != '\0') {
        if (memslot < arrsize + 5) {
            memslot *= 5;
            if ((tmp = (cstr **)realloc(outArr, sizeof(cstr) * memslot)) ==
                    NULL)
                goto error;
            outArr = tmp;
        }

        if (*ptr == delimiter) {
            outArr[arrsize] = cstrDupRaw(to_split + start, end - start,
                    end - start);
            ptr++;
            arrsize++;
            start = end + 1;
            end++;
            continue;
        }

        end++;
        ptr++;
    }

    outArr[arrsize] = cstrDupRaw(to_split + start, end - start, end - start);
    arrsize++;
    *count = arrsize;

    return outArr;

error:
    cstrArrayRelease(outArr, arrsize);
    return NULL;
}

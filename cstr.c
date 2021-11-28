#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "cstr.h"

void cstrRelease(void *str) {
    if (str) {
        cstr *s = str;
        s -= CSTR_PAD;
        s[4] = 'p';
        free(s);
    }
} 

void cstrArrayRelease(cstr **arr, int count) {
    if (arr) {
        for (int i = 0; i < count; ++i)
            cstrRelease(arr[i]);
        free(arr);
    }
} 

void cstrSetLen(cstr *str, unsigned int len) {
    unsigned char *_str = (unsigned char *)str;
    _str -= CSTR_PAD;
    _str[0] = (len >> 24);
    _str[1] = (len >> 16);
    _str[2] = (len >>  8);
    _str[3] = len;
    (_str)[4] = '\0';
}

/* Get the integer out of the string */
unsigned int cstrlen(cstr *str) {
    unsigned char *_str = (unsigned char *)str;

    _str -= CSTR_PAD;

    return ((unsigned int)_str[0] << 24) |
          ((unsigned int)_str[1] << 16) |
          ((unsigned int)_str[2] << 8)  |
          _str[3];
}

int cstrToString(cstr *str, char *outbuf, int outbuflen) {
    int len = cstrlen(str);
    if (outbuflen < len) return CSTR_ERR;

    memcpy(outbuf, str, len);
    outbuf[len] = '\0';

    return CSTR_OK;
}

/**
 * The first 5 bytes are reserved. 4 for an integer, with the 5th being
 * a null terminator.
 * Resultant buffer looks like this:
 *   LEN     LEN      LEN     LEN    END   STRING    END
 * ['0xFF', '0xFF', '0xFF', '0xFF', '\0', 'a', 'b', '\0'];
 *
 * With the pointer moved forward twice to the start of the string. OR all
 * of the LEN properties together to get the length of the string;
 */
cstr *cstrEmpty(unsigned int len) {
    cstr *out;

    if ((out = calloc(sizeof(char), len + 2 + CSTR_PAD)) == NULL)
        return NULL;

    out += CSTR_PAD;
    return out;
}

cstr *cstrCreate(char *original, unsigned int len) {
    cstr *outbuf;

    if ((outbuf = cstrEmpty(len)) == NULL)
        return NULL;

    cstrSetLen(outbuf, len);
    memcpy(outbuf, original, len);
    outbuf[len] = '\0';
    return outbuf;
}

cstr *cstrdup(cstr *original) {
    cstr *duped;
    int len;

    len = cstrlen(original);

    if ((duped = cstrEmpty(len)) == NULL)
        return NULL;

    cstrSetLen(duped, len);
    memcpy(duped, original, len);

    return duped;
}

/* this feels better than trying to guess the length of what we want */
cstr *cstrCopyUntil(char *original, char terminator) {
    unsigned int i;
    char *ptr;
    cstr *str;

    ptr = original;
    i = 0;

    // figure out how long the string is
    while (*ptr != terminator && *ptr != '\0') { 
        i++;
        ptr++;
    }

    if ((str = cstrEmpty(i)) == NULL) return NULL;
    memcpy(str, original, i);
    cstrSetLen(str, i);

    str[i] = '\0';

    return str;
}

/**
 * Split into csts on delimiter
 */
cstr **cstrSplit(char *to_split, char delimiter, int *count) {
    cstr **outArr, *ptr;
    int i;
    char tmp[BUFSIZ];

    if (*to_split == delimiter)
        to_split++;

    ptr = to_split;
    *count = 0;
    i = 0;

    if ((outArr = malloc(sizeof(char) * 1)) == NULL)
        return NULL;

    while (*ptr != '\0') {
        if (*ptr == delimiter) {
            tmp[i] = '\0';
            outArr = (char **)realloc(outArr, sizeof(char) * (*count + 1));
            outArr[*count] = cstrCreate(tmp, i);

            i = 0;
            ptr++;
            (*count)++;
            memset(tmp, 0, BUFSIZ);
            continue;
        }

        tmp[i++] = *ptr;
        ptr++;
    }

    tmp[i] = '\0';
    outArr[*count] = cstrCreate(tmp, i);
    (*count)++;

    return outArr;
}

/**
 * sort of casts a char* to an array of cstr**
 */
cstr **cstrCastArray(char *original) {
    cstr *str;
    cstr **array;

    if ((array = malloc(sizeof(cstr *))) == NULL)
        return NULL;

    if ((str = cstrCreate(original, strlen(original))) == NULL) {
        free(array);
        return NULL;
    }
    
    array[0] = str;

    return array;
}

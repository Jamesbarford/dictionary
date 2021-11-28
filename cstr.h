#ifndef __CSTR_H__
#define __CSTR_H__

#define CSTR_ERR -1
#define CSTR_OK   1
#define CSTR_PAD sizeof(int) + 1

typedef char cstr;

void cstrRelease(void *str);
void cstrArrayRelease(cstr **arr, int count);
void cstrSetLen(cstr *str, unsigned int len);
unsigned int cstrlen(cstr *str);

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
cstr *cstrEmpty(unsigned int len);
cstr *cstrCreate(char *tmp, unsigned int len);
cstr *cstrdup(cstr *original);
cstr **cstrSplit(char *to_split, char delimiter, int *count);
cstr *cstrCopyUntil(char *original, char terminator);
cstr **cstrCastArray(char *original);
int cstrToString(cstr *str, char *outbuf, int outbuflen);

#endif

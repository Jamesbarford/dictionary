#ifndef __CSTR_H
#define __CSTR_H

typedef unsigned char cstr;

#define CSTR_ERR -1
#define CSTR_OK 1

void cstrRelease(void *str);

void cstrSetLen(cstr *str, unsigned int len);
unsigned int cstrlen(cstr *str);
unsigned int cstrGetCapacity(cstr *str);

cstr *cstrExtendBufferIfNeeded(cstr *str, unsigned int additional);
cstr *cstrAlloc(unsigned int capacity);
cstr *cstrnew(void);

void cstrToLowerCase(cstr *c);
void cstrToUpperCase(cstr *c);

/* Append 1 character to the end of the string and increment the length, try
 * avoid using this with really big strings */
cstr *cstrPutChar(cstr *str, char ch);
unsigned char cstrUnPutChar(cstr *str);
cstr *cstrMutableSlice(cstr *s, unsigned int from, unsigned int to,
        unsigned int size);
cstr *cstrWrite(cstr *s, char *s2, unsigned int len);

int cstrnCmp(cstr *s1, cstr *s2, unsigned int size);
int cstrCmp(cstr *s1, cstr *s2);

/* Duplicate the string */
cstr *cstrDup(cstr *s);
cstr *cstrDupRaw(char *s, unsigned int len, unsigned int capacity);

cstr *cstrCatLen(cstr *str, const void *buf, unsigned int len);
cstr *cstrCat(cstr *str, char *s);
cstr *cstrCatPrintf(cstr *s, const char *fmt, ...);

cstr *cstrEscapeString(char *buf);
int cstrContainsPattern(void *buf, unsigned int buflen, char *pattern,
        unsigned int patternlen);

void cstrArrayRelease(cstr **arr, int count);
cstr **cstrSplit(char *to_split, char delimiter, int *count);

#endif

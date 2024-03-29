#ifndef __AOSTR_H
#define __AOSTR_H

#include <stddef.h>

typedef struct aoStr aoStr;

typedef struct aoStr {
    char *data;
    size_t offset;
    size_t len;
    size_t capacity;
} aoStr;

aoStr *aoStrAlloc(size_t capacity);
void aoStrRelease(aoStr *buf);

char *aoStrGetData(aoStr *buf);
void aoStrSetLen(aoStr *buf, size_t len);
size_t aoStrLen(aoStr *buf);
size_t aoStrGetOffset(aoStr *buf);
void aoStrSetOffset(aoStr *buf, size_t offset);
char *aoStrGetData(aoStr *buf);
char aoStrGetChar(aoStr *buf);
char aoStrGetCharAt(aoStr *buf, size_t at);
void aoStrAdvance(aoStr *buf);
void aoStrAdvanceBy(aoStr *buf, size_t by);
void aoStrRewindBy(aoStr *buf, size_t by);
int aoStrWouldOverflow(aoStr *buf, size_t additional);
int aoStrMatchChar(aoStr *buf, char ch);
int aoStrMatchCharAt(aoStr *buf, char ch, size_t at);
void aoStrSetCapacity(aoStr *buf, size_t capacity);
size_t aoStrCapacity(aoStr *buf);
int aoStrExtendBuffer(aoStr *buf, unsigned int additional);
int aoStrExtendBufferIfNeeded(aoStr *buf, size_t additional);
void aoStrToLowerCase(aoStr *buf);
void aoStrToUpperCase(aoStr *buf);
void aoStrPutChar(aoStr *buf, char ch);
char aoStrUnPutChar(aoStr *buf);
int mboxStrCmp(aoStr *b1, char *s);
int aoStrNCmp(aoStr *b1, aoStr *b2, size_t size);
int aoStrNCaseCmp(aoStr *b1, aoStr *b2, size_t size);
int aoStrStrNCmp(aoStr *b1, char *s, size_t len);
int aoStrStrNCaseCmp(aoStr *b1, char *s, size_t len);
int aoStrCmp(aoStr *b1, aoStr *b2);
int aoStrCaseCmp(aoStr *b1, aoStr *b2);
void aoStrSlice(aoStr *buf, size_t from, size_t to, size_t size);
aoStr *aoStrDupRaw(char *s, size_t len, size_t capacity);
aoStr *aoStrDup(aoStr *buf);
aoStr *aoStrMaybeDup(aoStr *buf);
size_t aoStrWrite(aoStr *buf, char *s, size_t len);

void aoStrCatLen(aoStr *buf, const void *d, size_t len);
void aoStrCatPrintf(aoStr *b, const char *fmt, ...);
aoStr *aoStrEscapeString(aoStr *buf);

int *aoStrComputePrefixTable(char *pattern, size_t patternlen);
int aoStrContainsPatternWithTable(aoStr *buf, int *table, char *pattern,
        size_t patternlen);
int aoStrContainsCasePatternWithTable(aoStr *buf, int *table, char *pattern,
        size_t patternlen);
int aoStrContainsPattern(aoStr *buf, char *pattern, size_t patternlen);
int aoStrContainsCasePattern(aoStr *buf, char *pattern, size_t patternlen);

void aoStrArrayRelease(aoStr **arr, int count);
aoStr **aoStrSplit(char *to_split, char delimiter, int *count);
int aoStrIsMimeEncoded(aoStr *buf);
void aoStrDecodeMimeEncodedInplace(aoStr *buf);
aoStr *aoStrDecodeMimeEncoded(aoStr *buf);

#endif

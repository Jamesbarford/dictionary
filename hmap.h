#ifndef __HMAP_H__
#define __HMAP_H__

#define HM_MIN_CAPACITY 1 << 16
#define HM_LOAD 0.67
#define HM_ERR 0
#define HM_OK 1
#define HM_FOUND -2
#define HM_NOT_FOUND -3

typedef int hmapKeyCompare(void *, unsigned int h1, void *, unsigned int h2);

typedef struct hmapType {
    hmapKeyCompare *keycmp;
    unsigned int (*hashFn)(void *);
    void (*freekey)(void *);
    void (*freevalue)(void *);
} hmapType;

typedef struct hmapEntry {
    void *key;
    void *value;
    unsigned int hash;
    struct hmapEntry *next;
} hmapEntry;

typedef struct hmap {
    unsigned int size;
    unsigned int capacity;
    unsigned int mask;
    unsigned int rebuildThreashold;
    int fixedsize;
    hmapType *type;
    hmapEntry **entries;
} hmap;

typedef struct hmapIterator {
    unsigned int idx;
    hmap *hm;
    hmapEntry *cur;
} hmapIterator;

#define hmapHash(h, k) ((h->type)->hashFn((k)))
#define hmapKeycmp(hm, k1, h1, k2, h2) \
    ((hm)->type->keycmp((k1), (h1), (k2), (h2)))
#define hmapKeyRelease(hm, k) ((hm)->type->freekey((k)))
#define hmapValueRelease(hm, k) ((hm)->type->freevalue((k)))

hmap *hmapCreate();
hmap *hmapCreateWithType(hmapType *type);
hmap *hmapCreateFixed(hmapType *type, unsigned int capacity);
void hmapRelease(hmap *hm);

int hmapContains(hmap *hm, void *key);
int hmapAdd(hmap *hm, void *key, void *value);
/* Return entry for user to free */
hmapEntry *hmapDelete(hmap *hm, void *key);
hmapEntry *hmapGetEntry(hmap *hm, void *key);
void *hmapGet(hmap *hm, void *key);

hmapIterator *hmapIteratorCreate(hmap *hm);
void hmapIteratorRelease(hmapIterator *iter);
int hmapIteratorGetNext(hmapIterator *iter);

#endif

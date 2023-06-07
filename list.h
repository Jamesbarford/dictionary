/* Copyright (C) 2023 James W M Barford-Evans
 * <jamesbarfordevans at gmail dot com>
 * All Rights Reserved
 *
 * This code is released under the BSD 2 clause license.
 * See the COPYING file for more information. */
#ifndef __LIST_H
#define __LIST_H

#include <pthread.h>
#include <stddef.h>
/* doubly circular linked list */

#ifdef __cplusplus
extern "C" {
#endif

typedef int listFindCallback(void *data, void *needle);
typedef void listFreeData(void *data);
typedef int listCmp(void *data, void *needle);
typedef void listPrintCallback(void *data);

typedef struct lNode {
    void *data;
    struct lNode *next;
    struct lNode *prev;
} lNode;

typedef struct list {
    size_t len;
    lNode *root;
    pthread_mutex_t lock; /* for if we want a thread safe list we can use this
                     with the thread safe variants of the functions below */
    listFreeData *freedata;
    listCmp *compare;
} list;

#define listSetFreedata(l, fn) ((l)->freedata = (fn))

list *listNew(void);
list *listTSNew(void);

void *listFind(list *l, void *search_data,
        listFindCallback *compare);
void listAddHead(list *l, void *val);
void listAddTail(list *l, void *val);
void *listRemoveHead(list *l);
void *listRemoveTail(list *l);
void listQSort(list *l, listCmp *compare);
void listPrint(list *l, listPrintCallback *print);

/* TS being thread safe */
void *listTSFind(list *l, void *search_data,
        listFindCallback *compare);
void listTSAddHead(list *l, void *val);
void listTSAddTail(list *l, void *val);
void *listTSRemoveHead(list *l);
void *listTSRemoveTail(list *l);

void listRelease(list *l);
list *listAppendListTail(list *main, list *aux);

#ifdef __cplusplus
}
#endif

#endif

/* Copyright (C) 2023 James W M Barford-Evans
 * <jamesbarfordevans at gmail dot com>
 * All Rights Reserved
 *
 * This code is released under the BSD 2 clause license.
 * See the COPYING file for more information. */
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"

static lNode *
lNodeNew(void *data)
{
    lNode *n = (lNode *)malloc(sizeof(lNode));
    n->data = data;
    n->next = n->prev = NULL;
    return n;
}

list *
listNew(void)
{
    list *l = (list *)malloc(sizeof(list));
    l->len = 0;
    l->root = NULL;
    l->freedata = NULL;
    pthread_mutex_init(&l->lock, NULL);
    return l;
}

void
listAddHead(list *l, void *data)
{
    lNode *n = lNodeNew(data);

    if (l->root == NULL) {
        l->root = n;
        n->next = n->prev = n;
    } else {
        lNode *tail = l->root->prev;
        lNode *head = l->root;

        n->next = head;
        n->prev = tail;

        tail->next = n;
        head->prev = n;

        l->root = n;
    }

    l->len++;
}

void
listAddTail(list *l, void *data)
{
    lNode *n = lNodeNew(data);

    if (l->root == NULL) {
        l->root = n;
        n->next = n->prev = n;
    } else {
        lNode *tail = l->root->prev;
        lNode *head = l->root;

        n->next = head;
        n->prev = tail;

        tail->next = n;
        head->prev = n;
    }

    l->len++;
}

void *
listRemoveHead(list *l)
{
    if (l->len == 0 || l->root == NULL) {
        return NULL;
    }
    lNode *head = l->root;
    void *val = head->data;

    if (l->len == 1) {
        l->root = NULL;
    } else {
        lNode *tail = l->root->prev;
        l->root = l->root->next;
        tail->next = l->root;
        l->root->prev = tail;
    }

    free(head);
    l->len--;
    return val;
}

void *
listRemoveTail(list *l)
{
    if (l->len == 0) {
        return NULL;
    }

    lNode *tail = l->root->prev;
    void *val = tail->data;

    if (l->len == 1) {
        l->root = NULL;
    } else {
        lNode *new_tail = tail->prev;
        l->root->prev = new_tail;
        new_tail->next = l->root;
    }

    free(tail);
    l->len--;
    return val;
}

void *
listFind(list *l, void *search_data, listFindCallback *compare)
{
    if (l->len == 0) {
        return NULL;
    }

    lNode *current = l->root;
    size_t count = l->len;

    do {
        if (compare(current->data, search_data)) {
            return current->data;
        }

        current = current->next;
        count--;
    } while (count > 0);

    return NULL;
}

list *
listTSNew(void)
{
    list *l = listNew();
    pthread_mutex_init(&l->lock, NULL);
    return l;
}

void *
listTSFind(list *l, void *search_data, listFindCallback *compare)
{
    void *retval = NULL;
    pthread_mutex_lock(&l->lock);
    retval = listFind(l, search_data, compare);
    pthread_mutex_unlock(&l->lock);
    return retval;
}

void
listTSAddHead(list *l, void *val)
{
    pthread_mutex_lock(&l->lock);
    listAddHead(l, val);
    pthread_mutex_unlock(&l->lock);
}

void
listTSAddTail(list *l, void *val)
{
    pthread_mutex_lock(&l->lock);
    listAddTail(l, val);
    pthread_mutex_unlock(&l->lock);
}

void *
listTSRemoveHead(list *l)
{
    void *retval = NULL;
    pthread_mutex_lock(&l->lock);
    retval = listRemoveHead(l);
    pthread_mutex_unlock(&l->lock);

    return retval;
}

void *
listTSRemoveTail(list *l)
{
    void *retval = NULL;
    pthread_mutex_lock(&l->lock);
    retval = listRemoveTail(l);
    pthread_mutex_unlock(&l->lock);

    return retval;
}

static lNode *
listQSortPivot(list *l, lNode *low, lNode *high, listCmp *compare)
{
    void *pivot = high->data;
    lNode *i = low->prev;
    lNode *j = NULL;

    for (j = low; j != high; j = j->next) {
        if (compare(j->data, pivot) < 1) {
            i = (i == NULL) ? low : i->next;
            void *tmp = i->data;
            i->data = j->data;
            j->data = tmp;
        }
    }
    i = (i == NULL) ? low : i->next;
    void *tmp = i->data;
    i->data = j->data;
    j->data = tmp;

    return i;
}

static void
mboxQSortHelper(list *l, lNode *low, lNode *high, int count, listCmp *compare)
{
    if (low == high || (count == 1 && low == high->next)) {
        return;
    }

    lNode *pivot = listQSortPivot(l, low, high, compare);
    mboxQSortHelper(l, low, pivot->prev, 1, compare);
    mboxQSortHelper(l, pivot->next, high, 1, compare);
}

/* Quick sort the list */
void
listQSort(list *l, listCmp *compare)
{
    pthread_mutex_lock(&l->lock);
    if (l->len <= 1 || compare == NULL) {
        return;
    }
    mboxQSortHelper(l, l->root, l->root->prev, 0, compare);
    pthread_mutex_unlock(&l->lock);
}

/* Print a list like an array */
void
listPrint(list *l, listPrintCallback *print)
{
    lNode *n = l->root;
    size_t count = l->len;

    printf("[");
    do {
        print(n->data);
        if (count - 1 != 0) {
            printf(", ");
        } else {
            printf(" ");
        }
        n = n->next;
        count--;
    } while (count);
    printf("]\n");
}

void
listRelease(list *l)
{
    if (l) {
        while (l->len > 0) {
            void *data = listRemoveHead(l);
            if (data && l->freedata) {
                l->freedata(data);
            }
        }
        pthread_mutex_destroy(&l->lock);
        free(l);
    }
}

/* Plonk aux onto the tail of main O(1) fast and unsophisticated, just how I
 * like cars*/
list *
listAppendListTail(list *main, list *aux)
{
    if (aux->len == 0) {
        return main;
    }
    lNode *aux_head = aux->root;

    if (main->len == 0) {
        main->root = aux_head;
    } else {
        lNode *tail = main->root->prev;
        lNode *head = main->root;
        lNode *aux_tail = aux_head->prev;

        aux_head->prev = tail;
        tail->next = aux_head;

        head->prev = aux_tail;
        aux_tail->next = head;
    }

    main->len += aux->len;
    pthread_mutex_destroy(&aux->lock);
    aux->len = 0;
    free(aux);

    return main;
}

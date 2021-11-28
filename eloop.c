#include <stdio.h>
#include <stdlib.h>

#include "eloop.h"

#if defined(__APPLE__) && defined(MAC_OS_X_VERSION_10_6) ||                    \
    defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include "kevent_loop.c"
#elif defined(__linux__)
#include "epoll_loop.c"
#else
#error "os does not support epoll or kqueue"
#endif

#define evtRead(ev, el, fd, mask) \
    ((ev)->read((el), (fd), (ev)->data, (mask)))
#define evtWrite(ev, el, fd, mask) \
    ((ev)->write((el), (fd), (ev)->data, (mask)))

static inline void _eloopSetEvtAdd(evt *evs, int eventcount) {
    for (int i = 0; i < eventcount; ++i)
        evs[i].mask = EVT_ADD;
}

eloop *eloopCreate(int eventcount) {
    int eventssize;
    eloop *el;
    evt *idle, *active;

    el = NULL;
    idle = NULL;
    active = NULL;

    if ((el = malloc(sizeof(eloop))) == NULL)
        goto error;

    eventssize = sizeof(evt) * eventcount;

    if ((idle = malloc(eventssize)) == NULL)
        goto error;

    if ((active = malloc(eventssize)) == NULL)
        goto error;

    el->max = -1;
    el->count = eventcount;
    el->idle = idle;
    el->active = active;
    _eloopSetEvtAdd(el->idle, el->count);
    if (eloopStateCreate(el) == EVT_ERR)
        goto error;

    return el;

error:
    if (el)
        free(el);
    if (idle)
        free(idle);
    if (active)
        free(active);

    return NULL;
}

int eloopAddEvent(eloop *el, int fd, int mask, evtCallback *cb, void *data) {
    evt *ev;

    if (fd >= el->count)
        return EVT_ERR;

    ev = &el->idle[fd];
    if (eloopStateAdd(el, fd, mask) == EVT_ERR)
        return EVT_ERR;

    ev->mask |= mask;
    ev->data = data;
    if (mask & EVT_READ)  ev->read = cb;
    if (mask & EVT_WRITE) ev->write = cb;
    if (fd > el->max) el->max = fd;

    return EVT_OK;
}

void eloopDeleteEvent(eloop *el, int fd, int mask) {
    evt *ev;

    if (fd >= el->count) return;
    ev = &el->idle[fd];
    if (ev->mask == EVT_ADD) return;

    eloopStateDelete(el, fd, mask);
    ev->mask = ev->mask & (~mask);
    if (fd == el->max && ev->mask == EVT_ADD) {
        int i;
        for (i = el->max - 1; i >= 0; --i)
            if (el->idle[i].mask != EVT_ADD)
                break;
        el->max = i;
    }
}

int eloopProcessEvents(eloop *el) {
    if (el->max == -1) return 0;
    int processed, eventcount, fd, mask, firedevts;
    evt *ev;

    processed = 0;
    eventcount = eloopPoll(el);

    for (int i = 0; i < eventcount; ++i) {
        fd = el->active[i].fd;
        ev = &el->idle[fd];
        mask = el->active[i].mask;
        firedevts = 0;

        if (ev->mask & mask & EVT_READ) {
            evtRead(ev, el, fd, mask);
            firedevts++;
            ev = &el->idle[fd];
        }

        if (ev->mask & mask & EVT_WRITE) {
            if (!firedevts || ev->read != ev->write) {
                evtWrite(ev, el, fd, mask);
                firedevts++;
            }
        }

        processed++;
    }

    return processed;
}

void eloopMain(eloop *el) {
    while (1) (void)eloopProcessEvents(el);
}

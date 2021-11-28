#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "eloop.h"

typedef struct evtState {
    int efd;
    struct epoll_event *events;
} evtState;

static int eloopStateCreate(eloop *el) {
    evtState *es;

    if ((es = malloc(sizeof(evtState))) == NULL)
        goto error;

    if ((es->events = malloc(sizeof(struct epoll_event) * el->count)) == NULL)
        goto error;

    if ((es->efd = epoll_create(65355)) == -1)
        goto error;

    el->state = es;

    return EVT_OK;

error:
    if (es && es->events)
        free(es->events);
    if (es)
        free(es);
    return EVT_ERR;
}

static void eloopStateRelease(eloop *el) {
    if (el) {
        evtState *es = eloopGetState(el);
        if (es) {
            close(es->efd);
            free(es->events);
            free(es);
        }
    }
}

static inline void _eloopStateSetMask(struct epoll_event *event, int mask) {
    if (mask & EVT_READ)
        event->events |= EPOLLIN;
    if (mask & EVT_WRITE)
        event->events |= EPOLLOUT;
}

static int eloopStateAdd(eloop *el, int fd, int mask) {
    evtState *es = eloopGetState(el);
    struct epoll_event event;
    int op;

    op = el->idle[fd].mask == EVT_ADD ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    event.events = 0;
    mask |= el->idle[fd].mask;
    _eloopStateSetMask(&event, mask);
    event.data.fd = fd;

    if (epoll_ctl(es->efd, op, fd, &event) == -1)
        return EVT_ERR;
    return EVT_OK;
}

static void eloopStateDelete(eloop *el, int fd, int mask) {
    evtState *es = eloopGetState(el);
    struct epoll_event event;
    int newmask;

    newmask = el->idle[fd].mask & (~mask);
    event.events = 0;
    newmask |= el->idle[fd].mask;
    event.data.fd = fd;

    _eloopStateSetMask(&event, newmask);

    if (newmask != EVT_DELETE)
        epoll_ctl(es->efd, EPOLL_CTL_ADD, fd, &event);
    else
        epoll_ctl(es->efd, EPOLL_CTL_DEL, fd, &event);
}

static int eloopPoll(eloop *el) {
    evtState *es = eloopGetState(el);
    int fdcount;

    fdcount = epoll_wait(es->efd, es->events, el->count, -1);

    if (fdcount > 0) {
        for (int i = 0; i < fdcount; ++i) {
            int newmask = 0;
            struct epoll_event *event = es->events + i;
            uint32_t evts = event->events;

            if (evts & EPOLLIN)
                newmask |= EVT_READ;
            if (evts & EPOLLOUT)
                newmask |= EVT_WRITE;
            if ((evts & EPOLLERR) || (evts & EPOLLHUP))
                newmask |= (EVT_READ | EVT_WRITE);
            el->active[i].fd = event->data.fd;
            el->active[i].mask = newmask;
        }
    } else if (fdcount == -1) {
        return EVT_ERR;
    }

    return fdcount;
}

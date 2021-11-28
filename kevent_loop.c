#include <sys/socket.h>
#include <sys/event.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "eloop.h"

typedef struct evtState {
	int kfd;
	struct kevent *events;
} evtState;

#define __kevent(kfd, evt) (kevent((kfd), (evt), 1, NULL, 0, NULL))

static int eloopStateCreate(eloop *el) {
	evtState *es;

	if ((es = malloc(sizeof(evtState))) == NULL)
		goto error;

	if ((es->events = malloc(sizeof(struct kevent) *el->count)) == NULL)
		goto error;

	if ((es->kfd = kqueue()) == -1)
		goto error;

	el->state = es;

	return EVT_OK;

error:
	if (es && es->events) free(es->events);
	if (es) free(es);
	return EVT_ERR;
}

static void eloopStateRelease(eloop *el) {
	if (el) {
		evtState *es = eloopGetState(el);
		if (es) {
			close(es->kfd);
			free(es->events);
			free(es);
		}
	}
}

static int eloopStateAdd(eloop *el, int fd, int mask) {
	evtState *es = eloopGetState(el);
	struct kevent event;

	if (mask & EVT_READ) {
		EV_SET(&event, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
		if (__kevent(es->kfd, &event) == -1) return EVT_ERR;
	}
	if (mask & EVT_WRITE) {
		EV_SET(&event, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
		if (__kevent(es->kfd, &event) == -1) return EVT_ERR;
	}

	return EVT_OK;
}

static void eloopStateDelete(eloop *el, int fd, int mask) {
	evtState *es = eloopGetState(el);
	struct kevent event;

	if (mask & EVT_READ) {
		EV_SET(&event, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
		__kevent(es->kfd, &event);
	}
	if (mask & EVT_WRITE) {
		EV_SET(&event, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
		__kevent(es->kfd, &event);
	}
}

static int eloopPoll(eloop *el) {
	evtState *es = eloopGetState(el);
	int fdcount = 0;
	
	fdcount = kevent(es->kfd, NULL, 0, es->events, el->count, NULL);

	if (fdcount > 0) {
		for (int i = 0; i < fdcount; ++i) {
			int newmask;
			struct kevent *event = es->events + i;
			short filter = event->filter;

			if (filter & EVFILT_READ)  newmask |= EVT_READ;
			if (filter & EVFILT_WRITE) newmask |= EVT_WRITE;
			el->active[i].fd = event->ident;
			el->active[i].mask = newmask;
		}
	} else if (fdcount == -1) return EVT_ERR;

	return fdcount;
}

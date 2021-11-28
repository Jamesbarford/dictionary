#ifndef __EVT_H__
#define __EVT_H__

#define EVT_ADD    0 //    0
#define EVT_READ   1 //    1
#define EVT_WRITE  2 //   10
#define EVT_DELETE 4 //  100

#define EVT_ERR 0
#define EVT_OK  1

struct eloop;

typedef void evtCallback(struct eloop *el, int fd, void *data, int type);

typedef struct evt {
	int fd;
	int mask;
	evtCallback *read;
	evtCallback *write;
	void *data;
} evt;

typedef struct eloop {
	int max;
	int count;
	evt *idle;
	evt *active;
	void *state;
} eloop;

#define eloopGetState(el) ((el)->state)

eloop *eloopCreate(int eventcount);
int eloopAddEvent(eloop *el, int fd, int mask, evtCallback *cb, void *data);
void eloopDeleteEvent(eloop *el, int fd, int mask);
void eloopMain(eloop *el);


#endif

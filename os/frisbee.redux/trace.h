#include <pthread.h>
#include <sys/time.h>
#include <netinet/in.h>

#define NEVENTS	8192

#ifdef NEVENTS
struct event {
	struct timeval	tstamp;
	struct in_addr	srcip;
	int		event;
	unsigned long	args[4];
};

extern struct event eventlog[];
extern struct event *evptr, *evend;
extern int evlogging, evcount;
extern pthread_mutex_t evlock;

#define EVENT(l, e, ip, a1, a2, a3, a4) \
if (evlogging >= (l)) { \
	pthread_mutex_lock(&evlock); \
	gettimeofday(&evptr->tstamp, 0); \
	evptr->event = (e); \
	evptr->srcip = (ip); \
	evptr->args[0] = (unsigned long)(a1); \
	evptr->args[1] = (unsigned long)(a2); \
	evptr->args[2] = (unsigned long)(a3); \
	evptr->args[3] = (unsigned long)(a4); \
	if (++evptr == evend) evptr = eventlog; \
	evcount++; \
	pthread_mutex_unlock(&evlock); \
}

#define CLEVENT(l, e, a1, a2, a3, a4) \
if (evlogging >= (l)) { \
	extern struct in_addr myipaddr; \
	pthread_mutex_lock(&evlock); \
	gettimeofday(&evptr->tstamp, 0); \
	evptr->event = (e); \
	evptr->srcip = myipaddr; \
	evptr->args[0] = (unsigned long)(a1); \
	evptr->args[1] = (unsigned long)(a2); \
	evptr->args[2] = (unsigned long)(a3); \
	evptr->args[3] = (unsigned long)(a4); \
	if (++evptr == evend) evptr = eventlog; \
	evcount++; \
	pthread_mutex_unlock(&evlock); \
}

#define EV_JOINREQ	1
#define EV_JOINREP	2
#define EV_LEAVEMSG	3
#define EV_REQMSG	4
#define EV_BLOCKMSG	5
#define EV_WORKENQ	6
#define EV_WORKDEQ	7
#define EV_READFILE	8

#define EV_CLIREQ	9
#define EV_OCLIMSG	10
#define EV_CLINOROOM	11
#define EV_CLIDUPCHUNK	12
#define EV_CLIDUPBLOCK	13
#define EV_CLIBLOCK	14
#define EV_CLICHUNK	15
#define EV_CLIREQCHUNK	16
#define EV_CLIJOINREQ	17
#define EV_CLIJOINREP	18
#define EV_CLILEAVE	19

#define EV_MAX		19

extern void ClientTraceInit(char *file);
extern void ServerTraceInit(char *file);
extern void TraceStart(int level);
extern void TraceStop(void);
extern void TraceDump(void);
#else
#define EVENT(l, e, ip, a1, a2, a3, a4)
#define CLEVENT(l, e, a1, a2, a3, a4)
#define ClientTraceInit(file)
#define ServerTraceInit(file)
#define TraceStart(level)
#define TraceStop()
#define TraceDump()
#endif

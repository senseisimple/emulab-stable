#include <pthread.h>
#include <sys/time.h>
#include <netinet/in.h>

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
#define EV_WORKOVERLAP	9
#define EV_WORKMERGE	10

#define EV_CLIREQ	11
#define EV_OCLIMSG	12
#define EV_CLINOROOM	13
#define EV_CLIDUPCHUNK	14
#define EV_CLIDUPBLOCK	15
#define EV_CLISCHUNK	16
#define EV_CLIECHUNK	17
#define EV_CLIREQCHUNK	18
#define EV_CLIJOINREQ	19
#define EV_CLIJOINREP	20
#define EV_CLILEAVE	21
#define EV_CLIREQMSG	22
#define EV_CLISTAMP	23
#define EV_CLIWRDONE	24
#define EV_CLIWRIDLE	25
#define EV_CLIBLOCK	26
#define EV_CLISTART	27
#define EV_CLIGOTPKT	28
#define EV_CLIRTIMO	29

#define EV_MAX		29

extern void ClientTraceInit(char *file);
extern void ClientTraceReinit(char *file);
extern void ServerTraceInit(char *file);
extern void TraceStart(int level);
extern void TraceStop(void);
extern void TraceDump(void);
#else
#define EVENT(l, e, ip, a1, a2, a3, a4)
#define CLEVENT(l, e, a1, a2, a3, a4)
#define ClientTraceInit(file)
#define ClientTraceReinit(file)
#define ServerTraceInit(file)
#define TraceStart(level)
#define TraceStop()
#define TraceDump()
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "decls.h"
#include "trace.h"
#include "log.h"

#ifdef NEVENTS

struct event eventlog[NEVENTS];
struct event *evptr = eventlog;
struct event *evend = &eventlog[NEVENTS];
int evlogging, evcount;
pthread_mutex_t evlock;
static int evisclient;
static FILE *fd;

static void
TraceInit(char *file)
{
	if (file) {
		fd = fopen(file, "a+");
		if (fd == NULL)
			pfatal("Cannot open logfile %s", file);
	} else
		fd = stderr;

	pthread_mutex_init(&evlock, 0);
}

void
ClientTraceInit(char *file)
{
	extern struct in_addr myipaddr;

	if (file) {
		char name[64];
		snprintf(name, sizeof(name),
			 "%s-%s.trace", file, inet_ntoa(myipaddr));
		TraceInit(name);
	} else
		TraceInit(0);

	evisclient = 1;
}

void
ServerTraceInit(char *file)
{
	extern struct in_addr myipaddr;

	if (file) {
		char name[64];
		snprintf(name, sizeof(name),
			 "%s-%s.trace", file, inet_ntoa(myipaddr));
		TraceInit(name);
	} else
		TraceInit(0);

	evisclient = 0;
}

void
TraceStart(int level)
{
	evlogging = level;
}

void
TraceStop(void)
{
	evlogging = 0;
}

#define timevalsub(vvp, uvp)						\
	do {								\
		(vvp)->tv_sec -= (uvp)->tv_sec;				\
		(vvp)->tv_usec -= (uvp)->tv_usec;			\
		if ((vvp)->tv_usec < 0) {				\
			(vvp)->tv_sec--;				\
			(vvp)->tv_usec += 1000000;			\
		}							\
	} while (0)

void
TraceDump(void)
{
	struct event *ptr;
	int done = 0;
	struct timeval startt, stamp;
	int oevlogging = evlogging;

	evlogging = 0;
	ptr = evptr;
	do {
		if (ptr->event > 0 && ptr->event <= EV_MAX) {
			if (!done) {
				done = 1;
				fprintf(fd, "%d events, start: %ld.%02ld:\n",
					evcount > NEVENTS ? NEVENTS : evcount,
					(long)ptr->tstamp.tv_sec,
					ptr->tstamp.tv_usec/10000);
				startt = ptr->tstamp;
			}
			stamp = ptr->tstamp;
			timevalsub(&stamp, &startt);
			fprintf(fd, " +%04ld.%02ld: ",
				(long)stamp.tv_sec, stamp.tv_usec/10000);
			switch (ptr->event) {
			case EV_JOINREQ:
				fprintf(fd, "%s: JOIN request, ID=%lx\n",
					inet_ntoa(ptr->srcip), ptr->args[0]);
				break;
			case EV_JOINREP:
				fprintf(fd, "%s: JOIN reply, blocks=%lu\n",
					inet_ntoa(ptr->srcip), ptr->args[0]);
				break;
			case EV_LEAVEMSG:
				fprintf(fd, "%s: LEAVE msg, ID=%lx, time=%lu\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1]);
				break;
			case EV_REQMSG:
				fprintf(fd, "%s: REQUEST msg, %lu@%lu/%lu\n",
					inet_ntoa(ptr->srcip), ptr->args[2],
					ptr->args[0], ptr->args[1]);
				break;
			case EV_BLOCKMSG:
				fprintf(fd, "send block, %lu/%lu\n",
					ptr->args[0], ptr->args[1]);
				break;
			case EV_WORKENQ:
				fprintf(fd, "enqueues, %lu@%lu/%lu\n",
					ptr->args[2],
					ptr->args[0], ptr->args[1]);
				break;
			case EV_WORKDEQ:
				fprintf(fd, "dequeues, %lu@%lu/%lu\n",
					ptr->args[2],
					ptr->args[0], ptr->args[1]);
				break;
			case EV_READFILE:
				fprintf(fd, "readfile, %lu@%lu -> %lu\n",
					ptr->args[1],
					ptr->args[0], ptr->args[2]);
				break;


			case EV_OCLIMSG:
			{
				struct in_addr ipaddr = { ptr->args[0] };

				fprintf(fd, "%s: got %s msg, ",
					inet_ntoa(ptr->srcip),
					(ptr->args[1] == PKTSUBTYPE_JOIN ?
					 "JOIN" :
					 (ptr->args[1] == PKTSUBTYPE_LEAVE ?
					  "LEAVE" : "REQUEST")));
				fprintf(fd, "ip=%s\n", inet_ntoa(ipaddr));
				break;
			}
			case EV_CLINOROOM:
				fprintf(fd, "%s: block %lu/%lu, no room\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1]);
				break;
			case EV_CLIDUPCHUNK:
				fprintf(fd, "%s: block %lu/%lu, dup chunk\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1]);
				break;
			case EV_CLIDUPBLOCK:
				fprintf(fd, "%s: block %lu/%lu, dup block\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1]);
				break;
			case EV_CLIBLOCK:
				fprintf(fd, "%s: block %lu/%lu, bcount=%lu\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1],
					ptr->args[2]);
				break;
			case EV_CLICHUNK:
				fprintf(fd, "%s: got chunk %lu\n",
					inet_ntoa(ptr->srcip), ptr->args[0]);
				break;
			case EV_CLIREQ:
				fprintf(fd, "%s: send REQUEST, %lu@%lu/%lu\n",
					inet_ntoa(ptr->srcip), ptr->args[2],
					ptr->args[0], ptr->args[1]);
				break;
			case EV_CLIREQCHUNK:
				fprintf(fd, "%s: request chunk, timeo=%lu\n",
					inet_ntoa(ptr->srcip), ptr->args[0]);
				break;
			case EV_CLIJOINREQ:
				fprintf(fd, "%s: send JOIN, ID=%lx\n",
					inet_ntoa(ptr->srcip), ptr->args[0]);
				break;
			case EV_CLIJOINREP:
				fprintf(fd, "%s: got JOIN reply, blocks=%lu\n",
					inet_ntoa(ptr->srcip), ptr->args[0]);
				break;
			case EV_CLILEAVE:
				fprintf(fd, "%s: send LEAVE, ID=%lx, time=%lu\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1]);
				break;
			}
		}
		if (++ptr == evend)
			ptr = eventlog;
	} while (ptr != evptr);
	fflush(fd);
	evlogging = oevlogging;
}
#endif

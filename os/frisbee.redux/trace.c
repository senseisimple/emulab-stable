/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
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
static struct timeval startt;

static void
TraceInit(char *file)
{
	static int called;

	if (file) {
		fd = fopen(file, "a+");
		if (fd == NULL)
			pfatal("Cannot open logfile %s", file);
	} else
		fd = stderr;

	if (!called) {
		called = 1;
		pthread_mutex_init(&evlock, 0);
	}
}

void
ClientTraceInit(char *prefix)
{
	extern struct in_addr myipaddr;

	evlogging = 0;

	if (fd != NULL && fd != stderr)
		fclose(fd);
	memset(eventlog, 0, sizeof eventlog);
	evptr = eventlog;
	evcount = 0;

	if (prefix && prefix[0]) {
		char name[64];
		snprintf(name, sizeof(name),
			 "%s-%s.trace", prefix, inet_ntoa(myipaddr));
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
	gettimeofday(&startt, 0);
}

void
TraceStop(void)
{
	evlogging = 0;
}

void
TraceDump(int serverrel, int level)
{
	struct event *ptr;
	int done = 0;
	struct timeval stamp;
	int oevlogging = evlogging;

	evlogging = 0;
	ptr = evptr;
	do {
		if (ptr->event > 0 && ptr->event <= EV_MAX) {
			if (!done) {
				done = 1;
				fprintf(fd, "%d of %d events, "
					"start=%ld.%03ld, level=%d:\n",
					evcount > NEVENTS ? NEVENTS : evcount,
					evcount, startt.tv_sec,
					startt.tv_usec/1000, level);
				/*
				 * Make all event stamps relative to the
				 * first event received.  This is specifically
				 * for the case where the client and server
				 * are running on the same machine and there
				 * is only a single client.  This makes the
				 * server timeline relative to the client's
				 * because the server's first event is the
				 * JOIN from the client which occurs at 0.000
				 * in the client's timeline.
				 *
				 * XXX we add 1us in order to make it possible
				 * to merge the timelines and preserve
				 * causality.
				 */
				if (serverrel) {
					struct timeval ms = { 0, 120 };
					timersub(&ptr->tstamp, &ms, &startt);
				}
			}
			timersub(&ptr->tstamp, &startt, &stamp);
			fprintf(fd, " +%03ld.%06ld: ",
				(long)stamp.tv_sec, stamp.tv_usec);
			fprintf(fd, "%c: ", evisclient ? 'C' : 'S');
			switch (ptr->event) {
			case EV_JOINREQ:
				fprintf(fd, "%s: JOIN request, ID=%x, vers=%u\n",
					inet_ntoa(ptr->srcip), ptr->args[0],
					ptr->args[1]);
				break;
			case EV_JOINREP:
			{
				unsigned long long bytes;
				bytes = (unsigned long long)ptr->args[2] << 32;
				bytes |= ptr->args[3];
				fprintf(fd, "%s: JOIN reply, "
					"chunksize=%u, blocksize=%u, "
					"imagebytes=%llu\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1], bytes);
				break;
			}
			case EV_LEAVEMSG:
				fprintf(fd, "%s: LEAVE msg, ID=%x, time=%u\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1]);
				break;
			case EV_REQMSG:
				fprintf(fd, "%s: REQUEST msg, %u[%u-%u]\n",
					inet_ntoa(ptr->srcip), 
					ptr->args[0], ptr->args[1],
					ptr->args[1]+ptr->args[2]-1);
				break;
			case EV_PREQMSG:
				fprintf(fd, "%s: PREQUEST msg, %u(%u)%s\n",
					inet_ntoa(ptr->srcip), ptr->args[0],
					ptr->args[1],
					ptr->args[2] ? " [RETRY]" : "");
				break;
			case EV_OVERRUN:
				stamp.tv_sec = ptr->args[0];
				stamp.tv_usec = ptr->args[1];
				timersub(&ptr->tstamp, &stamp, &stamp);
				fprintf(fd, "overrun by %ld.%03ld "
					"after %u[%u]\n",
					stamp.tv_sec, stamp.tv_usec/1000,
					ptr->args[2], ptr->args[3]);
				break;
			case EV_LONGBURST:
				fprintf(fd, "finished long burst %u (>%u) "
					"after %u[%u]\n",
					ptr->args[0], ptr->args[1],
					ptr->args[2], ptr->args[3]);
				break;
			case EV_BLOCKMSG:
				fprintf(fd, "sent block, %u[%u], retry=%u\n",
					ptr->args[0], ptr->args[1],
					ptr->args[2]);
				break;
			case EV_WORKENQ:
				fprintf(fd, "enqueues, %u(%u), "
					"%u ents\n",
					ptr->args[0], ptr->args[1],
					ptr->args[2]);
				break;
			case EV_WORKDEQ:
				fprintf(fd, "dequeues, %u[%u-%u], "
					"%u ents\n",
					ptr->args[0], ptr->args[1],
					ptr->args[1]+ptr->args[2]-1,
					ptr->args[3]);
				break;
			case EV_WORKOVERLAP:
				fprintf(fd, "queue overlap, "
					"old=[%u-%u], new=[%u-%u]\n",
					ptr->args[0],
					ptr->args[0]+ptr->args[1]-1,
					ptr->args[2],
					ptr->args[2]+ptr->args[3]-1);
				break;
			case EV_WORKMERGE:
				if (ptr->args[3] == ~0)
					fprintf(fd, "merged %u with current\n",
						ptr->args[0]);
				else
					fprintf(fd, "merged %u at ent %u, "
						"added %u to existing %u\n",
						ptr->args[0], ptr->args[3],
						ptr->args[2], ptr->args[1]);
				break;
			case EV_DUPCHUNK:
				fprintf(fd, "possible dupchunk %u\n",
					ptr->args[0]);
				break;
			case EV_READFILE:
				stamp.tv_sec = ptr->args[2];
				stamp.tv_usec = ptr->args[3];
				timersub(&ptr->tstamp, &stamp, &stamp);
				fprintf(fd, "readfile, %u@%u, %ld.%03lds\n",
					ptr->args[1], ptr->args[0],
					stamp.tv_sec, stamp.tv_usec/1000);
				break;


			case EV_CLISTART:
				fprintf(fd, "%s: starting\n",
					inet_ntoa(ptr->srcip));
				break;
			case EV_OCLIMSG:
			{
				struct in_addr ipaddr = { ptr->args[0] };

				fprintf(fd, "%s: got %s msg, ",
					inet_ntoa(ptr->srcip),
					(ptr->args[1] == PKTSUBTYPE_JOIN ?
					 "JOIN" : "LEAVE"));
				fprintf(fd, "ip=%s\n", inet_ntoa(ipaddr));
				break;
			}
			case EV_CLIREQMSG:
			{
				struct in_addr ipaddr = { ptr->args[0] };

				fprintf(fd, "%s: saw REQUEST for ",
					inet_ntoa(ptr->srcip));
				fprintf(fd, "%u[%u-%u], ip=%s\n",
					ptr->args[1], ptr->args[2],
					ptr->args[2]+ptr->args[3]-1,
					inet_ntoa(ipaddr));
				break;
			}
			case EV_CLIPREQMSG:
			{
				struct in_addr ipaddr = { ptr->args[0] };

				fprintf(fd, "%s: saw PREQUEST for ",
					inet_ntoa(ptr->srcip));
				fprintf(fd, "%u, ip=%s\n",
					ptr->args[1], inet_ntoa(ipaddr));
				break;
			}
			case EV_CLINOROOM:
				fprintf(fd, "%s: block %u[%u], no room "
					"(full=%u filling=%u)\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1],
					ptr->args[2], ptr->args[3]);
				break;
			case EV_CLIFOUNDROOM:
				fprintf(fd, "%s: block %u[%u], marked dubious"
					" (missed %u blocks)\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1],
					ptr->args[2]);
				break;
			case EV_CLIREUSE:
				fprintf(fd, "%s: block %u[%u], displaces "
					"%u blocks of dubious chunk %u from chunk buffer\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1],
					ptr->args[2], ptr->args[3]);
				break;
			case EV_CLIDUBPROMO:
				fprintf(fd, "%s: block %u[%u], no longer "
					"dubious\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1]);
				break;
			case EV_CLIDUPCHUNK:
				fprintf(fd, "%s: block %u[%u], dup chunk\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1]);
				break;
			case EV_CLIDUPBLOCK:
				fprintf(fd, "%s: block %u[%u], dup block\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1]);
				break;
			case EV_CLIBLOCK:
				fprintf(fd, "%s: block %u[%u], remaining=%u\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1],
					ptr->args[2]);
				break;
			case EV_CLISCHUNK:
				fprintf(fd, "%s: start chunk %u, block %u, "
					"%u chunks in progress (goodblks=%u)\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1],
					ptr->args[2], ptr->args[3]);
				break;
			case EV_CLIECHUNK:
				fprintf(fd, "%s: end chunk %u, block %u, "
					"%u chunks in progress (goodblks=%u)\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1],
					ptr->args[2], ptr->args[3]);
				break;
			case EV_CLILCHUNK:
				fprintf(fd, "%s: switched from incomplete "
					"chunk %u at block %u "
					"(%u blocks to go)\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1],
					ptr->args[2]);
				break;
			case EV_CLIREQ:
				fprintf(fd, "%s: send REQUEST, %u[%u-%u]\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1],
					ptr->args[1]+ptr->args[2]-1);
				break;
			case EV_CLIPREQ:
				fprintf(fd, "%s: send PREQUEST, %u(%u)\n",
					inet_ntoa(ptr->srcip), ptr->args[0],
					ptr->args[1]);
				break;
			case EV_CLIREQCHUNK:
				fprintf(fd, "%s: request chunk, timeo=%u\n",
					inet_ntoa(ptr->srcip), ptr->args[0]);
				break;
			case EV_CLIREQRA:
				fprintf(fd, "%s: issue readahead, "
					"empty=%u, filling=%u\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1]);
				break;
			case EV_CLIJOINREQ:
				fprintf(fd, "%s: send JOIN, ID=%x\n",
					inet_ntoa(ptr->srcip), ptr->args[0]);
				break;
			case EV_CLIJOINREP:
			{
				unsigned long long bytes;
				bytes = (unsigned long long)ptr->args[2] << 32;
				bytes |= ptr->args[3];
				fprintf(fd, "%s: got JOIN reply, "
					"chunksize=%u, blocksize=%u, "
					"imagebytes=%llu\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1], bytes);
				break;
			}
			case EV_CLILEAVE:
			{
				unsigned long long bytes;
				bytes = (unsigned long long)ptr->args[2] << 32;
				bytes |= ptr->args[3];
				fprintf(fd, "%s: send LEAVE, ID=%x, "
					"time=%u, bytes=%llu\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1], bytes);
				break;
			}
			case EV_CLISTAMP:
				fprintf(fd, "%s: update chunk %u, stamp %u.%06u\n",
					inet_ntoa(ptr->srcip), ptr->args[0],
					ptr->args[1], ptr->args[2]);
				break;
			case EV_CLIREDO:
				fprintf(fd, "%s: redo needed on chunk %u, "
					"lastreq at %u.%06u\n",
					inet_ntoa(ptr->srcip), ptr->args[0],
					ptr->args[1], ptr->args[2]);
				break;
			case EV_CLIDCSTART:
				fprintf(fd, "%s: decompressing chunk %u, "
					"idle=%u, (dblock=%u, widle=%u)\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1],
					ptr->args[2], ptr->args[3]);
				break;
			case EV_CLIDCDONE:
				fprintf(fd, "%s: chunk %u (%u bytes) "
					"decompressed, %u left\n",
					inet_ntoa(ptr->srcip),
					ptr->args[0], ptr->args[1],
					ptr->args[2]);
				break;
			case EV_CLIDCSTAT:
			{
				unsigned long long bytes;
				bytes = (unsigned long long)ptr->args[0] << 32;
				bytes |= ptr->args[1];
				fprintf(fd, "%s: decompressed %llu bytes total"
					" (dblock=%u, widle=%u)\n",
					inet_ntoa(ptr->srcip), bytes,
					ptr->args[2], ptr->args[3]);
				break;
			}
			case EV_CLIDCIDLE:
				fprintf(fd, "%s: decompressor IDLE\n",
					inet_ntoa(ptr->srcip));
				break;
			case EV_CLIWRSTATUS:
			{
				unsigned long long bytes;
				char *str = "??";

				switch (ptr->args[0]) {
				case 0:
					str = "writer STARTED";
					break;
				case 1:
					str = "writer IDLE";
					break;
				case 2:
					str = "writer RUNNING";
					break;
				case 3:
					str = "fsync STARTED";
					break;
				case 4:
					str = "fsync ENDED";
					break;
				}
				fprintf(fd, "%s: %s",
					inet_ntoa(ptr->srcip), str);

				bytes = (unsigned long long)ptr->args[1] << 32;
				bytes |= ptr->args[2];
				fprintf(fd, ", %llu bytes written\n",
					bytes);
				break;
			}
			case EV_CLIGOTPKT:
				stamp.tv_sec = ptr->args[0];
				stamp.tv_usec = ptr->args[1];
				timersub(&ptr->tstamp, &stamp, &stamp);
				fprintf(fd, "%s: got block, wait=%03ld.%03ld"
					", %u good blocks recv (%u  total)\n",
					inet_ntoa(ptr->srcip),
					stamp.tv_sec, stamp.tv_usec/1000,
					ptr->args[2], ptr->args[3]);
				break;
			case EV_CLIRTIMO:
				stamp.tv_sec = ptr->args[0];
				stamp.tv_usec = ptr->args[1];
				timersub(&ptr->tstamp, &stamp, &stamp);
				fprintf(fd, "%s: recv timeout, wait=%03ld.%03ld\n",
					inet_ntoa(ptr->srcip),
					stamp.tv_sec, stamp.tv_usec/1000);
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

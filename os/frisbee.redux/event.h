/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002, 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#define STRSIZE		64

/*
 * Event defs
 */
typedef struct {
	int type;
	union {
		struct {
			int startdelay;	/* range in sec of start interval */
			int startat;	/* start time (alt to startdelay) */
			int pkttimeout;	/* packet timeout in usec */
			int idletimer;	/* idle timer in pkt timeouts */
			int chunkbufs;  /* max receive buffers */
			int writebufmem;/* max disk write buffer memory */
			int maxmem;	/* max total memory */
			int readahead;  /* max readahead in packets */
			int inprogress; /* max packets in progress */
			int redodelay;	/* redo delay in usec */
			int idledelay;	/* writer idle delay in usec */
			int slice;	/* disk slice to write */
			int zerofill;	/* non-0 to zero fill free space */
			int randomize;	/* non-0 to randomize request list */
			int nothreads;	/* non-0 to single thread unzip */
			int dostype;	/* DOS partition type to set */
			int debug;	/* debug level */
			int trace;	/* tracing level */
			char traceprefix[STRSIZE];
					/* prefix for trace output file */
		} start;
		struct {
			int exitstatus;
		} stop;
	} data;
} Event_t;

#define EV_ANY		0
#define EV_START	1
#define EV_STOP		2

extern int EventInit(char *server);
extern int EventCheck(Event_t *event);
extern void EventWait(int eventtype, Event_t *event);

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#define SERVER_PORTNUM		16534
#define SOCKBUFSIZE		(1024 * 128)

/*
 * The barrier request structure sent to the daemon. There is no return
 * value; returning means go!
 */
typedef struct {
	char		name[64];	/* An arbitrary string */
	short		request;	/* Either init or wait */
	short		flags;		/* See below */
	int		count;		/* Number of waiters */
} barrier_req_t;

/* Request */
#define BARRIER_INIT		1
#define BARRIER_WAIT		2

/* Flags */
#define BARRIER_INIT_NOWAIT	0x1	/* Initializer does not wait! */

/* Default name is not specified */
#define DEFAULT_BARRIER "barrier"

/* Info */
#define CURRENT_VERSION 1

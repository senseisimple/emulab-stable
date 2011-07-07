/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#define SERVER_SERVNAME		"emulab_syncd"
#define SERVER_PORTNUM		16534
#define SOCKBUFSIZE		(1024 * 128)

/*
 * The barrier request structure sent to the daemon.  A single integer is
 * returned, its value is zero or the maximum of the error codes from the
 * clients.
 */
typedef struct {
	char		name[64];	/* An arbitrary string */
	short		request;	/* Either init or wait */
	short		flags;		/* See below */
	int		count;		/* Number of waiters */
	int		error;		/* Error code (0 == no error) */
} barrier_req_t;

/* Request */
#define BARRIER_INIT		1
#define BARRIER_WAIT		2

/* Flags */
#define BARRIER_INIT_NOWAIT	0x1	/* Initializer does not wait! */

/* Default name is not specified */
#define DEFAULT_BARRIER "barrier"

/* Info */
#define CURRENT_VERSION 2

/* Start of error codes for the server */
#define SERVER_ERROR_BASE	240
/* Error code for server got a SIGHUP */
#define SERVER_ERROR_SIGHUP	(SERVER_ERROR_BASE)

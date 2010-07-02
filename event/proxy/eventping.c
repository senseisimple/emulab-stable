/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */


#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "event.h"
#include "tbdefs.h"
#include "log.h"

static event_handle_t bosshandle;

void
usage(char *progname)
{
    fprintf(stderr, "Usage: %s -s server -p port\n", progname);
    exit(-1);
}


int
main(int argc, char **argv)
{
	char			*progname;
	char			*server = NULL;
	char			*port = NULL;
	char			buf[BUFSIZ];
	int                     c;

	progname = argv[0];
	
	while ((c = getopt(argc, argv, "s:p:")) != -1) {
		switch (c) {
		case 's':
			server = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		default:
			usage(progname);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc)
		usage(progname);

	if ((! server) || (!port)) 
		fatal("Must provide server and port");

	/*
	 * Convert server/port to elvin thing.
	 */
	snprintf(buf, sizeof(buf), "elvin://%s:%s",
		 server, port);
	server = buf;

	/* Register with the event system on boss */
	bosshandle = event_register_withkeydata_withretry(server, 0, NULL, 0, 0);

	if (bosshandle == NULL) {
		fatal("could not register with remote event system");
	}

	/* Unregister with the remote event system: */
	if (event_unregister(bosshandle) == 0) {
		fatal("could not unregister with remote event system");
	}

	return EXIT_SUCCESS;
}





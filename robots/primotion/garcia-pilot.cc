/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file garcia-pilot.cc
 *
 * The main body of the daemon that runs on the garcia robot.
 */

#include "config.h"

#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <ucontext.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include "acpGarcia.h"
#include "acpValue.h"

#include "pilotClient.hh"
#include "dashboard.hh"
#include "wheelManager.hh"
#include "pilotButtonCallback.hh"

/**
 * The default port to listen for client connections.
 */
#define PILOT_PORT 2531

/**
 * Default path to the battery log.
 */
static const char *BATTERY_LOG_PATH = "/var/log/battery.log";

/**
 * Determines whether or not to continue in the main loop.
 *
 * @see sigquit
 */
static volatile int looping = 1;

/**
 * Signal handler for SIGINT, SIGTERM, and SIGQUIT signals.  Sets looping to
 * zero and aborts() if it is called more than three times in case the program
 * is unable to return to the main loop.
 *
 * @param sig The actual signal number received.
 */
static void sigquit(int sig)
{
    static int quit_count = 3;
    
    if (quit_count == 0) {
	static char *msg = "error: wedged, aborting...\n";
	
	write(2, msg, strlen(msg));

	abort();
    }
    
    looping = 0;
    quit_count -= 1;
}

/**
 * Signal handler for SIGUSR1, just toggles the debugging level.
 *
 * @param sig The actual signal number received.
 */
static void sigdebug(int sig)
{
    if (debug == 3)
	debug = 1;
    else
	debug = 3;
}

/**
 * Signal handler for SIGSEGV and SIGBUS, just prints out the signal number and
 * aborts.
 */
static void sigpanic(int sig)
{
    fprintf(stderr,
	    "panic: signal=%d\n",
	    sig);

    abort();
}

/**
 * Prints the usage message for this daemon to stderr.
 */
static void usage(void)
{
    fprintf(stderr,
	    "usage: garcia-pilot -hd [-l logfile] [-i pidfile] [-p port]\n"
	    "  [-b battery-log]\n"
	    "Options:\n"
	    "  -h\t\tPrint this message\n"
	    "  -d\t\tTurn on debugging messages and do not daemonize\n"
	    "  -l logfile\tSend log output to the given file\n"
	    "  -i pidfile\tWrite the process ID to the given file\n"
	    "  -p port\tListen on the given port for MTP messages\n"
	    "  -b battery-log\tAppend battery log data to the given file\n"
	    "    \t\t(Default: %s)\n",
	    BATTERY_LOG_PATH);
}

int main(int argc, char *argv[])
{
    int c, port = PILOT_PORT, serv_sock, on_off = 1;
    const char *logfile = NULL, *pidfile = NULL;
    const char *batteryfile = BATTERY_LOG_PATH;
    int retval = EXIT_SUCCESS;
    unsigned long now;
    FILE *batterylog;
    acpGarcia garcia;
    aIOLib ioRef;
    aErr err;

    while ((c = getopt(argc, argv, "hdp:l:i:b:")) != -1) {
	switch (c) {
	case 'h':
	    usage();
	    exit(0);
	    break;
	case 'd':
	    debug += 1;
	    break;
	case 'l':
	    logfile = optarg;
	    break;
	case 'i':
	    pidfile = optarg;
	    break;
	case 'p':
	    if (sscanf(optarg, "%d", &port) != 1) {
		fprintf(stderr,
			"error: -p option is not a number: %s\n",
			optarg);
		usage();
		exit(1);
	    }
	    break;
	case 'b':
	    batteryfile = optarg;
	    break;
	}
    }

    if (getuid() != 0) {
	fprintf(stderr, "error: must run as root!\n");
	exit(1);
    }

    if (!debug) {
	/* Become a daemon */
	daemon(0, 0);

	if (logfile) {
	    FILE *file;

	    if ((file = fopen(logfile, "w")) != NULL) {
		dup2(fileno(file), 1);
		dup2(fileno(file), 2);
		stdout = file;
		stderr = file;
	    }
	}
    }

    if (pidfile) {
	FILE *fp;
	
	if ((fp = fopen(pidfile, "w")) != NULL) {
	    fprintf(fp, "%d\n", getpid());
	    (void) fclose(fp);
	}
    }

    if ((batterylog = fopen(batteryfile, "a")) == NULL) {
	fprintf(stderr,
		"error: cannot open battery log file - %s\n",
		batteryfile);
    }

    signal(SIGUSR1, sigdebug);

    signal(SIGQUIT, sigquit);
    signal(SIGTERM, sigquit);
    signal(SIGINT, sigquit);

    signal(SIGSEGV, (void (*)(int))sigpanic);
    signal(SIGBUS, (void (*)(int))sigpanic);
    
    signal(SIGPIPE, SIG_IGN);
    
    aIO_GetLibRef(&ioRef, &err);

    if (!wait_for_brainstem_link(ioRef, garcia)) {
	fprintf(stderr,
		"error: could not connect to robot %d\n",
		garcia.getNamedValue("status")->getIntVal());
	exit(1);
    }
    else {
	struct sockaddr_in saddr;
	acpValue av;

	if (debug) {
	    fprintf(stderr, "debug: connected to brainstem\n");
	}

	/* Make sure the units used by the robot are what we expect. */
	av.set("radians");
	garcia.setNamedValue("angle-units-string", &av);

	av.set("meters");
	garcia.setNamedValue("distance-units-string", &av);

	memset(&saddr, 0, sizeof(saddr));
#if !defined(linux)
	saddr.sin_len = sizeof(saddr);
#endif
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = INADDR_ANY;
	
	if ((serv_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	    perror("socket");
	}
	else if (setsockopt(serv_sock,
			    SOL_SOCKET,
			    SO_REUSEADDR,
			    &on_off,
			    sizeof(on_off)) == -1) {
	    perror("setsockopt");
	}
	else if (bind(serv_sock,
		      (struct sockaddr *)&saddr,
		      sizeof(saddr)) == -1) {
	    perror("bind");
	}
	else if (listen(serv_sock, 5) == -1) {
	    perror("listen");
	}
	else {
	    pilotClient::list clients;
	    fd_set readfds, writefds;
	    
	    dashboard db(garcia, batterylog);
	    wheelManager wm(garcia);
	    pilotButtonCallback pbc(db, wm);
	    
	    FD_ZERO(&readfds);
	    FD_ZERO(&writefds);
	    FD_SET(serv_sock, &readfds);
	    
	    wm.setDashboard(&db);
	    do {
		fd_set rreadyfds = readfds, wreadyfds = writefds;
		struct timeval tv_zero = { 0, 0 };
		int rc;
		
		/* Poll the file descriptors, don't block */
		rc = select(FD_SETSIZE,
			    &rreadyfds,
			    &wreadyfds,
			    NULL,
			    &tv_zero);
		if (rc > 0) {
		    bool do_telem = db.wasTelemetryUpdated();
		    pilotClient::iterator i, j;
		    struct mtp_packet tmp;
		    
		    if (FD_ISSET(serv_sock, &rreadyfds)) {
			struct sockaddr_in peer_sin;
			socklen_t slen;
			int cfd;
			
			slen = sizeof(peer_sin);
			if ((cfd = accept(serv_sock,
					  (struct sockaddr *)&peer_sin,
					  &slen)) == -1) {
			    perror("accept");
			}
			else {
			    pilotClient *pc = new pilotClient(cfd, wm);
			    
			    if (debug) {
				fprintf(stderr,
					"debug: connect from %s:%d\n",
					inet_ntoa(peer_sin.sin_addr),
					ntohs(peer_sin.sin_port));
			    }
			    
			    pc->setFD(&readfds);
			    pc->setFD(&writefds);
			    clients.push_back(pc);
			}
		    }
		    
		    if (do_telem) {
			mtp_init_packet(&tmp,
					MA_Opcode, MTP_TELEMETRY,
					MA_Role, MTP_ROLE_RMC,
					MA_GarciaTelemetry, db.getTelemetry(),
					MA_TAG_DONE);
		    }

		    for (i = clients.begin(); i != clients.end(); ) {
			bool bad_client = false;
			pilotClient *pc = *i;
			
			j = i++;
			if (pc->isFDSet(&rreadyfds)) {
			    do {
				struct mtp_packet mp;
				
				if ((mtp_receive_packet(pc->getHandle(),
							&mp) !=
				     MTP_PP_SUCCESS) ||
				    !pc->handlePacket(&mp, clients)) {
				    bad_client = true;
				}
				
				mtp_free_packet(&mp);
			    } while (pc && pc->getHandle()->mh_remaining);
			}

			if ((pc->getRole() == MTP_ROLE_EMULAB) &&
			    pc->isFDSet(&wreadyfds)) {
			    if (do_telem &&
				mtp_send_packet(pc->getHandle(), &tmp) !=
				MTP_PP_SUCCESS) {
				fprintf(stderr,
					"error: cannot send telemetry to "
					"client\n");

				bad_client = true;
			    }
			}
			
			if (bad_client) {
			    clients.erase(j);
			    
			    pc->clearFD(&readfds);
			    pc->clearFD(&writefds);
			    
			    delete pc;
			    pc = NULL;
			}
		    }
		}
		
		garcia.handleCallbacks(50);
		aIO_GetMSTicks(ioRef, &now, NULL);
	    } while (looping && db.update(now));
	    
	    garcia.flushQueuedBehaviors();
	    
	    garcia.handleCallbacks(1000);
	}
    }

    aIO_ReleaseLibRef(ioRef, &err);

    fclose(batterylog);
    batterylog = NULL;
    
    return retval;
}

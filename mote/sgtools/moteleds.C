/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * moteleds.C - program to read the mote LED pins on a Stargate, and display
 * them: on the stargate's LEDs, on the command line, or out a TCP socket
 */


/*
 * These seem to make things happier when compiling on Linux
 */
#ifdef __linux__
#define __PREFER_BSD
#define __USE_BSD
#define __FAVOR_BSD
#endif

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#ifdef __linux__
#include <linux/sockios.h>
#else
#include <sys/sockio.h>
#endif

#include "SGGPIO.h"

/*
 * Mote LEDs
 */
#define MOTELED_RED_PIN    24
#define MOTELED_GREEN_PIN  28
#define MOTELED_YELLOW_PIN 29

/*
 * Stargate LEDs
 */
#define SGLED_YELLOW_PIN 62
#define SGLED_GREEN_PIN  63
#define SGLED_RED_PIN    64

/*
 * LED IOCTL constants - derived from linux/led.h
 */
#define LED_DEV "/dev/platx/led" 
#define CONSUS_LED_IOCTL_MAGIG 'g'
#define CLED_IOSET          _IO (CONSUS_LED_IOCTL_MAGIG, 1)
#define CLED_IOGET          _IOR(CONSUS_LED_IOCTL_MAGIG, 2, int*)
#define CLED_IOSTARTAUTO    _IO (CONSUS_LED_IOCTL_MAGIG, 3)
#define CLED_IOSTOPAUTO     _IO (CONSUS_LED_IOCTL_MAGIG, 4)
#define CLED_IOAUTOSTATUS   _IOR(CONSUS_LED_IOCTL_MAGIG, 5, int*)
#define CLED_RADIO_RESET    _IO (CONSUS_LED_IOCTL_MAGIG, 6)

#define LED_GREEN      (1 << 13)
#define LED_RED        (1 << 15)
#define LED_YELLOW     (1 << 14)
#define LED_MASK       (LED_RED|LED_GREEN|LED_YELLOW)

// Port to listen on
#define PORT 1812

// How many clients we can handle at once
#define MAX_CLIENTS 64

// Which mode we're in
enum modes {
    MODE_NONE   = 0x0,
    MODE_MIRROR = 0x1,
    MODE_PRINT  = 0x2,
    MODE_EVENT  = 0x4,
    MODE_SOCKET = 0x8
};

void usage() {
	fprintf(stderr,"Usage: moteleds <-m | -p | -e | -s> [-t sleeptime]\n");
	fprintf(stderr,"  -m  Mirror mode - set stargate LEDs\n");
	fprintf(stderr,"  -p  Print mode - print LED status to stdout\n");
	//fprintf(stderr,"  -e  Event mode - set events on LED state change\n");
	fprintf(stderr,"  -s  Socket mode - open up a socket\n");
	fprintf(stderr,"  -d  daemonize\n");
	fprintf(stderr,"  -t  Sleep time in microseconds\n");
	exit(1);
}

int main(int argc, char **argv) {

    unsigned int mode = MODE_NONE;
    int naptime = 0;

    // Process command-line args
    int ch;
    bool daemonize = false;
    while ((ch = getopt(argc, argv, "mpesdt:")) != -1)
	switch (ch) {
	    case 'm':
		mode |= MODE_MIRROR;
		break;
	    case 'p':
		mode |= MODE_PRINT;
		break;
	    case 'e':
		mode |= MODE_EVENT;
		break;
	    case 's':
		mode |= MODE_SOCKET;
		break;
	    case 'd':
		daemonize = true;
		break;
	    case 't':
		if (!sscanf(optarg,"%i",&naptime)) {
		    usage();
		}
		break;
	    case '?':
	    default:
		usage();
	}
    argc -= optind;
    argv += optind;

    if (argc) {
	usage();
    }

    if (mode == MODE_NONE) {
	usage();
    }


    // Set the GPIO pins to read from the mote LEDs
    SGGPIO_PORT sggpio;
    sggpio.setDir(MOTELED_YELLOW_PIN,0);
    sggpio.setDir(MOTELED_GREEN_PIN,0);
    sggpio.setDir(MOTELED_RED_PIN,0);

    if (!naptime) {
	naptime = 10 * 1000; // 100Hz
    }

    // We're going to mirror the mote LEDs to the stargate LEDs, so open up
    // a file to control the SG LEDs
    int ledfd = -1;
    if (mode & MODE_MIRROR) {
	ledfd = open(LED_DEV, O_RDWR);
	if (ledfd < 0) {
	    fprintf(stderr, "Open error: %s\n", LED_DEV);
	    return 1;
	}
	// Turn off the automatic LED pattern
	if (ioctl (ledfd, CLED_IOSTOPAUTO) < 0) {
	    fprintf (stderr, "ioctl stop auto error: %s\n", LED_DEV);
	    return 1;
	}
    }

    // If we're supposed to write LED state change events to a socket, open
    // that up now
    int sockfd = -1;
    if (mode & MODE_SOCKET) {
	struct protoent *proto = getprotobyname("TCP");
	sockfd = socket(AF_INET,SOCK_STREAM,proto->p_proto);
	if (sockfd < 0) {     
	    perror("Creating socket");
	    exit(1);
	}                               

	// Set SO_RESEADDUR to make it easier to kill and restart this daemon
        int opt = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0)
                perror("SO_REUSEADDR");
	
	// Make the socket non-blocking
        if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0)
                perror("O_NONBLOCK");

	struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_port = htons(PORT);
        address.sin_addr.s_addr = INADDR_ANY;

        if (bind(sockfd,(struct sockaddr*)(&address),sizeof(address))) {
                perror("Binding socket");
                exit(1);
        }

        if (listen(sockfd,-1)) {
                perror("Listening on socket");
                exit(1);
        }

        /*
         * Ignore SIGPIPE - we'll detect the remote end closing the connection
         * by a failed write() .
         */
	struct sigaction action;
        action.sa_handler = SIG_IGN;
        sigaction(SIGPIPE,&action,NULL);

    }

    // Daemonize if we're supposed to
    if (daemonize) {
	daemon(0,0);
    }

    /*
     * Main loop
     */
    unsigned int current_status, old_status;
    // Bogus value so that we will detect a change the first time through
    old_status = 31337;

    // FD of the clients for our socket
    int clientfds[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) { clientfds[i] = -1; }
    bool newclient = false;
    int current_clients = 0;
    while(1) {
	// If we have a socket, try to accept connections
	if ((mode & MODE_SOCKET) && sockfd) {

	    struct sockaddr client_addr;
	    size_t client_addrlen;

	    int newclientfd = accept(sockfd,&client_addr,&client_addrlen);
	    if (newclientfd >= 0) {
		if (current_clients == MAX_CLIENTS) {
		    // Bummer, too many
		    close(newclientfd);
		}
		newclient = true;
		for (int i = 0; i < MAX_CLIENTS; i++) {
		    if (clientfds[i] == -1) {
			clientfds[i] = newclientfd;
			current_clients++;
			break;
		    }
		}
	    }
	}

	// Read mote pin connectors
	int yellow = sggpio.readPin(MOTELED_YELLOW_PIN);
	int green = sggpio.readPin(MOTELED_GREEN_PIN);
	int red = sggpio.readPin(MOTELED_RED_PIN);

	// Mote LEDs use negative logic, reverse them
	if (yellow) { yellow = 0; } else { yellow = 1; }
	if (green) { green = 0; } else { green = 1; }
	if (red) { red = 0; } else { red = 1; }

	// In mirroring mode, set up a patter for the SG LEDs and ioctl() it
	// into place
	if (mode & MODE_MIRROR) {
	    int ledpattern = 0;
	    if (yellow) { ledpattern |= LED_RED; }
	    if (green) { ledpattern |= LED_GREEN; }
	    if (red) { ledpattern |= LED_YELLOW; }
	    if (ioctl (ledfd, CLED_IOSET, ledpattern) < 0) {
		fprintf (stderr, "ioctl set error: %s\n", LED_DEV);
		exit(1);
	    } 
	}

	// Check to see if what we got this time was different from last time
	current_status = (red << 2) | (green << 1) | yellow;
	if (old_status != current_status) {
	    // In printing mode, do just that
	    if (mode & MODE_PRINT) {
		printf("%i %i %i\n",red,green,yellow);
	    }
	}

	if ((old_status != current_status) || newclient) {
	    // If we have a socket, print on that
	    if ((mode & MODE_SOCKET) && (current_clients > 0)) {
		char outbuf[1024];
		snprintf(outbuf,1024,"%i %i %i\n",red,green,yellow);
		for (int i = 0; i <= MAX_CLIENTS; i++) {
		    int clientfd = clientfds[i];
		    if (clientfd == -1) {
			continue;
		    }
		    if (write(clientfd,outbuf,strlen(outbuf)) < 0) {
			// Detect disconnected clients
			if (errno != EPIPE) {
			    perror("write");
			}
			close(clientfd);
			clientfds[i] = -1;
			current_clients--;
		    }
		}
	    }
	    newclient = false;
	}
	old_status = current_status;

	// Get some, go again
	usleep(naptime);
    }
}

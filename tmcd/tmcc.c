#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "decls.h"

void		sigcatcher(int foo);

char *usagestr = 
 "usage: tmcc [-p #] <command>\n"
 " -p portnum	   Specify a port number to connect to\n"
 "\n";

void
usage()
{
	fprintf(stderr, usagestr);
	exit(1);
}


int
main(int argc, char **argv)
{
	int			sock, data, n, cc, ch, portnum;
	struct sockaddr_in	name;
#if 0
	struct timeval		tv;
#endif
	struct hostent		*he;
	struct in_addr		serverip;
	char			buf[MYBUFSIZE], *bp, *response = "";
#ifdef UDPTEST
	int			useudp = 0;
	void			doudp(int argc, char **argv);

	if (argc > 1 && strcmp(argv[1], "-u") == 0) {
		useudp = 1;
		argc--, argv++;
	}
#endif

	portnum = TBSERVER_PORT;

	while ((ch = getopt(argc, argv, "p:")) != -1)
		switch(ch) {
		case 'p':
			portnum = atoi(optarg);
			break;
		default:
			usage();
		}

	argc -= optind;
	if (argc < 2 || argc > 4) {
		usage();
	}
	argv += optind;

#ifdef	LBS
	inet_aton(MASTERNODE, &serverip);
#else
	he = gethostbyname(MASTERNODE);
	if (he)
		memcpy((char *)&serverip, he->h_addr, he->h_length);
	else {
		fprintf(stderr, "gethostbyname(%s) failed\n", MASTERNODE); 
		exit(1);
	}
#endif

#ifdef UDPTEST
	if (useudp)
		doudp(argc, argv);
#endif
	while (1) {
		/* Create socket from which to read. */
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock < 0) {
			perror("creating stream socket:");
			exit(1);
		}

		/* Create name. */
		name.sin_family = AF_INET;
		name.sin_addr   = serverip;
		name.sin_port = htons(portnum);

		if (connect(sock,
			    (struct sockaddr *) &name, sizeof(name)) == 0) {
			break;
		}
		if (errno == ECONNREFUSED) {
			fprintf(stderr, "Connection to TMCD refused "
				"Sleeping a little while ...\n");
			sleep(10);
		}
		else {
			perror("connecting stream socket");
			exit(1);
		}
		close(sock);
	}

	data = 1;
	if (setsockopt(sock, SOL_SOCKET,
		       SO_KEEPALIVE, &data, sizeof(data)) < 0) {
		perror("setsockopt SO_KEEPALIVE");
		close(sock);
		exit(1);
	}
#if 0
#ifndef linux
	tv.tv_sec  = 30;
	tv.tv_usec = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		perror("setsockopt SO_RCVTIMEO");
		close(sock);
		exit(1);
	}
#endif
#endif
	/* Since we've gone through a getopt() pass, argv[0] is now the first argument */
	switch(argc) {
	case 1:
		n = snprintf(buf, sizeof(buf) - 1, "%s", argv[0]);
		break;
	case 2:
		n = snprintf(buf, sizeof(buf) - 1, "%s %s", argv[0], argv[1]);
		break;
	case 3:
		n = snprintf(buf, sizeof(buf) - 1, "%s %s %s",
			     argv[0], argv[1], argv[2]);
		break;
	default:
		fprintf(stderr, "Too many command arguments!\n");
		exit(1);
	}
		
	if (n >= sizeof(buf)) {
		fprintf(stderr, "Command too large!\n");
		exit(1);
	}

	/*
	 * Write the command to the socket and wait for the response
	 */
	bp = buf;
	while (n) {
		if ((cc = write(sock, bp, n)) <= 0) {
			if (cc < 0) {
				perror("Writing to socket:");
				exit(1);
			}
			fprintf(stderr, "write aborted");
			exit(1);
		}
		bp += cc;
		n  -= cc;
	}

	while (1) {
		if ((cc = read(sock, buf, sizeof(buf))) <= 0) {
			if (cc < 0) {
				perror("Reading from socket:");
				exit(1);
			}
			break;
		}
		buf[cc] = '\0';
		bp = (char *) malloc(strlen(response) + cc + 1);
		assert(bp);
		strcpy(bp, response);
		strcat(bp, buf);
		response = bp;
	}
	printf("%s", response);

	close(sock);
	exit(0);
}

void
sigcatcher(int foo)
{
}

#ifdef UDPTEST
/*
 * Not very robust, send a single request, read a single reply. 
 */
void
doudp(int argc, char **argv)
{
	int			sock, length, n, cc;
	struct sockaddr_in	name, client;
	struct hostent		*he;
	struct in_addr		serverip;
	char			buf[MYBUFSIZE], *bp, *response = "";

	if (argc < 2 || argc > 3) {
		fprintf(stderr, "usage: %s <command>\n", argv[0]);
		exit(1);
	}

#ifdef	LBS
	inet_aton(MASTERNODE, &serverip);
#else
	he = gethostbyname(MASTERNODE);
	if (he)
		memcpy((char *)&serverip, he->h_addr, he->h_length);
	else {
		fprintf(stderr, "gethostbyname(%s) failed\n", MASTERNODE); 
		exit(1);
	}
#endif

	/* Create socket from which to read. */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("creating dgram socket:");
		exit(1);
	}

	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr   = serverip;
	name.sin_port = htons(TBSERVER_PORT);

	if (argc == 2)
		n = snprintf(buf, sizeof(buf) - 1, "%s", argv[1]);
	else
		n = snprintf(buf, sizeof(buf) - 1, "%s %s", argv[1], argv[2]);
	if (n >= sizeof(buf)) {
		fprintf(stderr, "Command too large!\n");
		exit(1);
	}

	/*
	 * Write the command to the socket and wait for the response
	 */
	cc = sendto(sock, buf, n, 0, (struct sockaddr *)&name, sizeof(name));
	if (cc != n) {
		if (cc < 0) {
			perror("Writing to socket:");
			exit(1);
		}
		fprintf(stderr, "short write (%d != %d)\n", cc, n);
		exit(1);
	}

	cc = recvfrom(sock, buf, sizeof(buf), 0,
		      (struct sockaddr *)&client, &length);
	if (cc < 0) {
		perror("Reading from socket:");
		exit(1);
	}
	buf[cc] = '\0';
	bp = (char *) malloc(strlen(response) + cc + 1);
	assert(bp);
	strcpy(bp, response);
	strcat(bp, buf);
	response = bp;
	printf("%s", response);

	close(sock);
	exit(0);
}
#endif

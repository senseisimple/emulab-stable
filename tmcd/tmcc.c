#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "decls.h"

#ifdef LBS
#define	MASTERNODE	"206.163.153.25"
#else
#define	MASTERNODE	"boss.emulab.net"
#endif

void		sigcatcher(int foo);

main(int argc, char **argv)
{
	int			sock, length, data, n, cc, err, i;
	struct sockaddr_in	name, client;
	struct timeval		tv;
	struct itimerval	timo;
	struct hostent		*he;
	struct in_addr		serverip;
	char			buf[BUFSIZ], *bp, *response = "";

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
		name.sin_port = htons(TBSERVER_PORT);

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
#ifndef linux
	tv.tv_sec  = 30;
	tv.tv_usec = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		perror("setsockopt SO_KEEPALIVE");
		close(sock);
		exit(1);
	}
#endif
	if (argc == 2)
		n = snprintf(buf, sizeof(buf) - 1, "%s", argv[1]);
	else
		n = snprintf(buf, sizeof(buf) - 1, "%s %s", argv[1], argv[2]);
	if (n >= sizeof(buf)) {
		fprintf(stderr, "Command to large!\n");
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


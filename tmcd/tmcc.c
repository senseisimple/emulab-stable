/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
#include "ssl.h"
#ifndef STANDALONE
#include "config.h"
#endif

#undef  BOSSNODE
#ifndef KEYFILE
#define KEYFILE		"/etc/emulab.pkey"
#endif

/*
 * We search a couple of dirs for the bossnode file.
 */
static char *bossnodedirs[] = {
	"/etc/testbed",
	"/etc/rc.d/testbed",
	"/usr/local/etc/testbed",
	"/usr/local/etc/emulab",
	0
};

/*
 * Need to try several ports cause of firewalling. 
 */
static int portlist[] = {
	TBSERVER_PORT,
	TBSERVER_PORT2,
};
static int numports = sizeof(portlist)/sizeof(int);

void		sigcatcher(int foo);
char		*getbossnode(void);
#ifdef UDP
void		doudp(int, char **, int, struct in_addr, int, char *, int);
#endif

char *usagestr = 
 "usage: tmcc [-u] [-v versnum] [-p #] [-s server] <command>\n"
 " -s server	   Specify a tmcd server to connect to\n"
 " -p portnum	   Specify a port number to connect to\n"
 " -v versnum	   Specify a version number for tmcd\n"
 " -n vnodeid	   Specify the vnodeid\n"
 " -k keyfile	   Specify the private keyfile\n"
 " -u		   Use UDP instead of TCP\n"
 " -t timeout	   Timeout waiting for the controller.\n"
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
	int			sock, data, n, cc, ch;
	struct sockaddr_in	name;
	struct hostent		*he;
	struct in_addr		serverip;
	char			buf[MYBUFSIZE], *bp, *response = "";
	char			*bossnode = NULL;
	int			version = CURRENT_VERSION;
	char			*vnodeid = NULL;
	char			*keyfile = NULL;
	char			*privkey = NULL;
	int			waitfor = 0;
	FILE			*fp;
#ifdef UDP
	int			useudp  = 0;
#endif

	while ((ch = getopt(argc, argv, "v:s:p:un:t:k:")) != -1)
		switch(ch) {
		case 'p':
			portlist[0] = atoi(optarg);
			numports    = 1;
			break;
		case 's':
			bossnode = optarg;
			break;
		case 'n':
			vnodeid = optarg;
			break;
		case 'k':
			keyfile = optarg;
			break;
		case 'v':
			version = atoi(optarg);
			break;
		case 't':
			waitfor = atoi(optarg);
			break;
#ifdef UDP
		case 'u':
			useudp = 1;
			break;
#endif
		default:
			usage();
		}

	argc -= optind;
	if (argc < 1 || argc > 5) {
		usage();
	}
	argv += optind;

	/*
	 * How do we find our bossnode?
	 *
	 * 1. Command line.
	 * 2. From a file in the list above.
	 * 3. nameserver goo below.
	 */
	if (!bossnode) {
		/*
		 * Search for the file.
		 */
		char **cp = bossnodedirs;
		while (*cp) {
			sprintf(buf, "%s/%s", *cp, BOSSNODE_FILENAME);

			if (access(buf, R_OK) == 0)
				break;
			cp++;
		}
		if (*cp) {
			if ((fp = fopen(buf, "r")) != NULL) {
				if (fgets(buf, sizeof(buf), fp)) {
					if ((bp = strchr(buf, '\n')))
						*bp = (char) NULL;
					bossnode = strdup(buf);
				}
				fclose(fp);
			}
		}
	}
	if (!bossnode)
		bossnode = getbossnode();
	he = gethostbyname(bossnode);
	if (he)
		memcpy((char *)&serverip, he->h_addr, he->h_length);
	else {
		fprintf(stderr, "gethostbyname(%s) failed\n", bossnode); 
		exit(1);
	}

	/*
	 * Grab the key. Its not an error if we do not find it.
	 */
	if (keyfile) {
		/* Well, if one was specifed it has to exist. */
		if (access(keyfile, R_OK) < 0) {
			perror("keyfile"); 
			exit(1);
		}
	}
	else
		keyfile = KEYFILE;

	if ((fp = fopen(keyfile, "r")) != NULL) {
	    if (fgets(buf, sizeof(buf), fp)) {
		if ((bp = strchr(buf, '\n')))
		    *bp = (char) NULL;
		privkey = strdup(buf);
	    }
	    fclose(fp);
	}

	if (waitfor) {
		alarm(waitfor);
	}

	/*
	 * Handle built-in "bossinfo" command
	 */
	if (strcmp(argv[0], "bossinfo") == 0) {
		printf("%s %s\n", bossnode, inet_ntoa(serverip));
		exit(0);
	}

#ifdef UDP
	if (useudp) {
		doudp(argc, argv, version, serverip,
		      portlist[0], vnodeid, waitfor);
		/*
		 * Never returns.
		 */
		abort();
	}
#endif
#ifdef  WITHSSL
	if (tmcd_client_sslinit()) {
		printf("SSL initialization failed!\n");
		exit(1);
	}
#endif
	while (1) {
		for (n = 0; n < numports; n++) {
			/* Create socket from which to read. */
			sock = socket(AF_INET, SOCK_STREAM, 0);
			if (sock < 0) {
				perror("creating stream socket:");
				exit(1);
			}

			/* Create name. */
			name.sin_family = AF_INET;
			name.sin_addr   = serverip;
			name.sin_port   = htons(portlist[n]);

			if (CONNECT(sock, (struct sockaddr *) &name,
				    sizeof(name)) == 0) {
				goto foundit;
			}
			if (errno != ECONNREFUSED) {
				perror("connecting stream socket");
				CLOSE(sock);
				exit(1);
			}
			CLOSE(sock);
		}
		fprintf(stderr,
			"Connection to TMCD refused. Waiting a bit ...\n");
		sleep(10);
	}
 foundit:

	data = 1;
	if (setsockopt(sock, SOL_SOCKET,
		       SO_KEEPALIVE, &data, sizeof(data)) < 0) {
		perror("setsockopt SO_KEEPALIVE");
		goto bad;
	}

	/* Start with version number */
	sprintf(buf, "VERSION=%d ", version);

	/* Tack on vnodeid */
	if (vnodeid) {
		sprintf(&buf[strlen(buf)], "VNODEID=%s ", vnodeid);
	}

	/* Tack on privkey */
	if (privkey) {
		sprintf(&buf[strlen(buf)], "PRIVKEY=%s ", privkey);
	}

	/*
	 * Since we've gone through a getopt() pass, argv[0] is now the
	 * first argument
	 */
	n = strlen(buf);
	while (argc && n < sizeof(buf)) {
		n += snprintf(&buf[n], sizeof(buf) - n, "%s ", argv[0]);
		argc--;
		argv++;
	}
	if (n >= sizeof(buf)) {
		fprintf(stderr, "Command too large!\n");
		goto bad;
	}

	/*
	 * Write the command to the socket and wait for the response
	 */
	bp = buf;
	while (n) {
		if ((cc = WRITE(sock, bp, n)) <= 0) {
			if (cc < 0) {
				perror("Writing to socket:");
				goto bad;
			}
			fprintf(stderr, "write aborted");
			goto bad;
		}
		bp += cc;
		n  -= cc;
	}

	while (1) {
		if ((cc = READ(sock, buf, sizeof(buf))) <= 0) {
			if (cc < 0) {
				perror("Reading from socket:");
				goto bad;
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

	CLOSE(sock);
	exit(0);
 bad:
	CLOSE(sock);
	exit(1);
}

void
sigcatcher(int foo)
{
}

#ifndef BOSSNODE
#include <resolv.h>
#endif

char *
getbossnode(void)
{
#ifdef BOSSNODE
	return strdup(BOSSNODE);
#else
	struct hostent *he;

	res_init();
	he = gethostbyaddr((char *)&_res.nsaddr.sin_addr,
			   sizeof(struct in_addr), AF_INET);
	if (he && he->h_name)
		return strdup(he->h_name);
	return("UNKNOWN");
#endif
}

#ifdef UDP
/*
 * Not very robust, send a single request, read a single reply. 
 */
void
doudp(int argc, char **argv,
      int vers, struct in_addr serverip, int portnum, char *vnodeid,
      int waitfor)
{
	int			sock, length, n, cc;
	struct sockaddr_in	name, client;
	char			buf[MYBUFSIZE], *bp, *response = "";

	/* Create socket from which to read. */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("creating dgram socket:");
		exit(1);
	}

	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr   = serverip;
	name.sin_port = htons(portnum);

	/* Start with version number */
	sprintf(buf, "VERSION=%d ", vers);

	/* Tack on vnodeid */
	if (vnodeid) {
		sprintf(&buf[strlen(buf)], "VNODEID=%s ", vnodeid);
	}

	/*
	 * Since we've gone through a getopt() pass, argv[0] is now the
	 * first argument
	 */
	n = strlen(buf);
	while (argc && n < sizeof(buf)) {
		n += snprintf(&buf[n], sizeof(buf) - n, "%s ", argv[0]);
		argc--;
		argv++;
	}
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

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/fcntl.h>
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
#ifndef BOSSNODE
#include <resolv.h>
#endif

#ifndef KEYFILE
#define KEYFILE		"/etc/emulab.pkey"
#endif
#define UNIXSOCKVAR	"TMCCUNIXPATH"

/*
 * We search a couple of dirs for the bossnode file.
 */
static char *bossnodedirs[] = {
	"/etc/testbed",
	"/etc/emulab",
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
/* Locals */
static int	numports = sizeof(portlist)/sizeof(int);
static int	debug = 0;
static char    *logfile = NULL;

/* Forward decls */
static int	getbossnode(char **, int *);
static int	doudp(char *, int, struct in_addr, int);
static int	dotcp(char *, int, struct in_addr);
static int	dounix(char *, int, char *);
static void	beproxy(char *, struct in_addr, char *);
static int	dooutput(int, char *, int);

char *usagestr = 
 "usage: tmcc [options] <command>\n"
 " -d		   Turn on debugging\n"
 " -s server	   Specify a tmcd server to connect to\n"
 " -p portnum	   Specify a port number to connect to\n"
 " -v versnum	   Specify a version number for tmcd\n"
 " -n vnodeid	   Specify the vnodeid\n"
 " -k keyfile	   Specify the private keyfile\n"
 " -u		   Use UDP instead of TCP\n"
 " -l path	   Use named unix domain socket instead of TCP\n"
 " -t timeout	   Timeout waiting for the controller.\n"
 " -x path	   Be a tmcc proxy, using the named unix domain socket\n"
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
	int			n, ch;
	struct hostent		*he;
	struct in_addr		serverip;
	char			buf[MYBUFSIZE], *bp;
	FILE			*fp;
	int			useudp    = 0;
	char			*unixpath = NULL;
	char			*bossnode = NULL;
	int			version   = CURRENT_VERSION;
	char			*vnodeid  = NULL;
	char			*keyfile  = NULL;
	char			*privkey  = NULL;
	char			*proxypath= NULL;
	int			waitfor   = 0;

	while ((ch = getopt(argc, argv, "v:s:p:un:t:k:x:l:do:")) != -1)
		switch(ch) {
		case 'd':
			debug++;
			break;
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
		case 'u':
			useudp = 1;
			break;
		case 'x':
			proxypath = optarg;
			break;
		case 'l':
			unixpath  = optarg;
			break;
		case 'o':
			logfile  = optarg;
			break;
		default:
			usage();
		}

	argv += optind;
	argc -= optind;
	if (!proxypath && (argc < 1 || argc > 5)) {
		usage();
	}
	if (unixpath && proxypath)
		usage();

	/*
	 * Allow for environment to specify a unix socket, which is a
	 * connection to a tmcc proxy. Note that a specific -l option
	 * overrides the environment variable.
	 */
	if (!unixpath && !proxypath && (bp = getenv(UNIXSOCKVAR)))
		unixpath = bp;

	if (unixpath && (keyfile || bossnode)) {
		fprintf(stderr,
			"You may not use the -k or -s with the -l option\n");
		usage();
	}

	if (!bossnode) {
		int	port = 0;
		
		getbossnode(&bossnode, &port);
		/* In other words, do not override command line port spec! */
		if (port && numports > 1) {
			portlist[0] = port;
			numports    = 1;
		}
	}
	
	he = gethostbyname(bossnode);
	if (he)
		memcpy((char *)&serverip, he->h_addr, he->h_length);
	else {
		fprintf(stderr, "gethostbyname(%s) failed\n", bossnode); 
		exit(1);
	}

	/*
	 * Handle built-in "bossinfo" command
	 */
	if (!proxypath && (strcmp(argv[0], "bossinfo") == 0)) {
		printf("%s %s\n", bossnode, inet_ntoa(serverip));
		exit(0);
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

	/*
	 * Build up the command request string.
	 */
	buf[0] = '\0';
	
	/* Tack on vnodeid */
	if (vnodeid) {
		sprintf(&buf[strlen(buf)], "VNODEID=%s ", vnodeid);
	}

	/* Tack on privkey */
	if (privkey) {
		sprintf(&buf[strlen(buf)], "PRIVKEY=%s ", privkey);
	}

	/*
	 * If specified udp with unixpath, then pass through a UDP flag
	 * to the proxy, which will do what it thinks is appropriate.
	 */
	if (useudp && unixpath) {
		sprintf(&buf[strlen(buf)], "USEUDP=1 ");
		useudp = 0;
	}

	/*
	 * In proxy mode ...
	 */
	if (proxypath) {
		beproxy(proxypath, serverip, buf);
		exit(0);
	}

	/* Tack on version number */
	sprintf(&buf[strlen(buf)], "VERSION=%d ", version);

	/*
	 * Since we've gone through a getopt() pass, argv[0] is now the
	 * first argument
	 */
	n = strlen(buf);
	while (argc && n < sizeof(buf)) {
		n += snprintf(&buf[n], sizeof(buf) - (n + 1), "%s ", argv[0]);
		argc--;
		argv++;
	}
	if (n >= sizeof(buf)) {
		fprintf(stderr, "Command too large!\n");
		exit(-1);
	}
	buf[n] = '\0';

	/*
	 * When a timeout is requested, just let the signal kill us.
	 */
	if (waitfor) {
		alarm(waitfor);
	}

	if (useudp)
		n = doudp(buf, fileno(stdout), serverip, portlist[0]);
	else if (unixpath)
		n = dounix(buf, fileno(stdout), unixpath);
	else
		n = dotcp(buf, fileno(stdout), serverip);
	exit(n);
}

/*
 * Find the bossnode name if one was not specified on the command line.
 */
static int
getbossnode(char **bossnode, int *portp)
{
#ifdef BOSSNODE
	*bossnode = strdup(BOSSNODE);
	return 0;
#else
	struct hostent	*he;
	FILE		*fp;
	char		buf[BUFSIZ], **cp = bossnodedirs, *bp;

	/*
	 * Search for the file.
	 */
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
				fclose(fp);
				/*
				 * Look for port spec
				 */
				if ((bp = strchr(buf, ':'))) {
					*bp++  = NULL;
					*portp = atoi(bp);
				}
				*bossnode = strdup(buf);
				return 0;
			}
			fclose(fp);
		}
	}
	
	/*
	 * Nameserver goo 
	 */
	res_init();
	he = gethostbyaddr((char *)&_res.nsaddr.sin_addr,
			   sizeof(struct in_addr), AF_INET);
	if (he && he->h_name) 
		*bossnode = strdup(he->h_name);
	else
		*bossnode = strdup("UNKNOWN");
	return 0;
#endif
}

/*
 * TCP version, which uses ssl if compiled in.
 */
static int
dotcp(char *data, int outfd, struct in_addr serverip)
{
	int			n, sock, cc;
	struct sockaddr_in	name;
	char			*bp, buf[MYBUFSIZE];
	
#ifdef  WITHSSL
	if (tmcd_client_sslinit()) {
		printf("SSL initialization failed!\n");
		return -1;
	}
#endif
	while (1) {
		for (n = 0; n < numports; n++) {
			/* Create socket from which to read. */
			sock = socket(AF_INET, SOCK_STREAM, 0);
			if (sock < 0) {
				perror("creating stream socket:");
				return -1;
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
				return -1;
			}
			CLOSE(sock);
		}
		if (debug) 
			fprintf(stderr,
				"Connection to TMCD refused. Waiting ...\n");
		sleep(10);
	}
 foundit:

	n = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &n, sizeof(n)) < 0) {
		perror("setsockopt SO_KEEPALIVE");
		goto bad;
	}

	/*
	 * Write the command to the socket and wait for the response.
	 */
	bp = data;
	n  = strlen(data);
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
		if ((cc = READ(sock, buf, sizeof(buf) - 1)) <= 0) {
			if (cc < 0) {
				perror("Reading from socket:");
				goto bad;
			}
			break;
		}
		if (dooutput(outfd, buf, cc) < 0)
			goto bad;
	}
	CLOSE(sock);
	return 0;
 bad:
	CLOSE(sock);
	return -1;
}

/*
 * Not very robust, send a single request, read a single reply. 
 */
static int
doudp(char *data, int outfd, struct in_addr serverip, int portnum)
{
	int			sock, length, n, cc;
	struct sockaddr_in	name, client;
	char			buf[MYBUFSIZE];

	/* Create socket from which to read. */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("creating dgram socket:");
		return -1;
	}

	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr   = serverip;
	name.sin_port   = htons(portnum);

	/*
	 * Write the command to the socket and wait for the response
	 */
	n  = strlen(data);
	cc = sendto(sock, data, n, 0, (struct sockaddr *)&name, sizeof(name));
	if (cc != n) {
		if (cc < 0) {
			perror("Writing to socket:");
			return -1;
		}
		fprintf(stderr, "short write (%d != %d)\n", cc, n);
		return -1;
	}

	cc = recvfrom(sock, buf, sizeof(buf) - 1, 0,
		      (struct sockaddr *)&client, &length);

	if (cc < 0) {
		perror("Reading from socket:");
		return -1;
	}
	close(sock);
	if (dooutput(outfd, buf, cc) < 0)
		return -1;
	return 0;
}

/*
 * Unix domain version.
 */
static int
dounix(char *data, int outfd, char *unixpath)
{
#ifdef  linux
	fprintf(stderr, "unix domain socket mode not supported on linux!\n");
	return -1;
#else
	int			n, sock, cc, length;
	struct sockaddr_un	sunaddr;
	char			*bp, buf[MYBUFSIZE];

	sunaddr.sun_family = AF_UNIX;
	strlcpy(sunaddr.sun_path, unixpath, sizeof(sunaddr.sun_path));
	sunaddr.sun_len = length = SUN_LEN(&sunaddr)+1;

	/* Create socket from which to read. */
	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("creating stream socket:");
		return -1;
	}

	if (connect(sock, (struct sockaddr *) &sunaddr, length) < 0) {
		perror("connecting unix domain socket");
		close(sock);
		return -1;
	}

	/*
	 * Write the command to the socket and wait for the response.
	 */
	bp = data;
	n  = strlen(data);
	while (n) {
		if ((cc = write(sock, bp, n)) <= 0) {
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
		if ((cc = read(sock, buf, sizeof(buf) - 1)) <= 0) {
			if (cc < 0) {
				perror("Reading from socket:");
				goto bad;
			}
			break;
		}
		if (dooutput(outfd, buf, cc) < 0)
			goto bad;
	}
	close(sock);
	return 0;
 bad:
	close(sock);
	return -1;
#endif
}

/*
 * Be a proxy. Basically, listen for requests from the localhost and
 * forward. At the moment this is strictly for the benefit of jailed
 * environments on FreeBSD, hence we use a unix domain socket to provide
 * the security (the socket is visible to the jail only).
 *
 * Note that only TCP traffic goes through the proxy. The jailed tmcc
 * can speak directly to tmcd over udp since it never returns anything
 * sensitive that way.
 */
static void
beproxy(char *localpath, struct in_addr serverip, char *partial)
{
#ifdef  linux
	fprintf(stderr, "proxy mode not supported on linux!\n");
	exit(-1);
#else
	int			sock, newsock, cc, length;
	struct sockaddr_un	sunaddr, client;
	char			command[MYBUFSIZE], buf[MYBUFSIZE];
	char			*bp, *cp;
	
	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("creating unix domain socket");
		exit(-1);
	}

	unlink(localpath);
	memset(&sunaddr, 0, sizeof(sunaddr));
	sunaddr.sun_family = AF_UNIX;
	strlcpy(sunaddr.sun_path, localpath, sizeof(sunaddr.sun_path));
	sunaddr.sun_len = SUN_LEN(&sunaddr) + 1;
	if (bind(sock, (struct sockaddr *)&sunaddr, sunaddr.sun_len) < 0) {
		perror("binding unix domain socket");
		exit(-1);
	}
	if (listen(sock, 5) < 0) {
		perror("listen on unix domain socket");
		exit(-1);
	}

	if (logfile) {
		int	fd;

		/*
		 * Detach, but do not daemonize. The caller tracks the
		 * pid and kills us. Needs to be fixed at some point ...
		 */
		if ((fd = open("/dev/null", O_RDONLY, 0)) < 0) {
			perror("Could not open /dev/null");
			exit(-1);
		}
		(void)dup2(fd, STDIN_FILENO);
		if (fd != STDIN_FILENO)
			(void)close(fd);
		
		if ((fd = open(logfile, O_WRONLY|O_CREAT|O_APPEND,0640)) < 0) {
			perror("Could not open logfile");
			exit(-1);
		}
		(void)dup2(fd, STDOUT_FILENO);
		(void)dup2(fd, STDERR_FILENO);
		if (fd > STDERR_FILENO)
			(void)close(fd);
	}

	/*
	 * Wait for TCP connections.
	 */
	while (1) {
		int	rval, useudp = 0;
		
		length  = sizeof(client);
		newsock = accept(sock, (struct sockaddr *)&client, &length);
		if (newsock < 0) {
			perror("accepting Unix domain connection");
			continue;
		}

		/*
		 * Read in the command request.
		 */
		if ((cc = read(newsock, buf, sizeof(buf) - 1)) <= 0) {
			if (cc < 0)
				perror("Reading Unix domain request");
			fprintf(stderr, "Unix domain connection aborted\n");
			close(newsock);
			continue;
		}
		buf[cc] = '\0';

		/*
		 * Do not allow PRIVKEY or VNODE options to be specified
		 * by the proxy user. 
		 */
		strcpy(command, partial);
		bp = cp = buf;
		while ((bp = strsep(&cp, " ")) != NULL) {
			if (strstr(bp, "PRIVKEY=") ||
			    strstr(bp, "VNODE=")) {
				if (debug)
					fprintf(stderr,
						"Ignoring option: %s\n", bp);
				continue;
			}
			if (strstr(bp, "USEUDP=1")) {
				useudp = 1;
				continue;
			}
			strcat(command, bp);
			strcat(command, " ");
		}

		if (debug) {
			fprintf(stderr, "%s\n", command);
			fflush(stderr);
		}

		if (useudp)
			rval = doudp(command, newsock, serverip, portlist[0]);
		else
			rval = dotcp(command, newsock, serverip);
		
		if (rval < 0 && debug) {
			fprintf(stderr, "Request failed!\n");
			fflush(stderr);
		}
		close(newsock);
	}
#endif
}

/*
 * Little helper to write tmcd response to output. 
 */
static int
dooutput(int fd, char *buf, int len)
{
	int		cc, count = len;

	if (debug) {
		write(fileno(stderr), buf, len);
	}
	
	while (count) {
		if ((cc = write(fd, buf, count)) <= 0) {
			if (cc < 0) {
				perror("Writing to output stream:");
				return -1;
			}
			fprintf(stderr, "write to socket aborted");
			return -1;
		}
		buf   += cc;
		count -= cc;
	}
	return len;
}

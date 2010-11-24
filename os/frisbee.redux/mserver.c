/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Frisbee master server.  Listen for GET/PUT requests and fork off
 * handler processes.
 */
#include <paths.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include "decls.h"
#include "configdefs.h"

static void	get_options(int argc, char **argv);
static int	makesocket(int portnum, int *tcpsockp);
static void	handle_request(int sock);
static void	childdeath(int sig);
static int	reapchildren(void);

static int	daemonize = 1;
int		debug = 0;
static int	dumpconfig = 0;

static	sigjmp_buf env;

/* XXX the following just keep network.c happy */
int		portnum;
struct in_addr	mcastaddr;
struct in_addr	mcastif;

int
main(int argc, char **argv)
{
	int			tcpsock;
	FILE			*fp;
	char			buf[BUFSIZ];
	char			*pidfile = (char *) NULL;
	sigset_t		mask;

	get_options(argc, argv);

	MasterServerLogInit();

	log("mfrisbee daemon starting");
	config_init(1);

	/* Just dump the config to stdout in human readable form and exit. */
	if (dumpconfig) {
		config_dump(stdout);
		exit(0);
	}

	/*
	 * Create TCP server.
	 */
	if (makesocket(MS_PORTNUM, &tcpsock) < 0) {
		error("Could not make primary tcp socket!");
		exit(1);
	}
	/* Now become a daemon */
	if (daemonize)
		daemon(0, 0);

	/*
	 * Stash the pid away.
	 */
	if (!geteuid()) {
		if (!pidfile) {
			sprintf(buf, "%s/mfrisbee.pid", _PATH_VARRUN);
			pidfile = buf;
		}
		fp = fopen(pidfile, "w");
		if (fp != NULL) {
			fprintf(fp, "%d\n", getpid());
			(void) fclose(fp);
		}
	}

	/*
	 * Handle connections
	 */
	signal(SIGCHLD, childdeath);
	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
	while (1) {
		struct sockaddr_in client;
		socklen_t length;
		int newsock;
		
		sigprocmask(SIG_BLOCK, &mask, NULL);
		if (sigsetjmp(env, 1) == 0) {
			sigprocmask(SIG_UNBLOCK, &mask, NULL);
			length  = sizeof(client);
			newsock = accept(tcpsock, (struct sockaddr *)&client,
					 &length);
			if (newsock < 0)
				pwarning("accepting TCP connection");
			else {
				fcntl(newsock, F_SETFD, FD_CLOEXEC);
				handle_request(newsock);
				close(newsock);
			}
		} else {
			reapchildren();
			sigprocmask(SIG_UNBLOCK, &mask, NULL);
		}
	}
	close(tcpsock);
	log("daemon terminating");
	exit(0);
}

struct childinfo {
	struct childinfo *next;
	struct config_host_authinfo *ai;
	int method;
	int pid;
	struct in_addr iface;
	in_addr_t addr;
	in_port_t port;
};
static struct childinfo *findchild(struct config_host_authinfo *, int);
static struct childinfo *startchild(struct config_host_authinfo *,
				    struct in_addr *, struct in_addr *, int,
				    int *);

void
handle_get(int sock, struct sockaddr_in *sip, struct sockaddr_in *cip,
	   MasterMsg_t *msg)
{
	char imageid[MS_MAXIDLEN+1];
	char clientip[sizeof("XXX.XXX.XXX.XXX")+1];
	int len = ntohs(msg->body.getrequest.idlen);
	struct config_host_authinfo *ai;
	struct childinfo *ci;
	int rv, methods, wantstatus;
	char *op;

	memcpy(imageid, msg->body.getrequest.imageid, len);
	imageid[len] = '\0';
	methods = msg->body.getrequest.methods;
	wantstatus = msg->body.getrequest.status;
	op = wantstatus ? "STATUS" : "GET";

	strncpy(clientip, inet_ntoa(cip->sin_addr), sizeof clientip);
	log("%s: %s from %s, (methods: 0x%x)",
	    imageid, op, clientip, methods);

	memset(msg, 0, sizeof *msg);
	msg->type = htonl(MS_MSGTYPE_GETREPLY);

	/*
	 * See if node has access to the image.
	 * If not, return an error code immediately.
	 */
	rv = config_auth_by_IP(&cip->sin_addr, imageid, &ai);
	if (rv) {
		warning("%s: client %s %s failed: %s",
			imageid, clientip, op, config_perror(rv));
		msg->body.getreply.error = rv;
		goto reply;
	}
	assert(ai->numimages == 1 && ai->imageinfo != NULL);

	/*
	 * Figure out what transfer method to use.
	 */
	if (debug)
		info("  request methods: 0x%x, image methods: 0x%x",
		     methods, ai->imageinfo[0].get_methods);
	methods &= ai->imageinfo[0].get_methods;
	if (methods == 0) {
		rv = CONFIG_ERR_HA_NOMETHOD;
		warning("%s: client %s %s failed: %s",
			imageid, clientip, op, config_perror(rv));
		msg->body.getreply.error = rv;
		goto reply;
	}

	/*
	 * Otherwise see if there is a frisbeed already running, starting
	 * one if not.  Then construct a reply with the available info.
	 */
	ci = findchild(ai, methods);
	if (ci == NULL) {
		struct in_addr in;

		if (wantstatus) {
			config_free_host_authinfo(ai);
			msg->body.getreply.method = methods;
			msg->body.getreply.isrunning = 0;
			log("%s: STATUS is not running", imageid);
			goto reply;
		}
		ci = startchild(ai, &sip->sin_addr, &cip->sin_addr, methods,
				&rv);
		if (ci == NULL) {
			config_free_host_authinfo(ai);
			msg->body.getreply.error = rv;
			goto reply;
		}
		in.s_addr = htonl(ci->addr);
		log("%s: started server on %s:%d (pid %d)",
		    imageid, inet_ntoa(in), ci->port, ci->pid);
	} else {
		struct in_addr in;

		config_free_host_authinfo(ai);
		in.s_addr = htonl(ci->addr);
		if (wantstatus)
			log("%s: STATUS is running on %s:%d (pid %d)",
			    imageid, inet_ntoa(in), ci->port, ci->pid);
		else
			log("%s: server already running on %s:%d (pid %d)",
			    imageid, inet_ntoa(in), ci->port, ci->pid);
	}

	msg->body.getreply.method = ci->method;
	msg->body.getreply.isrunning = 1;
	msg->body.getreply.addr = htonl(ci->addr);
	msg->body.getreply.port = htons(ci->port);

reply:
	msg->body.getreply.error = htons(msg->body.getreply.error);
	len = sizeof msg->type + sizeof msg->body.getreply;
	if (!MsgSend(sock, msg, len, 10))
		error("%s: could not send reply", inet_ntoa(cip->sin_addr));
}

static void
handle_request(int sock)
{
	MasterMsg_t msg;
	int cc;
	struct sockaddr_in me, you;
	socklen_t len;

	len = sizeof me;
	if (getsockname(sock, (struct sockaddr *)&me, &len) < 0) {
		perror("getsockname");
		return;
	}
	len = sizeof you;
	if (getpeername(sock, (struct sockaddr *)&you, &len) < 0) {
		perror("getpeername");
		return;
	}

	cc = read(sock, &msg, sizeof msg);
	if (cc < sizeof msg.type) {
		if (cc < 0)
			perror("request message failed");
		else
			error("request message too small");
		return;
	}
	switch (ntohl(msg.type)) {
	case MS_MSGTYPE_GETREQUEST:
		if (cc < sizeof msg.type + sizeof msg.body.getrequest)
			error("request message too small");
		else
			handle_get(sock, &me, &you, &msg);
		break;
	default:
		error("unreconized message type %d", ntohl(msg.type));
		break;
	}
}

static void
usage(void)
{
	fprintf(stderr, "mfrisbeed [-d]\n");
	exit(-1);
}

static void
get_options(int argc, char **argv)
{
	int ch;

	while ((ch = getopt(argc, argv, "Ddh")) != -1)
		switch(ch) {
		case 'd':
			daemonize = 0;
			debug = 1;
			break;
		case 'D':
			dumpconfig = 1;
			break;
		case 'h':
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc) {
		fprintf(stderr,
			"Error: Unrecognized command line arguments: %s ...\n",
			argv[0]);
		usage();
	}
}

/*
 * Create socket on specified port.
 */
static int
makesocket(int portnum, int *tcpsockp)
{
	struct sockaddr_in	name;
	int			i, tcpsock;
	socklen_t		length;

	/*
	 * Setup TCP socket for incoming connections.
	 */

	/* Create socket from which to read. */
	tcpsock = socket(AF_INET, SOCK_STREAM, 0);
	if (tcpsock < 0) {
		pfatal("opening stream socket");
	}
	fcntl(tcpsock, F_SETFD, FD_CLOEXEC);

	i = 1;
	if (setsockopt(tcpsock, SOL_SOCKET, SO_REUSEADDR,
		       (char *)&i, sizeof(i)) < 0)
		pwarning("setsockopt(SO_REUSEADDR)");;
	
	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons((u_short) portnum);
	if (bind(tcpsock, (struct sockaddr *) &name, sizeof(name))) {
		pfatal("binding stream socket");
	}
	/* Find assigned port value and print it out. */
	length = sizeof(name);
	if (getsockname(tcpsock, (struct sockaddr *) &name, &length)) {
		pfatal("getsockname");
	}
	if (listen(tcpsock, 128) < 0) {
		pfatal("listen");
	}
	*tcpsockp = tcpsock;
	log("listening on TCP port %d", ntohs(name.sin_port));
	
	return 0;
}

static struct childinfo *children;

static struct childinfo *
findchild(struct config_host_authinfo *ai, int methods)
{
	struct childinfo *ci;

	assert(ai->numimages == 1 && ai->imageinfo != NULL);
	for (ci = children; ci != NULL; ci = ci->next)
		if (!strcmp(ci->ai->imageinfo[0].imageid,
			    ai->imageinfo[0].imageid) &&
		    (ci->method & methods) != 0)
			return ci;

	return NULL;
}

/*
 * Fire off a frisbeed process to handle an image.
 */
static struct childinfo *
startchild(struct config_host_authinfo *ai, struct in_addr *sip,
	   struct in_addr *cip, int methods, int *errorp)
{
	struct childinfo *ci;

	assert(findchild(ai, methods) == NULL);

	ci = calloc(1, sizeof(struct childinfo));
	if (ci == NULL) {
		if (errorp)
			*errorp = CONFIG_ERR_HA_FAILED;
		return NULL;
	}

	/*
	 * First find the appropriate multicast address and port to use.
	 */
	if (config_get_server_address(ai, methods,
				      &ci->addr, &ci->port, &ci->method)) {
		free(ci);
		if (errorp)
			*errorp = CONFIG_ERR_HA_FAILED;
		return NULL;
	}
	ci->ai = ai;

	/*
	 * Fork off the process
	 */
	ci->pid = fork();
	if (ci->pid < 0) {
		pwarning("startchild");
		free(ci);
		if (errorp)
			*errorp = CONFIG_ERR_HA_FAILED;
		return NULL;
	}
	if (ci->pid == 0) {
		static char argbuf[1024];
		char *argv[64], **ap, *args;
		int argc;
		struct in_addr in;
		char *serverip;
		char *pname = "/usr/testbed/sbin/frisbeed";

		serverip = strdup(inet_ntoa(*sip));
		in.s_addr = htonl(ci->addr);
		snprintf(argbuf, sizeof argbuf,
			 "%s -i %s %s -m %s -p %d %s",
			 pname, serverip, ci->ai->imageinfo[0].get_options,
			 inet_ntoa(in), ci->port, ci->ai->imageinfo[0].path);
		if (debug)
			info("execing: %s", argbuf);

		args = argbuf;
		argc = 0;
		for (ap = argv; (*ap = strsep(&args, " \t")) != 0; ) {
			if (**ap != '\0') {
				argc++;
				if (++ap >= &argv[64])
					break;
			}
		}
		argv[argc] = NULL;

		/* close descriptors, etc. */
		for (argc = getdtablesize() - 1; argc >= 3; argc--)
			close(argc);

		execv(pname, argv);
		exit(-1);
	}

	ci->next = children;
	children = ci;

	if (errorp)
		*errorp = 0;
	return ci;
}

int
killchild(struct childinfo *ci)
{
	return 0;
}

static int
reapchildren(void)
{
	int pid, status;
	struct childinfo **cip, *ci;

	while (1) {
		struct in_addr in;

		pid = waitpid(0, &status, WNOHANG);
		if (pid <= 0)
			return 0;
		for (cip = &children; *cip != NULL; cip = &(*cip)->next) {
			if ((*cip)->pid == pid)
				break;
		}
		ci = *cip;
		if (ci == NULL) {
			error("Child died that was not ours!?");
			continue;
		}
		*cip = ci->next;
		in.s_addr = htonl(ci->addr);
		log("%s: server process %d on %s:%d exited (status=0x%x)",
		    ci->ai->imageinfo[0].imageid, pid, inet_ntoa(in), ci->port,
		    status);
		/*
		 * XXX if it exits non-zero, we should try to restart?
		 */
	}
}

static void
childdeath(int sig)
{
	if (debug)
		info("got SIGCHLD");
	siglongjmp(env, 1);
}

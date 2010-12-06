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
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include "decls.h"
#include "utils.h"
#include "configdefs.h"

#define FRISBEE_SERVER	"/usr/testbed/sbin/frisbeed"
#define FRISBEE_CLIENT	"/usr/testbed/sbin/frisbee"

static void	get_options(int argc, char **argv);
static int	makesocket(int portnum, int *tcpsockp);
static void	handle_request(int sock);
static int	reapchildren(void);

static int	daemonize = 1;
int		debug = 0;
static int	dumpconfig = 0;
static int	fetchfromabove = 0;
static int	canredirect = 0;
static int	onlymethods = (MS_METHOD_UNICAST|MS_METHOD_MULTICAST);
static struct in_addr parentip;
static int	parentport = MS_PORTNUM;

/* XXX the following just keep network.c happy */
int		portnum = MS_PORTNUM;
struct in_addr	mcastaddr;
struct in_addr	mcastif;

int
main(int argc, char **argv)
{
	int			tcpsock;
	FILE			*fp;
	char			buf[BUFSIZ];
	char			*pidfile = (char *) NULL;
	struct timeval		tv;
	fd_set			ready;

	get_options(argc, argv);

	MasterServerLogInit();

	log("mfrisbee daemon starting, methods=%s (debug level %d)",
	    GetMSMethods(onlymethods), debug);
	config_init(1);

	/* Just dump the config to stdout in human readable form and exit. */
	if (dumpconfig) {
		config_dump(stdout);
		exit(0);
	}

	/*
	 * Create TCP server.
	 */
	if (makesocket(portnum, &tcpsock) < 0) {
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
	FD_ZERO(&ready);
	while (1) {
		struct sockaddr_in client;
		socklen_t length;
		int newsock, rv;

		FD_SET(tcpsock, &ready);
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		rv = select(tcpsock+1, &ready, NULL, NULL, &tv);
		if (rv < 0) {
			if (errno == EINTR)
				continue;
			pfatal("select failed");
		}
		if (rv) {
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
		}
		reapchildren();
	}
	close(tcpsock);
	log("daemon terminating");
	exit(0);
}

struct childinfo {
	struct childinfo *next;
	struct config_imageinfo *imageinfo;
	int ptype;
	int method;
	int pid;
	in_addr_t servaddr;	/* -S arg */
	in_addr_t ifaceaddr;	/* -i arg */
	in_addr_t addr;		/* -m arg */
	in_port_t port;		/* -p arg */
	void (*done)(struct childinfo *, int);
	void *extra;
};
#define PTYPE_CLIENT	1
#define PTYPE_SERVER	2

static struct childinfo *findchild(struct config_imageinfo *, int, int);
static int startchild(struct childinfo *);
static struct childinfo *startserver(struct config_imageinfo *,
				     in_addr_t, in_addr_t, int, int *);
static struct childinfo *startclient(struct config_imageinfo *,
				     in_addr_t, in_addr_t, in_addr_t,
				     in_port_t, int, int *);

static void
free_imageinfo(struct config_imageinfo *ii)
{
	if (ii) {
		if (ii->imageid)
			free(ii->imageid);
		if (ii->path)
			free(ii->path);
		if (ii->sig)
			free(ii->sig);
		if (ii->get_options)
			free(ii->get_options);
		free(ii);
	}
}

/*
 * (Deep) copy an imageinfo structure.
 * Returns pointer or null on error.
 */
static struct config_imageinfo *
copy_imageinfo(struct config_imageinfo *ii)
{
	struct config_imageinfo *nii;

	if ((nii = calloc(1, sizeof *nii)) == NULL)
		goto fail;
	if (ii->imageid && (nii->imageid = strdup(ii->imageid)) == NULL)
		goto fail;
	if (ii->path && (nii->path = strdup(ii->path)) == NULL)
		goto fail;
	if (ii->sig && (nii->sig = strdup(ii->sig)) == NULL)
		goto fail;
	nii->flags = ii->flags;
	if (ii->get_options &&
	    (nii->get_options = strdup(ii->get_options)) == NULL)
		goto fail;
	nii->get_methods = ii->get_methods;
	/* XXX don't care about put_options and extra right now */
	return nii;

 fail:
	free_imageinfo(nii);
	return NULL;
}

/*
 * Attempt to fetch an image from our parent by firing off a frisbee.
 * Or, hook the child up directly with our parent.
 * Or, both.
 */
int
fetch_parent(struct in_addr *myip, struct in_addr *pip, in_port_t pport,
	     struct config_imageinfo *ii, int *methodp,
	     in_addr_t *servaddrp, in_addr_t *addrp, in_port_t *portp)
{
	struct childinfo *ci;
	struct in_addr pif;
	GetReply reply;
	int rv;

	/*
	 * See if a fetch is already in progress.
	 * If so we will either return "try again later" or point them to
	 * our parent.
	 */
	ci = findchild(ii, PTYPE_CLIENT, ii->get_methods);
	if (ci != NULL) {
		/*
		 * XXX right now frisbeed doesn't support mutiple clients
		 * on the same unicast address.  We could do multiple servers
		 * for the same image, but we don't.
		 */
		if (ci->method == MS_METHOD_UNICAST)
			return MS_ERROR_TRYAGAIN;
		goto done;
	}

	/*
	 * Send our parent a GET request and see what it says.
	 */
	if (!ClientNetFindServer(ntohl(pip->s_addr), pport,
				 ii->imageid, ii->get_methods, 0, 5,
				 &reply, &pif))
		return MS_ERROR_NOIMAGE;
	if (reply.error)
		return reply.error;

	/*
	 * Parent has started up a frisbeed, spawn a frisbee to capture it.
	 *
	 * Note that servaddr from the reply might not be our parent (pip),
	 * if our parent in turn had to ask its parent and redirect was in
	 * effect.  Unfortunately, in this case, 'pif' might not be correct
	 * since it is always the interface from which our parent responded.
	 * It will work as long as our parent's ancestor(s) reach us via
	 * the same interface.
	 */
	ci = startclient(ii, ntohl(pif.s_addr), reply.servaddr,
			 reply.addr, reply.port, reply.method, &rv);
	if (ci == NULL)
		return rv;

 done:
	if (!canredirect)
		return MS_ERROR_TRYAGAIN;

	*methodp = ci->method;
	*servaddrp = ci->servaddr;
	*addrp = ci->addr;
	*portp = ci->port;
	return 0;
}

void
handle_get(int sock, struct sockaddr_in *sip, struct sockaddr_in *cip,
	   MasterMsg_t *msg)
{
	char imageid[MS_MAXIDLEN+1];
	char clientip[sizeof("XXX.XXX.XXX.XXX")+1];
	int len = ntohs(msg->body.getrequest.idlen);
	struct config_host_authinfo *ai;
	struct config_imageinfo *ii;
	struct childinfo *ci;
	struct stat sb;
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
	 * If they request a method we don't support, reject them before
	 * we do any other work.
	 */
	methods &= onlymethods;
	if (methods == 0)
		goto badmethod;

	/*
	 * See if node has access to the image.
	 * If not, return an error code immediately.
	 */
	rv = config_auth_by_IP(&cip->sin_addr, imageid, &ai);
	if (rv) {
		warning("%s: client %s %s failed: %s",
			imageid, clientip, op, GetMSError(rv));
		msg->body.getreply.error = rv;
		goto reply;
	}
	if (debug > 1)
		config_dump_host_authinfo(ai);
	if (ai->numimages > 1) {
		rv = MS_ERROR_INVALID;
		warning("%s: client %s %s failed: "
			"lookup returned multiple (%d) images",
			imageid, clientip, op, ai->numimages);
		msg->body.getreply.error = rv;
		goto reply;

	}
	ii = &ai->imageinfo[0];
	assert((ii->flags & CONFIG_PATH_ISFILE) != 0);

	/*
	 * See if image actually exists.
	 *
	 * If the file exists but is not a regular file, we return an error.
	 *
	 * If the file does not exist (it is possible to request an
	 * image that doesn't exist if the authentication check allows
	 * access to the containing directory), then for now we just return
	 * an error, but in the future, we could request the image from
	 * our parent mserver.
	 */
	if (stat(ii->path, &sb) == 0) {
		if (!S_ISREG(sb.st_mode)) {
			rv = MS_ERROR_INVALID;
			warning("%s: client %s %s failed: "
				"not a regular file",
				imageid, clientip, op);
			msg->body.getreply.error = rv;
			goto reply;
		}
	} else {
		if (errno == ENOENT) {
			if (!wantstatus && fetchfromabove) {
				int m;
				in_addr_t s, a;
				in_port_t p;

				rv = fetch_parent(&sip->sin_addr, &parentip,
						  parentport, ii,
						  &m, &s, &a, &p);
				if (rv == 0) {
					msg->body.getreply.method = m;
					msg->body.getreply.isrunning = 1;
					msg->body.getreply.servaddr = htonl(s);
					msg->body.getreply.addr = htonl(a);
					msg->body.getreply.port = htons(p);
					goto reply;
				}
			} else
				rv = MS_ERROR_NOIMAGE;
		} else {
			rv = MS_ERROR_INVALID;
		}
		warning("%s: client %s %s failed: %s",
			imageid, clientip, op, GetMSError(rv));
		msg->body.getreply.error = rv;
		goto reply;
	}

	/*
	 * Figure out what transfer method to use.
	 */
	if (debug)
		info("  request methods: 0x%x, image methods: 0x%x",
		     methods, ii->get_methods);
	methods &= ii->get_methods;
	if (methods == 0) {
	badmethod:
		rv = MS_ERROR_NOMETHOD;
		warning("%s: client %s %s failed: %s",
			imageid, clientip, op, GetMSError(rv));
		msg->body.getreply.error = rv;
		goto reply;
	}

	/*
	 * Otherwise see if there is a frisbeed already running, starting
	 * one if not.  Then construct a reply with the available info.
	 */
	ci = findchild(ii, PTYPE_SERVER, methods);

	/*
	 * XXX right now frisbeed doesn't support mutiple clients
	 * on the same unicast address.  We could do multiple servers
	 * for the same image, but we don't.
	 */
	if (ci != NULL && ci->method == MS_METHOD_UNICAST) {
		warning("%s: client %s %s failed: "
			"unicast server already running",
			imageid, clientip, op);
		msg->body.getreply.error = MS_ERROR_TRYAGAIN;
		goto reply;
	}

	if (ci == NULL) {
		struct in_addr in;

		if (wantstatus) {
			if (ii->sig != NULL) {
				int32_t mt = *(int32_t *)ii->sig;
				assert((ii->flags & CONFIG_SIG_ISMTIME) != 0);

				msg->body.getreply.sigtype =
					htons(MS_SIGTYPE_MTIME);
				*(int32_t *)msg->body.getreply.signature =
					htonl(mt);
			}
			config_free_host_authinfo(ai);
			msg->body.getreply.method = methods;
			msg->body.getreply.isrunning = 0;
			log("%s: STATUS is not running", imageid);
			goto reply;
		}
		ci = startserver(ii, ntohl(sip->sin_addr.s_addr),
				 ntohl(cip->sin_addr.s_addr),
				 methods, &rv);
		if (ci == NULL) {
			config_free_host_authinfo(ai);
			msg->body.getreply.error = rv;
			goto reply;
		}
		in.s_addr = htonl(ci->addr);
		log("%s: started %s server on %s:%d (pid %d)",
		    imageid, GetMSMethods(ci->method),
		    inet_ntoa(in), ci->port, ci->pid);
	} else {
		struct in_addr in;

		if (wantstatus && ii->sig != NULL) {
			int32_t mt = *(int32_t *)ii->sig;
			assert((ii->flags & CONFIG_SIG_ISMTIME) != 0);

			msg->body.getreply.sigtype = htons(MS_SIGTYPE_MTIME);
			*(int32_t *)msg->body.getreply.signature = htonl(mt);
		}
		config_free_host_authinfo(ai);
		in.s_addr = htonl(ci->addr);
		if (wantstatus)
			log("%s: STATUS is running %s on %s:%d (pid %d)",
			    imageid, GetMSMethods(ci->method),
			    inet_ntoa(in), ci->port, ci->pid);
		else
			log("%s: %s server already running on %s:%d (pid %d)",
			    imageid, GetMSMethods(ci->method),
			    inet_ntoa(in), ci->port, ci->pid);
	}

	msg->body.getreply.method = ci->method;
	msg->body.getreply.isrunning = 1;
	msg->body.getreply.servaddr = htonl(ci->servaddr);
	/*
	 * XXX tmp hack.  Currently, if we are unicasting, startserver
	 * returns the addr field set to the address of the client
	 * (see the XXX there for details).  However, we need to return our
	 * address to the client.
	 *
	 * When frisbeed is changed to support more than one unicast client,
	 * this will change.
	 */
	if (ci->method == MS_METHOD_UNICAST)
		msg->body.getreply.addr = msg->body.getreply.servaddr;
	else
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

	while ((ch = getopt(argc, argv, "C:DRS:P:p:dh")) != -1)
		switch(ch) {
		case 'C':
		{
			char *ostr, *str, *cp;
			int nm = 0;

			str = ostr = strdup(optarg);
			while ((cp = strsep(&str, ",")) != NULL) {
				if (strcmp(cp, "ucast") == 0)
					nm |= MS_METHOD_UNICAST;
				else if (strcmp(cp, "mcast") == 0)
					nm |= MS_METHOD_MULTICAST;
				else if (strcmp(cp, "bcast") == 0)
					nm |= MS_METHOD_BROADCAST;
				else if (strcmp(cp, "any") == 0)
					nm = MS_METHOD_ANY;
			}
			free(ostr);
			if (nm == 0) {
				fprintf(stderr,
					"-C should specify one or more of: "
					"'ucast', 'mcast', 'bcast', 'any'\n");
				exit(1);
			}
			onlymethods = nm;
			break;
		}
		case 'd':
			daemonize = 0;
			debug++;
			break;
		case 'D':
			dumpconfig = 1;
			break;
		case 'R':
			canredirect = 1;
			break;
		case 'S':
			if (!inet_aton(optarg, &parentip)) {
				fprintf(stderr, "Invalid server IP `%s'\n",
					optarg);
				exit(1);
			}
			fetchfromabove = 1;
			break;
		case 'P':
			parentport = atoi(optarg);
			break;
		case 'p':
			portnum = atoi(optarg);
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
makesocket(int port, int *tcpsockp)
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
	name.sin_port = htons((u_short) port);
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
static int nchildren;

static struct childinfo *
findchild(struct config_imageinfo *ii, int ptype, int methods)
{
	struct childinfo *ci, *bestci;
	assert((methods & ~onlymethods) == 0);

	bestci = NULL;
	for (ci = children; ci != NULL; ci = ci->next)
		if (ci->ptype == ptype && (ci->method & methods) != 0 &&
		    !strcmp(ci->imageinfo->imageid, ii->imageid)) {
			if (bestci == NULL)
				bestci = ci;
			else if (ci->method == MS_METHOD_BROADCAST)
				bestci = ci;
			else if (ci->method == MS_METHOD_MULTICAST &&
				 bestci->method == MS_METHOD_UNICAST)
				bestci = ci;
			else
				pfatal("multiple unicast servers for %s",
				       ii->imageid);
		}

	return bestci;
}

/*
 * Fire off a frisbee server or client process to serve or download an image.
 */
static int
startchild(struct childinfo *ci)
{
	/*
	 * Fork off the process
	 */
	ci->pid = fork();
	if (ci->pid < 0) {
		pwarning("startchild");
		return MS_ERROR_FAILED;
	}
	if (ci->pid == 0) {
		static char argbuf[1024];
		char ifacestr[sizeof("XXX.XXX.XXX.XXX")+1];
		char servstr[sizeof("XXX.XXX.XXX.XXX")+1];
		char *argv[64], **ap, *args;
		int argc;
		struct in_addr in;
		char *pname;

		pname = (ci->ptype == PTYPE_SERVER) ?
			FRISBEE_SERVER : FRISBEE_CLIENT;
		in.s_addr = htonl(ci->ifaceaddr);
		strncpy(ifacestr, inet_ntoa(in), sizeof ifacestr);
		in.s_addr = htonl(ci->servaddr);
		strncpy(servstr, inet_ntoa(in), sizeof servstr);
		in.s_addr = htonl(ci->addr);
		if (ci->ptype == PTYPE_SERVER)
			snprintf(argbuf, sizeof argbuf,
				 "%s -i %s %s -m %s -p %d %s",
				 pname, ifacestr,
				 ci->imageinfo->get_options,
				 inet_ntoa(in), ci->port, ci->imageinfo->path);
		else
			snprintf(argbuf, sizeof argbuf,
				 "%s -N -S %s -i %s %s -m %s -p %d %s",
				 pname, servstr, ifacestr,
				 debug > 1 ? "" : "-q",
				 inet_ntoa(in), ci->port, ci->imageinfo->path);
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
	nchildren++;

	return 0;
}

static struct childinfo *
startserver(struct config_imageinfo *ii, in_addr_t meaddr, in_addr_t youaddr,
	    int methods, int *errorp)
{
	struct childinfo *ci;

	assert(findchild(ii, PTYPE_SERVER, methods) == NULL);
	assert(errorp != NULL);

	ci = calloc(1, sizeof(struct childinfo));
	if (ci == NULL) {
		*errorp = MS_ERROR_FAILED;
		return NULL;
	}

	/*
	 * Find the appropriate address, port and method to use.
	 */
	if (config_get_server_address(ii, methods, 1, &ci->addr,
				      &ci->port, &ci->method)) {
		free(ci);
		*errorp = MS_ERROR_FAILED;
		return NULL;
	}

	/*
	 * XXX tmp hack.  Currently, if we are unicasting, the addr field
	 * (-m arg of frisbeed) needs to be the address of the client.
	 * When frisbeed is changed to support more than one unicast client,
	 * this will change.
	 */
	if (ci->method == MS_METHOD_UNICAST)
		ci->addr = youaddr;

	ci->servaddr = meaddr;
	ci->ifaceaddr = meaddr;
	ci->imageinfo = copy_imageinfo(ii);
	ci->ptype = PTYPE_SERVER;
	if ((*errorp = startchild(ci)) != 0) {
		free_imageinfo(ci->imageinfo);
		free(ci);
		return NULL;
	}

	return ci;
}

static void
finishclient(struct childinfo *ci, int status)
{
	char *bakname, *tmpname, *realname;
	int len, didbackup;

	assert(ci->extra != NULL);
	realname = ci->extra;
	ci->extra = NULL;
	tmpname = ci->imageinfo->path;
	ci->imageinfo->path = realname;

	if (status != 0) {
		error("%s: download failed, removing tmpfile %s",
		      realname, tmpname);
		unlink(tmpname);
		return;
	}

	len = strlen(realname) + 5;
	bakname = malloc(len);
	snprintf(bakname, len, "%s.bak", realname);
	didbackup = 1;
	if (rename(realname, bakname) < 0)
		didbackup = 0;
	if (rename(tmpname, realname) < 0) {
		error("%s: failed to install new version, leaving as %s",
		      realname, tmpname);
		if (didbackup)
			rename(bakname, realname);
	}
	free(tmpname);
	free(bakname);
	log("%s: download complete", realname);
}

static struct childinfo *
startclient(struct config_imageinfo *ii, in_addr_t meaddr, in_addr_t youaddr,
	    in_addr_t addr, in_port_t port, int methods, int *errorp)
{
	struct childinfo *ci;
	char *tmpname;
	int len;

	assert(findchild(ii, PTYPE_CLIENT, methods) == NULL);
	assert(errorp != NULL);

	ci = calloc(1, sizeof(struct childinfo));
	if (ci == NULL) {
		*errorp = MS_ERROR_FAILED;
		return NULL;
	}
	ci->servaddr = youaddr;
	ci->ifaceaddr = meaddr;
	ci->addr = addr;
	ci->port = port;
	ci->method = methods;
	ci->imageinfo = copy_imageinfo(ii);
	ci->ptype = PTYPE_CLIENT;

	/*
	 * Arrange to download the image as <path>.tmp and then
	 * rename it into place when done.
	 */
	len = strlen(ci->imageinfo->path) + 5;
	if ((tmpname = malloc(len)) != NULL) {
		ci->extra = ci->imageinfo->path;
		snprintf(tmpname, len, "%s.tmp", ci->imageinfo->path);
		ci->imageinfo->path = tmpname;
		ci->done = finishclient;
	}
	if ((*errorp = startchild(ci)) != 0) {
		if (ci->extra)
			free(ci->extra);
		free_imageinfo(ci->imageinfo);
		free(ci);
		return NULL;
	}

	return ci;
}

int
killchild(struct childinfo *ci)
{
	return 0;
}

/*
 * Cleanup zombies.
 */
static int
reapchildren(void)
{
	int pid, status;
	struct childinfo **cip, *ci;

	if (nchildren == 0) {
		assert(children == NULL);
		return 0;
	}

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
		nchildren--;
		in.s_addr = htonl(ci->addr);
		log("%s: %s process %d on %s:%d exited (status=0x%x)",
		    ci->imageinfo->imageid,
		    ci->ptype == PTYPE_SERVER ? "server" : "client",
		    pid, inet_ntoa(in), ci->port, status);
		if (ci->done)
			ci->done(ci, status);
		if (ci->extra)
			free(ci->extra);
		free_imageinfo(ci->imageinfo);
		free(ci);
	}
}

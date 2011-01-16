/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2010-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Frisbee master server.  Listen for GET/PUT requests and fork off
 * handler processes.
 * 
 * TODO:
 * - timeouts for child frisbee processes
 * - record the state of running frisbeeds in persistant store so that
 *   they can be restarted if we die and restart
 * - related: make sure we don't leave orphans when we die!
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
#define FRISBEE_RETRIES	3

static void	get_options(int argc, char **argv);
static int	makesocket(int portnum, int *tcpsockp);
static void	handle_request(int sock);
static int	reapchildren(int apid, int *status);

static int	daemonize = 1;
int		debug = 0;
static int	dumpconfig = 0;
static int	onlymethods = (MS_METHOD_UNICAST|MS_METHOD_MULTICAST);
static int	parentmethods = (MS_METHOD_UNICAST|MS_METHOD_MULTICAST);

/*
 * For recursively GETing images:
 *   parentip/parentport  address of a parent master server (-S,-P)
 *   fetchfromabove       true if a parent has been specified
 *   canredirect          if true, redirect our clients to our parent (-R)
 *   usechildauth         if true, pass client's authinfo to our parent (-A)
 */
static struct in_addr parentip;
static int	parentport = MS_PORTNUM;
static int	fetchfromabove = 0;
static int	canredirect = 0;
static int	usechildauth = 0;
static int	mirrormode = 0;
static char     *configstyle = "null";

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
	if (fetchfromabove)
		log("  using parent %s:%d%s, methods=%s",
		    inet_ntoa(parentip), parentport,
		    mirrormode ? " in mirror mode" : "",
		    GetMSMethods(parentmethods));
	config_init(configstyle, 1);

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
		(void) reapchildren(0, NULL);
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
	int retries;
	in_addr_t servaddr;	/* -S arg */
	in_addr_t ifaceaddr;	/* -i arg */
	in_addr_t addr;		/* -m arg */
	in_port_t port;		/* -p arg */
	void (*done)(struct childinfo *, int);
	void *extra;
};
#define PTYPE_CLIENT	1
#define PTYPE_SERVER	2

struct clientextra {
	char *realname;
	/* info from our parent */
	uint16_t sigtype;
	uint8_t signature[MS_MAXSIGLEN];
	uint32_t hisize;
	uint32_t losize;
};

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
 * Fetch an image from our parent by getting image info from it and firing
 * off a frisbee.  If canredirect is non-zero, we also hook our child up
 * directly with our parent so that it can fetch in parallel.  If statusonly
 * is non-zero, we just request info from our parent and return that.
 * Returns zero if Reply struct contains the desired info, TRYAGAIN if we
 * have started up a frisbee to fetch from our parent, or an error otherwise.
 * Status is returned in host order.
 */
int
fetch_parent(struct in_addr *myip, struct in_addr *hostip,
	     struct in_addr *pip, in_port_t pport,
	     struct config_imageinfo *ii, int statusonly, GetReply *replyp)
{
	struct childinfo *ci;
	struct clientextra *ce;
	struct in_addr pif;
	in_addr_t authip;
	GetReply reply;
	int rv, methods;

	/*
	 * If usechildauth is set, we pass the child host IP to our parent
	 * for authentication, otherwise we use our own.
	 */
	authip = usechildauth ? ntohl(hostip->s_addr) : 0;

	/*
	 * Allowed image methods are constrained by any parent methods
	 */
	methods = ii->get_methods & parentmethods;
	if (methods == 0)
		return MS_ERROR_NOMETHOD;

	/*
	 * See if a fetch is already in progress.
	 * If so we will either return "try again later" or point them to
	 * our parent.
	 */
	ci = findchild(ii, PTYPE_CLIENT, methods);
	if (ci != NULL) {
		if (debug)
			info("%s: fetch from %s in progress",
			     ii->imageid, inet_ntoa(*pip));

		/*
		 * Since a download is in progress we don't normally need
		 * to revalidate since we wouldn't be downloading the image
		 * if we didn't have access.
		 *
		 * However, when acting for a child, we do need to validate
		 * every time since the child making the current request
		 * might not have the same access as the child we are
		 * currently downloading for.
		 */
		if (authip) {
			if (!ClientNetFindServer(ntohl(pip->s_addr),
						 pport, authip, ii->imageid,
						 methods, 1, 5,
						 &reply, &pif))
				return MS_ERROR_NOIMAGE;
			if (reply.error)
				return reply.error;
		}
		/*
		 * We return the info we got from our parent.
		 */
		assert(ci->extra != NULL);
		ce = ci->extra;

		memset(replyp, 0, sizeof *replyp);
		replyp->sigtype = ce->sigtype;
		memcpy(replyp->signature, ce->signature, MS_MAXSIGLEN);
		replyp->hisize = ce->hisize;
		replyp->losize = ce->losize;
		if (statusonly) {
			if (debug)
				info("%s: parent status: "
				     "sigtype=%d, sig=0x%08x..., size=%x/%x",
				     ii->imageid,
				     ce->sigtype, *(uint32_t *)ce->signature,
				     ce->hisize, ce->losize);
			return 0;
		}

		/*
		 * A real get request.
		 * We either tell them to try again later or redirect them
		 * to our parent.
		 */
		goto done;
	}

	if (debug)
		info("%s: requesting %simage from %s",
		     ii->imageid, (statusonly ? "status of ": ""),
		     inet_ntoa(*pip));

	/*
	 * Image fetch is not in progress.
	 * Send our parent a GET request and see what it says.
	 */
	if (!ClientNetFindServer(ntohl(pip->s_addr), pport, authip,
				 ii->imageid, methods, statusonly, 5,
				 &reply, &pif))
		return MS_ERROR_NOIMAGE;
	if (reply.error)
		return reply.error;

	/*
	 * Return the image size and signature from our parent
	 */
	memset(replyp, 0, sizeof *replyp);
	replyp->sigtype = reply.sigtype;
	memcpy(replyp->signature, reply.signature, MS_MAXSIGLEN);
	replyp->hisize = reply.hisize;
	replyp->losize = reply.losize;
	if (statusonly) {
		if (debug)
			info("%s: parent status: "
			     "sigtype=%d, sig=0x%08x..., size=%x/%x",
			     ii->imageid,
			     reply.sigtype, *(uint32_t *)reply.signature,
			     reply.hisize, reply.losize);
		return 0;
	}

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

	/*
	 * Cache the size/signature info for use in future calls when
	 * we are busy (the case above).
	 */
	assert(ci->extra != NULL);
	ce = ci->extra;
	ce->sigtype = replyp->sigtype;
	memcpy(ce->signature, replyp->signature, MS_MAXSIGLEN);
	ce->hisize = replyp->hisize;
	ce->losize = replyp->losize;
	if (debug)
		info("%s cache status: "
		     "sigtype=%d, sig=0x%08x..., size=%x/%x",
		     ii->imageid,
		     ce->sigtype, *(uint32_t *)ce->signature,
		     ce->hisize, ce->losize);

 done:
	if (!canredirect)
		return MS_ERROR_TRYAGAIN;

	/*
	 * XXX more unicast "fer now" hackary...if we are talking unicast with
	 * our parent, we cannot redirect the child there as well.
	 */
	if (ci->method == MS_METHOD_UNICAST)
		return MS_ERROR_TRYAGAIN;

	memset(replyp, 0, sizeof *replyp);
	replyp->method = ci->method;
	replyp->servaddr = ci->servaddr;
	replyp->addr = ci->addr;
	replyp->port = ci->port;
	return 0;
}

/*
 * Handle a GET request.
 *
 * This can be either a request to get the actual image (status==0)
 * or a request to get just the status of the image (status==1).
 *
 * A request for the actual image causes us to start up a frisbee server
 * for the image if one is not already running.  If we are configured with
 * a parent, we may need to first fetch the image from our parent with a
 * GET request.  In this case, we may return a TRYAGAIN status while the
 * image transfer is in progress. Alternatively, if we are configured to
 * allow redirection, we could just point our client to our parent and allow
 * them to download the image along side us.
 *
 * A request for status will not fire up a server, but may require a status
 * call on our parent.  A status call will never return TRYAGAIN, it will
 * always return the most recent status for an image or some other error.
 * If we have a parent, we will always get its status for the image and
 * return that if "newer" than ours. Thus status is a synchronous call that
 * will return the attributes of the image that will ultimately be transferred
 * to the client upon a GET request.
 */
void
handle_get(int sock, struct sockaddr_in *sip, struct sockaddr_in *cip,
	   MasterMsg_t *msg)
{
	struct in_addr host;
	char imageid[MS_MAXIDLEN+1];
	char clientip[sizeof("XXX.XXX.XXX.XXX")+1];
	int len;
	struct config_host_authinfo *ai;
	struct config_imageinfo *ii = NULL;
	struct childinfo *ci;
	struct stat sb;
	uint64_t isize;
	int rv, methods, wantstatus;
	int getfromparent;
	char *op;

	/*
	 * If an explicit host was listed, use that as the host we are
	 * authenticating, otherwise use the caller's IP.  config_auth_by_IP
	 * will reject the former if the caller is not allowed to proxy for
	 * the node in question.
	 */
	if (msg->body.getrequest.hostip)
		host.s_addr = msg->body.getrequest.hostip;
	else
		host.s_addr = cip->sin_addr.s_addr;

	len = ntohs(msg->body.getrequest.idlen);
	memcpy(imageid, msg->body.getrequest.imageid, len);
	imageid[len] = '\0';
	methods = msg->body.getrequest.methods;
	wantstatus = msg->body.getrequest.status;
	op = wantstatus ? "GETSTATUS" : "GET";

	strncpy(clientip, inet_ntoa(cip->sin_addr), sizeof clientip);
	if (host.s_addr != cip->sin_addr.s_addr)
		log("%s: %s from %s (for %s), methods: 0x%x",
		    imageid, op, clientip, inet_ntoa(host), methods);
	else
		log("%s: %s from %s, (methods: 0x%x)",
		    imageid, op, clientip, methods);

	memset(msg, 0, sizeof *msg);
	msg->hdr.type = htonl(MS_MSGTYPE_GETREPLY);
	strncpy((char *)msg->hdr.version, MS_MSGVERS_1,
		sizeof(msg->hdr.version));

	/*
	 * If they request a method we don't support, reject them before
	 * we do any other work.  XXX maybe the method should not matter
	 * for a status-only call?
	 */
	methods &= onlymethods;
	if (methods == 0)
		goto badmethod;

	/*
	 * In mirrormode, we first validate access with our parent
	 * before doing anything locally.
	 */
	if (mirrormode) {
		struct in_addr pif;
		GetReply reply;
		in_addr_t authip;

		authip = usechildauth ? ntohl(host.s_addr) : 0;

		if (!ClientNetFindServer(ntohl(parentip.s_addr),
					 parentport, authip, imageid,
					 methods, 1, 5,
					 &reply, &pif))
			reply.error = MS_ERROR_NOIMAGE;
		if (reply.error) {
			msg->body.getreply.error = reply.error;
			goto reply;
		}
	}

	/*
	 * See if node has access to the image.
	 * If not, return an error code immediately.
	 */
	rv = config_auth_by_IP(&cip->sin_addr, &host, imageid, &ai);
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
	ii = copy_imageinfo(&ai->imageinfo[0]);
	config_free_host_authinfo(ai);		
	assert((ii->flags & CONFIG_PATH_ISFILE) != 0);

	/*
	 * See if image actually exists.
	 *
	 * If the file exists but is not a regular file, we return an error.
	 *
	 * If the file does not exist (it is possible to request an image
	 * that doesn't exist if the authentication check allows access to
	 * the containing directory) and we have a parent, we request it
	 * from the parent.
	 */
	isize = 0;
	getfromparent = 0;
	if (stat(ii->path, &sb) == 0) {
		if (!S_ISREG(sb.st_mode)) {
			rv = MS_ERROR_INVALID;
			warning("%s: client %s %s failed: "
				"not a regular file",
				imageid, clientip, op);
			msg->body.getreply.error = rv;
			goto reply;
		}
		isize = sb.st_size;

		/*
		 * If the file exists and we have a parent, get the signature
		 * from the parent and see if we need to update our copy.
		 * We only do this for mirror mode.
		 */
		if (mirrormode) {
			GetReply r;

			log("%s: have local copy, GETSTATUS from parent",
			    imageid, op);

			rv = fetch_parent(&sip->sin_addr, &host,
					  &parentip, parentport, ii, 1, &r);
			/*
			 * If our parent returns failure for any reason
			 * and we are mirroring, then we propogate the error
			 * to our client.  Otherwise, we use the local copy
			 * under the assumption that this is a strictly
			 * local image and any overlap with the parent is
			 * strictly coincidental.
			 *
			 * XXX this latter assumption is dubious; may need
			 * to revisit or make the behavior explicit.
			 */
			if (rv) {
				log("%s: failed getting parent status: %s, %s",
				    imageid, GetMSError(rv),
				    (mirrormode ?
				     "failing" : "using local copy"));
				if (mirrormode) {
					msg->body.getreply.error = rv;
					goto reply;
				}
			}
			/*
			 * Signature is out of date, fetch from our parent.
			 * Return the attributes we got via the check above.
			 *
			 * XXX need checks for other signature types.
			 */
			if (rv == 0 &&
			    (r.sigtype == MS_SIGTYPE_MTIME &&
			     *(time_t *)r.signature > sb.st_mtime)) {
				msg->body.getreply.sigtype = htons(r.sigtype);
				if (r.sigtype == MS_SIGTYPE_MTIME) {
					uint32_t mt;
					mt = *(uint32_t *)r.signature;
					*(uint32_t *)r.signature = htonl(mt);
				}
				memcpy(msg->body.getreply.signature,
				       r.signature, MS_MAXSIGLEN);
				msg->body.getreply.hisize = htonl(r.hisize);
				msg->body.getreply.losize = htonl(r.losize);

				if (wantstatus)
					goto reply;

				log("%s: local copy out of date, "
				    "GET from parent", imageid);
				getfromparent = 1;
			}
		}
	} else if (fetchfromabove) {
		GetReply r;

		log("%s: no local copy, %s from parent", imageid, op);

		/*
		 * We don't have the image, but we have a parent,
		 * request the status as we did above. Any error is reflected
		 * to our caller.
		 */
		rv = fetch_parent(&sip->sin_addr, &host,
				  &parentip, parentport, ii, 1, &r);
		if (rv) {
			log("%s: failed getting parent status: %s, failing",
			    imageid, GetMSError(rv));
			msg->body.getreply.error = rv;
			goto reply;
		}
		/*
		 * And we must always fetch from the parent.
		 * Return the attributes we got via the check above.
		 */
		msg->body.getreply.sigtype = htons(r.sigtype);
		if (r.sigtype == MS_SIGTYPE_MTIME) {
			uint32_t mt;
			mt = *(uint32_t *)r.signature;
			*(uint32_t *)r.signature = htonl(mt);
		}
		memcpy(msg->body.getreply.signature, r.signature,
		       MS_MAXSIGLEN);
		msg->body.getreply.hisize = htonl(r.hisize);
		msg->body.getreply.losize = htonl(r.losize);

		if (wantstatus)
			goto reply;
		getfromparent = 1;
	} else {
		/*
		 * No image and no parent, just flat fail.
		 */
		rv = (errno == ENOENT) ? MS_ERROR_NOIMAGE : MS_ERROR_INVALID;
		warning("%s: client %s %s failed: %s",
			imageid, clientip, op, GetMSError(rv));
		msg->body.getreply.error = rv;
		goto reply;
	}

	/*
	 * Either we did not have the image or our copy is out of date,
	 * attempt to fetch from our parent.
	 */
	if (getfromparent) {
		GetReply r;

		rv = fetch_parent(&sip->sin_addr, &host,
				  &parentip, parentport, ii, 0, &r);
		/*
		 * Redirecting to parent.
		 * Can only do this if our parents method is compatible
		 * with the client's request.
		 */
		if (rv == 0) {
			if ((r.method & methods) != 0) {
				msg->body.getreply.method = r.method;
				msg->body.getreply.isrunning = 1;
				msg->body.getreply.servaddr =
					htonl(r.servaddr);
				msg->body.getreply.addr =
					htonl(r.addr);
				msg->body.getreply.port =
					htons(r.port);
				log("%s: redirecting %s to our parent",
				    imageid, clientip);
				goto reply;
			}
			log("%s: cannot redirect %s to parent; "
			    "incompatible transfer methods",
			    imageid, clientip);
			rv = MS_ERROR_TRYAGAIN;
		}
		/*
		 * If parent callout failed, but we have a copy
		 * use our stale version.
		 */
		if (rv != MS_ERROR_TRYAGAIN && isize > 0) {
			warning("%s: client %s %s from parent failed: %s, "
				"using our stale copy",
				imageid, clientip, op, GetMSError(rv));
		}
		/*
		 * Otherwise, we are busy fetching the new copy (TRYAGAIN),
		 * or we had a real failure and we don't have a copy.
		 */
		else {
			warning("%s: client %s %s from parent failed: %s",
				imageid, clientip, op, GetMSError(rv));
			msg->body.getreply.error = rv;
			goto reply;
		}
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
	if (!wantstatus && ci != NULL && ci->method == MS_METHOD_UNICAST) {
		warning("%s: client %s %s failed: "
			"unicast server already running",
			imageid, clientip, op);
		msg->body.getreply.error = MS_ERROR_TRYAGAIN;
		goto reply;
	}

	if (ci == NULL) {
		struct in_addr in;
		int stat;

		if (ii->sig != NULL) {
			int32_t mt = *(int32_t *)ii->sig;
			assert((ii->flags & CONFIG_SIG_ISMTIME) != 0);

			msg->body.getreply.sigtype = htons(MS_SIGTYPE_MTIME);
			*(int32_t *)msg->body.getreply.signature = htonl(mt);
		}
		if (wantstatus) {
			msg->body.getreply.method = methods;
			msg->body.getreply.isrunning = 0;
			msg->body.getreply.hisize = htonl(isize >> 32);
			msg->body.getreply.losize = htonl(isize);
			log("%s: STATUS is not running", imageid);
			goto reply;
		}
		ci = startserver(ii, ntohl(sip->sin_addr.s_addr),
				 ntohl(cip->sin_addr.s_addr),
				 methods, &rv);
		if (ci == NULL) {
			msg->body.getreply.error = rv;
			goto reply;
		}

		in.s_addr = htonl(ci->addr);
		log("%s: started %s server on %s:%d (pid %d)",
		    imageid, GetMSMethods(ci->method),
		    inet_ntoa(in), ci->port, ci->pid);

		/*
		 * Watch for an immediate death so we don't tell our client
		 * a server is running when it really isn't.
		 *
		 * XXX what is the right response? Right now we just tell
		 * them to try again and hope the problem is transient.
		 */
		sleep(2);
		if (reapchildren(ci->pid, &stat)) {
			msg->body.getreply.error = MS_ERROR_TRYAGAIN;
			log("%s: server immediately exited (stat=%d), ",
			    "telling client to try again!",
			    imageid, stat);
			goto reply;
		}
	} else {
		struct in_addr in;

		if (ii->sig != NULL) {
			int32_t mt = *(int32_t *)ii->sig;
			assert((ii->flags & CONFIG_SIG_ISMTIME) != 0);

			msg->body.getreply.sigtype = htons(MS_SIGTYPE_MTIME);
			*(int32_t *)msg->body.getreply.signature = htonl(mt);
		}
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

	msg->body.getreply.hisize = htonl(isize >> 32);
	msg->body.getreply.losize = htonl(isize);
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
	if (debug) {
		info("%s reply: sigtype=%d, sig=0x%08x..., size=%x/%x",
		     op, ntohs(msg->body.getreply.sigtype),
		     ntohl(*(uint32_t *)msg->body.getreply.signature),
		     ntohl(msg->body.getreply.hisize),
		     ntohl(msg->body.getreply.losize));
	}
	len = sizeof msg->hdr + sizeof msg->body.getreply;
	if (!MsgSend(sock, msg, len, 10))
		error("%s: could not send reply", inet_ntoa(cip->sin_addr));
	if (ii)
		free_imageinfo(ii);
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
	if (cc < sizeof msg.hdr) {
		if (cc < 0)
			perror("request message failed");
		else
			error("request message too small");
		return;
	}
	switch (ntohl(msg.hdr.type)) {
	case MS_MSGTYPE_GETREQUEST:
		if (cc < sizeof msg.hdr + sizeof msg.body.getrequest)
			error("request message too small");
		else
			handle_get(sock, &me, &you, &msg);
		break;
	default:
		error("unreconized message type %d", ntohl(msg.hdr.type));
		break;
	}
}

static void
usage(void)
{
	fprintf(stderr, "mfrisbeed [-ADRd] [-X method] [-I imagedir] [-S parentIP] [-P parentport] [-p port]\n");
	fprintf(stderr, "Basic:\n");
	fprintf(stderr, "  -C <style>  configuration style: emulab, file, or null\n");
	fprintf(stderr, "  -I <dir>    default directory where images are stored\n");
	fprintf(stderr, "  -x <methods> transfer methods to allow from clients: ucast, mcast, bcast or any\n");
	fprintf(stderr, "  -X <method> transfer method to request from parent\n");
	fprintf(stderr, "  -p <port>   port to listen on\n");
	fprintf(stderr, "Debug:\n");
	fprintf(stderr, "  -d          debug mode; does not daemonize\n");
	fprintf(stderr, "  -D          dump configuration and exit\n");
	fprintf(stderr, "Proxying:\n");
	fprintf(stderr, "  -S <parent> parent name or IP\n");
	fprintf(stderr, "  -P <pport>  parent port to contact\n");
	fprintf(stderr, "  -A          pass on authentication info from our child to parent\n");
	fprintf(stderr, "  -M          act as a strict mirror for our parent\n");
	fprintf(stderr, "  -R          redirect child to parent if local image not available\n");
	exit(-1);
}

static void
get_options(int argc, char **argv)
{
	int ch;

	while ((ch = getopt(argc, argv, "AC:DI:MRX:S:P:p:dh")) != -1)
		switch(ch) {
		case 'A':
			usechildauth = 1;
			break;
		case 'C':
			if (strcmp(optarg, "emulab") == 0 ||
			    strcmp(optarg, "file") == 0 ||
			    strcmp(optarg, "null") == 0)
				configstyle = optarg;
			else {
				fprintf(stderr,
					"-C should specify one: "
					"'emulab', 'file', 'null'\n");
				exit(1);
			}
			break;
		case 'x':
		case 'X':
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
					"-%c should specify one or more of: "
					"'ucast', 'mcast', 'bcast', 'any'\n",
					ch);
				exit(1);
			}
			if (ch == 'x')
				onlymethods = nm;
			else
				parentmethods = nm;
			break;
		}
		case 'd':
			daemonize = 0;
			debug++;
			break;
		case 'D':
			dumpconfig = 1;
			break;
		case 'I':
		{
			extern char *imagedir;
			imagedir = optarg;
			break;
		}
		case 'M':
			mirrormode = 1;
			break;
		case 'R':
			canredirect = 1;
			break;
		case 'S':
			if (!GetIP(optarg, &parentip)) {
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

	if (mirrormode && !fetchfromabove) {
		fprintf(stderr,
			"Error: Must specify a parent (-S) in mirror mode\n");
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
 * Return zero on success, an error code on failure.
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
				 "%s -N -S %s -i %s %s %s -m %s -p %d %s",
				 pname, servstr, ifacestr,
				 debug > 1 ? "" : "-q",
				 ci->method == CONFIG_IMAGE_UCAST ? "-O" : "",
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
	ci->retries = FRISBEE_RETRIES;
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
	struct clientextra *ce;
	time_t mtime = 0;

	assert(ci->extra != NULL);
	ce = ci->extra;
	realname = ce->realname;
	if (ce->sigtype == MS_SIGTYPE_MTIME)
		mtime = *(time_t *)ce->signature;
	free(ci->extra);
	ci->extra = NULL;
	tmpname = ci->imageinfo->path;
	ci->imageinfo->path = realname;

	if (status != 0) {
		error("%s: download failed, removing tmpfile %s",
		      realname, tmpname);
		unlink(tmpname);
		free(tmpname);
		return;
	}

	if (mtime > 0) {
		struct timeval tv[2];
		gettimeofday(&tv[0], NULL);
		tv[1].tv_sec = mtime;
		tv[1].tv_usec = 0;
		if (utimes(tmpname, tv) < 0)
			warning("%s: failed to set mtime", tmpname);
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
	struct clientextra *ce;
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
	ci->retries = 0;

	ce = ci->extra = malloc(sizeof(struct clientextra));
	if (ce == NULL) {
		free(ci);
		*errorp = MS_ERROR_FAILED;
		return NULL;
	}
	memset(ce, 0, sizeof(*ce));

	/*
	 * Arrange to download the image as <path>.tmp and then
	 * rename it into place when done.
	 */
	len = strlen(ci->imageinfo->path) + 5;
	if ((tmpname = malloc(len)) != NULL) {
		ce->realname = ci->imageinfo->path;
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
 * If pid is non-zero, we wait for that specific process.
 * Returns the number of children reaped.
 */
static int
reapchildren(int wpid, int *statusp)
{
	int pid, status;
	struct childinfo **cip, *ci;
	int corpses = 0;

	if (nchildren == 0) {
		assert(children == NULL);
		return 0;
	}

	while (1) {
		struct in_addr in;

		pid = waitpid(wpid, &status, WNOHANG);
		if (debug && wpid)
			log("wait for %d returns %d, status=%x",
			    wpid, pid, pid > 0 ? status : 0);
		if (pid <= 0)
			return 0;
		for (cip = &children; *cip != NULL; cip = &(*cip)->next) {
			if ((*cip)->pid == pid)
				break;
		}
		ci = *cip;
		if (ci == NULL) {
			error("Child died that was not ours!?");
			if (wpid)
				break;
			continue;
		}
		*cip = ci->next;
		nchildren--;
		in.s_addr = htonl(ci->addr);
		log("%s: %s process %d on %s:%d exited (status=0x%x)",
		    ci->imageinfo->imageid,
		    ci->ptype == PTYPE_SERVER ? "server" : "client",
		    pid, inet_ntoa(in), ci->port, status);

		/*
		 * Special case exit value.
		 * Server (or client) could not bind to the port we gave it.
		 * For the server, we get a new address and try again up to
		 * FRISBEE_RETRIES times.  For a client, we just fail right
		 * now.  Maybe we should sleep awhile and try again or
		 * reask our server?
		 */
		if (ci->retries > 0 && WEXITSTATUS(status) == EADDRINUSE) {
			ci->retries--;
			if (ci->ptype == PTYPE_SERVER &&
			    !config_get_server_address(ci->imageinfo,
						       ci->method, 0,
						       &ci->addr, &ci->port,
						       &ci->method) &&
			    !startchild(ci))
				continue;
		}
		if (ci->done)
			ci->done(ci, status);
		if (ci->extra)
			free(ci->extra);
		free_imageinfo(ci->imageinfo);
		free(ci);
		corpses++;
		if (wpid) {
			if (debug)
				error("  process %d exited immediately", wpid);
			if (statusp)
				*statusp = status;
			break;
		}
	}

	return corpses;
}

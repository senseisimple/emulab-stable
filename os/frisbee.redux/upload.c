/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2010-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Client-side program for uploading a file/image via the
 * frisbee master server.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <setjmp.h>
#include "decls.h"
#include "utils.h"
#include "uploadio.h"

static char *mshost = "boss";
static struct in_addr msip;
static in_port_t msport = MS_PORTNUM;

static struct in_addr serverip;
static char *imageid;
static int askonly;
static char *uploadpath;
static uint64_t filesize;
static uint32_t mtime;
static int bufsize = (64 * 1024);;
static int timeout = -1;
static int usessl = 0;
static int verify = 1;

/* Globals */
int debug = 0;
int portnum;

/* XXX not used but needed by network.c */
struct in_addr	mcastaddr;
struct in_addr	mcastif;

static void usage(void);
static void parse_args(int argc, char **argv);
static int send_file(void);
static int put_request(char *imageid, in_addr_t sip, in_port_t sport,
		       in_addr_t hostip, uint64_t isize, uint32_t mtime,
		       int timeout, int askonly, int reqtimo, PutReply *reply);

int
main(int argc, char **argv)
{
	PutReply reply;
	int timo = 5; /* XXX */
	int rv;

	ClientLogInit();
	parse_args(argc, argv);

	/* Special case: streaming from stdin */
	if (strcmp(uploadpath, "-") == 0) {
		filesize = 0;
		mtime = 0;
		if (timeout == -1)
			timeout = 0;
	}
	/* otherwise make sure file exists and see how big it is */
	else {
		struct stat sb;

		if (stat(uploadpath, &sb))
			pfatal(uploadpath);
		if (!S_ISREG(sb.st_mode))
			fatal("%s: not a regular file\n", uploadpath);
		filesize = sb.st_size;
		mtime = (uint32_t)sb.st_mtime;

		/* timeout is a function of file size */
		if (timeout == -1) {
			timeout = (int)(filesize / MIN_UPLOAD_RATE);
			/* file is stupid-huge, no timeout */
			if (timeout < 0)
				timeout = 0;
			/* impose a minimum reasonable time */
			else if (timeout < 10)
				timeout = 10;
		}
	}

	if (imageid) {
		if (!put_request(imageid, ntohl(msip.s_addr), msport, 0,
				 filesize, mtime, timeout, askonly, timo,
				 &reply))
			fatal("Could not get upload info for '%s'", imageid);

		if (askonly) {
			PrintPutInfo(imageid, &reply, 1);
			exit(0);
		}
		if (reply.error) {
			/*
			 * XXX this is a bit of a hack: MS_ERROR_TOOBIG
			 * returns the max allowed size as a courtesy so
			 * we don't have to make a second, status-only call
			 * to find out the max.
			 */
			if (reply.error == MS_ERROR_TOOBIG) {
				uint64_t maxsize;

				maxsize = ((uint64_t)reply.himaxsize << 32) |
					reply.lomaxsize;
				fatal("%s: file too large for server"
				      " (%llu > %llu)",
				      imageid, filesize, maxsize);
			} else
				fatal("%s: server returned error: %s", imageid,
				      GetMSError(reply.error));
		}

		serverip.s_addr = htonl(reply.addr);
		portnum = reply.port;
	} else {
		/*
		 * XXX hack: if no image id, treat -S/-p as address of
		 * image upload server and not the frisbee master server.
		 */
		serverip = msip;
		portnum = msport;

		imageid = "<NONE>";
		verify = 0;
	}
		
	log("%s: upload to %s:%d from %s",
	    imageid, inet_ntoa(serverip), portnum, uploadpath);

	rv = send_file();

	if (rv == 0 && verify) {
		uint64_t isize = 0;
		uint32_t mt = 0;
		int retries = 3;
		int bad = 1;

		/*
		 * Try a few times to get status.
		 * We sleep first to give the uploader a chance to finish
		 * and get reaped before we ask.
		 */
		while (1) {
			sleep(1);
			if (!put_request(imageid, ntohl(msip.s_addr), msport,
					 0, 0, 0, 0, 1, timo, &reply)) {
				warning("%s: status request failed",
					imageid);
				goto vdone;
			}
			if (reply.error == 0)
				break;

			if (reply.error &&
			    (reply.error != MS_ERROR_TRYAGAIN ||
			     --retries == 0)) {
				warning("%s: status returned error %s",
					imageid, GetMSError(reply.error));
				goto vdone;
			}
		}

		/* dpes it exist? */
		if (!reply.exists) {
			warning("%s: uploaded file does not exist?!",
				imageid);
			goto vdone;
		}

		/* is it the right size? */
		isize = ((uint64_t)reply.hisize << 32) | reply.losize;
		if (filesize && isize != filesize) {
			warning("%s: uploaded file is the wrong size: "
				"%llu bytes but should be %llu bytes",
				imageid, isize, filesize);
			goto vdone;
		}

		/* signature okay? */
		switch (reply.sigtype) {
		case MS_SIGTYPE_MTIME:
			mt = *(uint32_t *)reply.signature;
			if (mtime && mt != mtime) {
				warning("%s: uploaded file has wrong mtime: "
					"%u but should be %u",
					imageid, mt, mtime);
				goto vdone;
			}
			break;
		default:
			break;
		}

		bad = 0;

	vdone:
		if (bad)
			warning("%s: uploaded image did not verify!", imageid);
	}

	exit(rv);
}

static void
parse_args(int argc, char **argv)
{
	int ch;
	while ((ch = getopt(argc, argv, "S:p:F:Q:sb:T:N")) != -1) {
		switch (ch) {
		case 'S':
			mshost = optarg;
			break;
		case 'p':
			msport = atoi(optarg);
			break;
		case 'F':
			imageid = optarg;
			break;
		case 'Q':
			imageid = optarg;
			askonly = 1;
			break;
		case 'b':
			bufsize = atoi(optarg);
			if (bufsize < 0 || bufsize > MAX_BUFSIZE) {
				fprintf(stderr, "Invalid buffer size %d\n",
					bufsize);
				exit(1);
			}
			break;
		case 'T':
			timeout = atoi(optarg);
			if (timeout < 0 || timeout > (24 * 60 * 60)) {
				fprintf(stderr, "Invalid timeout %d\n",
					timeout);
				exit(1);
			}
			break;
		case 's':
			usessl = 1;
			break;
		case 'N':
			verify = 0;
			break;
		default:
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();
	uploadpath = argv[0];

	if (!GetIP(mshost, &msip)) {
		fprintf(stderr, "Invalid server name '%s'\n", mshost);
		exit(1);
	}
}

static void
usage(void)
{
	char *usagestr =
    	"usage: upload [options] -F <fileid> <file-to-upload>\n"
        "Upload a file via the frisbee master server.\n"
        "  <fileid> identifies the file/image at the server.\n"
        "  <file-to-upload> is the local path of the file to upload.\n"
	"Options:\n"
	"  -S <IP>      Specify the IP address of the master server.\n"
	"  -p <port>    Specify the port number of the master server.\n"
        "  -b <bufsize> Specify the IO buffer size to use (64K by default)\n"
        "  -T <timeout> Time in seconds to wait for the upload to finish.\n"
	"  -Q <fileid>  Use in place of -F to just ask the server about\n"
        "               the indicated file (image). Tells whether the\n"
        "               image location is accessible by the caller.\n"
        "  -N           Do not attempt to verify the upload.\n"
    	"  -s           Use encryption.\n\n"
#if 0
        "or:\n\n"
        "upload [options] -S <server> -p <port> <file-to-upload>\n"
        "Upload without using the master server. Here -S and -p identify\n"
        "the actual upload daemon rather than the master server.\n"
        "That daemon must be started manually in advance.\n"
	"Options:\n"
        "  -b <bufsize> Specify the IO buffer size to use (64K by default)\n"
        "  -T <timeout> Time in seconds to wait for the upload to finish.\n"
    	"  -s           Use encryption.\n\n"
#endif
    	"\n\n";
    	fprintf(stderr, "Frisbee upload/download client\n%s", usagestr);
	exit(1);
}

static sigjmp_buf toenv;

static void
send_timeout(int sig)
{
	siglongjmp(toenv, 1);
}

/*
 * XXX multithread (file read, socket write)
 */
static int
send_file(void)
{
	/* volatiles are for setjmp/longjmp */
	conn * volatile conn = NULL;
	char * volatile rbuf = NULL;
	volatile int fd = -1;
	volatile uint64_t remaining = filesize;
	struct timeval st, et;
	int rv = 1;
	char *stat;

	if (timeout > 0) {
		struct itimerval it;

		if (sigsetjmp(toenv, 1)) {
			rv = 2;
			goto done;
		}
		it.it_value.tv_sec = timeout;
		it.it_value.tv_usec = 0;
		it.it_interval = it.it_value;

		signal(SIGALRM, send_timeout);
		setitimer(ITIMER_REAL, &it, NULL);
	}

	rbuf = malloc(bufsize);
	if (rbuf == NULL) {
		error("Could not allocate %d byte buffer, try using -b",
		      bufsize);
		goto done;
	}

	if (strcmp(uploadpath, "-") == 0)
		fd = STDIN_FILENO;
	else
		fd = open(uploadpath, O_RDONLY);
	if (fd < 0) {
		pwarning(uploadpath);
		goto done;
	}

	conn = conn_open(ntohl(serverip.s_addr), portnum, usessl);
	if (conn == NULL) {
		error("Could not open connection with server %s:%d",
		      inet_ntoa(serverip), portnum);
		goto done;
	}

	gettimeofday(&st, NULL);
	while (filesize == 0 || remaining > 0) {
		ssize_t cc, ncc;

		if (filesize == 0 || remaining > bufsize)
			cc = bufsize;
		else
			cc = remaining;
		ncc = read(fd, rbuf, cc);
		if (ncc < 0) {
			pwarning("file read");
			goto done;
		}
		if (ncc == 0)
			break;

		cc = conn_write(conn, rbuf, ncc);
		if (cc < 0) {
			pwarning("socket write");
			goto done;
		}
		remaining -= cc;
		if (cc != ncc) {
			error("short write on socket (%d != %d)", cc, ncc);
			goto done;
		}
	}
	if (filesize == 0 || remaining == 0)
		rv = 0;

 done:
	gettimeofday(&et, NULL);

	if (timeout) {
		struct itimerval it;

		it.it_value.tv_sec = 0;
		it.it_value.tv_usec = 0;
		setitimer(ITIMER_REAL, &it, NULL);

	}
	if (conn != NULL)
		conn_close(conn);
	if (fd >= 0)
		close(fd);
	if (rbuf != NULL)
		free(rbuf);

	timersub(&et, &st, &et);

	/* Note that remaining will be negative if filesize==0 */
	stat = rv == 0 ? "completed" : (rv == 1 ? "terminated" : "timed-out");
	if (filesize && remaining)
		log("%s: upload %s after %llu (of %llu) bytes "
		    "in %d.%03d seconds",
		    imageid, stat, filesize-remaining, filesize,
		    et.tv_sec, et.tv_usec/1000);
	else
		log("%s: upload %s after %llu bytes in %d.%03d seconds",
		    imageid, stat, filesize-remaining,
		    et.tv_sec, et.tv_usec/1000);

	/* Set filesize to bytes written so verify has something to check */
	if (verify && rv == 0 && filesize == 0)
		filesize = filesize - remaining;

	return rv;
}

/*
 * Contact the master server to negotiate an upload for a 'file' to store
 * under the given 'imageid'.
 *
 * 'sip' and 'sport' are the addr/port of the master server, 'askonly' is
 * set to just see if the upload is allowed and to get characteristics of
 * any existing copy of the image, 'timeout' is how long to wait for a
 * response.
 *
 * If 'hostip' is not zero, then we are requesting information on behalf of
 * that node.  The calling node (us) must have "proxy" permission on the
 * server for this to work.
 *
 * On success, return non-zero with 'reply' filled in with the server's
 * response IN HOST ORDER.  On failure returns zero.
 */
static int
put_request(char *imageid, in_addr_t sip, in_port_t sport, in_addr_t hostip,
	    uint64_t isize, uint32_t mtime, int timeout, int askonly,
	    int reqtimo, PutReply *reply)
{
	struct sockaddr_in name;
	MasterMsg_t msg;
	int msock, len;
	
	if ((msock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("Could not allocate socket for master server");
		return 0;
	}
	if (sport == 0)
		sport = MS_PORTNUM;

	name.sin_family = AF_INET;
	name.sin_addr.s_addr = htonl(sip);
	name.sin_port = htons(sport);
	if (connect(msock, (struct sockaddr *)&name, sizeof(name)) < 0) {
		perror("Connecting to master server");
		close(msock);
		return 0;
	}

	memset(&msg, 0, sizeof msg);
	strncpy((char *)msg.hdr.version, MS_MSGVERS_1,
		sizeof(msg.hdr.version));
	msg.hdr.type = htonl(MS_MSGTYPE_PUTREQUEST);
	msg.body.putrequest.hostip = htonl(hostip);
	if (askonly)
		msg.body.putrequest.status = 1;
	len = strlen(imageid);
	if (len > MS_MAXIDLEN)
		len = MS_MAXIDLEN;
	msg.body.putrequest.idlen = htons(len);
	strncpy((char *)msg.body.putrequest.imageid, imageid, MS_MAXIDLEN);
	if (filesize > 0) {
		msg.body.putrequest.hisize = htonl(filesize >> 32);
		msg.body.putrequest.losize = htonl(filesize);
	}
	if (mtime)
		msg.body.putrequest.mtime = htonl(mtime);
	if (timeout)
		msg.body.putrequest.timeout = htonl(timeout);

	len = sizeof msg.hdr + sizeof msg.body.putrequest;
	if (!MsgSend(msock, &msg, len, reqtimo)) {
		close(msock);
		return 0;
	}

	memset(&msg, 0, sizeof msg);
	len = sizeof msg.hdr + sizeof msg.body.putreply;
	if (!MsgReceive(msock, &msg, len, reqtimo)) {
		close(msock);
		return 0;
	}
	close(msock);

	if (strncmp((char *)msg.hdr.version, MS_MSGVERS_1,
		    sizeof(msg.hdr.version))) {
		fprintf(stderr, "Got incorrect version from master server\n");
		return 0;
	}
	if (ntohl(msg.hdr.type) != MS_MSGTYPE_PUTREPLY) {
		fprintf(stderr, "Got incorrect reply from master server\n");
		return 0;
	}

	/*
	 * Convert the reply info to host order
	 */
	*reply = msg.body.putreply;
	reply->error = ntohs(reply->error);
	reply->addr = ntohl(reply->addr);
	reply->port = ntohs(reply->port);
	reply->sigtype = ntohs(reply->sigtype);
	if (reply->sigtype == MS_SIGTYPE_MTIME)
		*(uint32_t *)reply->signature =
			ntohl(*(uint32_t *)reply->signature);
	reply->hisize = ntohl(reply->hisize);
	reply->losize = ntohl(reply->losize);
	reply->himaxsize = ntohl(reply->himaxsize);
	reply->lomaxsize = ntohl(reply->lomaxsize);
	return 1;
}

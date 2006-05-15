/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004, 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <paths.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <db.h>
#include <fcntl.h>
#include "log.h"
#include "tbdefs.h"
#include "bootwhat.h"
#include "bootinfo.h"

/*
 * Minimum number of seconds that must pass before we send another
 * event for a node. This is to decrease the number of spurious events
 * we get from nodes when bootinfo packets are lost. 
 */
#define MINEVENTTIME	10

static void	log_bootwhat(struct in_addr ipaddr, boot_what_t *bootinfo);
static void	onhup(int sig);
static char	*progname;
static int	bicache_init(void);
static int	bicache_shutdown(void);
static int	bicache_needevent(struct in_addr ipaddr);
int		debug = 0;

void
usage()
{
	fprintf(stderr,
		"Usage: %s <options> [-d]\n"
		"options:\n"
		"-d         - Turn on debugging\n"
		"-p port    - Specify port number to listen on\n",
		progname);
	exit(-1);
}

int
main(int argc, char **argv)
{
	int			sock, length, mlen, err, c;
	struct sockaddr_in	name, client;
	boot_info_t		boot_info;
	boot_what_t	       *boot_whatp = (boot_what_t *) &boot_info.data;
	int		        port = BOOTWHAT_DSTPORT;
	char			buf[BUFSIZ];
	FILE			*fp;
	extern char		build_info[];

	progname = argv[0];

	while ((c = getopt(argc, argv, "p:dhv")) != -1) {
		switch (c) {
		case 'd':
			debug++;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'v':
		    	fprintf(stderr, "%s\n", build_info);
			exit(0);
			break;
		case 'h':
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc)
		usage();

	if (debug) 
		loginit(0, 0);
	else {
		/* Become a daemon */
		daemon(0, 0);
		loginit(1, "bootinfo");
	}
	info("%s\n", build_info);

	/*
	 * Write out a pidfile.
	 */
	sprintf(buf, "%s/bootinfo.pid", _PATH_VARRUN);
	fp = fopen(buf, "w");
	if (fp != NULL) {
		fprintf(fp, "%d\n", getpid());
		(void) fclose(fp);
	}

	/* Initialize data base */
	err = open_bootinfo_db();
	if (err) {
		error("could not open database");
		exit(1);
	}
	err = bicache_init();
	if (err) {
		error("could not initialize cache");
		exit(1);
	}
#ifdef EVENTSYS
	err = bievent_init();
	if (err) {
		error("could not initialize event system");
		exit(1);
	}
#endif
	/* Create socket from which to read. */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		errorc("opening datagram socket");
		exit(1);
	}
	
	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons((u_short) port);
	if (bind(sock, (struct sockaddr *) &name, sizeof(name))) {
		errorc("binding datagram socket");
		exit(1);
	}
	/* Find assigned port value and print it out. */
	length = sizeof(name);
	if (getsockname(sock, (struct sockaddr *) &name, &length)) {
		errorc("getting socket name");
		exit(1);
	}
	info("listening on port %d\n", ntohs(name.sin_port));

	signal(SIGHUP, onhup);
	while (1) {
		int	needevent = 1;
		
		if ((mlen = recvfrom(sock, &boot_info, sizeof(boot_info),
				     0, (struct sockaddr *)&client, &length))
		    < 0) {
			errorc("receiving datagram packet");
			exit(1);
		}

		switch (boot_info.opcode) {
		case BIOPCODE_BOOTWHAT_REQUEST:
		case BIOPCODE_BOOTWHAT_INFO:
			info("%s: REQUEST (vers %d)\n",
			     inet_ntoa(client.sin_addr), boot_info.version);
#ifdef	EVENTSYS
			needevent = bicache_needevent(client.sin_addr);
			if (needevent)
				bievent_send(client.sin_addr,
					     TBDB_NODESTATE_PXEBOOTING);
#endif
			err = query_bootinfo_db(client.sin_addr,
						boot_info.version,
						boot_whatp);
			break;

		default:
			info("%s: invalid packet %d\n",
			     inet_ntoa(client.sin_addr), boot_info.opcode);
			continue;
		}
		
		if (err)
			boot_info.status = BISTAT_FAIL;
		else {
			boot_info.status = BISTAT_SUCCESS;
			log_bootwhat(client.sin_addr, boot_whatp);
#ifdef	EVENTSYS
			if (needevent) {
				switch (boot_whatp->type) {
				case BIBOOTWHAT_TYPE_PART:
				case BIBOOTWHAT_TYPE_SYSID:
				case BIBOOTWHAT_TYPE_MB:
				case BIBOOTWHAT_TYPE_MFS:
					bievent_send(client.sin_addr,
						     TBDB_NODESTATE_BOOTING);
					break;
					
				case BIBOOTWHAT_TYPE_WAIT:
					bievent_send(client.sin_addr,
						     TBDB_NODESTATE_PXEWAIT);
					break;

				case BIBOOTWHAT_TYPE_REBOOT:
					bievent_send(client.sin_addr,
						     TBDB_NODESTATE_REBOOTING);
					break;

				default:
					error("%s: invalid boot directive: %d\n",
					      inet_ntoa(client.sin_addr),
					      boot_whatp->type);
					break;
				}
			}
#endif
		}
		boot_info.opcode = BIOPCODE_BOOTWHAT_REPLY;
		
		client.sin_family = AF_INET;
		client.sin_port = htons(BOOTWHAT_SRCPORT);
		if (sendto(sock, (char *)&boot_info, sizeof(boot_info), 0,
			(struct sockaddr *)&client, sizeof(client)) < 0)
			errorc("sendto");
	}
	close(sock);
	close_bootinfo_db();
#ifdef  EVENTSYS
	bievent_shutdown();
#endif
	bicache_shutdown();
	info("daemon terminating\n");
	exit(0);
}

static void
onhup(int sig)
{
	int err;

	info("re-initializing configuration database\n");
	close_bootinfo_db();
	err = open_bootinfo_db();
	if (err) {
		error("Could not reopen database");
		exit(1);
	}
}

/*
 * Simple cache to prevent dups when bootinfo packets get lost.
 */
static DB      *dbp;

/*
 * Initialize an in-memory DB
 */
static int
bicache_init(void)
{
	if ((dbp =
	     dbopen(NULL, O_CREAT|O_TRUNC|O_RDWR, 664, DB_HASH, NULL)) == NULL) {
		errorc("failed to initialize the bootinfo DBM");
		return 1;
	}
	return 0;
}

static int
bicache_shutdown(void)
{
	if (dbp) {
		if (dbp->close(dbp) < 0) {
			errorc("failed to sutdown the bootinfo DBM");
			return 1;
		}
	}
	return 0;
}

/*
 * This does both a check and an insert. The idea is that we store the
 * current time of the request, returning yes/no to the caller if the
 * current request is is within a small delta of the previous request.
 * This should keep the number of repeats to a minimum, since a requests
 * coming within a few seconds of each other indicate lost bootinfo packets.
 */
static int
bicache_needevent(struct in_addr ipaddr)
{
	DBT	key, item;
	time_t  tt = time(NULL);
	int	rval = 1, r;

	key.data = (void *) &ipaddr;
	key.size = sizeof(ipaddr);

	/*
	 * First find current value.
	 */
	if ((r = (dbp->get)(dbp, &key, &item, NULL)) != NULL) {
		if (r == -1) {
			errorc("Could not retrieve entry from DBM for %s\n",
			       inet_ntoa(ipaddr));
		}
	}
	if (r == NULL) {
		time_t	oldtt = *((time_t *)item.data);

		if (debug) {
			info("Timestamps: old:%ld new:%ld\n", oldtt, tt);
		}

		if (tt - oldtt <= MINEVENTTIME) {
			rval = 0;
			info("%s: no event will be sent: last:%ld cur:%ld\n",
			     inet_ntoa(ipaddr), oldtt, tt);
		}
	}
	if (rval) {
		item.data = (void *) &tt;
		item.size = sizeof(tt);

		if ((dbp->put)(dbp, &key, &item, NULL) != NULL) {
			errorc("Could not insert DBM entry for %s\n",
			       inet_ntoa(ipaddr));
		}
	}
	return rval;
}

static void
log_bootwhat(struct in_addr ipaddr, boot_what_t *bootinfo)
{
	char ipstr[32];

	strncpy(ipstr, inet_ntoa(ipaddr), sizeof ipstr);
	switch (bootinfo->type) {
	case BIBOOTWHAT_TYPE_PART:
		info("%s: REPLY: boot from partition %d\n",
		     ipstr, bootinfo->what.partition);
		break;
	case BIBOOTWHAT_TYPE_SYSID:
		info("%s: REPLY: boot from partition with sysid %d\n",
		     ipstr, bootinfo->what.sysid);
		break;
	case BIBOOTWHAT_TYPE_MB:
		info("%s: REPLY: boot multiboot image %s:%s\n",
		       ipstr, inet_ntoa(bootinfo->what.mb.tftp_ip),
		       bootinfo->what.mb.filename);
		break;
	case BIBOOTWHAT_TYPE_WAIT:
		info("%s: REPLY: wait mode\n", ipstr);
		break;
	case BIBOOTWHAT_TYPE_MFS:
		info("%s: REPLY: boot from mfs %s\n", ipstr, bootinfo->what.mfs);
		break;
	case BIBOOTWHAT_TYPE_REBOOT:
		info("%s: REPLY: reboot (alternate PXE boot)\n", ipstr);
		break;
	}
	if (bootinfo->cmdline[0]) {
		info("%s: REPLY: command line: %s\n", ipstr, bootinfo->cmdline);
	}
}


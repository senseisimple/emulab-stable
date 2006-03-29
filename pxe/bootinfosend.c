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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#include "config.h"
#include "log.h"
#include "tbdefs.h"
#include "bootwhat.h"
#include "bootinfo.h"

static void	log_bootwhat(struct in_addr ipaddr, boot_what_t *bootinfo);
int		debug = 0;
static char	*progname;

void
usage()
{
	fprintf(stderr,
		"Usage: %s <options> [-d] [-r | -q] target\n"
		"options:\n"
		"-d         - Turn on debugging\n"
		"-q         - Tell node to query bootinfo again\n"
		"-r         - Tell node to reboot\n",
		progname);
	exit(-1);
}

int
main(int argc, char **argv)
{
	int			sock, err, c, reboot = 0, query = 0;
	struct sockaddr_in	name, target;
	boot_info_t		boot_info;
	boot_what_t	       *boot_whatp = (boot_what_t *) &boot_info.data;
	extern char		build_info[];
	struct hostent	       *he;

	progname = argv[0];

	while ((c = getopt(argc, argv, "dhvrq")) != -1) {
		switch (c) {
		case 'd':
			debug++;
			break;
		case 'r':
			reboot++;
			break;
		case 'q':
			query++;
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

	if (!argc)
		usage();
	if (query && reboot)
		usage();

	if (debug) 
		loginit(0, 0);
	else
		loginit(1, "bootinfo");

	if (debug)
		info("%s\n", build_info);

	/* Make sure we can map target. */
	if ((he = gethostbyname(argv[0])) == NULL) {
		errorc("gethostbyname(%s)", argv[0]);
		exit(1);
	}

	bzero(&target, sizeof(target));
	memcpy((char *)&target.sin_addr, he->h_addr, he->h_length);
	target.sin_family = AF_INET;
	target.sin_port   = htons((u_short) BOOTWHAT_SRCPORT);

	err = open_bootinfo_db();
	if (err) {
		error("could not open database");
		exit(1);
	}
#ifdef EVENTSYS
	err = bievent_init();
	if (err) {
		error("could not initialize event system");
		exit(1);
	}
#endif
	/* Create socket */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		errorc("opening datagram socket");
		exit(1);
	}
	err = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT,
		       (char *)&err, sizeof(err)) < 0)
		errorc("setsockopt(SO_REUSEADDR)");
	
	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons((u_short) BOOTWHAT_SENDPORT);
	if (bind(sock, (struct sockaddr *) &name, sizeof(name))) {
		errorc("binding datagram socket");
		exit(1);
	}

	bzero(&boot_info, sizeof(boot_info));
	boot_info.version = BIVERSION_CURRENT;
	if (reboot) {
		boot_whatp->type = BIBOOTWHAT_TYPE_REBOOT;
#ifdef	EVENTSYS
		bievent_send(target.sin_addr, TBDB_NODESTATE_SHUTDOWN);
#endif
	}
	else if (query) {
		boot_whatp->type = BIBOOTWHAT_TYPE_AUTO;
#ifdef	EVENTSYS
		bievent_send(target.sin_addr, TBDB_NODESTATE_PXEWAKEUP);
#endif
	}
	else {
		err = query_bootinfo_db(target.sin_addr,
					boot_info.version,
					boot_whatp);
		if (err) {
			fatal("Could not send bootinfo packet!");
		}
#ifdef	EVENTSYS
		bievent_send(target.sin_addr, TBDB_NODESTATE_PXEBOOTING);
		switch (boot_whatp->type) {
		case BIBOOTWHAT_TYPE_PART:
		case BIBOOTWHAT_TYPE_SYSID:
		case BIBOOTWHAT_TYPE_MB:
		case BIBOOTWHAT_TYPE_MFS:
			bievent_send(target.sin_addr, TBDB_NODESTATE_BOOTING);
			break;
				
		case BIBOOTWHAT_TYPE_WAIT:
			bievent_send(target.sin_addr, TBDB_NODESTATE_PXEWAIT);
			break;
		default:
			error("%s: invalid boot directive: %d\n",
			      inet_ntoa(target.sin_addr), boot_whatp->type);
			break;
		}
#endif
	}

	log_bootwhat(target.sin_addr, boot_whatp);
	boot_info.status  = BISTAT_SUCCESS;
	boot_info.opcode  = BIOPCODE_BOOTWHAT_ORDER;

#ifdef  ELABINELAB
	/* This is too brutal to even describe */
	elabinelab_hackcheck(&target);
#endif
	if (sendto(sock, (char *)&boot_info, sizeof(boot_info), 0,
		   (struct sockaddr *)&target, sizeof(target)) < 0)
		errorc("sendto");

	close(sock);
	close_bootinfo_db();
#ifdef  EVENTSYS
	bievent_shutdown();
#endif
	exit(0);
}

static void
log_bootwhat(struct in_addr ipaddr, boot_what_t *bootinfo)
{
	char ipstr[32];

	strncpy(ipstr, inet_ntoa(ipaddr), sizeof ipstr);
	switch (bootinfo->type) {
	case BIBOOTWHAT_TYPE_PART:
		info("%s: SEND: boot from partition %d\n",
		     ipstr, bootinfo->what.partition);
		break;
	case BIBOOTWHAT_TYPE_SYSID:
		info("%s: SEND: boot from partition with sysid %d\n",
		     ipstr, bootinfo->what.sysid);
		break;
	case BIBOOTWHAT_TYPE_MB:
		info("%s: SEND: boot multiboot image %s:%s\n",
		       ipstr, inet_ntoa(bootinfo->what.mb.tftp_ip),
		       bootinfo->what.mb.filename);
		break;
	case BIBOOTWHAT_TYPE_WAIT:
		info("%s: SEND: wait mode\n", ipstr);
		break;
	case BIBOOTWHAT_TYPE_REBOOT:
		info("%s: SEND: reboot\n", ipstr);
		break;
	case BIBOOTWHAT_TYPE_AUTO:
		info("%s: SEND: query bootinfo\n", ipstr);
		break;
	case BIBOOTWHAT_TYPE_MFS:
		info("%s: SEND: boot from mfs %s\n", ipstr, bootinfo->what.mfs);
		break;
	}
}

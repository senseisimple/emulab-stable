/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/param.h>
#include <paths.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <db.h>
#include <fcntl.h>
#include <time.h>
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
static char     pidfile[MAXPATHLEN];
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

static void
cleanup()
{
	unlink(pidfile);
	exit(0);
}

int
main(int argc, char **argv)
{
	int			sock, length, mlen, err, c;
	struct sockaddr_in	name, client;
	boot_info_t		boot_info;
	int		        port = BOOTWHAT_DSTPORT;
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

	signal(SIGTERM, cleanup);
	/*
	 * Write out a pidfile.
	 */
	sprintf(pidfile, "%s/bootinfo.pid", _PATH_VARRUN);
	fp = fopen(pidfile, "w");
	if (fp != NULL) {
		fprintf(fp, "%d\n", getpid());
		(void) fclose(fp);
	}

	err = bootinfo_init();
	if (err) {
		error("could not initialize bootinfo");
		exit(1);
	}
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
		if ((mlen = recvfrom(sock, &boot_info, sizeof(boot_info),
				     0, (struct sockaddr *)&client, &length))
		    < 0) {
			errorc("receiving datagram packet");
			exit(1);
		}
		err = bootinfo(client.sin_addr, (char *) NULL,
			       &boot_info, (void *) NULL, 0);
		if (err < 0)
			continue;
		if (boot_info.status == BISTAT_SUCCESS)
			log_bootwhat(client.sin_addr,
				     (boot_what_t *) &boot_info.data);

		boot_info.opcode = BIOPCODE_BOOTWHAT_REPLY;
		
		client.sin_family = AF_INET;
		client.sin_port = htons(BOOTWHAT_SRCPORT);
		if (sendto(sock, (char *)&boot_info, sizeof(boot_info), 0,
			(struct sockaddr *)&client, sizeof(client)) < 0)
			errorc("sendto");
	}
	close(sock);
	close_bootinfo_db();
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


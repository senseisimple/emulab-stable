#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <syslog.h>
#include <signal.h>
#include "bootwhat.h"

static void log_bootwhat(struct in_addr ipaddr, boot_what_t *bootinfo);
static void onhup(int sig);
static int version = 1;

main()
{
	int			sock, length, data, i, mlen, err;
	struct sockaddr_in	name, client;
	boot_info_t		boot_info;
	boot_what_t	       *boot_whatp = (boot_what_t *) &boot_info.data;

	openlog("bootinfo", LOG_PID, LOG_USER);
	syslog(LOG_NOTICE, "daemon starting (version %d)", version);

	(void)daemon(0, 0);

	/* Initialize data base */
	err = open_bootinfo_db();
	if (err) {
		syslog(LOG_ERR, "could not open database");
		exit(1);
	}
 
	/* Create socket from which to read. */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		syslog(LOG_ERR, "opening datagram socket: %m");
		exit(1);
	}
	
	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons(BOOTWHAT_DSTPORT);
	if (bind(sock, (struct sockaddr *) &name, sizeof(name))) {
		syslog(LOG_ERR, "binding datagram socket: %m");
		exit(1);
	}
	/* Find assigned port value and print it out. */
	length = sizeof(name);
	if (getsockname(sock, (struct sockaddr *) &name, &length)) {
		syslog(LOG_ERR, "getting socket name: %m");
		exit(1);
	}
	syslog(LOG_NOTICE, "listening on port %d", ntohs(name.sin_port));

	signal(SIGHUP, onhup);
	while (1) {
		int ack = 0;
		
		if ((mlen = recvfrom(sock, &boot_info, sizeof(boot_info),
				     0, (struct sockaddr *)&client, &length))
		    < 0) {
			syslog(LOG_ERR, "receiving datagram packet: %m");
			exit(1);
		}

		switch (boot_info.opcode) {
		case BIOPCODE_BOOTWHAT_REQUEST:
			syslog(LOG_INFO, "%s: REQUEST",
			       inet_ntoa(client.sin_addr));
			err = query_bootinfo_db(client.sin_addr, boot_whatp);
			break;

		case BIOPCODE_BOOTWHAT_ACK:
			syslog(LOG_INFO, "%s: ACK",
			       inet_ntoa(client.sin_addr));
			ack_bootinfo_db(client.sin_addr, boot_whatp);
			continue;

		default:
			syslog(LOG_INFO, "%s: invalid packet",
			       inet_ntoa(client.sin_addr));
			continue;
		}
		
		if (err) {
			boot_info.status = BISTAT_FAIL;
		} else {
			log_bootwhat(client.sin_addr, boot_whatp);
			boot_info.status = BISTAT_SUCCESS;
		}
		boot_info.opcode = BIOPCODE_BOOTWHAT_REPLY;
		
		client.sin_family = AF_INET;
		client.sin_port = htons(BOOTWHAT_SRCPORT);
		if (sendto(sock, (char *)&boot_info, sizeof(boot_info), 0,
			(struct sockaddr *)&client, sizeof(client)) < 0)
			syslog(LOG_ERR, "sendto: %m");
	}
	close(sock);
	close_bootinfo_db();
	syslog(LOG_NOTICE, "daemon terminating");
	exit(0);
}

static void
onhup(int sig)
{
	int err;

	syslog(LOG_NOTICE, "re-initializing configuration database");
	close_bootinfo_db();
	err = open_bootinfo_db();
	if (err) {
		syslog(LOG_ERR, "Could not open database");
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
		syslog(LOG_INFO, "%s: REPLY: boot from partition %d",
		       ipstr, bootinfo->what.partition);
		break;
	case BIBOOTWHAT_TYPE_SYSID:
		syslog(LOG_INFO, "%s: REPLY: boot from partition with sysid %d",
		       ipstr, bootinfo->what.sysid);
		break;
	case BIBOOTWHAT_TYPE_MB:
		syslog(LOG_INFO, "%s: REPLY: boot multiboot image %s:%s\n",
		       ipstr,
		       inet_ntoa(bootinfo->what.mb.tftp_ip),
		       bootinfo->what.mb.filename);
		break;
	}
}

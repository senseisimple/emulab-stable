/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003, 2005, 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <assert.h>
#include <stdarg.h>
#include <mysql/mysql.h>
#include <sys/time.h>
#include <grp.h>
#include "capdecls.h"
#include "config.h"
#include "tbdb.h"

#define TESTMODE

static int	debug = 0;
static int	portnum = SERVERPORT;
static gid_t	admingid;

char *usagestr = 
 "usage: capserver [-d] [-p #]\n"
 " -d              Turn on debugging.\n"
 " -p portnum	   Specify a port number to listen on.\n"
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
	MYSQL_RES		*res;	
	MYSQL_ROW		row;
	int			tcpsock, ch;
	int			length, i, err = 0;
	struct sockaddr_in	name;
	struct timeval		timeout;
	struct group		*group;

	while ((ch = getopt(argc, argv, "dp:")) != -1)
		switch(ch) {
		case 'p':
			portnum = atoi(optarg);
			break;
		case 'd':
			debug++;
			break;
		case 'h':
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc)
		usage();

	openlog("capserver", LOG_PID, LOG_TESTBED);
	syslog(LOG_NOTICE, "daemon starting");

	if (!debug)
		(void)daemon(0, 0);

	if (!dbinit()) {
		syslog(LOG_ERR, "Could not connect to DB!");
		exit(1);
	}

	/*
	 * Grab the GID for the default group.
	 */
	if ((group = getgrnam(TBADMINGROUP)) == NULL) {
		syslog(LOG_ERR, "Getting GID for %s", TBADMINGROUP);
		exit(1);
	}
	admingid = group->gr_gid;

	/*
	 * Setup TCP socket
	 */

	/* Create socket from which to read. */
	tcpsock = socket(AF_INET, SOCK_STREAM, 0);
	if (tcpsock < 0) {
		syslog(LOG_ERR, "opening stream socket: %m");
		exit(1);
	}

	i = 1;
	if (setsockopt(tcpsock, SOL_SOCKET, SO_REUSEADDR,
		       (char *)&i, sizeof(i)) < 0)
		syslog(LOG_ERR, "control setsockopt: %m");;
	
	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons((u_short) portnum);
	if (bind(tcpsock, (struct sockaddr *) &name, sizeof(name))) {
		syslog(LOG_ERR, "binding stream socket: %m");
		exit(1);
	}
	/* Find assigned port value and print it out. */
	length = sizeof(name);
	if (getsockname(tcpsock, (struct sockaddr *) &name, &length)) {
		syslog(LOG_ERR, "getting socket name: %m");
		exit(1);
	}
	if (listen(tcpsock, 40) < 0) {
		syslog(LOG_ERR, "listening on socket: %m");
		exit(1);
	}
	syslog(LOG_NOTICE, "listening on TCP port %d", ntohs(name.sin_port));

	while (1) {
		struct sockaddr_in client;
		int		   clientsock, length = sizeof(client);
		int		   cc, port;
		whoami_t	   whoami;
		unsigned char	   buf[BUFSIZ], node_id[64];
		secretkey_t        secretkey;
		tipowner_t	   tipown;
		void		  *reply = &tipown;
		size_t		   reply_size = sizeof(tipown);

		if ((clientsock = accept(tcpsock,
					 (struct sockaddr *)&client,
					 &length)) < 0) {
			syslog(LOG_ERR, "accept failed: %m");
			exit(1);
		}
		port = ntohs(client.sin_port);
		syslog(LOG_INFO, "%s connected from port %d",
		       inet_ntoa(client.sin_addr), port);

		/*
		 * Check port number of sender. Must be a reserved port.
		 */
		if (port >= IPPORT_RESERVED || port < IPPORT_RESERVED / 2) {
			syslog(LOG_ERR, "Illegal port! Ignoring.");
			goto done;
		}

		/*
		 * Set timeouts
		 */
		timeout.tv_sec  = 6;
		timeout.tv_usec = 0;
		
		if (setsockopt(clientsock, SOL_SOCKET, SO_RCVTIMEO,
			       &timeout, sizeof(timeout)) < 0) {
			syslog(LOG_ERR, "SO_RCVTIMEO failed: %m");
			goto done;
		}
		if (setsockopt(clientsock, SOL_SOCKET, SO_SNDTIMEO,
			       &timeout, sizeof(timeout)) < 0) {
			syslog(LOG_ERR, "SO_SNDTIMEO failed: %m");
			goto done;
		}
		
		/*
		 * Read in the whoami info.
		 */
		if ((cc = read(clientsock, &whoami, sizeof(whoami))) <= 0) {
			if (cc < 0)
				syslog(LOG_ERR, "Reading request: %m");
			syslog(LOG_ERR, "Connection aborted (read)");
			goto done;
		}
		if (cc != sizeof(whoami)) {
			syslog(LOG_ERR, "Wrong byte count (read)!");
			goto done;
		}

		/*
		 * Make sure there is an entry for this tipline in
		 * the DB. If not, we just drop the info with an error
		 * message in the log file. Local tip will still work but
		 * remote tip will not.
		 */
		res = mydb_query("select server,node_id,portnum from tiplines "
				 "where tipname='%s'",
				 3, whoami.name);
		if (!res) {
			syslog(LOG_ERR, "DB Error getting tiplines for %s!",
			       whoami.name);
			goto done;
		}
		if ((int)mysql_num_rows(res) == 0) {
			syslog(LOG_ERR, "No tipline info for %s!",
			       whoami.name);
			mysql_free_result(res);
			goto done;
		}
		row = mysql_fetch_row(res);
		strcpy(node_id, row[1]);
		port = -1;
		sscanf(row[2], "%d", &port);
		mysql_free_result(res);

		/*
		 * Figure out current owner. Might not be a reserved node,
		 * in which case set it to root/wheel by default. 
		 */
		res = mydb_query("select g.unix_gid from reserved as r "
				 "left join experiments as e on "
				 " r.pid=e.pid and r.eid=e.eid "
				 "left join groups as g on "
				 " g.pid=e.pid and g.gid=e.gid "
				 "where r.node_id='%s'",
				 1, node_id);

		if (!res) {
			syslog(LOG_ERR, "DB Error getting info for %s/%s!",
			       node_id, whoami.name);
			goto done;
		}
		if ((int)mysql_num_rows(res)) {
			row = mysql_fetch_row(res);

			tipown.uid = 0;
			if (row[0])
				tipown.gid = atoi(row[0]);
			else
				tipown.gid = admingid;
		}
		else {
			/*
			 * Default to root/root.
			 */
			tipown.uid = 0;
			tipown.gid = admingid;
		}
		mysql_free_result(res);

		if (whoami.portnum == -1) {
			reply = &port;
			reply_size = sizeof(port);
		}
		/*
		 * Update the DB.
		 */
		else if (! mydb_update("update tiplines set portnum=%d, "
				       "keylen=%d, keydata='%s' "
				       "where tipname='%s'", 
				       whoami.portnum,
				       whoami.key.keylen, whoami.key.key,
				       whoami.name)) {
			syslog(LOG_ERR, "DB Error updating tiplines for %s!",
			       whoami.name);
			goto done;
		}

		/*
		 * And now send the reply.
		 */
		if ((cc = write(clientsock, reply, reply_size)) <= 0) {
			if (cc < 0)
				syslog(LOG_ERR, "Writing reply: %m");
			syslog(LOG_ERR, "Connection aborted (write)");
			goto done;
		}
		if (cc != reply_size) {
			syslog(LOG_ERR, "Wrong byte count (write)!");
			goto done;
		}

		syslog(LOG_INFO,
		       "Tipline %s/%s, Port %d, Keylen %d, Key %s, Group %d\n",
		       node_id, whoami.name, whoami.portnum,
		       whoami.key.keylen, whoami.key.key, tipown.gid);
	done:
		close(clientsock);
	}
	close(tcpsock);
	syslog(LOG_NOTICE, "daemon terminating");
	exit(0);
}


/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004, 2006, 2007 University of Utah and the Flux Group.
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

static int	bicache_init(void);
static int	bicache_needevent(struct in_addr ipaddr);

int
bootinfo_init(void)
{
	int	err;
	
	/* Initialize data base */
	err = open_bootinfo_db();
	if (err) {
		error("could not open database");
		return -1;
	}
	err = bicache_init();
	if (err) {
		error("could not initialize cache");
		return -1;
	}
#ifdef EVENTSYS
	err = bievent_init();
	if (err) {
		error("could not initialize event system");
		return -1;
	}
#endif
	return 0;
}

int
bootinfo(struct in_addr ipaddr, struct boot_info *boot_info, void *opaque)
{
	int		needevent = 0;
	int		err;
	boot_what_t	*boot_whatp = (boot_what_t *) &boot_info->data;

	switch (boot_info->opcode) {
	case BIOPCODE_BOOTWHAT_KEYED_REQUEST:
		info("%s: KEYED REQUEST (key=[%s], vers %d)\n",
			inet_ntoa(ipaddr), boot_info->data, boot_info->version);
#ifdef	EVENTSYS
		needevent = bicache_needevent(ipaddr);
		if (needevent)
			bievent_send(ipaddr, opaque,
				     TBDB_NODESTATE_PXEBOOTING);
#endif
		err = query_bootinfo_db(ipaddr, boot_info->version, boot_whatp, boot_info->data);
		break;
	case BIOPCODE_BOOTWHAT_REQUEST:
	case BIOPCODE_BOOTWHAT_INFO:
		info("%s: REQUEST (vers %d)\n",
		     inet_ntoa(ipaddr), boot_info->version);
#ifdef	EVENTSYS
		needevent = bicache_needevent(ipaddr);
		if (needevent)
			bievent_send(ipaddr, opaque,
				     TBDB_NODESTATE_PXEBOOTING);
#endif
		err = query_bootinfo_db(ipaddr,
					boot_info->version, boot_whatp, NULL);
		break;

	default:
		info("%s: invalid packet %d\n",
		     inet_ntoa(ipaddr), boot_info->opcode);
		return -1;
	}
	if (err)
		boot_info->status = BISTAT_FAIL;
	else {
		boot_info->status = BISTAT_SUCCESS;
#ifdef	EVENTSYS
		if (needevent) {
			switch (boot_whatp->type) {
			case BIBOOTWHAT_TYPE_PART:
			case BIBOOTWHAT_TYPE_SYSID:
			case BIBOOTWHAT_TYPE_MB:
			case BIBOOTWHAT_TYPE_MFS:
				bievent_send(ipaddr, opaque,
					     TBDB_NODESTATE_BOOTING);
				break;
					
			case BIBOOTWHAT_TYPE_WAIT:
				bievent_send(ipaddr, opaque,
					     TBDB_NODESTATE_PXEWAIT);
				break;

			case BIBOOTWHAT_TYPE_REBOOT:
				bievent_send(ipaddr, opaque, 
					     TBDB_NODESTATE_REBOOTING);
				break;

			default:
				error("%s: invalid boot directive: %d\n",
				      inet_ntoa(ipaddr), boot_whatp->type);
				break;
			}
		}
#endif
	}
	return 0;
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
	if ((dbp = dbopen(NULL, O_CREAT|O_TRUNC|O_RDWR, 664, DB_HASH, NULL))
	    == NULL) {
		pfatal("failed to initialize the bootinfo DBM");
		return -1;
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

	/* So we can include bootinfo into tmcd; always send the event. */
	if (!dbp)
		return 1;

	key.data = (void *) &ipaddr;
	key.size = sizeof(ipaddr);

	/*
	 * First find current value.
	 */
	if ((r = (dbp->get)(dbp, &key, &item, 0)) != 0) {
		if (r == -1) {
			errorc("Could not retrieve entry from DBM for %s\n",
			       inet_ntoa(ipaddr));
		}
	}
	if (r == 0) {
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

		if ((dbp->put)(dbp, &key, &item, 0) != 0) {
			errorc("Could not insert DBM entry for %s\n",
			       inet_ntoa(ipaddr));
		}
	}
	return rval;
}


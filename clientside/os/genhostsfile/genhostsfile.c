/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004 University of Utah and the Flux Group.
 * All rights reserved.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include "config.h"
#include "log.h"
#include "tbdefs.h"

/* Output macro to check for string overflow */
#define OUTPUT(buf, size, format...) \
({ \
	int __count__ = snprintf((buf), (size), ##format); \
        \
        if (__count__ >= (size)) { \
		error("Not enough room in output buffer! line %d.\n", __LINE__);\
		return 1; \
	} \
	__count__; \
})

void
usage()
{
	fprintf(stderr, "genhosts vname\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	char		buf[2 * BUFSIZ];
	int		hostcount, nrows;
	int		rv = 0;
	char		*nickname;	/* The host we are generating for */

	/*
	 * We build up a canonical host table using this data structure.
	 * There is one item per node/iface. We need a shared structure
	 * though for each node, to allow us to compute the aliases.
	 */
	struct shareditem {
	    	int	hasalias;
		char	*firstvlan;	/* The first vlan to another node */
		int	is_me;          /* 1 if this node is the tmcc client */
	};
	struct hostentry {
		char	vname[TBDB_FLEN_VNAME];
		char	vlan[TBDB_FLEN_VNAME];
		int	virtiface;
		struct in_addr	  ipaddr;
		struct shareditem *shared;
		struct hostentry  *next;
	} *hosts = 0, *host;

	if (argc != 2)
		usage();
	nickname = argv[1];

	/*
	 * First line of input is the number of hosts.
	 */
	if (fgets(buf, sizeof(buf), stdin) == NULL) {
		error("genhostsfile: EOF reading number of nodes");
		exit(-1);
	}
	nrows = atoi(buf);

	/*
	 * Parse the list, creating an entry for each node/IP pair.
	 *
	 * The input format is "vname,ips" ...
	 */
	while (nrows--) {
		char		  *vname, *ips;
		char		  *bp, *ip, *cp;
		struct shareditem *shareditem;

		if (fgets(buf, sizeof(buf), stdin) == NULL) {
			error("genhostsfile: EOF reading host!\n");
			exit(-1);
		}
		vname = buf;
		if ((cp = index(buf, ',')) == NULL) {
			error("genhostsfile: Bad host: %s!\n", buf);
			exit(-1);
		}
		*cp = '\0';
		ips = cp + 1;
		
		if (! (shareditem = (struct shareditem *)
		       calloc(1, sizeof(*shareditem)))) {
			error("HOSTNAMES: Out of memory for shareditem!\n");
			exit(1);
		}

		/*
		 * Check to see if this is the node we're talking to
		 */
		if (!strcmp(vname, nickname)) {
		    shareditem->is_me = 1;
		}

		bp = ips;
		while (bp) {
			/*
			 * Note that the ips column is a space separated list
			 * of X:IP where X is a logical interface number.
			 */
			cp = strsep(&bp, ":");
			ip = strsep(&bp, " ");

			if (! (host = (struct hostentry *)
			              calloc(1, sizeof(*host)))) {
				error("HOSTNAMES: Out of memory!\n");
				exit(1);
			}

			strcpy(host->vname, vname);
			host->virtiface = atoi(cp);
			host->shared = shareditem;
			inet_aton(ip, &host->ipaddr);
			host->next = hosts;
			hosts = host;
		}
	}

	/*
	 * Now read virt_lans.
	 *
	 * The input format is "vname,member" ...
	 */
	
	/*
	 * First is the number of virt_lans.
	 */
	if (fgets(buf, sizeof(buf), stdin) == NULL) {
		error("genhostsfile: EOF reading number of lans");
		exit(-1);
	}
	nrows = atoi(buf);

	while (nrows--) {
		char	*vname, *member, *bp, *cp;
		int	virtiface;

		if (fgets(buf, sizeof(buf), stdin) == NULL) {
			error("genhostsfile: EOF reading lan!\n");
			exit(-1);
		}
		vname = buf;
		if ((cp = index(buf, ',')) == NULL) {
			error("genhostsfile: Bad lan: %s!\n", buf);
			exit(-1);
		}
		*cp = '\0';
		member = cp + 1;

		/*
		 * Note that the members column looks like vname:X
		 * where X is a logical interface number we got above.
		 * Loop through and find the entry and stash the vlan
		 * name.
		 */
		bp = member;
		cp = strsep(&bp, ":");
		virtiface = atoi(bp);

		host = hosts;
		while (host) {
			if (host->virtiface == virtiface &&
			    strcmp(cp, host->vname) == 0) {
				strcpy(host->vlan, vname);
			}
			host = host->next;
		}
	}

	/*
	 * The last part of the puzzle is to determine who is directly
	 * connected to this node so that we can add an alias for the
	 * first link to each connected node (could be more than one link
	 * to another node). Since we have the vlan names for all the nodes,
	 * its a simple matter of looking in the table for all of the nodes
	 * that are in the same vlan as the node that we are making the
	 * host table for.
	 */
	host = hosts;
	while (host) {
		/*
		 * Only care about this nodes vlans.
		 */
		if (strcmp(host->vname, nickname) == 0 && host->vlan) {
			struct hostentry *tmphost = hosts;

			while (tmphost) {
				if (strlen(tmphost->vlan) &&
				    strcmp(host->vlan, tmphost->vlan) == 0 &&
				    strcmp(host->vname, tmphost->vname) &&
				    (!tmphost->shared->firstvlan ||
				     !strcmp(tmphost->vlan,
					     tmphost->shared->firstvlan))) {
					
					/*
					 * Use as flag to ensure only first
					 * link flagged as connected (gets
					 * an alias), but since a vlan could
					 * be a real lan with more than one
					 * other member, must tag all the
					 * members.
					 */
					tmphost->shared->firstvlan =
						tmphost->vlan;
				}
				tmphost = tmphost->next;
			}
		}
		host = host->next;
	}
#if 0
	host = hosts;
	while (host) {
		printf("%s %s %d %s %d\n", host->vname, 
		       host->vlan, host->virtiface, inet_ntoa(host->ipaddr),
		       host->connected);
		host = host->next;
	}
#endif

	/*
	 * Okay, spit the entries out!
	 */
	hostcount = 0;
	host = hosts;
	while (host) {
		char	*alias = " ";

		if ((host->shared->firstvlan &&
		     !strcmp(host->shared->firstvlan, host->vlan)) ||
		    /* Directly connected, first interface on this node gets an
		       alias */
		    (!strcmp(host->vname, nickname) && !host->virtiface)){
			alias = host->vname;
		} else if (!host->shared->firstvlan &&
			   !host->shared->hasalias &&
			   !host->shared->is_me) {
		    /* Not directly connected, but we'll give it an alias
		       anyway */
		    alias = host->vname;
		    host->shared->hasalias = 1;
		}

		OUTPUT(buf, sizeof(buf),
		       "NAME=%s-%s IP=%s ALIASES='%s-%i %s'\n",
		       host->vname, host->vlan,
		       inet_ntoa(host->ipaddr),
		       host->vname, host->virtiface, alias);

		printf("%s", buf);
		host = host->next;
		hostcount++;
	}
	host = hosts;
	while (host) {
		struct hostentry *tmphost = host->next;
		free(host);
		host = tmphost;
	}
	return rv;
}

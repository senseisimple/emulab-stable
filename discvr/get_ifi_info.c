/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

/* 
 * This is a very slightly modified version of
 * example code from Unix Netwok Programming, edition 2.
 */

/*
 * need: 
 * - flags (IFF_UP, IFF_LOOPBACK)
 * - index (really need string name which we get from if_indextoname()
 * - broadcast addr (brdaddr)
 * set:
 * - nbors
 * 
 */

#include "discvr.h"

#define min(a,b) ((a) < (b) ? (a) : (b))

struct ifi_info *
get_ifi_info(int family, int doaliases)
{
	int 			flags;
	char			*buf, *next, *lim;
	size_t			len;
	struct if_msghdr	*ifm;
	struct ifa_msghdr	*ifam;
	struct sockaddr		*sa, *rti_info[RTAX_MAX];
	struct sockaddr_dl	*sdl;
	struct ifi_info		*ifi, *ifisave, *ifihead, **ifipnext;

	// Get the temporary list of interfaces in the "buf" buffer and the 
	// length of the list in "len"... 
	// there may be some interfaces which are not up... or some may not 
	// be having network addresses already... -ik
	buf = net_rt_iflist(family, 0, &len);

	ifihead = NULL;
	ifipnext = &ifihead;

	lim = buf + len;
	for (next = buf; next < lim; next += ifm->ifm_msglen) {
		ifm = (struct if_msghdr *) next;
		// "ifm_type" is the interface information message type
		// RTM_IFINFO: interface going up or down
		if (ifm->ifm_type == RTM_IFINFO) {
			if ( ((flags = ifm->ifm_flags) & IFF_UP) == 0)
				continue;	/* ignore if interface not up */

			// Interface is up.. get the list of addresses on the back of
			// message header and proceed
			sa = (struct sockaddr *) (ifm + 1);

			// Get the sockadders in the rti_info array after reading from
			// "sa" list
			get_rtaddrs(ifm->ifm_addrs, sa, rti_info);

			// The sockaddr at "RTAX_IFP" index has the name and link level
			// address for the interface.
			if ( (sa = rti_info[RTAX_IFP]) != NULL) {

				// Make a node of the link list "ifi" to store information 
				// about an interface..
				ifi = calloc(1, sizeof(struct ifi_info));
				*ifipnext = ifi;			/* prev points to this new one */
				ifipnext = &ifi->ifi_next;	/* ptr to next one goes here */

				ifi->ifi_flags = flags;

				// If its not a link layer interface then ignore
				if (sa->sa_family == AF_LINK) {

					// copy the sock adder to link level sockaddr
					// coz we now know its a link level address
					sdl = (struct sockaddr_dl *) sa;

					// Now, get the name and address of the link level
					// interface...
					if (sdl->sdl_nlen > 0)
						snprintf(ifi->ifi_name, IFNAMSIZ, "%*s",
								 sdl->sdl_nlen, &sdl->sdl_data[0]);
					else
						snprintf(ifi->ifi_name, IFNAMSIZ, "index %d",
								 sdl->sdl_index);

					if ( (ifi->ifi_hlen = sdl->sdl_alen) > 0)
						memcpy(ifi->ifi_haddr, LLADDR(sdl),
							   min(IFHADDRSIZ, sdl->sdl_alen));
				}
			}

		// Check if address is being added to the interface...
		} else if (ifm->ifm_type == RTM_NEWADDR) {
			if (ifi->ifi_addr) {	/* already have an IP addr for i/f */
				if (doaliases == 0)
					continue;

					/* 4we have a new IP addr for existing interface */
				ifisave = ifi;
				ifi = (struct ifi_info *)calloc(1, sizeof(struct ifi_info));
				*ifipnext = ifi;			/* prev points to this new one */
				ifipnext = &ifi->ifi_next;	/* ptr to next one goes here */
				ifi->ifi_flags = ifisave->ifi_flags;
				ifi->ifi_hlen = ifisave->ifi_hlen;
				memcpy(ifi->ifi_name, ifisave->ifi_name, IFNAMSIZ);
				memcpy(ifi->ifi_haddr, ifisave->ifi_haddr, IFHADDRSIZ);
			}

			ifam = (struct ifa_msghdr *) next;
			sa = (struct sockaddr *) (ifam + 1);
			get_rtaddrs(ifam->ifam_addrs, sa, rti_info);

			if ( (sa = rti_info[RTAX_IFA]) != NULL) {
				ifi->ifi_addr = (struct sockaddr *)calloc(1, sa->sa_len);
				memcpy(ifi->ifi_addr, sa, sa->sa_len);
			}

			if ((flags & IFF_BROADCAST) &&
				(sa = rti_info[RTAX_BRD]) != NULL) {
				ifi->ifi_brdaddr = (struct sockaddr *)calloc(1, sa->sa_len);
				memcpy(ifi->ifi_brdaddr, sa, sa->sa_len);
			}

			if ((flags & IFF_POINTOPOINT) &&
				(sa = rti_info[RTAX_BRD]) != NULL) {
				ifi->ifi_dstaddr = (struct sockaddr *)calloc(1, sa->sa_len);
				memcpy(ifi->ifi_dstaddr, sa, sa->sa_len);
			}

		} else {
			fprintf(stderr, "unexpected message type %d", ifm->ifm_type);
			exit(1);
		}
	}
	/* "ifihead" points to the first structure in the linked list */
	return(ifihead);	/* ptr to first structure in linked list */
}

void
free_ifi_info(struct ifi_info *ifihead)
{
	struct ifi_info	*ifi, *ifinext;

	for (ifi = ifihead; ifi != NULL; ifi = ifinext) {
		if (ifi->ifi_addr != NULL)
			free(ifi->ifi_addr);
		if (ifi->ifi_brdaddr != NULL)
			free(ifi->ifi_brdaddr);
		if (ifi->ifi_dstaddr != NULL)
			free(ifi->ifi_dstaddr);
		ifinext = ifi->ifi_next;		/* can't fetch ifi_next after free() */
		free(ifi);					/* the ifi_info{} itself */
	}
}


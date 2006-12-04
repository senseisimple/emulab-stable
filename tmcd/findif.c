/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2001-2006 University of Utah and the Flux Group.
 * All rights reserved.
 *
 * BSD code derived from:
 * $FreeBSD: src/sbin/ifconfig/ifconfig.c,v 1.51.2.21 2004/03/15 07:25:30 ru Exp $
 */

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * A hugely silly program to map a MAC to the eth/fxp/whatever device.
 * Complicated by that fact that no OS agrees on how this info should
 * be presented.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#ifndef __CYGWIN__
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#endif /* __CYGWIN__ */
#ifdef __FreeBSD__
#include <err.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#endif

#define ADDR_MAC	0
#define ADDR_IPV4	1
static int	addrtype = ADDR_MAC;

static int	find_iface(char *mac);

void
usage()
{
	fprintf(stderr, "usage: findif [-mi] <addr>\n");
	fprintf(stderr, "         -m   addr is a MAC addr (the default)\n");
	fprintf(stderr, "         -i   addr is an IPv4 addr\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int ch;

	while ((ch = getopt(argc, argv, "im")) != -1) {
		switch (ch) {
		case 'i':
			addrtype = ADDR_IPV4;
			break;
		case 'm':
			addrtype = ADDR_MAC;
			break;
		default:
			usage();
		}
	}
	argv += optind;
	argc -= optind;
	if (argc != 1)
		usage();

	exit(find_iface(argv[0]));
}

#ifdef __FreeBSD__
#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

void
rt_xaddrs(char *cp, char *cplim, struct rt_addrinfo *rtinfo)
{
	struct sockaddr *sa;
	int i;

	memset(rtinfo->rti_info, 0, sizeof(rtinfo->rti_info));
	for (i = 0; (i < RTAX_MAX) && (cp < cplim); i++) {
		if ((rtinfo->rti_addrs & (1 << i)) == 0)
			continue;
		rtinfo->rti_info[i] = sa = (struct sockaddr *)cp;
		ADVANCE(cp, sa);
	}
}

static int
find_iface(char *macaddr)
{
	struct	if_msghdr	*ifm;
	struct	ifa_msghdr	*ifam;
	struct	sockaddr_dl	*sdl;
	char			*buf, *lim, *next, *cp, *name;
	size_t			needed;
	int			n, addrs, mib[6];

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;
	mib[3] = 0;	/* address family */
	mib[4] = NET_RT_IFLIST;
	mib[5] = 0;

	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
		errx(1, "iflist-sysctl-estimate");
	if ((buf = (char *) malloc(needed)) == NULL)
		errx(1, "malloc");
	if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0)
		errx(1, "actual retrieval of interface table");
	lim = buf + needed;

	next = buf;
	while (next < lim) {
		ifm = (struct if_msghdr *) next;

		if (ifm->ifm_type == RTM_IFINFO) {
			sdl = (struct sockaddr_dl *)(ifm + 1);
		}
		else {
			fprintf(stderr, "error parsing IFLIST\n");
			exit(1);
		}
		next += ifm->ifm_msglen;

		ifam = NULL;
		for (addrs = 0; next < lim; addrs++) {
			struct if_msghdr *nextifm = (struct if_msghdr *)next;

			if (nextifm->ifm_type != RTM_NEWADDR)
				break;

			if (ifam == NULL)
				ifam = (struct ifa_msghdr *)nextifm;

			next += nextifm->ifm_msglen;
		}
		
		cp = (char *)LLADDR(sdl);
		if ((n = sdl->sdl_alen) <= 0 || sdl->sdl_type != IFT_ETHER)
			continue;
		name = sdl->sdl_data;

		if (addrtype == ADDR_MAC) {
			char enet[BUFSIZ], *bp = enet;

			*bp = 0;
			while (--n >= 0) {
				sprintf(bp, "%02x", *cp++ & 0xff);
				bp += 2;
			}
			*bp = 0;

			if (strcasecmp(enet, macaddr) == 0) {
				printf("%s\n", name);
				return 0;
			}
		}
		else if (addrtype == ADDR_IPV4) {
			struct rt_addrinfo info;
			struct sockaddr_in *sin;

			while (addrs > 0) {
				info.rti_addrs = ifam->ifam_addrs;

				/* Expand the compacted addresses */
				rt_xaddrs((char *)(ifam + 1),
					  ifam->ifam_msglen + (char *)ifam,
					  &info);

				sin = (struct sockaddr_in *)
					info.rti_info[RTAX_IFA];
				if (sin && sin->sin_family == AF_INET) {
					char *enet = inet_ntoa(sin->sin_addr);

					if (!strcmp(enet, macaddr)) {
						printf("%s\n", name);
						return 0;
					}
				}

				addrs--;
				ifam = (struct ifa_msghdr *)
					((char *)ifam + ifam->ifam_msglen);
			}
		}
	}
	return 1;
}
#endif

#ifdef linux
static int
find_iface(char *macaddr)
{
	int		sock;
	struct ifreq    ifrbuf, *ifr = &ifrbuf;
	FILE	       *fp;
	char		buf[BUFSIZ], *bp, enet[BUFSIZ];

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket()");
		return -1;
	}

	/*
	 * Get a list of all the interfaces.
	 *
	 * If we are looking for IP addresses, just use SIOCGIFCONF
	 * which only returns configured interfaces but does return
	 * alias devices.  Otherwise use /proc/net which returns all
	 * interfaces but NOT alias devices.
	 */
	if (addrtype == ADDR_IPV4) {
		struct ifconf ifc;
		static struct ifreq reqbuf[128];
		int n;

		ifc.ifc_buf = (void *)reqbuf;
		ifc.ifc_len = sizeof(reqbuf);

		if (ioctl(sock, SIOCGIFCONF, &ifc) < 0) {
			perror("SIOCGIFCONF");
			return -1;
		}

		ifr = ifc.ifc_req;
		for (n = 0; n < ifc.ifc_len; n += sizeof(struct ifreq)) {
			sprintf(enet, "%u.%u.%u.%u",
			       (unsigned char) ifr->ifr_addr.sa_data[2],
			       (unsigned char) ifr->ifr_addr.sa_data[3],
			       (unsigned char) ifr->ifr_addr.sa_data[4],
			       (unsigned char) ifr->ifr_addr.sa_data[5]);
			/* printf("%s %s\n", ifr->ifr_name, enet); */
		
			if (strcmp(enet, macaddr) == 0) {
				printf("%s\n", ifr->ifr_name);
				return 0;
			}
			ifr++;
		}
		return 1;
	}

	if ((fp = fopen("/proc/net/dev", "r")) == NULL) {
		fprintf(stderr, "Could not open /proc/net/dev\n");
		return -1;
	}
	/* Eat a couple of lines */
	fgets(buf, sizeof(buf), fp);
	fgets(buf, sizeof(buf), fp);

	while (fgets(buf, sizeof(buf), fp)) {
		sscanf(buf, "%s:", ifr->ifr_name);
		if ((bp = strchr(ifr->ifr_name, ':')))
			*bp = '\0';
				
		ifr->ifr_addr.sa_family = AF_INET;
		if (ioctl(sock, SIOCGIFHWADDR, ifr) < 0)
			continue;

		/*
		 * Great, the tulip driver mis-reports the MAC address
		 * on at least one type of lite-on card.  Looks like it
		 * byte-swaps pairs of octets.  We recognize the offending
		 * vendor ID (as reported, it is not a valid ID, so there
		 * is no ambiguity)
		 */
		if ((unsigned char)ifr->ifr_addr.sa_data[1] == 0x00 &&
		    (unsigned char)ifr->ifr_addr.sa_data[0] == 0x02 &&
		    (unsigned char)ifr->ifr_addr.sa_data[3] == 0xE3) {
			unsigned short *usp;

			usp = (unsigned short *)ifr->ifr_addr.sa_data;
			usp[0] = ntohs(usp[0]);
			usp[1] = ntohs(usp[1]);
			usp[2] = ntohs(usp[2]);
		}

		sprintf(enet, "%02x%02x%02x%02x%02x%02x",
			(unsigned char) ifr->ifr_addr.sa_data[0],
			(unsigned char) ifr->ifr_addr.sa_data[1],
			(unsigned char) ifr->ifr_addr.sa_data[2],
			(unsigned char) ifr->ifr_addr.sa_data[3],
			(unsigned char) ifr->ifr_addr.sa_data[4],
			(unsigned char) ifr->ifr_addr.sa_data[5]);

		/* printf("%s %s\n", ifr->ifr_name, enet); */
		
		if (strcasecmp(enet, macaddr) == 0) {
			printf("%s\n", ifr->ifr_name);
			fclose(fp);
			return 0;
		}
	}
	fclose(fp);
	return 1;
}
#endif

#ifdef __CYGWIN__
static int
find_iface(char *macaddr)
{
#if 1
	/* We cache the getmac output early on in rc.cygwin, while all of the
	 * interfaces are still enabled.  Later, we get no MAC info.
	 */
	FILE *fp = fopen("/var/emulab/boot/getmac-cache", "r");
#else
	/* We copy the Windows getmac.exe and library framedyn.dll into $BINDIR
	 * because our path gets "untainted" into a pure Unix form in rc scripts.
	 */
	FILE *fp = popen(
		"bash -c 'cd /usr/local/etc/emulab; ./getmac /nh /v /fo csv'", "r");
#endif
	char buf[BUFSIZ];
	
	while (fgets(buf, BUFSIZ, fp) != NULL) {
		char *comma1, *comma2, mac[13], *mc;
		int i;

		/* The first comma-separated field is the name with quotes, and the
		 * third is the MAC addr, with quotes and dash separators.
		 */
		comma1 = index(buf, ',');
		if ( comma1 == NULL ) continue;	/* Skip blank line. */
		comma2 = index(comma1+1, ',');
		if ( comma2 == NULL ) continue;	/* Shouldn't happen. */

		/* The MAC argument we're comparing against has no separators.
		 * Dump the dashes.
		 */ 
		mc = comma2+2;
		for (i = 0; i < 6; i++ ) {
			mac[i*2] = *mc++;
			mac[i*2 + 1] = *mc++;
			mc++;
		}
		mac[12] = '\0';
		/* printf("%s %s\n", buf, mac); */
		
		if (strcasecmp(mac, macaddr) == 0) {
			*(comma1-1) = '\0';	/* Prune the quotes. */
			printf("%s\n", buf+1);
			pclose(fp);
			return 0;
		}
	}
	pclose(fp);
	return 1;
}
#endif /* __CYGWIN__ */

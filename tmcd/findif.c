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
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#ifdef __FreeBSD__
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#endif
#ifdef linux
#include <net/if.h>
#include <string.h>
#endif

static int	find_iface(char *mac);

void
usage()
{
	fprintf(stderr, "usage: foo <macaddr>\n");
	exit(1);
}

main(int argc, char **argv)
{
	unsigned char	ether_addr[ETHER_ADDR_LEN];
	
	if (argc != 2)
		usage();

	exit(find_iface(argv[1]));
}

#ifdef __FreeBSD__
static int
find_iface(char *macaddr)
{
	struct	if_msghdr	*ifm;
	struct	sockaddr_dl	*sdl;
	char			*buf, *lim, *next, *cp;
	size_t			needed;
	int			n, mib[6];

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

		while (next < lim) {
			struct	if_msghdr *nextifm = (struct if_msghdr *)next;

			if (nextifm->ifm_type != RTM_NEWADDR)
				break;

			next += nextifm->ifm_msglen;
		}
		
		cp = (char *)LLADDR(sdl);
		if ((n = sdl->sdl_alen) > 0 &&
		    sdl->sdl_type == IFT_ETHER) {
			char	enet[BUFSIZ], *bp = enet;

			*bp = 0;
			while (--n >= 0) {
				sprintf(bp, "%02x", *cp++ & 0xff);
				bp += 2;
			}
			*bp = 0;

			if (strcasecmp(enet, macaddr) == 0) {
				printf("%s\n", sdl->sdl_data);
				return 0;
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
	int		sock, i;
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
	 * SIOCGIFCONF appears to return a list of just the configured
	 * interfaces, but we need all of them.
	 */
	if ((fp = fopen("/proc/net/dev", "r")) == NULL) {
		fprintf(stderr, "Could not open /proc/net/dev\n");
		return -1;
	}
	/* Eat a couple of lines */
	fgets(buf, sizeof(buf), fp);
	fgets(buf, sizeof(buf), fp);

	while (fgets(buf, sizeof(buf), fp)) {
		sscanf(buf, "%s:", ifr->ifr_name);
		if (bp = strchr(ifr->ifr_name, ':'))
			*bp = (char *)0;
				
		ifr->ifr_addr.sa_family = AF_INET;
		if (ioctl(sock, SIOCGIFHWADDR, ifr) < 0)
			continue;

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

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <net/if_dl.h>
#include <unistd.h>
#include <errno.h>
/*#include <netinet/ip_fw.h> */
/* ipfw data structures have changed between FBSD 4.3 and FBSD 4.5 .
   boss has the former but the experimental nodes run the latter. 
   Therefore use a local version of ip_fw.h which is also in cvs */
#include "ip_fw.h"
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "tbnexthop.h"

static int sockfd;
static char buf[512];
static pid_t mypid = 0;
static int myseq;

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

uint16_t
get_nexthop_if(struct in_addr addr)
{
	struct rt_msghdr *rtm;
	struct sockaddr_dl *ifp;
	struct sockaddr_in *sin;
	struct sockaddr *sa;
	int i, len;

	if (mypid == 0) {
		mypid = getpid();
		sockfd = socket(AF_ROUTE, SOCK_RAW, 0);
		if (sockfd < 0) {
			perror("AF_ROUTE");
			exit(1);
		}
	}

	rtm = (struct rt_msghdr *) buf;
	rtm->rtm_msglen = sizeof(struct rt_msghdr) +
		ROUNDUP(sizeof(*sin)) + ROUNDUP(sizeof(*ifp));
	rtm->rtm_version = RTM_VERSION;
	rtm->rtm_type = RTM_GET;
	rtm->rtm_addrs = RTA_DST|RTA_IFP;
	rtm->rtm_pid = mypid;
	rtm->rtm_seq = ++myseq;
	rtm->rtm_flags = RTF_UP|RTF_HOST|RTF_GATEWAY|RTF_STATIC;

	sin = (struct sockaddr_in *)&rtm[1];
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_addr = addr;

	ifp = (struct sockaddr_dl *)((char *)sin + ROUNDUP(sizeof(*sin)));
	ifp->sdl_len = sizeof(struct sockaddr_dl);
	ifp->sdl_family = AF_LINK;

	if (write(sockfd, rtm, rtm->rtm_msglen) < 0) {
		perror("write");
		exit(3);
	}

	do {
		if (read(sockfd, rtm, sizeof(buf)) < 0) {
			perror("read");
			exit(3);
		}
	} while (rtm->rtm_type != RTM_GET ||
		 rtm->rtm_seq != myseq || rtm->rtm_pid != mypid);

	if (rtm->rtm_version != RTM_VERSION) {
		fprintf(stderr, "wrong version of routing code (%d != %d)\n",
			rtm->rtm_version, RTM_VERSION);
		exit(3);
	}
	if (rtm->rtm_errno) {
		fprintf(stderr, "route read returns error %d\n",
			rtm->rtm_errno);
		exit(3);
	}

	if ((rtm->rtm_addrs & RTA_IFP) == 0)
		return 0;

	sa = (struct sockaddr *)&rtm[1];
	for (i = 1; i != 0; i <<= 1) {
		len = ROUNDUP(sa->sa_len);
		switch (i & rtm->rtm_addrs) {
		case 0:
			break;
		case RTA_IFP:
			if (sa->sa_family == AF_LINK) {
				ifp = (struct sockaddr_dl *)sa;
				return ifp->sdl_index;
			}
			/* fall into ... */
		default:
			sa = (struct sockaddr *)((char *)sa + len);
			break;
		}
	}

	return 0;
}

/* Does a
   ipfw add fwd <nexthop> ip from <src> to <dst>:<dstmask> [out]
   and returns the number of the added rule
*/

static int ipfw_sock = -1;

int
ipfw_addfwd(struct in_addr nexthop, struct in_addr src, struct in_addr dst,
	      struct in_addr dstmask, bool out) {

  struct ip_fw rule;

  if( ipfw_sock == -1 ) {
    ipfw_sock = socket( AF_INET, SOCK_RAW, IPPROTO_RAW );
    if( ipfw_sock < 0 ) {
      fprintf(stderr, "can't create raw socket: %s\n", strerror(errno));
      return(-1);
    }
  }

  memset(&rule, 0, sizeof rule);

  /* rule.fw_flg |= IP_FW_F_FWD | IP_FW_F_DMSK;*/
  rule.fw_flg |= IP_FW_F_FWD;
  rule.fw_prot |= IPPROTO_IP;
 
  /* filling next hop information */
  rule.fw_fwd_ip.sin_len = sizeof(struct sockaddr_in);
  rule.fw_fwd_ip.sin_family = AF_INET;
  rule.fw_fwd_ip.sin_port = 0;
  rule.fw_fwd_ip.sin_addr = nexthop;
  // printf( "nexthop = %s\n", inet_ntoa(nexthop));

  rule.fw_src = src;
  //printf( "src = %s\n", inet_ntoa(src));
  rule.fw_smsk.s_addr = ~0;
  //printf( "smsk = %s\n", inet_ntoa(rule.fw_smsk));
  
  rule.fw_dst = dst;
  //printf( "dst = %s\n", inet_ntoa(dst));
  rule.fw_dmsk = dstmask;
  //printf( "dmsk = %s\n", inet_ntoa(rule.fw_dmsk));

  if( out ) {
    rule.fw_flg |= IP_FW_F_OUT;    
  }

  /* getsockopt and setsockopt do the same thing for ipfw rules
     except that in the former, the rule is copied back. Since
     we need the rule number that ipfirewall chose, we use
     getsockopt() */
  socklen_t i = sizeof(rule);
  if (getsockopt(ipfw_sock, IPPROTO_IP, IP_FW_ADD, &rule, &i) == -1) {
    fprintf(stderr, "getsockopt(IP_FW_ADD): %s\n", strerror(errno));
    return(-1);
  }

  return(rule.fw_number);
}

#ifdef TBNEXTHOP_TESTME
int
main(int argc, char **argv)
{
	struct in_addr addr;
	int ifn;

	if (argc < 2) {
		fprintf(stderr, "usage: %s IPaddr ...\n", argv[0]);
		exit(2);
	}

	while (*++argv != 0) {
		if (inet_aton(*argv, &addr) == 0) {
			fprintf(stderr, "bad IP address: %s\n", *argv);
			exit(2);
		}

		ifn = get_nexthop_if(&addr);
		if (ifn == 0)
			printf("%s: no route\n", *argv);
		else
			printf("%s: through if%d\n", *argv, ifn);
	}

	exit(0);
}
#endif

#ifdef IPFW_ADDFWD_TESTME

int
main(int argc, char *argv[])
{
	struct in_addr src, dst, mask, nexthop;
	int rulenum;

	if (argc < 5) {
		fprintf(stderr, "usage: %s src dst mask nexthop\n", argv[0]);
		exit(2);
	}
        printf("argc = %d\n", argc );

	if (inet_aton(argv[1], &src) == 0) {
	  fprintf(stderr, "bad src IP address: %s\n", argv[1]);
	  exit(2);
	}
	if (inet_aton(argv[2], &dst) == 0) {
	  fprintf(stderr, "bad dst IP address: %s\n", argv[2]);
	  exit(3);
	}
	if (inet_aton(argv[3], &mask) == 0) {
	  fprintf(stderr, "bad IP mask: %s\n", argv[3]);
	  exit(4);
	}
	if (inet_aton(argv[4], &nexthop) == 0) {
	  fprintf(stderr, "bad nexthop IP address: %s\n", argv[4]);
	  exit(5);
	}
	
	rulenum = ipfw_addfwd(nexthop, src, dst, mask, true);
	printf("rulenum = %d\n", rulenum);

	exit(0);
}
#endif

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 *  Send the magical ping of death ICMP type
 */


#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>

#define IPOD_ICMPTYPE	6
#define IPOD_ICMPCODE	6
#define IPOD_IPLEN	666
#define IPOD_IDLEN	32

int icmpid = 0;
static char myid[IPOD_IDLEN];
static int myidlen = 0;

u_short in_cksum(u_short *addr, int len);
void icmpmap_init();  /* For getting information */
void icmp_info(struct icmp *icmp, char *outbuf, int maxlen);

/*
 * We perform lookups on the hosts, and then store them in a chain
 * here.
 */

struct hostdesc {
	char *hostname;
	struct in_addr hostaddr;
	struct hostdesc *next;
};

struct hostdesc *hostnames;
struct hostdesc *hosttail;

/*
 * Set up the list of hosts.  Return the count.
 */

int makehosts(char **hostlist)
{
	int i;
	struct hostent *hp;
	struct in_addr tmpaddr;
	int hostcount = 0;
	
	for (i = 0; hostlist[i]; i++) {
#ifdef DEBUG
		printf("Resolving %s\n", hostlist[i]);
#endif
		if (!hostlist[i] ||
		    !hostlist[i][0] ||
		    strlen(hostlist[i]) > MAXHOSTNAMELEN) {
		    fprintf(stderr, "bad host entry, exiting\n");
		    exit(-1);
		}
		if (!inet_aton(hostlist[i], &tmpaddr)) {
			if ((hp = gethostbyname(hostlist[i])) == NULL) {
				/* Could not resolve it.  Skip it. */
				fprintf(stderr, "%s: unknown host\n",
					hostlist[i]);
				continue;
			}
			else {
				memcpy(&tmpaddr.s_addr,
				       hp->h_addr_list[0],
				       hp->h_length);
			}
		}

		/* The host has been resolved.  Put it in the chain */
		/* We want to stick it on the end. */
		if (hostnames == NULL) {
			hostnames = (struct hostdesc *)
				malloc(sizeof(*hostnames));
			if (hostnames == NULL) {
				perror("hostnames malloc failed");
				exit(-1);
			}
			hosttail = hostnames;
		} else {
			hosttail->next = (struct hostdesc *)
				malloc(sizeof(*hostnames));
			if (hosttail->next == NULL) {
				perror("hosttail->next malloc failed");
				exit(-1);
			}
			hosttail = hosttail->next;
		}
		hosttail->hostname = strdup(hostlist[i]);
		if (hosttail->hostname == NULL) {
			perror("strdup failed");
			exit(-1);
		}
		hosttail->hostaddr = tmpaddr;
		hosttail->next = NULL;
		hostcount++;
	}
	return hostcount;
}

void usage(char *prog)
{
   fprintf(stderr, "%s [ -i identityfile ] target [ target ... ]\n", prog);
}

/*
 * Set up a packet.  Returns the length of the ICMP portion.
 */

void initpacket(char *buf, int querytype, struct in_addr fromaddr)
{
   struct ip *ip = (struct ip *)buf;
   struct icmp *icmp = (struct icmp *)(ip + 1);

   /* things we customize */
   int icmplen = 0;

   ip->ip_src = fromaddr;	/* if 0,  have kernel fill in */
   ip->ip_v   = 4;		/* Always use ipv4 for now */
   ip->ip_hl  = sizeof *ip >> 2;
   ip->ip_tos = 0;
   ip->ip_id  = htons(4321);
   ip->ip_ttl = 255;
   ip->ip_p   = 1;
   ip->ip_sum = 0;                 /* kernel fills in */

   icmp->icmp_seq   = 1;
   icmp->icmp_cksum = 0;           /* We'll compute it later. */
   icmp->icmp_type  = querytype; 
   icmp->icmp_code  = IPOD_ICMPCODE;
   if (myidlen)
      memcpy(icmp->icmp_data, myid, myidlen);

   ip->ip_len = IPOD_IPLEN;
   icmplen = IPOD_IPLEN - sizeof(struct ip);
   icmp->icmp_cksum = in_cksum((u_short *)icmp, icmplen);
}

/*
 * Send all of the ICMP queries.
 */

void sendpings(int s, int querytype, struct hostdesc *head, int delay,
	       struct in_addr fromaddr)
     
{
	char buf[1500];
	struct ip *ip = (struct ip *)buf;
	struct sockaddr_in dst;

	bzero(buf, 1500);
	initpacket(buf, querytype, fromaddr);
	dst.sin_family = AF_INET;
#ifdef DA_HAS_SIN_LEN
	dst.sin_len = sizeof(dst);
#endif

	while (head != NULL) {
		int rc;
#ifdef DEBUG
		printf("pinging %s\n", head->hostname);
#endif
		ip->ip_dst.s_addr = head->hostaddr.s_addr;
		dst.sin_addr = head->hostaddr;
		rc = sendto(s, buf, ip->ip_len, 0,
			   (struct sockaddr *)&dst,
			   sizeof(dst));
                if (rc != ip->ip_len) {
			perror("sendto");
		}
		/* Don't flood small pipes. */
		if (delay)
			usleep(delay);
		head = head->next;
	}
}

/*
 * Handles our timeout for us.  Called by the signal handler
 * when we get a SIGARLM.
 */

void myexit(int whatsig)
{
	exit(0);
}

/*
 * Open a raw socket for receiving ICMP.  Tell the kernel we want
 * to supply the IP headers.
 */

int get_icmp_socket()
{
	int s;
	int on = 1;
	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
		perror("socket");
		exit(1);
	}
	if (setsockopt(s, IPPROTO_IP, IP_HDRINCL,
		       (const char *)&on, sizeof(on)) < 0) {
		perror("IP_HDRINCL");
		exit(1);
	}
	return s;
}

int
main(int argc, char **argv)
{
   int s;

   char *progname;
   extern char *optarg;         /* getopt variable declarations */
   extern int optind;
   char ch;                     /* Holds the getopt result */
   int hostcount;
   int delay = 0;
   int querytype = ICMP_TSTAMP;
   struct in_addr fromaddr;
   int timeout = 5;  /* Default to 5 seconds */
   int identityfile;

   fromaddr.s_addr = 0;

   progname = argv[0];

   querytype = IPOD_ICMPTYPE;  /* the magical death packet number */

   while ((ch = getopt(argc, argv, "i:")) != -1)
      switch(ch)
      {
      case 'i':
	 if (optarg[0] == '-')
	    identityfile = 0;
	 else if ((identityfile = open(optarg, 0)) < 0)
	 {
	    perror(optarg);
	    exit(1);
	 }
	 myidlen = read(identityfile, myid, IPOD_IDLEN);
	 if (optarg[0] != '-')
	    close(identityfile);
         if (myidlen != IPOD_IDLEN)
	 {
	    fprintf(stderr, "%s: cannot read %d-byte identity\n",
		    optarg[0] != '-' ? optarg : "<stdin>", IPOD_IDLEN);
	    exit(2);
	 }
         break;
      default:
         usage(progname);
	 exit(-1);
      }

   argc -= optind;
   argv += optind;
   if (!argv[0] || !strlen(argv[0])) 
   {
      usage(progname);
      exit(-1);
   }

   hostcount = makehosts(argv);

   s = get_icmp_socket();

   signal(SIGALRM, myexit);
   alarm(timeout);
   sendpings(s, querytype, hostnames, delay, fromaddr);
   exit(0);
}
   
/*
 * in_cksum --
 *	Checksum routine for Internet Protocol family headers (C Version)
 *      From FreeBSD's ping.c
 */

u_short
in_cksum(addr, len)
	u_short *addr;
	int len;
{
	register int nleft = len;
	register u_short *w = addr;
	register int sum = 0;
	u_short answer = 0;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}


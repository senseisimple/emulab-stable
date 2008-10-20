/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 *
 * whol.c - Send a 'Whack-on-LAN' packet to node
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#if defined(__FreeBSD__)
#include <net/if.h>
#include <net/bpf.h>
#elif defined(__linux__)
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#endif

#include <net/ethernet.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

u_short in_cksum(u_short *addr, int len);

const int MAX_BPFNUM = 16;

const int packet_len = 666;

int whackanode(char *, char *);

int
main(int argc, char **argv)
{
    int failed = 0;
    char *iface;

    /*
     * Handle command line args
     */
    if (argc < 3) {
        fprintf(stderr,"Usage: whol <interface> <dst_address ...>\n");
        fprintf(stderr,"interface: Name of the interface to send packet on\n");
        fprintf(stderr,"dst_addr: MAC addresses in hex, without puncuation\n");
        exit(1);
    }
    argc--, argv++;

    iface = *argv;
    argc--, argv++;

    while (argc--)
	if (whackanode(iface, *argv++))
	    failed++;

    exit(failed);
}

int
whackanode(char *iface, char *victim)
{
#if defined(__FreeBSD__)
    int bpfnum;
    int bpffd;
#elif defined(__linux__)
    int sock;
#endif
    struct ifreq req;
    void *buf;
    ssize_t send_result;
    int i,j,length;
    struct ether_header *eheader;
    struct ip *ip;
    struct udphdr *udp;
    char *pbody;
    u_char dst_mac[6];
#if defined(__linux__)
    u_char src_mac[6];
    struct sockaddr_ll socket_address;
#endif

    /*
     * Convert the victim MAC address from ASCII into bytes
     */
    if (strlen(victim) != 12) {
        fprintf(stderr,"Bad format for MAC address\n");
	return 1;
    }
    for (i = 0; i < 6; i++) {
        char digits[3];
        unsigned int tmp;
        strncpy(digits, victim + (i*2),3); /* Copy in two digits */
        digits[2] = '\0'; /* Null-terminate */
        if (sscanf(digits,"%x",&tmp) != 1) {
            printf("Bad hex value in dst_addr: %s\n",digits);
	    return 1;
        }
        dst_mac[i] = tmp;
    }

#if defined(__FreeBSD__)
    /*
     * Find and open a BPF device to send the packet on
     */
    for (bpfnum = 0; bpfnum < MAX_BPFNUM; bpfnum++) {
        char bpfpath[1024];
        sprintf(bpfpath,"/dev/bpf%i",bpfnum);
        bpffd = open(bpfpath,O_WRONLY);
        if (bpffd >= 0) {
            break;
        }
    }

    if (bpffd < 0) {
        fprintf(stderr,"Failed to find an open-able BPF device\n");
	return 1;
    }

    /*
     * Attach it to the correct interface
     */
    strcpy(req.ifr_name,iface);

    if (ioctl(bpffd,BIOCSETIF,&req) < 0) {
        perror("BIOSETIF failed");
	return 1;
    }
#elif defined(__linux__)
    if ((sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
	perror("socket");
	return (1);
    }

    strcpy(req.ifr_name,iface);
    if (ioctl(sock,SIOCGIFHWADDR,&req) < 0) {
        perror("SIOGCIFHWADDR failed");
	return 1;
    }

    memcpy(src_mac, req.ifr_hwaddr.sa_data, sizeof(src_mac));

    strcpy(req.ifr_name,iface);
    if (ioctl(sock,SIOCGIFINDEX,&req) < 0) {
        perror("SIOGCIFINDEX failed");
	return 1;
    }

    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sll_family = PF_PACKET;
    socket_address.sll_protocol = htons(ETH_P_IP); /* XXX correct? */
    socket_address.sll_hatype = ARPHRD_ETHER; /* XXX is this needed? */
    socket_address.sll_pkttype = PACKET_OTHERHOST;
    socket_address.sll_halen = ETH_ALEN;
    socket_address.sll_ifindex = req.ifr_ifindex;
#endif

    /*
     * Build up a packet ourselves
     */
    buf = (void*)malloc(packet_len);
    bzero(buf,packet_len);

    eheader = buf;
    ip = buf + ETHER_HDR_LEN;
    udp = buf + ETHER_HDR_LEN + sizeof(*ip);
    pbody = buf + ETHER_HDR_LEN + sizeof(*ip) + sizeof(*udp);

    /*
     * Fill in the destination MAC, and make it look like an IP packet so that
     * the switch will deliver it.
     */
    memcpy(eheader->ether_dhost, dst_mac, sizeof(eheader->ether_dhost));
    eheader->ether_type = htons(ETHERTYPE_IP);

#if defined(__FreeBSD__)
    /* Note: The NIC will fill in the src MAC automagically on FreeBSD */
#elif defined(__linux__)
    memcpy(eheader->ether_shost, src_mac, sizeof(eheader->ether_shost));
#endif

    /*
     * Make an IP header
     */
    ip->ip_hl = (sizeof *ip >> 2);
    ip->ip_v = 4;
    ip->ip_tos = 0;
    ip->ip_len = htons(packet_len - ETHER_HDR_LEN);
    ip->ip_id = 0;
    ip->ip_off = 0;
    ip->ip_ttl = 1;
    ip->ip_p = 17; /* UDP, really oughta use getprotobyname */
    ip->ip_src.s_addr = 0xffffffff;
    ip->ip_dst.s_addr = 0xffffffff;
    ip->ip_sum = in_cksum((u_short *)ip,sizeof(*ip));

    /*
     * Make a UDP header; plain IP will work too
     */
#if defined(__FreeBSD__)
    udp->uh_sport = 0;
    udp->uh_dport = 0;
    udp->uh_ulen = htons(packet_len - ETHER_HDR_LEN - sizeof(*ip) - sizeof(*udp));
    udp->uh_sum = 0;
#else
    udp->source = 0;
    udp->dest = 0;
    udp->len = htons(packet_len - ETHER_HDR_LEN - sizeof(*ip) - sizeof(*udp));
    udp->check = 0;
#endif

    /*
     * Put in the magic juice that makes the victim reboot - 6 bytes of 1s,
     * then 16 repititions of the victim's MAC address
	 * 
	 * 11/05 NOTE: Our Intel cards ignore the 6 * 0xFF synchronization.
     */
    length = 0;
    for (i = 0; i < 6; i++) {
        pbody[length++] = (char)0xff;
    }

    for (i = 0; i < 16; i++) {
        for (j = 0; j < 6; j++) {
            pbody[length++] = dst_mac[j];
        }
    }

    /*
     * Whack away!
     */
#if defined(__FreeBSD__)
    send_result = write(bpffd,buf,packet_len);
#elif defined(__linux__)
    send_result = sendto(sock, buf, packet_len, 0,
                         (struct sockaddr *)&socket_address,
                         sizeof(socket_address));
#endif
    if (send_result < 0) {
        perror("whol: failed to send packet");
    }

    return 0;
}

/*
 * in_cksum --
 *      Checksum routine for Internet Protocol family headers (C Version)
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
    sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
    sum += (sum >> 16);                     /* add carry */
    answer = ~sum;                          /* truncate to 16 bits */
    return(answer);
}

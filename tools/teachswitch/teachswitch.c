/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 *
 * teachswitch.c - Send a packet directly on each interface of a machine, so 
 * that the switch can learn our MAC
 */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <net/ethernet.h>
#if defined(__FreeBSD__)
#include <net/if.h>
#include <net/bpf.h>
#elif defined(__linux__)
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#endif

const int MAX_INTERFACES = 8;
const int SLEEP_TIME = 30;
#if defined(__FreeBSD__)
const int MAX_BPFNUM = 16;
#endif

/*
 * Destination Ethernet address - the 'Ethernet Loopback Test' one
 */
u_char dst_mac[6] = { 0xCF, 0x00, 0x00, 0x00, 0x00, 0x00 };
//u_char dst_mac[6] = { 0x01, 0x00, 0x5e, 0x2a, 0x18, 0x0d };

const int packet_len = ETHER_MIN_LEN;

int MAX(int a, int b) { if ((a) > (b)) return (a); else return (b); }

int main(int argc, char **argv) {
    int interfaces[MAX_INTERFACES];
#if defined(__FreeBSD__)
    int bpfnum;
#elif defined(__linux__)
    struct sockaddr hwaddrs[MAX_INTERFACES];
    struct sockaddr_ll socket_address;
#endif
    struct ifreq req;
    void *buffer;
    int i;
    struct ether_header *eheader;
    char *pbody;
    int sock;
    struct ifconf ifc;
    int num_interfaces;
    void *ptr;
    char lastname[1024];

    /*
     * Get a list of all interfaces, so that we can sent packets on
     * each one.
     */
#if defined(__FreeBSD__)
    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
#elif defined(__linux__)
    if ((sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
#endif
	perror("socket");
	return (1);
    }

    ifc.ifc_buf = (caddr_t)malloc(MAX_INTERFACES * 100 * sizeof (struct ifreq));
    ifc.ifc_len = MAX_INTERFACES * 100 * sizeof (struct ifreq);
    if (ioctl(sock,SIOCGIFCONF,&ifc) < 0) {
	perror("SIOCGIFCONF");
	exit(1);
    }

#if defined(__FreeBSD__)
    bpfnum = 0;
#endif
    num_interfaces = 0;
    strcpy(lastname,"");
    for (ptr = ifc.ifc_buf; ptr < (void*)(ifc.ifc_buf + ifc.ifc_len); ) {
        char *iface;
	struct ifreq *ifr;

	ifr = (struct ifreq *)ptr;

	iface = ifr->ifr_name;

#if defined(__FreeBSD__)
	ptr = ptr + sizeof(ifr->ifr_name) +
	    MAX(sizeof(struct sockaddr),ifr->ifr_addr.sa_len);
#elif defined(__linux__)
	ptr += sizeof(*ifr);
#endif


	if (!strcmp(iface,lastname)) {
	    continue;
	} else {
	    strcpy(lastname,iface);
	}

	/*
	 * Get interface flags
	 */
	memset(&req, 0, sizeof(req));
	strcpy(req.ifr_name, iface);
	if (ioctl(sock,SIOCGIFFLAGS, (caddr_t)&req) < 0) {
	    perror("Getting interface flags");
	    exit(1);
	}

	/*
	 *
	 */
	if (req.ifr_flags & IFF_LOOPBACK) {
	    continue;
	}

	/*
	 * If the interface is down, bring it up
	 */
	if (!(req.ifr_flags & IFF_UP)) {
	    req.ifr_flags |= IFF_UP;
	    if (ioctl(sock,SIOCSIFFLAGS, (caddr_t)&req) < 0) {
		perror("Setting interface flags");
		exit(1);
	    }
	}
#if defined(__FreeBSD__)
	printf("Looking for a BPF device for %s\n",iface);

	/*
	 * Find and open a BPF device
	 */
	for (; bpfnum < MAX_BPFNUM; bpfnum++) {
	    char bpfpath[1024];
	    sprintf(bpfpath,"/dev/bpf%i",bpfnum);
	    interfaces[num_interfaces] = open(bpfpath,O_WRONLY);
	    if (interfaces[num_interfaces] < 0) {
	    } else {
		printf("Opened %s\n",bpfpath);
		break;
	    }
	}

	if (interfaces[num_interfaces] < 0) {
	    printf("Failed to find an open-able BPF device\n");
	    continue;
	}

	/*
	 * Attach it to the correct interface
	 */
	strcpy(req.ifr_name,iface);

	if (ioctl(interfaces[num_interfaces],BIOCSETIF,&req) < 0) {
	    perror("BIOSETIF failed");
	    continue;
	}
#elif defined(__linux__)
	/*
	 * Get interface index
	 */
	memset(&req, 0, sizeof(req));
	strcpy(req.ifr_name, iface);
	if (ioctl(sock,SIOCGIFINDEX, (caddr_t)&req) < 0) {
	    perror("Getting interface index");
	    exit(1);
	}


	interfaces[num_interfaces] = req.ifr_ifindex;

	/*
	 * Get interface MAC address
	 */
	memset(&req, 0, sizeof(req));
	strcpy(req.ifr_name, iface);
	if (ioctl(sock,SIOCGIFHWADDR, (caddr_t)&req) < 0) {
	    perror("Getting interface hwaddr");
	    exit(1);
	}

	memcpy(&hwaddrs[num_interfaces], &req.ifr_hwaddr,
	       sizeof (struct sockaddr));
#endif

	num_interfaces++;
    }

    /*
     * Build up a packet ourselves
     */
    buffer = (void*)malloc(packet_len);
    memset(buffer, 0, packet_len);

    eheader = buffer;
    pbody = buffer + ETHER_HDR_LEN;

    /*
     * Fill in the MAC, and make the packet type 'Loopback Test', so that it
     * hopefully won't go anywhere.
     */

    /* Set up the frame header */
    memcpy(eheader->ether_dhost, dst_mac, sizeof(dst_mac));
    eheader->ether_type = ETHERTYPE_LOOPBACK;

    strncpy(pbody,"Testbed MAC discovery",packet_len - ETHER_HDR_LEN);

#if defined(__linux__)
    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sll_family = PF_PACKET;
    socket_address.sll_protocol = htons(ETH_P_LOOP); /* XXX correct? */
    socket_address.sll_hatype = ARPHRD_ETHER; /* XXX is this needed? */
    socket_address.sll_pkttype = PACKET_LOOPBACK;
    socket_address.sll_halen = ETH_ALEN;
    memcpy(socket_address.sll_addr, dst_mac, sizeof(dst_mac));
#endif

    /*
     * Loop forever, periodically sending out packets on all interfaces.
     */
    while (1) {
	printf("Sending out packets\n");
	for (i = 0; i < num_interfaces; i++) {
	    int send_result = 0;
#if defined(__FreeBSD__)
	    send_result = write(interfaces[i],buffer,packet_len);
#elif defined(__linux__)
	    socket_address.sll_ifindex = interfaces[i];
    	    memcpy(eheader->ether_shost, hwaddrs[i].sa_data, ETH_ALEN);
	    send_result = sendto(sock, buffer, ETHER_MIN_LEN, 0,
	                         (struct sockaddr *)&socket_address,
				 sizeof(socket_address));
#endif
	    if (send_result < 0) {
		perror("Failed to send packet");
	    }
	}

	sleep(SLEEP_TIME);
    }
    
}

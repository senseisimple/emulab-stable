/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 *
 * teachswitch.c - Send a packet directly on each interface of a machine, so 
 * that the switch can learn our MAC
 */

#include <stdio.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <net/bpf.h>

#include <sys/socket.h>
#include <net/if.h>
#include <net/ethernet.h>

const int MAX_BPFNUM = 16;
const int MAX_INTERFACES = 8;
const int SLEEP_TIME = 60;

/*
 * Destination Ethernet address - the 'Ethernet Loopback Test' one
 */
u_char dst_mac[6] = { 0xCF, 0x00, 0x00, 0x00, 0x00, 0x00 };
//u_char dst_mac[6] = { 0x01, 0x00, 0x5e, 0x2a, 0x18, 0x0d };

const int packet_len = ETHER_MIN_LEN;

int MAX(int a, int b) { if ((a) > (b)) return (a); else return (b); }

int main(int argc, char **argv) {
    int bpfnum;
    int bpffd[MAX_INTERFACES];
    struct ifreq req;
    void *buf;
    ssize_t written;
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
    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
	perror("socket");
	return (1);
    }

    ifc.ifc_buf = (caddr_t)malloc(MAX_INTERFACES * 100 * sizeof (struct ifreq));
    ifc.ifc_len = MAX_INTERFACES * 100 * sizeof (struct ifreq);
    if (ioctl(sock,SIOCGIFCONF,&ifc) < 0) {
	perror("SIOCGIFCONF");
	exit(1);
    }

    bpfnum = 0;
    num_interfaces = 0;
    strcpy(lastname,"");
    for (ptr = ifc.ifc_buf; ptr < (void*)(ifc.ifc_buf + ifc.ifc_len); ) {
        char *iface;
	struct ifreq *ifr, flag_ifr;

	ifr = (struct ifreq *)ptr;

	iface = ifr->ifr_name;
	ptr = ptr + sizeof(ifr->ifr_name) +
	    MAX(sizeof(struct sockaddr),ifr->ifr_addr.sa_len);


	if (!strcmp(iface,lastname)) {
	    continue;
	} else {
	    strcpy(lastname,iface);
	}

	/*
	 * Get interface flags
	 */
	bzero(&flag_ifr,sizeof(flag_ifr));
	strcpy(flag_ifr.ifr_name, iface);
	if (ioctl(sock,SIOCGIFFLAGS, (caddr_t)&flag_ifr) < 0) {
	    perror("Getting interface flags");
	    exit(1);
	}

	if (flag_ifr.ifr_flags & IFF_LOOPBACK) {
	    continue;
	}

	/*
	 * If the interface is down, bring it up
	 */
	if (!(flag_ifr.ifr_flags & IFF_UP)) {
	    flag_ifr.ifr_flags |= IFF_UP;
	    if (ioctl(sock,SIOCSIFFLAGS, (caddr_t)&flag_ifr) < 0) {
		perror("Setting interface flags");
		exit(1);
	    }
	}

	printf("Looking for a BPF device for %s\n",iface);

	/*
	 * Find and open a BPF device
	 */
	for (; bpfnum < MAX_BPFNUM; bpfnum++) {
	    char bpfpath[1024];
	    sprintf(bpfpath,"/dev/bpf%i",bpfnum);
	    bpffd[num_interfaces] = open(bpfpath,O_WRONLY);
	    if (bpffd[num_interfaces] < 0) {
	    } else {
		printf("Opened %s\n",bpfpath);
		break;
	    }
	}

	if (bpffd[num_interfaces] < 0) {
	    printf("Failed to find an open-able BPF device\n");
	    exit(1);
	}

	/*
	 * Attach it to the correct interface
	 */
	strcpy(req.ifr_name,iface);

	if (ioctl(bpffd[num_interfaces],BIOCSETIF,&req) < 0) {
	    perror("BIOSETIF failed");
	    exit(-1);
	}

	num_interfaces++;
    }

    /*
     * Build up a packet ourselves
     */
    buf = (void*)malloc(packet_len);
    bzero(buf,packet_len);

    eheader = buf;
    pbody = buf + ETHER_HDR_LEN;

    /*
     * Fill in the MAC, and make the packet type 'Loopback Test', so that it
     * hopefully won't go anywhere.
     */
    for (i = 0; i < ETHER_ADDR_LEN; i++) {
	eheader->ether_dhost[i] = dst_mac[i];
    }
    eheader->ether_type = htons(ETHERTYPE_LOOPBACK);
    strncpy(pbody,"Testbed MAC discovery",packet_len - ETHER_HDR_LEN);


    /*
     * Loop forever, periodically sending out packets on all the BPF
     * descriptors.
     */
    while (1) {
	printf("Sending out packets\n");
	for (i = 0; i < num_interfaces; i++) {
	    written = write(bpffd[i],buf,packet_len);
	    if (written < 0) {
		perror("Write failed");
	    }
	}

	sleep(SLEEP_TIME);
    }
    
}

#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include "divert.h"

struct msockinfo *msock, *outsock;

#define DPRINTF(fmt, args...) /* Shut up */

void cleanup(int sig) {
	DPRINTF("Sig %d caught\n", sig);
	free_socket(msock);
	exit(0);
}


int main(int argc, char **argv) {
	int port = 69;

	if (argc > 1) {
		port = atoi(argv[1]);
	}

	signal(SIGINT, cleanup);

	msock = get_socket();
	outsock = get_socket();
	
	add_mask_port(msock, port);
	
	while (1) {

		struct in_addr srcAddr, destAddr;
		short srcPort, destPort;
		int length, wlen;
		char packetbuf[MAXPACKET];

		length = receive_snoop(msock,
				       packetbuf,
					MAXPACKET,
				       &srcAddr, &srcPort,
				       &destAddr, &destPort);
		
		if (length < 0) {
			DPRINTF("Breaking\n");
			break;
		}
		DPRINTF("Received packet, dst 0x%lx port %d\n",
			ntohl(destAddr.s_addr),
			ntohs(destPort));

		DPRINTF("                 src 0x%lx port %d\n",
			ntohl(srcAddr.s_addr),
			ntohs(srcPort));
		
		DPRINTF("Packet contents: %s\n",
		       packetbuf);
#if 1
		add_mask_port(outsock, ntohs(srcPort));
#endif

		wlen = send_snoop(msock,
				  packetbuf,
				  length,
				  srcAddr, srcPort,
				  destAddr, destPort);

		DPRINTF("Sent %d bytes\n", wlen);
	}
	free_socket(msock);
	DPRINTF("Exiting\n");
	exit(0);
}


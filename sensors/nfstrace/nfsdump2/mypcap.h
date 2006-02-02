
#ifndef _mypcap_h
#define _mypcap_h

int mypcap_init(pcap_t *pd, pcap_handler callback);
int mypcap_read(pcap_t *pd, void *user);

#endif

#include "stub.h"

/* tcpdump header (ether.h) defines ETHER_HDRLEN) */
#ifndef ETHER_HDRLEN 
#define ETHER_HDRLEN 14
#endif


u_int16_t handle_ethernet(u_char *args,const struct pcap_pkthdr* pkthdr,const u_char* packet);
u_int16_t handle_IP(u_char *args,const struct pcap_pkthdr* pkthdr,const u_char* packet);


/*
 * Structure of an internet header, naked of options.
 *
 * Stolen from tcpdump source (thanks tcpdump people)
 *
 * We declare ip_len and ip_off to be short, rather than u_short
 * pragmatically since otherwise unsigned comparisons can result
 * against negative integers quite easily, and fail in subtle ways.
 */
struct my_ip {
	u_int8_t	ip_vhl;		/* header length, version */
#define IP_V(ip)	(((ip)->ip_vhl & 0xf0) >> 4)
#define IP_HL(ip)	((ip)->ip_vhl & 0x0f)
	u_int8_t	ip_tos;		/* type of service */
	u_int16_t	ip_len;		/* total length */
	u_int16_t	ip_id;		/* identification */
	u_int16_t	ip_off;		/* fragment offset field */
#define	IP_DF 0x4000			/* dont fragment flag */
#define	IP_MF 0x2000			/* more fragments flag */
#define	IP_OFFMASK 0x1fff		/* mask for fragmenting bits */
	u_int8_t	ip_ttl;		/* time to live */
	u_int8_t	ip_p;		/* protocol */
	u_int16_t	ip_sum;		/* checksum */
	struct	in_addr ip_src,ip_dst;	/* source and dest address */
};

struct sniff_record {
  struct timeval captime;
  unsigned long  seq_start;
  unsigned long  seq_end;
};
typedef struct sniff_record sniff_record;
struct sniff_path {
  sniff_record records[SNIFF_WINSIZE];
  short start; //circular buffer pointers
  short end;
};
typedef struct sniff_path sniff_path;
sniff_path sniff_rcvdb[CONCURRENT_RECEIVERS];
pcap_t* descr;
int pcapfd;

void init_sniff_rcvdb(void) {
  int i;
  
  for (i=0; i<CONCURRENT_RECEIVERS; i++){
    sniff_rcvdb[i].start = 0;
    sniff_rcvdb[i].end = 0;
  }
}

int push_sniff_rcvdb(int path_id, u_long start_seq, u_long end_seq, const struct timeval *ts){
  struct in_addr addr;
  sniff_path *path;
  int next;

  path = &(sniff_rcvdb[path_id]);
  next = path->end;
  //The circular buffer is full when the start and end pointers are back to back
  if ((path->start-next)%SNIFF_WINSIZE == (SNIFF_WINSIZE-1)){
    addr.s_addr =rcvdb[path_id].ip;
    printf("Error: circular buffer is full for the path to %s", inet_ntoa(addr));
    return -1;
  }
  path->records[next].seq_start = start_seq;
  path->records[next].seq_end   = end_seq;
  path->records[next].captime.tv_sec  = ts->tv_sec;
  path->records[next].captime.tv_usec = ts->tv_usec;
  path->end=(next+1)%SNIFF_WINSIZE;
  return 0;
}


int search_sniff_rcvdb(int path_id, u_long seqnum) {
  sniff_path *path = &(sniff_rcvdb[path_id]);
  int next = path->start;

  while (next != (path->end)){
    if ((path->records[next].seq_start)==(path->records[next].seq_end)){//no payload
      if ((path->records[next].seq_end)==seqnum){
	return next;
      }
    } else if ((path->records[next].seq_start)<=seqnum && (path->records[next].seq_end)>seqnum) {
      return next;
    }
    next = (next+1) % SNIFF_WINSIZE;
  }
  return -1;
}

void pop_sniff_rcvdb(int path_id, u_long to_seqnum){
  int to_index = search_sniff_rcvdb(path_id, to_seqnum);
  if (to_index != -1) {
    //if the packet has no payload or the last sent seqnum equals the pop number
    if ((sniff_rcvdb[path_id].records[to_index].seq_end==sniff_rcvdb[path_id].records[to_index].seq_start) 
      || (sniff_rcvdb[path_id].records[to_index].seq_end-1 == to_seqnum)) {
      sniff_rcvdb[path_id].start = (to_index+1)%SNIFF_WINSIZE; //complete pop-up 
    } else {
      sniff_rcvdb[path_id].start = to_index; //partial pop-up
      sniff_rcvdb[path_id].records[to_index].seq_start = to_seqnum+1;
    }
  }
}

/* looking at ethernet headers */
void my_callback(u_char *args,const struct pcap_pkthdr* pkthdr,const u_char* packet){
  u_int16_t type = handle_ethernet(args,pkthdr,packet);
  
  if(type == ETHERTYPE_IP) { 
    /* handle IP packet */
    handle_IP(args,pkthdr,packet);
  }else if(type == ETHERTYPE_ARP) { 
    /* handle arp packet */    
  }else if(type == ETHERTYPE_REVARP){ 
    /* handle reverse arp packet */    
  }else {
    /* handle_ethernet returned -1 */
  }
}

u_int16_t handle_IP(u_char *args, const struct pcap_pkthdr* pkthdr, const u_char* packet){
  const struct my_ip* ip;
  const struct tcphdr *tp;
  const struct udphdr *up;
  u_char *cp;
  u_int length = pkthdr->len;
  u_int caplen = pkthdr->caplen;
  u_short len, hlen, version, tcp_hlen, ack_bit;
  u_long  seq_start, seq_end, ack_seq, ip_src, ip_dst;
  int path_id, record_id, msecs, end;
  sniff_path *path;

  /* jump pass the ethernet header */
  ip = (struct my_ip*)(packet + sizeof(struct ether_header));
  length -= sizeof(struct ether_header); 
  caplen -= sizeof(struct ether_header); 

  /* check if we have captured enough to continue parse */
  if (caplen < sizeof(struct my_ip)) {
    printf("Error: truncated ip header - %d bytes missing \n",sizeof(struct my_ip)-caplen);
    return -1;
  }

  len     = ntohs(ip->ip_len);
  hlen    = IP_HL(ip); /* header length */
  version = IP_V(ip);  /* ip version */

  /* check version, we will take care of ipv6 later */
  if(version != 4){
    fprintf(stdout,"Error: unknown version %d\n",version);
    return -1;
  }

  /* check header length */
  if(hlen < 5 ){
    fprintf(stdout,"Error: bad ip header length: %d \n",hlen);
    return -1;
  }

  /* see if we have as much packet as we should */
  if(length < len) {
    printf("Error: truncated ip - %d bytes missing\n",len - length);
    return -1;
  }

  /* Check only the unfragmented datagram or the last fragment */
  /* Note: assume all fregments have the same header fields except the segmentation offset and flag */
  if((ntohs(ip->ip_off) & IP_MF) != 1 ) { /* aka the 14 bit != 1 */
    ip_src = ip->ip_src.s_addr;
    ip_dst = ip->ip_dst.s_addr;

    if (flag_debug){    
      //Note:inet_ntoa returns the same string if called twice in one line due to static string buffer
      fprintf(stdout,"IP src:%s ", inet_ntoa(ip->ip_src));
      fprintf(stdout,"dst:%s hlen:%d version:%d len:%d\n",inet_ntoa(ip->ip_dst),hlen,version,len);
    }
    /*jump pass the ip header */
    cp = (u_char *)ip + (hlen * 4);
    caplen -=  (hlen * 4); 
    length -=  (hlen * 4); 

    switch (ip->ip_p) {
    case IPPROTO_TCP:
      if (caplen < sizeof(struct tcphdr)) {
	printf("Error: truncated tcp header - %d bytes missing\n", sizeof(struct tcphdr)-caplen);
	return -1;
      }	    

      tp = (struct tcphdr *)cp;
      tcp_hlen  = ((tp)->doff & 0x000f);
      length   -= (tcp_hlen * 4); //jump pass the tcp header
      seq_start = ntohl(tp->seq);      
      seq_end   = seq_start+length;
      ack_bit= ((tp)->ack  & 0x0001);

      path_id = search_rcvdb(ip_dst);
      if (path_id != -1) { //a monitored outgoing packet  
	path = &(sniff_rcvdb[path_id]);

	//ignore the pure outgoing ack
	if ((ack_bit==1) && (seq_end==seq_start)) {
	  return 0;
	}

	//find the real received end index
	if (path->end == path->start){ //no previous packet
	  return push_sniff_rcvdb(path_id, seq_start, seq_end, &(pkthdr->ts)); //new packet	
	} else {
	  end  = (path->end-1)%SNIFF_WINSIZE;
	  /* Note: we discard resent-packet records for the delay estimation because 
	   * TCP don't use them to calculate the sample RTT in the RTT estimation */
	  //if the packet has no payload
	  if (seq_end == seq_start) {
	    if ((path->records[end].seq_end==path->records[end].seq_start) &&  (path->records[end].seq_end==seq_end)) {
	      //the last packet also has no payload and has the same seqnum
	      pop_sniff_rcvdb(path_id, seq_end); //pure resent
	    } else { 
	      return push_sniff_rcvdb(path_id, seq_start, seq_end, &(pkthdr->ts)); //new packet
	    }	  
	  } else if (seq_start >= path->records[end].seq_end) { //new packet
	    return push_sniff_rcvdb(path_id, seq_start, seq_end, &(pkthdr->ts));
	  } else {
	    if (seq_end > path->records[end].seq_end){ //partial resend
	      pop_sniff_rcvdb(path_id, path->records[end].seq_end-1);
	      return push_sniff_rcvdb(path_id, path->records[end].seq_end+1, seq_end, &(pkthdr->ts));
	    } else { //pure resend
	      pop_sniff_rcvdb(path_id, seq_end-1);
	    }	  
	  } // if has payload and resent   
	}
   
      } else {
	path_id = search_rcvdb(ip_src);
	if (path_id != -1) { //a monitored incoming packet
	  if (ack_bit == 1) { //has an acknowledgement
	    ack_seq  = ntohl(tp->ack_seq);
	    record_id = search_sniff_rcvdb(path_id, ack_seq-1);
	    if (record_id != -1) {
	      msecs = floor((pkthdr->ts.tv_usec-sniff_rcvdb[path_id].records[record_id].captime.tv_usec)/1000.0+0.5);
	      delays[path_id] = (pkthdr->ts.tv_sec-sniff_rcvdb[path_id].records[record_id].captime.tv_sec)*1000 + msecs;
	      pop_sniff_rcvdb(path_id, ack_seq-1);
	    } //ack in rcvdb
	  } //has ack
	} //if incoming
      } //if outgoing

      if (flag_debug) {
	printf("TCP start_seq:%lu end_seq:%lu length:%d\n", seq_start, seq_end, length);
	printf("TCP ack_seq:%lu ack_flag:%d hlen:%d\n", ack_seq, ack_bit, tcp_hlen);      
      }
      break;
    case IPPROTO_UDP:
      up = (struct udphdr *)cp;
      //sport = ntohs(up->source); //uh_sport in BSD
      //dport = ntohs(up->dest);   //uh_dport in BSD
      //printf("UDP source port: %d dest port: %d \n", sport, dport);
      if (flag_debug) printf("A UDP packet captured.\n");
      break;
    case IPPROTO_ICMP:
      if (flag_debug) printf("A ICMP packet captured.\n");
      break;
    default:
      if (flag_debug) printf("An unknow packet captured: %d \n", ip->ip_p);
      break;
    } //switch
  } //if unfragmented or last fragment

  return 0;
}

/* handle ethernet packets, much of this code gleaned from
 * print-ether.c from tcpdump source
 */
u_int16_t handle_ethernet (u_char *args,const struct pcap_pkthdr* pkthdr,const u_char* packet){
    u_int caplen = pkthdr->caplen;
    u_int length = pkthdr->len;
    struct ether_header *eptr;  /* net/ethernet.h */
    u_short ether_type;

    if (caplen < ETHER_HDRLEN) {
        fprintf(stdout,"Error: Captured packet length less than ethernet header length\n");
        return -1;
    }

    /* lets start with the ether header... */
    eptr = (struct ether_header *) packet;
    ether_type = ntohs(eptr->ether_type);

    if (flag_debug) {
      /* Lets print SOURCE DEST TYPE LENGTH */
      fprintf(stdout,"ETH: ");
      fprintf(stdout,"%s ",ether_ntoa((struct ether_addr*)eptr->ether_shost));
      fprintf(stdout,"%s ",ether_ntoa((struct ether_addr*)eptr->ether_dhost));

      /* check to see if we have an ip packet */
      if (ether_type == ETHERTYPE_IP){
	fprintf(stdout,"(IP)");
      }else if (ether_type == ETHERTYPE_ARP){
	fprintf(stdout,"(ARP)");
      }else  if (eptr->ether_type == ETHERTYPE_REVARP){
	fprintf(stdout,"(RARP)");
      }else {
	fprintf(stdout,"(?)");
      }
      fprintf(stdout," %d\n",length); //total ethernet packet length
    } //if
    return ether_type;
}

void init_pcap(int to_ms) {
    char *dev; 
    char errbuf[PCAP_ERRBUF_SIZE];
    struct bpf_program fp;      /* hold compiled program     */
    bpf_u_int32 maskp;          /* subnet mask               */
    bpf_u_int32 netp;           /* ip                        */
    char string_filter[128];
    //struct in_addr addr;

    dev = "vnet"; //"eth0";

    /* ask pcap for the network address and mask of the device */
    pcap_lookupnet(dev,&netp,&maskp,errbuf);
    //For an unknown reason, netp has the wrong 4th number
    //addr.s_addr = netp;
    sprintf(string_filter, "port %d and tcp", SENDER_PORT);

    /* open device for reading. 
     * NOTE: We use non-promiscuous */
    descr = pcap_open_live(dev, BUFSIZ, 0, to_ms, errbuf);
    if(descr == NULL) { 
      printf("Error: pcap_open_live(): %s\n",errbuf); 
      exit(1); 
    }

 
    // Lets try and compile the program, optimized 
    if(pcap_compile(descr, &fp, string_filter, 1, maskp) == -1) {
      fprintf(stderr,"Error: calling pcap_compile\n"); 
      exit(1); 
    }
    // set the compiled program as the filter 
    if(pcap_setfilter(descr,&fp) == -1) {
      fprintf(stderr,"Error: setting filter\n"); 
      exit(1); 
    }

    /*
    if (pcap_setnonblock(descr, 1, errbuf) == -1){
      printf("Error: pcap_setnonblock(): %s\n",errbuf); 
      exit(1); 
    }
    */

    pcapfd = pcap_fileno(descr);
    init_sniff_rcvdb();
}

void sniff(void) { 
    u_char* args = NULL;

    /* use dispatch here instead of loop 
     * Note: no max pkt num is specified */ 
    pcap_dispatch(descr,-1,my_callback,args);
 
}


#include "stub.h"
#include "log.h"

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


sniff_path sniff_rcvdb[CONCURRENT_RECEIVERS];
pcap_t* descr;
int pcapfd;
FILE  *loss_log;

ThroughputAckState throughput[CONCURRENT_RECEIVERS];

// Returns true if sequence is between the firstUnknown and the
// nextSequence. Takes account of wraparound.
int throughputInWindow(ThroughputAckState * state, unsigned int sequence)
{
  return sequence >= state->firstUnknown
         || (state->nextSequence < state->firstUnknown
             && sequence < state->nextSequence);
}

// Reset the state of a connection completely.
void throughputInit(ThroughputAckState * state, unsigned int sequence)
{
  if (state->isValid == 0)
  {
    state->firstUnknown = sequence;
    state->nextSequence = sequence;
    state->ackSize = 0;
    state->repeatSize = 0;
    gettimeofday(&state->lastTime, NULL);
    state->isValid = 1;
  }
}

// Notify the throughput monitor that a new packet has been sent
// out. This updates the expected nextSequence number.
void throughputProcessSend(ThroughputAckState * state, unsigned int sequence,
     unsigned int size)
{
  if (sequence == state->nextSequence)
  {
    if (size == 0) {
        size = 1;
    }
    state->nextSequence += size;
  }
  else if (throughputInWindow(state, sequence))
  {
    unsigned int maxRepeat = state->nextSequence - sequence;
    if (size < maxRepeat)
    {
        state->repeatSize += size;
    }
    else
    {
        state->repeatSize += maxRepeat;
        state->nextSequence += size - maxRepeat;
    }
  }
}

// Notify the throughput monitor that some bytes have been acknowledged.
void throughputProcessAck(ThroughputAckState * state, unsigned int sequence)
{
  if (! state->isValid) {
      printf("throughputProcessAck() called with invalid state\n");
  }
  if (throughputInWindow(state, sequence))
  {
    state->ackSize += sequence - state->firstUnknown + 1;
    state->firstUnknown = sequence + 1;
  }
}

// How many bytes have been acknowledged since the last call to
// throughputTick()?
unsigned int bytesThisTick(ThroughputAckState * state) {
    return state->ackSize;
}

// What is the bandwidth of the acknowledged bytes since the last call to
// throughputTick()?
unsigned int throughputTick(ThroughputAckState * state)
{
  double result = 0.0;
  double divisor = 1.0;
  struct timeval now;
  gettimeofday(&now, NULL);
  divisor = now.tv_sec - state->lastTime.tv_sec;
  divisor += (now.tv_usec - state->lastTime.tv_usec)/1000000.0;
  result = (state->ackSize * 8.0) / (divisor * 1000.0);
//  printf("ByteCount: %u\n", state->ackSize);
//  printf("UnAck ByteCount: %i (%i - %i)\n",
//          state->nextSequence - state->firstUnknown,
//          state->nextSequence, state->firstUnknown);
  state->ackSize = 0;
  state->repeatSize = 0;
  state->lastTime = now;
  return (unsigned int) result;
}

//A modulus function that returns only a non-negative remainder
//precondition: modulus > 0
int nnmod(int num, int modulus){
  int remainder, tmpint;

  if (modulus <=0){
    printf("Error: modulus is not a positive number - %d", modulus);
    exit(1);
  }
  remainder = num % modulus;
  if (remainder >= 0) {
    return remainder;
  }
  tmpint = remainder+modulus;
  if (tmpint < 0){
    printf("Error: remainder+modulus<0 - %d+%d", remainder, modulus);
    exit(1);
  }
  return tmpint;
}

//Check if the seq is in the sequence block [seq_start, seq_end)
//Take account of the seq wrap-arround
int in_sequence_block(unsigned long seq_start, unsigned long seq_end, unsigned long seq) {
  if (seq_start < seq_end) {
    if (seq_start<=seq && seq<seq_end) 
      return 1; //in range
    else 
      return 0; //out of range
  }

  if (seq_start > seq_end) {
    if (seq_start<=seq || seq<seq_end) 
      return 1; //in range
    else 
      return 0; //out of range
  }

  //seq_start == seq_end
  if (seq_start == seq) 
    return 1; //in range
  else
    return 0; //out of range 
}

void init_sniff_rcvdb(void) {
  int i;
  
  for (i=0; i<CONCURRENT_RECEIVERS; i++){
    sniff_rcvdb[i].start = 0;
    sniff_rcvdb[i].end = 0;
    throughput[i].isValid = 0;
  }
}

int push_sniff_rcvdb(int path_id, u_long start_seq, u_long end_seq, const struct timeval *ts){
  struct in_addr addr;
  sniff_path *path;
  int next;

  path = &(sniff_rcvdb[path_id]);
  next = path->end;
  //The circular buffer is full when the start and end pointers are back to back
  if (nnmod(next-(path->start), SNIFF_WINSIZE) == (SNIFF_WINSIZE-1)){
    addr.s_addr =rcvdb[path_id].ip;
    printf("Error: circular buffer is full for the path to %s \n", inet_ntoa(addr));
    return -1;
  }
  path->records[next].seq_start = start_seq;
  path->records[next].seq_end   = end_seq;
  path->records[next].captime.tv_sec  = ts->tv_sec;
  path->records[next].captime.tv_usec = ts->tv_usec;
  path->end=nnmod(next+1, SNIFF_WINSIZE);
  return 0;
}


int search_sniff_rcvdb(int path_id, u_long seqnum) {
  sniff_path *path = &(sniff_rcvdb[path_id]);
  int next = path->start;

  while (next != (path->end)){
    if (in_sequence_block(path->records[next].seq_start, path->records[next].seq_end, seqnum)){
      return next;
    }
    next = nnmod(next+1, SNIFF_WINSIZE);
  }
  return -1;
}

void pop_sniff_rcvdb(int path_id, u_long to_seqnum){
  int to_index = search_sniff_rcvdb(path_id, to_seqnum);
  if (to_index != -1) {
    //if the packet has no payload or the last sent seqnum equals the pop number
    if ((sniff_rcvdb[path_id].records[to_index].seq_end==sniff_rcvdb[path_id].records[to_index].seq_start) 
      || (((unsigned long)(sniff_rcvdb[path_id].records[to_index].seq_end-1)) == to_seqnum)) {
      sniff_rcvdb[path_id].start = nnmod(to_index+1, SNIFF_WINSIZE); //complete pop-up 
    } else {
      sniff_rcvdb[path_id].start = to_index; //partial pop-up
      sniff_rcvdb[path_id].records[to_index].seq_start = ((unsigned long)(to_seqnum+1));
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
  unsigned short source_port = 0;
  unsigned short dest_port = 0;
  int path_id, record_id, msecs, end, flag_resend=0;
  sniff_path *path;
//  struct in_addr debug_addr;

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
      caplen   -= (tcp_hlen * 4); //jump pass the tcp header
      seq_start = ntohl(tp->seq);      
      seq_end   = ((unsigned long)(seq_start+length));
      ack_bit= ((tp)->ack & 0x0001);
      source_port = htons(tp->source);
      dest_port = htons(tp->dest);

//      path_id = search_rcvdb(ip_dst);

      // If there is a fake entry, the stub_port entry will be the
      // destination port requested by the command line.
//      debug_addr.s_addr = ip_src;
//      printf("ip_src: %s ", inet_ntoa(debug_addr));
//      debug_addr.s_addr = ip_dst;
//      printf("ip_dst: %s, dest_port: %d, source_port: %d\n",
//	     inet_ntoa(debug_addr), dest_port, source_port);
      path_id = find_by_stub_port(ip_dst, dest_port);
//      printf("outgoing path_id: %d\n", path_id);
      if (path_id == -1 || rcvdb[path_id].source_port != 0
	  || rcvdb[path_id].dest_port != 0)
      {
	// I contacted the receiver. Therefore, my port is unique and
	// the receiver's port is fixed. The destination is the
	// receiver, therefore my port is the one that is of interest.
	path_id = find_by_stub_port(ip_dst, source_port);
//	printf("stub path_id (outgoing): %d\n", path_id);
      }
      if (path_id != -1) { //a monitored outgoing packet
        //ignore the pure outgoing ack
        if ((ack_bit==1) && (seq_end==seq_start)) {
          return 0;
        }

	path = &(sniff_rcvdb[path_id]);
	loss_records[path_id].total_counter++;

	if (path->end == path->start){ //no previous packet
	  throughputInit(&throughput[path_id], seq_start);
	  throughputProcessSend(&throughput[path_id], seq_start, length);
	  return push_sniff_rcvdb(path_id, seq_start, seq_end, &(pkthdr->ts)); //new packet	
	} else {
	  throughputProcessSend(&throughput[path_id], seq_start, length);
	  //find the real received end index
	  end  = nnmod(path->end-1, SNIFF_WINSIZE);

	  /* Note: we use flag_resend to igore resend-affected-packets in the delay estimation 
	   * because TCP don't use them to calculate the sample RTT in the RTT estimation */

	  //if the packet has no payload
	  if (seq_end == seq_start) {
	    if ((path->records[end].seq_end==path->records[end].seq_start) &&  (path->records[end].seq_end==seq_end)) {
	      //the last packet also has no payload and has the same seqnum
	      flag_resend = 1; //pure resent
	      loss_records[path_id].loss_counter++;
	      fprintf(loss_log, "Resent: %lu\n",seq_end); //loss log
	      fflush(loss_log); //loss log
	    } else { 
	      return push_sniff_rcvdb(path_id, seq_start, seq_end, &(pkthdr->ts)); //new packet
	    }	  
	  } else if (seq_start >= path->records[end].seq_end) { //new packet
	    return push_sniff_rcvdb(path_id, seq_start, seq_end, &(pkthdr->ts));
	  } else { //resend
            flag_resend = 1;
            loss_records[path_id].loss_counter++;
	    if (seq_end > path->records[end].seq_end){ //partial resend
	      fprintf(loss_log, "Resent: %lu to %lu\n",seq_start, path->records[end].seq_end-1); //loss log
	      fflush(loss_log); //loss log
	      return push_sniff_rcvdb(path_id, path->records[end].seq_end+1, seq_end, &(pkthdr->ts));
	    }
	    fprintf(loss_log, "Resent: %lu to %lu\n",seq_start, seq_end-1); //loss log
	    fflush(loss_log); //loss log
	  } // if has payload and resent   
	}
   
      } else {
//	path_id = search_rcvdb(ip_src);
	// If there is a fake entry, and the packet is incoming, then
	// the source_port will be the remote port requested on the
	// command line.
	path_id = find_by_stub_port(ip_src, source_port);
//	printf("incoming path_id: %d\n", path_id);

	if (path_id == -1 || rcvdb[path_id].source_port != 0
	    || rcvdb[path_id].dest_port != 0)
	{
	  // I contacted the receiver, so my port is unique and their
	  // port is the same every time. This means that if a packet is
	  // coming from them, the destination port is the one of
	  // interest.
	  path_id = find_by_stub_port(ip_src, dest_port);
//	  printf("stub path_id (incoming): %d\n", path_id);
	}
	if (path_id != -1) { //a monitored incoming packet
	  if (ack_bit == 1) { //has an acknowledgement
	    ack_seq  = ntohl(tp->ack_seq);	   

	    throughputProcessAck(&throughput[path_id], ack_seq);

	    record_id = search_sniff_rcvdb(path_id, (unsigned long)(ack_seq-1));
	    if (record_id != -1) { //new ack received
	      if (flag_resend) { //if the ack is triggered by a resend, skip the delay calculation.
		flag_resend = 0; 
	      } else { //calculate the delay
		msecs = floor((pkthdr->ts.tv_usec-sniff_rcvdb[path_id].records[record_id].captime.tv_usec)/1000.0+0.5);
		delays[path_id] = (pkthdr->ts.tv_sec-sniff_rcvdb[path_id].records[record_id].captime.tv_sec)*1000 + msecs;
		append_delay_sample(path_id, delays[path_id]);
		logWrite(DELAY_DETAIL, &(pkthdr->ts),
			 "Delay: %lu", delays[path_id]);
	      }
	      pop_sniff_rcvdb(path_id, (unsigned long)(ack_seq-1)); //advance the sniff window base
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
  } else { //if unfragmented or last fragment
      printf("Got an incomplete fragment\n");
  }

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

void init_pcap(int to_ms, unsigned short port, char * device, int is_live) {
    char errbuf[PCAP_ERRBUF_SIZE];

    if (flag_debug) {
      printf("The sniff_interface: %s \n", device);
    }
    if (is_live)
    {
      logWrite(MAIN_LOOP, NULL,
	       "Initializing pcap live interface with device %s, and port %d",
	       device, port);
      // We are running using live data. device refers to the network
      // device.
      struct bpf_program fp;      /* hold compiled program     */
      bpf_u_int32 maskp;          /* subnet mask               */
      bpf_u_int32 netp;           /* ip                        */
      char string_filter[128];

      /* ask pcap for the network address and mask of the device */
      pcap_lookupnet(device, &netp, &maskp, errbuf);
      //For an unknown reason, netp has the wrong 4th number
      //addr.s_addr = netp;
      sprintf(string_filter, "port %d and tcp", port);

      /* open device for reading. 
       * NOTE: We use non-promiscuous */
      descr = pcap_open_live(device, BUFSIZ, 0, to_ms, errbuf);
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
    }
    else
    {
      logWrite(MAIN_LOOP, NULL, "Initializing pcap replay interface");
      // We are running offline using data recorded by tcpdump
      // earlier. device is the filename. If device == '-', then we
      // read from stdin.
      descr = pcap_open_offline(device, errbuf);
      if(descr == NULL) { 
	printf("Error: pcap_open_offline(): %s\n", errbuf); 
	exit(1); 
      }
    }

    pcapfd = pcap_get_selectable_fd(descr);
    if (pcapfd == -1)
    {
      fprintf(stderr, "Error: pcap file descriptor is not selectable\n");
      exit(1);
   }
    init_sniff_rcvdb();

    loss_log = fopen("loss.log", "w"); //loss log

}

void sniff(void) { 
    u_char* args = NULL;

    /* use dispatch here instead of loop 
     * Note: no max pkt num is specified */ 
    pcap_dispatch(descr,-1,my_callback,args);
 
}


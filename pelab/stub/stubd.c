/****************************************************************************
 *  stubd.c: PlanetLab emulator stub program
 *  Copyright (C) 2005-2006
 *  Junxing Zhang
 *  Flux research group, University of Utah
 *
 ****************************************************************************/


#include "stub.h"

//Global  
short  flag_debug;
connection rcvdb[CONCURRENT_RECEIVERS];
unsigned long delays[CONCURRENT_RECEIVERS]; //delay is calculated at the sender side
unsigned long last_delays[CONCURRENT_RECEIVERS];
loss_record loss_records[CONCURRENT_RECEIVERS]; //loss is calculated at the sender side
unsigned long last_loss_rates[CONCURRENT_RECEIVERS]; //loss per billion

connection snddb[CONCURRENT_SENDERS];
unsigned long throughputs[CONCURRENT_SENDERS], last_throughputs[CONCURRENT_SENDERS];
fd_set read_fds,write_fds;

void init(void) {
  int i;
  
  for (i=0; i<CONCURRENT_RECEIVERS; i++){
    rcvdb[i].valid = 0;
    loss_records[i].loss_counter=0;
    loss_records[i].total_counter=0;
    last_loss_rates[i]=0;
    delays[i]=0;
    last_delays[i]=0;
  }
  for (i=0; i<CONCURRENT_SENDERS; i++){
    snddb[i].valid = 0;
  }
}

int insert_db(unsigned long ip, int sockfd, int dbtype) {
  int i, record_number, next = -1;
  time_t now  = time(NULL); 
  double thisdiff, maxdiff = 0;
  connection *db;
  
  if (dbtype == 0 ) {
    db = rcvdb;
    record_number = CONCURRENT_RECEIVERS;
  } else {
    db = snddb;
    record_number = CONCURRENT_SENDERS;
  }

  //find an unused entry or LRU entry
  for (i=0; i<record_number; i++){
    if (db[i].valid == 0) {
      next = i;
      break;
    } else {
      thisdiff = difftime(now, db[i].last_usetime);
      if (thisdiff > maxdiff) {
        maxdiff = thisdiff;
        next    = i;
      }
    }
  }
  if (db[next].valid == 1) {
    if (dbtype == 0 ) { 
      //if it is a rcvdb record, reset the corresponding sniff_rcvdb record
      sniff_rcvdb[next].start= sniff_rcvdb[next].end;
    }
    FD_CLR(db[next].sockfd, &read_fds);
    close(db[next].sockfd);
  }
  db[next].valid = 1;
  db[next].ip    = ip;
  db[next].sockfd= sockfd;
  db[next].last_usetime = now;
  return next;
}

int search_rcvdb(unsigned long indexip){
  int i;

  for (i=0; i<CONCURRENT_RECEIVERS; i++){
    if (rcvdb[i].valid==1 && rcvdb[i].ip == indexip) {
      rcvdb[i].last_usetime = time(NULL);
      return i;
    } 
  }
  return -1; //no sockfd is -1
}

void clean_exit(int code){
  int i;

  for (i=0; i<CONCURRENT_RECEIVERS; i++){
    if (rcvdb[i].valid == 1){
      close(rcvdb[i].sockfd); 
    } 
  }
  for (i=0; i<CONCURRENT_SENDERS; i++){
    if (snddb[i].valid == 1){
      close(snddb[i].sockfd); 
    } 
  }
  exit(code);
}

int get_rcvdb_index(unsigned long destaddr){
  int dbindex, sockfd;
  struct sockaddr_in their_addr;  // connector's address information 

  if ((dbindex=search_rcvdb(destaddr)) == -1) {
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("socket");
      clean_exit(1);
    }

    their_addr.sin_family = AF_INET;    // host byte order 
    their_addr.sin_port = htons(SENDER_PORT);  // short, network byte order 
    their_addr.sin_addr.s_addr = destaddr;
    memset(&(their_addr.sin_zero), '\0', 8);  // zero the rest of the struct 
    if (flag_debug) printf("Try to connect to %s \n", inet_ntoa(their_addr.sin_addr));

    if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
      perror("connect");
      clean_exit(1);
    }
    dbindex=insert_db(destaddr, sockfd, 0); //insert rcvdb
  }
  return dbindex;
}

void print_header(char *buf){
  int i, j, len=14;

  printf("Buffer header len: %d \n", len);
  for (i=0; i<len; i++){
    j = buf[i] & 0x000000ff;
    printf("%d: %u \n", i, buf[i]);
  }
}

int recv_all(int sockfd, char *buf, int size){
  int numbytes, nleft;
  char *next;

  next = buf;
  nleft= size;
  while (nleft > 0) {
    if ((numbytes=recv(sockfd, next, nleft, 0)) == -1) {
      perror("recv_all(): recv");
      clean_exit(1);
    }      
    if ( numbytes == 0) return 0;
    nleft -= numbytes;
    next  += numbytes;
  }
  if (flag_debug) printf("recv_all(): from sockfd %d \n", sockfd);
  return size;
}

int send_all(int sockfd, char *buf, int size) { 
  int total = 0; // how many bytes we have sent 
  int bytesleft = size; // how many we have left to send 
  int n; 

  while(total < size) { 
    if ((n = send(sockfd, buf+total, bytesleft, 0)) == -1) {   
      if (errno == ECONNRESET) { //receivor closed the connection
	return 0;
      }
      perror("send_all(): send");
      clean_exit(1);      
    } 
    total += n; 
    bytesleft -= n; 
  } 
  if (flag_debug) printf("send_all(): to sockfd %d \n", sockfd);
  return size; 
}

void receive_sender(int i) {
  char inbuf[MAX_PAYLOAD_SIZE];
  //unsigned long tmpulong, sndsec, sndusec;
  //struct timeval rcvtime; 

  if (recv_all(snddb[i].sockfd, inbuf, MAX_PAYLOAD_SIZE)== 0) { //connection closed
    snddb[i].valid = 0;
    FD_CLR(snddb[i].sockfd, &read_fds);
  } else {
    /* outdated since we use sniff for delay measurement now
    gettimeofday(&rcvtime, NULL);
    memcpy(&tmpulong, inbuf, SIZEOF_LONG);
    sndsec = ntohl(tmpulong);
    memcpy(&tmpulong, inbuf+SIZEOF_LONG, SIZEOF_LONG);
    sndusec = ntohl(tmpulong);
    delays[i] = (rcvtime.tv_sec-sndsec)*1000+floor((rcvtime.tv_usec-sndusec)/1000+0.5);
    if (flag_debug) printf("One Way Delay (msec): %ld \n",delays[i]);
    */
  }
}

void send_receiver(unsigned long destaddr, char *buf){
  int i, index;
  int sockfd;
  //struct timeval sendtime;
  //unsigned long tmpulong;


  index = get_rcvdb_index(destaddr);
  sockfd= rcvdb[index].sockfd;
  srandom(getpid());
  for (i=0; i<MAX_PAYLOAD_SIZE; i++) buf[i]=(char)(random()&0x000000ff);

  /* outdated since we use sniff for delay measurement now
   * put the send time at the first eight bytes
  gettimeofday(&sendtime, NULL);
  tmpulong = htonl(sendtime.tv_sec);
  memcpy(buf, &tmpulong,  SIZEOF_LONG);
  tmpulong = htonl(sendtime.tv_usec);
  memcpy(buf+SIZEOF_LONG, &tmpulong,  SIZEOF_LONG);
  */

  //send packets
  while (send_all(sockfd, buf, MAX_PAYLOAD_SIZE) == 0){ //rcv conn closed
    rcvdb[index].valid = 0;
    FD_CLR(rcvdb[index].sockfd, &read_fds);
    index = get_rcvdb_index(destaddr);
    sockfd= rcvdb[index].sockfd;
  }
}

int receive_monitor(int sockfd) {
  char buf[MAX_PAYLOAD_SIZE];
  char *nextptr;
  unsigned long tmpulong, destnum, destaddr;
  int i;

  //receive first two longs
  if (recv_all(sockfd, buf, 2*SIZEOF_LONG)==0) {
    return 0;
  }
  nextptr = buf+SIZEOF_LONG;
  memcpy(&tmpulong, nextptr, SIZEOF_LONG);
  destnum = ntohl(tmpulong);

  //return success if no dest addr is given
  if (destnum == 0){
    return 1;
  }
  //otherwise, receive dest addrs
  if (recv_all(sockfd, buf, destnum*SIZEOF_LONG)==0) {
    return 0;
  }
  nextptr=buf;
  for (i=0; i<destnum; i++){
    memcpy(&tmpulong, nextptr, SIZEOF_LONG);
    destaddr = tmpulong; //address should stay in Network Order!
    nextptr += SIZEOF_LONG;   
    send_receiver(destaddr, buf);
  } //for
  return 1;
}

int send_monitor(int sockfd) {
  char outbuf_delay[3*SIZEOF_LONG], outbuf_loss[3*SIZEOF_LONG];
  unsigned long tmpulong, loss_rate;
  int i;

  tmpulong = htonl(CODE_DELAY);
  memcpy(outbuf_delay+SIZEOF_LONG, &tmpulong, SIZEOF_LONG);
  tmpulong = htonl(CODE_LOSS);
  memcpy(outbuf_loss+SIZEOF_LONG, &tmpulong, SIZEOF_LONG);
  for (i=0; i<CONCURRENT_RECEIVERS; i++){
    if (rcvdb[i].valid == 1) {
      //send delay
      if (delays[i] != last_delays[i]) {
	memcpy(outbuf_delay, &(rcvdb[i].ip), SIZEOF_LONG); //the receiver ip
	tmpulong = htonl(delays[i]);
	memcpy(outbuf_delay+SIZEOF_LONG+SIZEOF_LONG, &tmpulong, SIZEOF_LONG);
	if (send_all(sockfd, outbuf_delay, 3*SIZEOF_LONG) == 0){
	  return 0;
	}
	last_delays[i] = delays[i];	
	printf("Sent delay: %ld\n", delays[i]);	
      } //if measurement changed since last send

      //send loss
      if (loss_records[i].total_counter == 0){
	loss_rate = 0;
      } else {
	loss_rate = floor(loss_records[i].loss_counter*1000000000.0f/loss_records[i].total_counter+0.5f); //loss per billion
      }
      if (loss_rate != last_loss_rates[i]) {
	memcpy(outbuf_loss, &(rcvdb[i].ip), SIZEOF_LONG); //the receiver ip
	tmpulong = htonl(loss_rate);
	memcpy(outbuf_loss+SIZEOF_LONG+SIZEOF_LONG, &tmpulong, SIZEOF_LONG);
	if (send_all(sockfd, outbuf_loss, 3*SIZEOF_LONG) == 0){
	  return 0;
	}
	last_loss_rates[i] = loss_rate;	
	printf("Sent loss: %d/%d=%ld \n", loss_records[i].loss_counter, loss_records[i].total_counter, loss_rate);	
      } //if measurement changed since last send
      loss_records[i].loss_counter=0;
      loss_records[i].total_counter=0;
      

    } //if connection is valid
  } //for 
  return 1;
}

int have_time(struct timeval *start_tvp, struct timeval *left_tvp){
  struct timeval current_tv;
  long   left_usec, past_usec;

  gettimeofday(&current_tv, NULL);
  past_usec = (current_tv.tv_sec-start_tvp->tv_sec)*1000000+ 
    (current_tv.tv_usec-start_tvp->tv_usec);
  left_usec = QUANTA*1000-past_usec; //QUANTA is in msec
  if (left_usec > 0) {
    left_tvp->tv_sec = left_usec/1000000;
    left_tvp->tv_usec= left_usec%1000000;
    return 1;
  }
  return 0;
}

int main(void) {
  int sockfd_snd, sockfd_rcv_sender, sockfd_rcv_monitor, sockfd_monitor=-1;  
  struct sockaddr_in my_addr;	// my address information
  struct sockaddr_in their_addr; // connector's address information
  fd_set read_fds_copy, write_fds_copy;
  socklen_t sin_size;
  struct timeval start_tv, left_tv;
  int yes=1, maxfd, i, flag_send_monitor=0;

  //set up debug flag
  if (getenv("Debug")!=NULL) 
    flag_debug=1;
  else 
    flag_debug=0;

  //set up the sender connection listener
  if ((sockfd_rcv_sender = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }
  if (setsockopt(sockfd_rcv_sender, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }  
  my_addr.sin_family = AF_INET;		  // host byte order
  my_addr.sin_port = htons(SENDER_PORT);  // short, network byte order
  my_addr.sin_addr.s_addr = INADDR_ANY;   // automatically fill with my IP
  memset(&(my_addr.sin_zero), '\0', 8);   // zero the rest of the struct
  if (flag_debug) printf("Listen on %s\n",inet_ntoa(my_addr.sin_addr));  
  if (bind(sockfd_rcv_sender, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
    perror("bind");
    exit(1);
  }  
  if (listen(sockfd_rcv_sender, PENDING_CONNECTIONS) == -1) {
    perror("listen");
    exit(1);
  }

  //set up the monitor connection listener
  if ((sockfd_rcv_monitor = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }
  if (setsockopt(sockfd_rcv_monitor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }  
  my_addr.sin_family = AF_INET;		  // host byte order
  my_addr.sin_port = htons(MONITOR_PORT); // short, network byte order
  my_addr.sin_addr.s_addr = INADDR_ANY;   // automatically fill with my IP
  memset(&(my_addr.sin_zero), '\0', 8);   // zero the rest of the struct
  if (flag_debug) printf("Listen on %s\n",inet_ntoa(my_addr.sin_addr));  
  if (bind(sockfd_rcv_monitor, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
    perror("bind");
    exit(1);
  }  
  if (listen(sockfd_rcv_monitor, 1) == -1) {
    perror("listen");
    exit(1);
  }

  //initialization
  init();
  init_pcap(SNIFF_TIMEOUT);
  FD_ZERO(&read_fds);
  FD_ZERO(&read_fds_copy);
  FD_ZERO(&write_fds);
  FD_ZERO(&write_fds_copy);
  FD_SET(pcapfd,  &read_fds);
  FD_SET(sockfd_rcv_sender,  &read_fds);
  FD_SET(sockfd_rcv_monitor, &read_fds);
  maxfd = pcapfd; //socket order
  sin_size = sizeof(struct sockaddr_in);

  //main loop - the stubd runs forever
  while (1) {
    flag_send_monitor=0; //reset flag for each quanta
    gettimeofday(&start_tv, NULL); //reset start time for each quanta

    //while in a quanta
    while(have_time(&start_tv, &left_tv)) {  
      read_fds_copy  = read_fds;
      write_fds_copy = write_fds;

      if (select(maxfd+1, &read_fds_copy, &write_fds_copy, NULL, &left_tv) == -1) { 
	perror("select"); 
	clean_exit(1); 
      }
      
      //handle new sender packets
      for (i=0; i<CONCURRENT_SENDERS; i++){
	if (snddb[i].valid==1 && FD_ISSET(snddb[i].sockfd, &read_fds_copy)) {	
	  receive_sender(i);
	} 
      }
      
      //handle new senders 
      if (FD_ISSET(sockfd_rcv_sender, &read_fds_copy)) { 
	if ((sockfd_snd = accept(sockfd_rcv_sender, (struct sockaddr *)&their_addr, &sin_size)) == -1) { 
	  perror("accept"); 
	  continue;
	} else {
	  insert_db(their_addr.sin_addr.s_addr, sockfd_snd, 1); //insert snddb
	  FD_SET(sockfd_snd, &read_fds); // add to master set 
	  if (sockfd_snd > maxfd) { // keep track of the maximum 
	    maxfd = sockfd_snd; 
	  } 
	  if (flag_debug) printf("server: got connection from %s\n",inet_ntoa(their_addr.sin_addr));
	} 
      }
      
      //handle the new monitor
      if (FD_ISSET(sockfd_rcv_monitor, &read_fds_copy)) {  
	if ((sockfd_monitor = accept(sockfd_rcv_monitor, (struct sockaddr *)&their_addr, &sin_size)) == -1) { 
	  perror("accept"); 
	  continue;
	} else {
	  FD_CLR(sockfd_rcv_monitor, &read_fds);  //allow only one monitor connection	  
	  FD_SET(sockfd_monitor, &read_fds);  //check the monitor connection for read
	  FD_SET(sockfd_monitor, &write_fds); //check the monitor connection for write
	  if (sockfd_monitor > maxfd) { //keep track of the maximum 
	    maxfd = sockfd_monitor; 
	  } 
	  if (flag_debug) printf("server: got connection from %s\n",inet_ntoa(their_addr.sin_addr));
	} 
      }

      //receive from the monitor
      if (sockfd_monitor!=-1 && FD_ISSET(sockfd_monitor, &read_fds_copy)) { 
	if (receive_monitor(sockfd_monitor) == 0) { //socket_monitor closed by peer
	  FD_CLR(sockfd_monitor, &read_fds); //stop checking the monitor socket
	  FD_CLR(sockfd_monitor, &write_fds);
	  sockfd_monitor = -1;
	  FD_SET(sockfd_rcv_monitor, &read_fds); //start checking the receiver control socket
	  if (sockfd_rcv_monitor > maxfd) { // keep track of the maximum 
	    maxfd = sockfd_rcv_monitor; 
	  } 	 	  
	} 
      }

      //send measurements to the monitor once in each quanta
      if (sockfd_monitor!=-1 && flag_send_monitor==0 && FD_ISSET(sockfd_monitor, &write_fds_copy)) {  	
	if (send_monitor(sockfd_monitor) == 0) { //socket_monitor closed by peer
	  FD_CLR(sockfd_monitor, &read_fds); //stop checking the monitor socket
	  FD_CLR(sockfd_monitor, &write_fds);
	  sockfd_monitor = -1;
	  FD_SET(sockfd_rcv_monitor, &read_fds); //start checking the receiver control socket
	  if (sockfd_rcv_monitor > maxfd) { // keep track of the maximum 
	    maxfd = sockfd_rcv_monitor; 
	  } 	 	  
	} else {
	  flag_send_monitor=1;
	}
      }
         
      //sniff packets
      if (FD_ISSET(pcapfd, &read_fds_copy)) { 
	sniff();     
      }

    } //while in quanta
  } //while forever
 
  return 0;
}


/****************************************************************************
 *  stubd.c: PlanetLab emulator stub program
 *  Copyright (C) 2005-2006
 *  Junxing Zhang
 *  Flux research group, University of Utah
 *
 ****************************************************************************/


#include "stub.h"

/*
 * For getopt()
 */
#include <unistd.h>
extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

void clean_exit(int);

//Global  
short  flag_debug, flag_standalone;
char sniff_interface[128];
connection rcvdb[CONCURRENT_RECEIVERS];
unsigned long delays[CONCURRENT_RECEIVERS]; //delay is calculated at the sender side
unsigned long last_delays[CONCURRENT_RECEIVERS];
loss_record loss_records[CONCURRENT_RECEIVERS]; //loss is calculated at the sender side
unsigned long last_loss_rates[CONCURRENT_RECEIVERS]; //loss per billion
int flag_testmode=0;
enum {TEST_NOTTESTING, TEST_NOTSTARTED, TEST_RUNNING, TEST_DONE } test_state;
unsigned long long total_bytes = 0;

connection snddb[CONCURRENT_SENDERS];
fd_set read_fds,write_fds;

int maxfd;

char random_buffer[MAX_PAYLOAD_SIZE];

int total_size = 0;

typedef struct packet_buffer_node_tag
{
  struct packet_buffer_node_tag * next;
  char * buffer;
  int size;
} packet_buffer_node;

typedef struct
{
  unsigned long ip;
  long delta;
  long size;
} packet_info;

packet_buffer_node * packet_buffer_head;
packet_buffer_node * packet_buffer_tail;
int packet_buffer_index;

void packet_buffer_init(void)
{
  packet_buffer_head = NULL;
  packet_buffer_tail = NULL;
  packet_buffer_index = 0;
}

void packet_buffer_cleanup(void)
{
  packet_buffer_node * old_head;
  while (packet_buffer_head != NULL)
  {
    old_head = packet_buffer_head;
    packet_buffer_head = old_head->next;
    free(old_head->buffer);
    free(old_head);
  }
  packet_buffer_tail = NULL;
  packet_buffer_index = 0;
}

// Add a buffer with data about packets to send to the end of the list.
void packet_buffer_add(char * buffer, int size)
{
  packet_buffer_node * newbuf = malloc(sizeof(packet_buffer_node));
  if (newbuf == NULL)
  {
    perror("allocate");
    clean_exit(1);
  }
  newbuf->next = NULL;
  newbuf->buffer = buffer;
  newbuf->size = size;
  if (packet_buffer_tail == NULL)
  {
    packet_buffer_head = newbuf;
    packet_buffer_tail = newbuf;
    packet_buffer_index = 0;
  }
  else
  {
    packet_buffer_tail->next = newbuf;
    packet_buffer_tail = newbuf;
  }
}

// Get info about the next packet to send
packet_info packet_buffer_front(void)
{
  packet_info result;
  if (packet_buffer_head == NULL)
  {
    printf("packet_buffer_head == NULL in front\n");
    clean_exit(1);
  }
  else
  {
    char * base = packet_buffer_head->buffer + packet_buffer_index;
    memcpy(&result.ip, base, SIZEOF_LONG);
    memcpy(&result.delta, base + SIZEOF_LONG, SIZEOF_LONG);
    result.delta = ntohl(result.delta);
    memcpy(&result.size, base + SIZEOF_LONG + SIZEOF_LONG, SIZEOF_LONG);
    result.size = ntohl(result.size);
  }
  return result;
}

// Are there any packets to get info about?
int packet_buffer_more(void)
{
  return packet_buffer_head != NULL;
}

// Move to the next packet, cleaning up as we go.
void packet_buffer_advance(void)
{
  packet_buffer_index += 3*SIZEOF_LONG;
  if (packet_buffer_index >= packet_buffer_head->size)
  {
    packet_buffer_node * old_head = packet_buffer_head;
    packet_buffer_head = old_head->next;
    packet_buffer_index = 0;
    free(old_head->buffer);
    free(old_head);
    if (packet_buffer_head == NULL)
    {
      packet_buffer_tail = NULL;
    }
  }
}

void init_random_buffer(void)
{
  int i = 0;
  srandom(getpid());
  for (i=0; i<MAX_PAYLOAD_SIZE; i++)
  {
    random_buffer[i]=(char)(random()&0x000000ff);
  }
}

//Initialize or reset state varialbes related to a receiver connection
void reset_rcv_entry(int i) {
  rcvdb[i].valid = 0;
  loss_records[i].loss_counter=0;
  loss_records[i].total_counter=0;
  last_loss_rates[i]=0;
  delays[i]=0;
  last_delays[i]=0;
}

void init(void) {
  int i;
  
  for (i=0; i<CONCURRENT_RECEIVERS; i++){
    reset_rcv_entry(i);
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
    close(db[next].sockfd);

    if (dbtype == 0 ) { //if rcvdb
      //reset related state variables
      sniff_rcvdb[next].start = 0;
      sniff_rcvdb[next].end   = 0;
      throughput[next].isValid   = 0;
      FD_CLR(db[next].sockfd, &write_fds);
      reset_rcv_entry(next);
    } else { //if snddb
      FD_CLR(db[next].sockfd, &read_fds);
    }
  }
  db[next].valid = 1;
  db[next].ip    = ip;
  db[next].sockfd= sockfd;
  db[next].last_usetime = now;
  db[next].pending = 0;
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
  packet_buffer_cleanup();
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

    if (sockfd > maxfd) {
      maxfd = sockfd;
    }
    dbindex=insert_db(destaddr, sockfd, 0); //insert rcvdb
  }
  return dbindex;
}

void remove_pending(int index)
{
    if (rcvdb[index].pending == 0)
    {
	FD_CLR(rcvdb[index].sockfd, &write_fds);
    }
}

void add_pending(int index, int size)
{
    if (rcvdb[index].pending == 0 && size > 0)
    {
	FD_SET(rcvdb[index].sockfd, &write_fds);
    }
    rcvdb[index].pending += size;
}

void try_pending(int index, fd_set * write_fds_copy)
{
    if (rcvdb[index].pending > 0 && FD_ISSET(rcvdb[index].sockfd,
					     write_fds_copy))
    {
	int size = 0;
	int error = 0;
	if (rcvdb[index].pending > LOW_WATER_MARK)
	{
	    size = LOW_WATER_MARK;
	}
	else
	{
	    size = rcvdb[index].pending;
	}
	error = send(rcvdb[index].sockfd, random_buffer, size, 0);
	if (error == -1)
	{
	    perror("try_pending");
	    clean_exit(1);
	}
	rcvdb[index].pending -= error;
	total_size += error;
//	printf("Total: %d, Pending: %d\n", total_size, rcvdb[index].pending);
	remove_pending(index);
    }
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

  if (recv(snddb[i].sockfd, inbuf, MAX_PAYLOAD_SIZE, 0)== 0) { //connection closed
    snddb[i].valid = 0; //no additional clean-up because no other state varialbe is related
    FD_CLR(snddb[i].sockfd, &read_fds);
  }
}


void send_receiver(unsigned long destaddr, long size, fd_set * write_fds_copy){
  int index;
  int sockfd;
  int error = 1, retry=0;
  struct in_addr addr;

  index = get_rcvdb_index(destaddr);
  sockfd= rcvdb[index].sockfd;

  if (size <= 0) {
    size = 1;
  }
  if (rcvdb[index].pending > 0) {
    add_pending(index, size);
    return;
  }
  if (size > MAX_PAYLOAD_SIZE){
    add_pending(index, size - MAX_PAYLOAD_SIZE);
    size = MAX_PAYLOAD_SIZE;
  }

  error = send(sockfd, random_buffer, size, MSG_DONTWAIT);
  // Handle failed connection
  while (error == -1 && errno == ECONNRESET && retry < 3) {
    // TODO: Think hard about what resetting a connection means for sniffing
    // traffic.

    //reset the related state variables
    int pending = rcvdb[index].pending;
    sniff_rcvdb[index].start = 0;
    sniff_rcvdb[index].end   = 0;
    throughput[index].isValid   = 0;
    FD_CLR(rcvdb[index].sockfd, &write_fds);
    reset_rcv_entry(index);
    //try again
    index = get_rcvdb_index(destaddr);
    rcvdb[index].pending = pending;
    sockfd= rcvdb[index].sockfd;
    error = send(sockfd, random_buffer, size, MSG_DONTWAIT);
    retry++;
  }
  //if still disconnected, reset
  if (error == -1 && errno == ECONNRESET) {
    rcvdb[index].valid = 0;
    addr.s_addr = destaddr;
    printf("Error: send_receiver() - failed send to %s three times. \n", inet_ntoa(addr)); 
  }
  else if (error == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    add_pending(index, size);
  }
  else if (error == -1) {
    perror("send_receiver: send");
    clean_exit(1);
  }
  else {
    total_size += error;
//    printf("Total: %d, Pending: %d\n", total_size, rcvdb[index].pending);
    add_pending(index, size - error);
  }
}

int receive_monitor(int sockfd, struct timeval * deadline) {
  char buf[MAX_PAYLOAD_SIZE];
  char *nextptr;
  unsigned long tmpulong, destnum;
  char * packet_buffer = NULL;

  //receive first two longs
  if (recv_all(sockfd, buf, 2*SIZEOF_LONG)==0) {
    return 0;
  }
  nextptr = buf+SIZEOF_LONG;
  memcpy(&tmpulong, nextptr, SIZEOF_LONG);
  destnum = ntohl(tmpulong);
  packet_buffer = malloc(destnum*3*SIZEOF_LONG);

  //return success if no dest addr is given
  if (destnum == 0){
    return 1;
  }
  //otherwise, receive dest addrs
  if (recv_all(sockfd, packet_buffer, destnum*3*SIZEOF_LONG)==0) {
    free(packet_buffer);
    return 0;
  }
  if (!packet_buffer_more())
  {
    gettimeofday(deadline, NULL);
  }
  packet_buffer_add(packet_buffer, destnum*3*SIZEOF_LONG);

//  nextptr=buf;
//  for (i=0; i<destnum; i++){
//    memcpy(&tmpulong, nextptr, SIZEOF_LONG);
//    destaddr = tmpulong; //address should stay in Network Order!
//    nextptr += SIZEOF_LONG;
//    send_receiver(destaddr, buf);
//  } //for

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
//        printf("delays: %ld last: %ld\n", delays[i], last_delays[i]);     
//        unsigned int through = throughputTick(&throughput[i]);
//        printf("throughput(kbps) = %u\n", through);
      //send delay
      if (delays[i] != last_delays[i]) {
	memcpy(outbuf_delay, &(rcvdb[i].ip), SIZEOF_LONG); //the receiver ip
	tmpulong = htonl(delays[i]);
	memcpy(outbuf_delay+SIZEOF_LONG+SIZEOF_LONG, &tmpulong, SIZEOF_LONG);
	if (send_all(sockfd, outbuf_delay, 3*SIZEOF_LONG) == 0){
	  return 0;
	}
	last_delays[i] = delays[i];	
//	printf("Sent delay: %ld\n", delays[i]);	
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
//	if (send_all(sockfd, outbuf_loss, 3*SIZEOF_LONG) == 0){
//	  return 0;
//	}
	last_loss_rates[i] = loss_rate;	
//	printf("Sent loss: %d/%d=%ld \n", loss_records[i].loss_counter, loss_records[i].total_counter, loss_rate);	
      } //if measurement changed since last send
      loss_records[i].loss_counter=0;
      loss_records[i].total_counter=0;
      

    } //if connection is valid
  } //for 
  return 1;
}

void print_measurements(void) {
  float loss_rate;
  int i;

  for (i=0; i<CONCURRENT_RECEIVERS; i++){
    if (rcvdb[i].valid == 1) {     
      // Note, this has to be done before throughputTick, since that
      // obliterates the byte count
      unsigned int bytes = bytesThisTick(&throughput[i]);
      unsigned int through = throughputTick(&throughput[i]);
      if (flag_testmode) {
          // We might need to do a state transition
          if (test_state == TEST_NOTSTARTED && bytes > 0) {
              test_state = TEST_RUNNING;
          } else if (test_state == TEST_RUNNING && bytes == 0) {
              test_state = TEST_DONE;
          }
      }

      // Decide if we're going to print this quanta
      short print;
      if (flag_testmode && test_state != TEST_RUNNING) {
          print = 0;
      } else {
          print = 1;
      }

      if (print) printf("Throughput(kbps) = %u\n", through);
      if (print) printf("Bytecount = %u\n", bytes);
      total_bytes += bytes;

      //print delay
      if (delays[i] != last_delays[i]) {
	last_delays[i] = delays[i];	
	if (print) printf("New delay: %ld\n", delays[i]);	
      } else {
	if (flag_debug) printf("Unchanged delay: %ld\n", delays[i]);	
      }

      //print loss
      if (loss_records[i].total_counter == 0){
	loss_rate = 0;
      } else {
	loss_rate = (float)loss_records[i].loss_counter/loss_records[i].total_counter; //float loss
      }
      if (loss_rate != last_loss_rates[i]) {
	last_loss_rates[i] = loss_rate;	
	if (print) printf("New loss: %d/%d=%f \n", loss_records[i].loss_counter, loss_records[i].total_counter, loss_rate);	
      } else {
	if (flag_debug) printf("Unchanged loss: %f \n", loss_rate);     
      }
      loss_records[i].loss_counter=0;
      loss_records[i].total_counter=0;      

    } //if connection is valid
  } //for 
}

void handle_packet_buffer(struct timeval * deadline, fd_set * write_fds_copy)
{
  struct timeval now;
  packet_info packet;
  gettimeofday(&now, NULL);

  if (packet_buffer_more())
  {
    packet = packet_buffer_front();
  }
  while (packet_buffer_more() && (deadline->tv_sec < now.tv_sec ||
        (deadline->tv_sec == now.tv_sec && deadline->tv_usec < now.tv_usec)))
  {
//    struct in_addr debug_temp;
//    debug_temp.s_addr = packet.ip;
//    printf("Sending packet to %s of size %ld\n", inet_ntoa(debug_temp),
//           packet.size);

    send_receiver(packet.ip, packet.size, write_fds_copy);

    packet_buffer_advance();
    if (packet_buffer_more())
    {
      packet = packet_buffer_front(); 
      deadline->tv_usec += packet.delta * 1000;
      if (deadline->tv_usec > 1000000)
      { 
        deadline->tv_sec += deadline->tv_usec / 1000000;
        deadline->tv_usec = deadline->tv_usec % 1000000; 
      } 
    }
  }
}

int have_time(struct timeval *start_tvp, struct timeval *left_tvp){
  struct timeval current_tv;
  long long   left_usec, past_usec; //64-bit integer

  gettimeofday(&current_tv, NULL);
  past_usec = ((long long)(current_tv.tv_sec-start_tvp->tv_sec))*1000000+
    (current_tv.tv_usec-start_tvp->tv_usec);
  left_usec = QUANTA*1000-past_usec; //QUANTA is in msec
  if (left_usec > 0) {
    left_tvp->tv_sec = left_usec/1000000;
    left_tvp->tv_usec= left_usec%1000000;
    return 1;
  }
  return 0;
}

void usage() {
  fprintf(stderr,"Usage: stubd [-t] [-d] [-s] <sniff-interface> [remote_IPaddr]\n");
  fprintf(stderr,"       -d:  Enable debugging mode\n");
  fprintf(stderr,"       -s:  Enable standalone mode\n");
  fprintf(stderr,"       -t:  Enable testing mode\n");
  fprintf(stderr," remote_IPaddr is mandatory when using -s\n");
}

int main(int argc, char *argv[]) {
  int sockfd_snd, sockfd_rcv_sender, sockfd_rcv_monitor, sockfd_monitor=-1;  
  struct sockaddr_in my_addr;	// my address information
  struct sockaddr_in their_addr; // connector's address information
  fd_set read_fds_copy, write_fds_copy;
  socklen_t sin_size;
  struct timeval start_tv, left_tv;
  int yes=1, i, flag_send_monitor=0;
  struct timeval packet_deadline;
  struct in_addr addr;
  int flag_measure=0;
  char ch;

  gettimeofday(&packet_deadline, NULL);
  init();

  //set up debug flag
  if (getenv("Debug")!=NULL) 
    flag_debug=1;
  else 
    flag_debug=0;
  flag_standalone = 0;

  /*
   * Process command-line arguments
   */
  while ((ch = getopt(argc,argv,"dst")) != -1) {
    switch (ch) {
      case 'd':
        flag_debug = 1; break;
      case 's':
        flag_standalone = 1; break;
      case 't':
        flag_testmode = 1; break;
      default:
        fprintf(stderr,"Unknown option %c\n",ch);
        usage(); exit(1);
    }
  }
  argc -= optind;
  argv += optind;

  if (flag_standalone) {
    if (argc != 2) {
      fprintf(stderr,"Wrong number of options for standalone: %i\n",argc);
      usage();
      exit(1);
    } else {
      flag_measure = 1;
      rcvdb[0].valid = 1;
      inet_aton(argv[1], &addr);
      rcvdb[0].ip    =  addr.s_addr;
      rcvdb[0].sockfd= -1; //show error if used
      rcvdb[0].last_usetime = time(NULL);
    }
    printf("Running in standalone mode\n");
  } else {
    if (argc != 1) {
      fprintf(stderr,"Wrong number of options: %i\n",argc);
      usage();
      exit(1);
    }
  }

  if (flag_testmode) {
      printf("Running in testmode\n");
      test_state = TEST_NOTSTARTED;
  }

  if (strlen(argv[0]) > 127) {
    fprintf(stderr,"Error: the <sniff-interface> name must be less than 127 characters \n");
    exit(1);
  }

  strcpy(sniff_interface, argv[0]);


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
  packet_buffer_init();
  init_random_buffer();
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

    printf("Total: %d\n", total_size);
    if (flag_debug) printf("quanta\n");

    //while in a quanta
    while(have_time(&start_tv, &left_tv)) {  
      read_fds_copy  = read_fds;
      write_fds_copy = write_fds;

      if (select(maxfd+1, &read_fds_copy, &write_fds_copy, NULL, &left_tv) == -1) { 
	perror("select"); 
	clean_exit(1); 
      }

      // Send out packets to our peers if the deadline has passed.
      handle_packet_buffer(&packet_deadline, &write_fds_copy);
      
      //receive from existent senders
      for (i=0; i<CONCURRENT_SENDERS; i++){
	// Send pending data if it exists.
        try_pending(i, &write_fds_copy);
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
	if (receive_monitor(sockfd_monitor, &packet_deadline) == 0) { //socket_monitor closed by peer
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

    // In testmode, we only start printing in the quanta we first see a packet
    if (flag_standalone) {
      print_measurements();
    }
    
    // If running in testmode, and the test is over, exit!
    if (flag_testmode && test_state == TEST_DONE) {
      printf("Test done - total bytes transmitted: %llu\n",total_bytes);
      break;
    }
     
  } //while forever

  packet_buffer_cleanup(); 

  return 0;
}


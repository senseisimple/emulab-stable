/****************************************************************************
 *  stubd.c: PlanetLab emulator stub program
 *  Copyright (C) 2005-2006
 *  Junxing Zhang
 *  Flux research group, University of Utah
 *
 ****************************************************************************/

#include "stub.h"
#include "log.h"

/*
 * For getopt()
 */
#include <unistd.h>
extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

//Global  
int is_live = 1;

short  flag_debug, flag_standalone;
connection rcvdb[CONCURRENT_RECEIVERS];
unsigned long delays[CONCURRENT_RECEIVERS]; //delay is calculated at the sender side
unsigned long last_delays[CONCURRENT_RECEIVERS];
unsigned long delay_count[CONCURRENT_RECEIVERS];
delay_record delay_records[CONCURRENT_RECEIVERS]; //delay list is calculated at the sender side
loss_record loss_records[CONCURRENT_RECEIVERS]; //loss is calculated at the sender side
unsigned long last_loss_rates[CONCURRENT_RECEIVERS]; //loss per billion
int last_through[CONCURRENT_RECEIVERS]; 
int buffer_full[CONCURRENT_RECEIVERS];
int flag_testmode=0;
int bandwidth_method = BANDWIDTH_VEGAS;
enum {TEST_NOTTESTING, TEST_NOTSTARTED, TEST_RUNNING, TEST_DONE } test_state;
unsigned long long total_bytes = 0;

connection snddb[CONCURRENT_SENDERS];
fd_set read_fds,write_fds;


int maxfd;

char random_buffer[MAX_PAYLOAD_SIZE];

int total_size = 0;

char * ipToString(unsigned long ip)
{
  struct in_addr address;
  address.s_addr = ip;
  return inet_ntoa(address);
}

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
  unsigned short type;
  long value;
  unsigned short source_port;
  unsigned short dest_port;
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

void read_packet_info(packet_info * dest, char * source)
{

  // Get ip
  memcpy(&dest->ip, source, SIZEOF_LONG);
  source += SIZEOF_LONG;

  // Get source port
  memcpy(&dest->source_port, source, sizeof(dest->source_port));
  dest->source_port = ntohs(dest->source_port);
  source += sizeof(dest->source_port);

  // Get dest port
  memcpy(&dest->dest_port, source, sizeof(dest->dest_port));
  dest->dest_port = ntohs(dest->dest_port);
  source += sizeof(dest->dest_port);

  // Get delta time
  memcpy(&dest->delta, source, SIZEOF_LONG);
  dest->delta = ntohl(dest->delta);
  source += SIZEOF_LONG;

  // Get value
  memcpy(&dest->value, source, SIZEOF_LONG);
  dest->value = ntohl(dest->value);
  source += SIZEOF_LONG;

  // Get type
  memcpy(&dest->type, source, sizeof(dest->type));
  dest->type = ntohs(dest->type);
  source += sizeof(dest->type);
}

// Get info about the next packet to send
packet_info packet_buffer_front(void)
{
  packet_info result;
  static char * lastAddress = NULL;
  char * thisAddress = packet_buffer_head->buffer + packet_buffer_index;
  if (packet_buffer_head == NULL)
  {
    printf("packet_buffer_head == NULL in front\n");
    clean_exit(1);
  }
  else
  {
    read_packet_info(&result,
		     packet_buffer_head->buffer + packet_buffer_index);
  }
  if (thisAddress != lastAddress)
  {
      logWrite(PACKET_BUFFER_DETAIL, NULL,
	       "Looking at packet: type(%hu) value(%lu) delta(%lu) "
	       "source_port(%hu) dest_port(%hu) ip(%s)",
	       result.type, result.value, result.delta, result.source_port,
	       result.dest_port, ipToString(result.ip));
  }
  lastAddress = thisAddress;
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
  packet_buffer_index += MONITOR_RECORD_SIZE;
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

packet_info * write_buffer = NULL;
int write_buffer_size = 0;
int write_buffer_index = 0;

void init_random_buffer(void)
{
  int i = 0;
  srandom(getpid());
  for (i=0; i<MAX_PAYLOAD_SIZE; i++)
  {
    random_buffer[i]=(char)(random()&0x000000ff);
  }
}

//Append a delay sample to the tail of the ith delay-record queue.
void append_delay_sample(int path_id, long sample_value,
			 struct timeval const * timestamp) {
  delay_sample *sample = malloc(sizeof(delay_sample));
  if (sample == NULL) {
    perror("allocate");
    clean_exit(1);
  }
  sample->next  = NULL;
  sample->value = sample_value;
  sample->time = *timestamp;
  if (delay_records[path_id].tail == NULL) {
    delay_records[path_id].head = sample;
    delay_records[path_id].tail = sample;
  } else {
    delay_records[path_id].tail->next = sample;
    delay_records[path_id].tail = sample;
  }
  delay_records[path_id].sample_number++;
}

//Copy delay samples into the given buffer.
//Input: path_id, maximum count
//Output: buffer to write samples into
//Return: void
void save_delay_samples(int path_id, char * buffer, int maxCount) {
  char *pos = buffer;
  delay_sample *current = delay_records[path_id].head;
  delay_sample * previous = NULL;
  unsigned long interval = 0;
  int count = 1;

  current = delay_records[path_id].head;
  interval = 0;
  count = 1;
  while (current != NULL && count < maxCount)
  {
    // Format: interval, value
    unsigned long tmp = htonl(interval);
    memcpy(pos, &tmp, sizeof(unsigned long));
    pos += sizeof(unsigned long);

    tmp = htonl(current->value);
    memcpy(pos, &tmp, sizeof(unsigned long));
    pos += sizeof(unsigned long);

    previous = current;
    current = current->next;
    if (current != NULL)
    {
      unsigned long msecs = (current->time.tv_usec
			     - previous->time.tv_usec)/1000;
      interval = (current->time.tv_sec - previous->time.tv_sec)*1000 + msecs;
    }
    ++count;
  }
}

void remove_delay_samples(int path_id)
{
  while (delay_records[path_id].head != NULL)
  {
    delay_sample * tmp = delay_records[path_id].head;
    free(delay_records[path_id].head);
    delay_records[path_id].head = tmp;
  }
  delay_records[path_id].head = NULL;
  delay_records[path_id].tail = NULL;
  delay_records[path_id].sample_number = 0;
}

void init(void) {
  int i;
  
  for (i = 0; i < CONCURRENT_RECEIVERS; i++)
  {
      add_empty_receiver(i);
      init_connection(& rcvdb[i]);
  }
  for (i = 0; i < CONCURRENT_SENDERS; i++)
  {
      add_empty_sender(i);
      init_connection(& snddb[i]);
  }
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
  logCleanup();
  if (write_buffer != NULL)
  {
    free(write_buffer);
  }
  exit(code);
}

int send_with_reconnect(int index, int size);

void try_pending(int index)
{
  pending_list * pending = &rcvdb[index].pending;
  struct timeval now;
  gettimeofday(&now, NULL);
  if (pending->is_pending
      && (pending->deadline.tv_sec < now.tv_sec ||
	  (pending->deadline.tv_sec == now.tv_sec
	   && pending->deadline.tv_usec < now.tv_usec)))
  {
    int size = pending->writes[pending->current_index].size;
    int error = send_with_reconnect(index, size);

    if (error == 0)
    {
      // Complete success in writing.
      pop_pending_write(index);
    }
    else if (error > 0)
    {
      // Partial success in writing.
      pending->writes[pending->current_index].size = error;
    }
    else
    {
      // Disconnected and cannot reconnect.
      // Do nothing. We don't care about this connection anymore.
    }
  }
}

void init_pending_list(int index, long size, struct timeval time)
{
  pending_list * pending = &(rcvdb[index].pending);
  if (! pending->is_pending)
  {
    pending->is_pending = 1;
    pending->deadline = time;
    pending->last_write = time;
    pending->writes[0].size = size;
    pending->writes[0].delta = 0;
    pending->current_index = 0;
    pending->free_index = 1;

    set_pending(index, &write_fds);
  }
}

void push_pending_write(int index, pending_write current)
{
  pending_list * pending = &(rcvdb[index].pending);
  if (pending->is_pending)
  {
    int used_buffer = 0;
    if (pending->free_index < pending->current_index)
    {
      used_buffer = PENDING_SIZE -
	(pending->current_index - pending->free_index);
    }
    else
    {
      used_buffer = pending->free_index - pending->current_index;
    }
    logWrite(PEER_WRITE, NULL, "Pending insert. Used buffer: %d", used_buffer);
    // If free_index is equal to current_index, then we are out of
    // space and we want to delete the cell at current_index.
    if (pending->free_index == pending->current_index)
    {
//      pop_pending_write(index);

//      pending->current_index = (pending->current_index + PENDING_SIZE/2)
//	% PENDING_SIZE;

      int delta = pending->writes[(pending->current_index + 1)
				  %PENDING_SIZE].delta
	- pending->writes[pending->current_index].delta;
      pending->deadline.tv_usec += delta * 1000;
      while (pending->deadline.tv_usec < 0)
      {
	pending->deadline.tv_sec -= 1;
	pending->deadline.tv_usec += 1000000;
      }
      while (pending->deadline.tv_usec >= 1000000)
      {
	pending->deadline.tv_sec += 1;
	pending->deadline.tv_usec -= 1000000;
      }

      pending->current_index = (pending->current_index + 1) % PENDING_SIZE;

    }
    pending->writes[pending->free_index] = current;
    pending->free_index = (pending->free_index + 1) % PENDING_SIZE;
  }
}

void pop_pending_write(int index)
{
  pending_list * pending = &(rcvdb[index].pending);
  if (pending->is_pending)
  {
    pending->current_index = (pending->current_index + 1) % PENDING_SIZE;
    if (pending->free_index == pending->current_index)
    {
      pending->is_pending = 0;
      clear_pending(index, &write_fds);
    }
    else
    {
      long seconds = pending->writes[pending->current_index].delta / 1000;
      long millis = pending->writes[pending->current_index].delta % 1000;
      pending->deadline.tv_usec += millis * 1000;
      pending->deadline.tv_sec += seconds
	+ (pending->deadline.tv_usec / 1000000);
      pending->deadline.tv_usec %= 1000000;
    }
  }
}

// Updates the last_write time on the pending_list structure and
// returns the difference in millisecond betweeen the old time and the
// new time.
long update_write_time(int index, struct timeval deadline)
{
  long result = deadline.tv_usec / 1000
    - rcvdb[index].pending.last_write.tv_usec / 1000;
  result += deadline.tv_sec * 1000
    - rcvdb[index].pending.last_write.tv_sec * 1000;
  rcvdb[index].pending.last_write = deadline;
  return result;
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
  static char inbuf[MAX_PAYLOAD_SIZE];
  int receive_count = 0;

  receive_count = recv(snddb[i].sockfd, inbuf, MAX_PAYLOAD_SIZE, 0);
  if (receive_count == 0) {
    //connection closed
    remove_sender_index(i, &read_fds);
  }
  logWrite(PEER_READ, NULL, "Read %d bytes", receive_count);
}


void send_receiver(int index, int packet_size, struct timeval deadline)
{
  if (rcvdb[index].pending.is_pending)
  {
    pending_write next;
    next.size = packet_size;
    next.delta = update_write_time(index, deadline);
    push_pending_write(index, next);
  }
  else
  {
    int error = send_with_reconnect(index, packet_size);
    if (error > 0)
    {
      // This means that there was a successful write, but
      // incomplete. So we need to set up a pending write.
      init_pending_list(index, error, deadline);
    }
  }

/*
  int sockfd;
  int error = 1, retry=0;
  struct in_addr addr;

  sockfd = rcvdb[index].sockfd;

  if (packet_size <= 0) {
    packet_size = 1;
  }
  if (rcvdb[index].pending > 0) {
    add_pending(index, packet_size);
    return;
  }
  if (packet_size > MAX_PAYLOAD_SIZE){
    add_pending(index, packet_size - MAX_PAYLOAD_SIZE);
    packet_size = MAX_PAYLOAD_SIZE;
  }

  error = send(sockfd, random_buffer, packet_size, MSG_DONTWAIT);
  logWrite(PEER_WRITE, NULL, "Wrote %d bytes", error);
  // Handle failed connection
  while (error == -1 && errno == ECONNRESET && retry < 3) {
    reconnect_receiver(index);
    sockfd= rcvdb[index].sockfd;
    error = send(sockfd, random_buffer, packet_size, MSG_DONTWAIT);
    logWrite(PEER_WRITE, NULL, "Wrote %d reconnected bytes", error);
    retry++;
  }
  //if still disconnected, reset
  if (error == -1 && errno == ECONNRESET) {
    remove_index(index, &write_fds);
    addr.s_addr = rcvdb[index].ip;
    printf("Error: send_receiver() - failed send to %s three times. \n", inet_ntoa(addr)); 
  }
  else if (error == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    add_pending(index, packet_size);
  }
  else if (error == -1) {
    perror("send_receiver: send");
    clean_exit(1);
  }
  else {
    total_size += error;
//    printf("Total: %d, Pending: %d\n", total_size, rcvdb[index].pending);
    add_pending(index, packet_size - error);
  }
*/
}

// Returns the number of bytes remaining. This means a 0 if everything
// goes OK, and a positive number if some of the bytes couldn't be
// written. Returns -1 if the connection was reset and reconnection failed.
int send_with_reconnect(int index, int size)
{
  int result = 0;
  int bytes_remaining = size;
  int done = 0;
  int error = 0;
  int sockfd = rcvdb[index].sockfd;
  if (bytes_remaining <= 0)
  {
    bytes_remaining = 1;
  }

  while (!done && bytes_remaining > 0)
  {
    int retry = 0;
    int write_size = bytes_remaining;
    if (write_size > MAX_PAYLOAD_SIZE)
    {
      write_size = MAX_PAYLOAD_SIZE;
    }

    error = send(sockfd, random_buffer, write_size, MSG_DONTWAIT);
    logWrite(PEER_WRITE, NULL, "Wrote %d bytes", error);
    // Handle failed connection
    while (error == -1 && errno == ECONNRESET && retry < 3) {
      reconnect_receiver(index);
      sockfd= rcvdb[index].sockfd;
      error = send(sockfd, random_buffer, size, MSG_DONTWAIT);
      logWrite(PEER_WRITE, NULL, "Wrote %d reconnected bytes", error);
      retry++;
    }
    //if still disconnected, reset
    if (error == -1 && errno == ECONNRESET) {
      remove_index(index, &write_fds);
      printf("Error: send_receiver() - failed send to %s three times. \n", ipToString(rcvdb[index].ip)); 
      result = -1;
    }
    else if (error == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      result = bytes_remaining;
      done = 1;
    }
    else if (error == -1) {
      perror("send_receiver: send");
      clean_exit(1);
    }
    else {
      total_size += error;
      bytes_remaining -= error;
      if (error < write_size)
      {
	done = 1;
      }
      result = bytes_remaining;
//    printf("Total: %d, Pending: %d\n", total_size, rcvdb[index].pending);
    }
  }
  return result;
}

void change_socket_buffer_size(int sockfd, int optname, int value)
{
  int newSize = 0;
  int newSizeLength = sizeof(newSize);
  int error = setsockopt(sockfd, SOL_SOCKET, optname, &value, sizeof(value));
  if (error == -1)
  {
    perror("setsockopt");
    clean_exit(1);
  }
  error = getsockopt(sockfd, SOL_SOCKET, optname, &newSize, &newSizeLength);
  if (error == -1)
  {
    perror("getsockopt verifying setsockopt");
    clean_exit(1);
  }
  logWrite(CONTROL_RECEIVE | DELAY_DETAIL, NULL,
	   "Socket buffer size is now %d", newSize);
}

void process_control_packet(packet_info packet
#ifdef USE_PACKET_BUFFER
, struct timeval deadline
#endif
){
  int index = -1;
  int sockfd = -1;

  index = insert_by_address(packet.ip, packet.source_port, packet.dest_port);
  if (index == -1)
  {
      printf("No more connection slots.\n");
      clean_exit(1);
  }
  sockfd = rcvdb[index].sockfd;
  switch(packet.type)
  {
  case PACKET_WRITE:
#ifdef USE_PACKET_BUFFER
    logWrite(CONTROL_RECEIVE, NULL, "Told to write %d bytes", packet.value);
    send_receiver(index, packet.value, deadline);
#endif
    break;
  case PACKET_SEND_BUFFER:
    logWrite(CONTROL_RECEIVE | DELAY_DETAIL, NULL,
	     "Told to set SEND buffer to %d bytes", packet.value);
    change_socket_buffer_size(sockfd, SO_SNDBUF, packet.value);
    break;
  case PACKET_RECEIVE_BUFFER:
    logWrite(CONTROL_RECEIVE | DELAY_DETAIL, NULL,
	     "Told to set RECEIVE buffer to %d bytes",
	     packet.value);
    change_socket_buffer_size(sockfd, SO_RCVBUF, packet.value);
    break;
  default:
    fprintf(stderr, "Unknown control packet code: %d\n", packet.type);
    clean_exit(1);
  }
}

int receive_monitor(int sockfd, struct timeval * deadline) {
  char buf[2*SIZEOF_LONG];
  int buffer_size = 0;
  unsigned long destnum = 0;
  char * packet_buffer = NULL;

  int i = 0;

  //receive first two longs
  if (recv_all(sockfd, buf, 2*SIZEOF_LONG)==0) {
    return 0;
  }
  memcpy(&destnum, buf + SIZEOF_LONG, SIZEOF_LONG);
  destnum = ntohl(destnum);
  //return success if no dest addr is given
  if (destnum == 0){
    return 1;
  }

  logWrite(CONTROL_RECEIVE, NULL, "Received %d control records", destnum);

  buffer_size = (int)(destnum * MONITOR_RECORD_SIZE);
  packet_buffer = malloc(buffer_size);

  //otherwise, receive dest addrs
  if (recv_all(sockfd, packet_buffer, buffer_size)==0) {
    free(packet_buffer);
    return 0;
  }
#ifdef USE_PACKET_BUFFER
  if (!packet_buffer_more())
  {
    gettimeofday(deadline, NULL);
  }
  packet_buffer_add(packet_buffer, buffer_size);
#else
  logWrite(CONTROL_RECEIVE, NULL, "Processing buffer from monitor");
  if (write_buffer != NULL)
  {
    free(write_buffer);
  }
  write_buffer = malloc(destnum * sizeof(packet_info));
  write_buffer_size = destnum;
  write_buffer_index = 0;
  for (i = 0; i < destnum; ++i)
  {
    char * packet_pos = packet_buffer + i*MONITOR_RECORD_SIZE;
    packet_info * write_pos = write_buffer + i;
    read_packet_info(write_pos, packet_pos);
    process_control_packet(*write_pos);
  }
  free(packet_buffer);
  gettimeofday(deadline, NULL);
  while (write_buffer[write_buffer_index].type != PACKET_WRITE)
  {
    write_buffer_index = (write_buffer_index + 1) % write_buffer_size;
  }
  logWrite(CONTROL_RECEIVE, NULL, "Finished processing buffer from monitor");
#endif
  
  
  return 1;
}

char * save_receiver_address(char * buf, int index)
{
  unsigned short port;
  // Insert IP address
  memcpy(buf, &(rcvdb[index].ip), SIZEOF_LONG); //the receiver ip
  buf += SIZEOF_LONG;

  // Insert source port
  port = htons(rcvdb[index].source_port);
  memcpy(buf, &port, sizeof(port));
  buf += sizeof(port);

  // Insert destination port
  port = htons(rcvdb[index].dest_port);
  memcpy(buf, &port, sizeof(port));
  buf += sizeof(port);

  return buf;
}

int send_delay_to_monitor(int monitor, int index)
{
  int buffer_size = 3*SIZEOF_LONG + 2*sizeof(unsigned short);
  char outbuf_delay[buffer_size];
  unsigned long delay = 0;
  unsigned long tmpulong;

//  delay = delays[index];
//  if (delay_count[index] > 0)
//  {
//      delay /= delay_count[index];
//  }
//  else
//  {
//      delay = last_delays[index];
//  }

  delay = base_rtt[index];
  if (delay_count[index] == 0)
  {
      delay = last_delays[index];
  }

  // If measurement changed since last send
//  if (abs((long)delays[index] - (long)last_delays[index])
//      > (long)(last_delays[index]/5)) {
  if (delay != last_delays[index])
  {
    // Insert the address info
    char * buf = save_receiver_address(outbuf_delay, index);

    logWrite(CONTROL_SEND, NULL, "Sending delay(%d) about stream(%hu:%s:%hu)",
	     delay, rcvdb[index].source_port,
	     ipToString(rcvdb[index].ip), rcvdb[index].dest_port);

    // Insert the code number for delay
    tmpulong = htonl(CODE_DELAY);
    memcpy(buf, &tmpulong, SIZEOF_LONG);
    buf += SIZEOF_LONG;

    // Insert the delay value
    tmpulong = htonl(delay);
    memcpy(buf, &tmpulong, SIZEOF_LONG);
    buf += SIZEOF_LONG;

    if (send_all(monitor, outbuf_delay, buffer_size) == 0){
      return 0;
    }

    logWrite(CONTROL_SEND, NULL, "Sending delay success");

    {
	static struct timeval earlier = {0, 0};
	struct timeval now;
	gettimeofday(&now, NULL);
	if (earlier.tv_sec != 0)
	{
	    logWrite(TCPTRACE_SEND, NULL, "RTT!orange");
	    logWrite(TCPTRACE_SEND, NULL, "RTT!line %.6f %d %.6f %d",
		     earlier.tv_sec + earlier.tv_usec/1000000000.0,
		     last_delays[index],
		     now.tv_sec + now.tv_usec/1000000000.0,
		     last_delays[index]);
	}
	logWrite(TCPTRACE_SEND, NULL, "RTT!orange");
	logWrite(TCPTRACE_SEND, NULL, "RTT!line %.6f %d %.6f %d",
		 now.tv_sec + now.tv_usec/1000000000.0,
		 last_delays[index],
		 now.tv_sec + now.tv_usec/1000000000.0,
		 delay);
	earlier = now;
    }
//  printf("Sent delay: %ld\n", delays[i]);
  }
  last_delays[index] = delay;
  delays[index] = 0;
  delay_count[index] = 0;
  base_rtt[index] = LONG_MAX;

  return 1;
}

int send_bandwidth_to_monitor(int monitor, int index)
{
  int buffer_size = 3*SIZEOF_LONG + 2*sizeof(unsigned short);
  char outbuf[buffer_size];
  unsigned long code = htonl(CODE_BANDWIDTH);
  unsigned long bandwidth;

  if (bandwidth_method == BANDWIDTH_AVERAGE) {
    bandwidth = throughputTick(&throughput[index]);
  } else if (bandwidth_method == BANDWIDTH_MAX
	     || bandwidth_method == BANDWIDTH_VEGAS) {
    bandwidth = max_throughput[index];
  } else if (bandwidth_method == BANDWIDTH_BUFFER) {
    bandwidth = throughputTick(&throughput[index]);
    if (buffer_full[index] != 1 && bandwidth <= last_through[index])
    {
      bandwidth = last_through[index];
    }
  }

  if (bandwidth != 0 && bandwidth != last_through[index]) {
    // Insert the address info
    char * buf = save_receiver_address(outbuf, index);

    logWrite(CONTROL_SEND, NULL,
	     "Sending bandwidth(%lukbps) about stream(%hu:%s:%hu)",
	     bandwidth, rcvdb[index].source_port, ipToString(rcvdb[index].ip),
	     rcvdb[index].dest_port);

    // Insert the code number for bandwidth
    memcpy(buf, &code, SIZEOF_LONG);
    buf += SIZEOF_LONG;

    // Insert the bandwidth
    bandwidth = htonl(bandwidth);
    memcpy(buf, &bandwidth, SIZEOF_LONG);
    buf += SIZEOF_LONG;

    if (send_all(monitor, outbuf, buffer_size) == 0){
      return 0;
    }

    {
	struct timeval now;
	unsigned long hostBand = ntohl(bandwidth);
	gettimeofday(&now, NULL);
	logWrite(TCPTRACE_SEND, NULL, "BANDWIDTH!purple");
	logWrite(TCPTRACE_SEND, NULL, "BANDWIDTH!line %.6f %d %.6f %d",
		 now.tv_sec + now.tv_usec/1000000000.0,
		 0,
		 now.tv_sec + now.tv_usec/1000000000.0,
		 hostBand*1000/8);
    }
    last_through[index] = bandwidth;
  }
  max_throughput[index] = 0;
  return 1;
}

int send_loss_to_monitor(int monitor, int index)
{
  int buffer_size = 3*SIZEOF_LONG + 2*sizeof(unsigned short);
  char outbuf_loss[buffer_size];
  unsigned long tmpulong, loss_rate;

  // Calculate loss
  if (loss_records[index].total_counter == 0){
    loss_rate = 0;
  } else {
    // Loss per billion
    loss_rate = floor(loss_records[index].loss_counter*1000000000.0f
		      /loss_records[index].total_counter+0.5f);
  }

  // If measurement changed since last send
  if (loss_rate != last_loss_rates[index]) {
    // Insert address info
    char * buf = save_receiver_address(outbuf_loss, index);

    // Insert the code number for loss
    tmpulong = htonl(CODE_LOSS);
    memcpy(buf, &tmpulong, SIZEOF_LONG);
    buf += SIZEOF_LONG;

    // Insert the loss rate
    tmpulong = htonl(loss_rate);
    memcpy(buf, &tmpulong, SIZEOF_LONG);
    buf += SIZEOF_LONG;

    if (send_all(monitor, outbuf_loss, buffer_size) == 0){
      loss_records[index].loss_counter=0;
      loss_records[index].total_counter=0;    
      return 0;
    }
    last_loss_rates[index] = loss_rate;	
    printf("Sent loss: %d/%d=%ld \n", loss_records[index].loss_counter, loss_records[index].total_counter, loss_rate);	
  }
  loss_records[index].loss_counter=0;
  loss_records[index].total_counter=0;
  return 1;
}

int send_delay_list_to_monitor(int monitor, int index)
{
  enum { maxSend = 9000 };
  enum { buffer_size = SIZEOF_LONG + 2*sizeof(unsigned short)
    + SIZEOF_LONG + 2*SIZEOF_LONG*maxSend };
  static char outbuf[buffer_size];
  unsigned long code = htonl(CODE_LIST_DELAY);
  unsigned long count = htonl(delay_records[index].sample_number);
  int sending_size = SIZEOF_LONG + 2*sizeof(unsigned short) + SIZEOF_LONG
    + 2*SIZEOF_LONG*delay_records[index].sample_number;
  int error = 1;

  if (delay_records[index].sample_number > 0)
  {
    char * buf = save_receiver_address(outbuf, index);

    logWrite(CONTROL_SEND, NULL,
	     "Sending delay list of size(%d) to monitor(%s)",
	     delay_records[index].sample_number, ipToString(rcvdb[index].ip));

    memcpy(buf, &code, SIZEOF_LONG);
    buf += SIZEOF_LONG;

    memcpy(buf, &count, SIZEOF_LONG);
    buf += SIZEOF_LONG;

    save_delay_samples(index, buf, maxSend);

    error = send_all(monitor, outbuf, sending_size) == 0;
    logWrite(CONTROL_SEND, NULL,
	     "Sending delay list finished with code(%d)", error);
  }

  remove_delay_samples(index);

  return error;
}

int send_monitor(int sockfd) {
  int result = 1;

  if (result == 1) {
    result = for_each_to_monitor(send_delay_to_monitor, sockfd);
  }
  if (result == 1) {
    result = for_each_to_monitor(send_bandwidth_to_monitor, sockfd);
  }
//  if (result == 1) {
//    result = for_each_to_monitor(send_loss_to_monitor, sockfd);
//  }
//  if (result == 1) {
//    result = for_each_to_monitor(send_delay_list_to_monitor, sockfd);
//  }
  return result;
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
  int index = 0;
  int error = 0;

#ifdef USE_PACKET_BUFFER
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

    process_control_packet(packet, *deadline);

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
#else
  if (write_buffer != NULL)
  {
    packet = write_buffer[write_buffer_index];

    while (deadline->tv_sec < now.tv_sec ||
	   (deadline->tv_sec == now.tv_sec && deadline->tv_usec < now.tv_usec))
    {
      index = insert_by_address(packet.ip, packet.source_port,
				packet.dest_port);
      if (index == -1)
      {
	printf("No more connection slots.\n");
	clean_exit(1);
      }
      logWrite(CONTROL_RECEIVE, NULL, "Told to write %d bytes", packet.value);
      error = send_with_reconnect(index, packet.value);
      if (error > 0)
      {
	// There are bytes that were unwritten.
	buffer_full[index] = 1;
      }
  
      write_buffer_index = (write_buffer_index + 1) % write_buffer_size;
      packet = write_buffer[write_buffer_index];
      while (packet.type != PACKET_WRITE)
      {
	write_buffer_index = (write_buffer_index + 1) % write_buffer_size;
	packet = write_buffer[write_buffer_index];
      }
      deadline->tv_usec += packet.delta * 1000;
      if (deadline->tv_usec > 1000000)
      { 
	deadline->tv_sec += deadline->tv_usec / 1000000;
	deadline->tv_usec = deadline->tv_usec % 1000000; 
      }
    }
  }
#endif
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
  fprintf(stderr,"       -f <filename>: Save logs into filename. By default, stderr is used.\n");
  fprintf(stderr,"       -l <option>:  Enable logging for a particular part of the stub. 'everything' eneables all logging 'nothing' disables all logging.\n");
  fprintf(stderr,"                     control-send -- Control messages sent from the stub to the monitor\n");
  fprintf(stderr,"                     control-receive -- Control messages received from the monitor\n"); 
  fprintf(stderr,"                     tcptrace-send -- Control messages sent in xplot format (for comparison with tcptrace)\n");
  fprintf(stderr,"                     tcptrace-receive -- Control messages received in xplot format (for comparison with tcptrace)\n");
  fprintf(stderr,"                     sniff-send -- Outgoing packets detected by stub-pcap\n");
  fprintf(stderr,"                     sniff-receive -- Incoming packets detected by stub-pcap\n");
  fprintf(stderr,"                     peer-write -- Writes made to other stubs\n");
  fprintf(stderr,"                     peer-read -- Reads made from other stubs\n");
  fprintf(stderr,"                     main-loop -- Print out quanta information and the stages of the main loop\n");
  fprintf(stderr,"                     lookup-db -- Manipulations of the connection db\n");
  fprintf(stderr,"                     delay-detail -- The finest grain delay measurements\n");
  fprintf(stderr,"       -b <option>:  Specify alternative bandwidth measurement algorithms.\n");
  fprintf(stderr,"                     average -- the average goodput measurement with the whole packet sizes.\n"
                 "                                (Averaged out over each quantum).\n");
  fprintf(stderr,"                     max -- the maximum instantaneous goodput measurement\n"
                 "                            (Maximum over each quantum).\n");
  fprintf(stderr,"                     vegas -- the optimization based on vegas measurement. Default.\n");
  fprintf(stderr,"                     buffer -- Use the average goodput measurement. Include only measurements when the buffer is full or when the measurements are high.\n");
  fprintf(stderr,"       -r:  Enable replay mode. This also turns on standalone mode. The device is now used as a filename.\n");
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
  int yes=1;//, flag_send_monitor=0;
  struct timeval packet_deadline;
  struct in_addr addr;
  int flag_measure=0;
  char ch;
  unsigned short standalone_port = SENDER_PORT;
  // Do we use live data? Or previously recorded data?
  FILE * logfile = NULL;
  unsigned long quantum_no=0;
  int logflags = LOG_NOTHING;
  int select_count = 0;

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
  while ((ch = getopt(argc,argv,"df:l:b:rst")) != -1) {
    switch (ch) {
      case 'd':
        flag_debug = 1; break;
      case 'f':
	if (logfile == NULL)
	{
	  logfile = fopen(optarg, "a");
	  if (logfile == NULL)
	  {
	    perror("Log fopen()");
	    exit(1);
	  }
	}
	break;
      case 'b':
	if (strcmp(optarg, "average") == 0)
	{
	  bandwidth_method = BANDWIDTH_AVERAGE;
	} else if (strcmp(optarg, "max") == 0) {
	  bandwidth_method = BANDWIDTH_MAX;
	} else if (strcmp(optarg, "vegas") == 0) {
	  bandwidth_method = BANDWIDTH_VEGAS;
	} else if (strcmp(optarg, "buffer") == 0) {
	  bandwidth_method = BANDWIDTH_BUFFER;
	} else {
	  fprintf(stderr, "Unknown bandwidth method\n");
	  usage();
	  exit(1);
	}
	break;
      case 'l':
	if (strcmp(optarg, "everything") == 0)
	{
	  logflags = LOG_EVERYTHING;
	}
	else if (strcmp(optarg, "nothing") == 0)
	{
	  logflags = LOG_NOTHING;
	}
	else if (strcmp(optarg, "control-send") == 0)
	{
	  logflags = logflags | CONTROL_SEND;
	}
	else if (strcmp(optarg, "control-receive") == 0)
	{
	  logflags = logflags | CONTROL_RECEIVE;
	}
	else if (strcmp(optarg, "tcptrace-send") == 0)
	{
	  logflags = logflags | TCPTRACE_SEND;
	}
	else if (strcmp(optarg, "tcptrace-receive") == 0)
	{
	  logflags = logflags | TCPTRACE_RECEIVE;
	}
	else if (strcmp(optarg, "sniff-send") == 0)
	{
	  logflags = logflags | SNIFF_SEND;
	}
	else if (strcmp(optarg, "sniff-receive") == 0)
	{
	  logflags = logflags | SNIFF_RECEIVE;
	}
	else if (strcmp(optarg, "peer-write") == 0)
	{
	  logflags = logflags | PEER_WRITE;
	}
	else if (strcmp(optarg, "peer-read") == 0)
	{
	  logflags = logflags | PEER_READ;
	}
	else if (strcmp(optarg, "main-loop") == 0)
	{
	  logflags = logflags | MAIN_LOOP;
	}
	else if (strcmp(optarg, "lookup-db") == 0)
	{
	  logflags = logflags | LOOKUP_DB;
	}
	else if (strcmp(optarg, "delay-detail") == 0)
	{
	    logflags = logflags | DELAY_DETAIL;
	}
	else if (strcmp(optarg, "packet-buffer-detail") == 0)
	{
	    logflags = logflags | PACKET_BUFFER_DETAIL;
	}
	else
	{
	    fprintf(stderr, "Unknown logging option %s\n", optarg);
	    usage();
	    exit(1);
	}
	break;
      case 's':
        flag_standalone = 1; break;
      case 'r':
	flag_standalone = 1;
	is_live = 0;
	break;
      case 't':
	flag_testmode = 1; break;
      default:
        fprintf(stderr,"Unknown option %c\n",ch);
        usage(); exit(1);
    }
  }
  argc -= optind;
  argv += optind;

  if (logfile == NULL)
  {
    logfile = stderr;
  }

  logInit(logfile, logflags, 1);

  if (flag_standalone) {
    if (argc != 3) {
      fprintf(stderr,"Wrong number of options for standalone: %i\n",argc);
      usage();
      exit(1);
    } else {
      flag_measure = 1;
//      rcvdb[0].valid = 1;
      standalone_port = atoi(argv[2]);
      inet_aton(argv[1], &addr);
      insert_fake(addr.s_addr, standalone_port);
//      rcvdb[0].ip    =  addr.s_addr;
//      rcvdb[0].sockfd= -1; //show error if used
//      rcvdb[0].last_usetime = time(NULL);
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

//  strcpy(sniff_interface, argv[0]);


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
//  memset(&(my_addr.sin_zero), '\0', 8);   // zero the rest of the struct
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
//  memset(&(my_addr.sin_zero), '\0', 8);   // zero the rest of the struct
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
  init_pcap(SNIFF_TIMEOUT, standalone_port, argv[0]);
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
//    flag_send_monitor=0; //reset flag for each quanta
    gettimeofday(&start_tv, NULL); //reset start time for each quanta

//    printf("Total: %d\n", total_size);
//    printf("========== Quantum %lu ==========\n", quantum_no);
    logWrite(MAIN_LOOP, NULL, "Quantum %lu", quantum_no);
    if (is_live)
    {
	update_stats();
	logWrite(MAIN_LOOP, NULL, "PCAP Received: %u Dropped: %u",
		 received_stat(), dropped_stat());
    }
    quantum_no++;

    //while in a quanta
    while(have_time(&start_tv, &left_tv)) {  
      read_fds_copy  = read_fds;
      write_fds_copy = write_fds;

      select_count = select(maxfd+1, &read_fds_copy, &write_fds_copy, NULL,
			    &left_tv);
      if (select_count == -1)
      {
	perror("select"); 
	clean_exit(1); 
      }
//      fprintf(stderr, "Select count: %d\n", select_count);
      // Send out packets to our peers if the deadline has passed.
//      logWrite(MAIN_LOOP, NULL, "Send normal packets to peers");
      handle_packet_buffer(&packet_deadline, &write_fds_copy);

      // send to destinations which are writeable and are behind.
//      logWrite(MAIN_LOOP, NULL, "Send pending packets to peers");
#ifdef USE_PACKET_BUFFER
      for_each_pending(try_pending, &write_fds_copy);
#endif

      // receive from existing senders
//      logWrite(MAIN_LOOP, NULL, "Receive packets from peers");
      for_each_readable_sender(receive_sender, &read_fds_copy);

/*
      //receive from existent senders
      for (i=0; i<CONCURRENT_SENDERS; i++){
	// Send pending data if it exists.
	if (snddb[i].valid==1 && FD_ISSET(snddb[i].sockfd, &read_fds_copy)) {	
	  receive_sender(i);
	} 
      }
*/    
      //handle new senders 
      if (FD_ISSET(sockfd_rcv_sender, &read_fds_copy)) { 
	if ((sockfd_snd = accept(sockfd_rcv_sender, (struct sockaddr *)&their_addr, &sin_size)) == -1) { 
	  perror("accept"); 
	  continue;
	} else {
	  logWrite(MAIN_LOOP, NULL, "Accept new peer (%s)",
		   inet_ntoa(their_addr.sin_addr));
	  replace_sender_by_stub_port(their_addr.sin_addr.s_addr,
				      ntohs(their_addr.sin_port), sockfd_snd,
				      &read_fds);
//	  insert_db(their_addr.sin_addr.s_addr, their_addr.sin_port,
//		    SENDER_PORT, sockfd_snd, 1); //insert snddb
//	  FD_SET(sockfd_snd, &read_fds); // add to master set 
//	  if (sockfd_snd > maxfd) { // keep track of the maximum 
//	    maxfd = sockfd_snd; 
//	  } 
	  if (flag_debug) printf("server: got connection from %s\n",inet_ntoa(their_addr.sin_addr));
	} 
      }

      //handle the new monitor
      if (FD_ISSET(sockfd_rcv_monitor, &read_fds_copy)) {  
	if ((sockfd_monitor = accept(sockfd_rcv_monitor, (struct sockaddr *)&their_addr, &sin_size)) == -1) { 
	  perror("accept"); 
	  continue;
	} else {
	  int nodelay = 1;
	  int nodelay_error = setsockopt(sockfd_monitor, IPPROTO_TCP,
					 TCP_NODELAY, &nodelay,
					 sizeof(nodelay));
	  if (nodelay_error == -1)
	  {
	    perror("setsockopt (TCP_NODELAY)");
	    clean_exit(1);
	  }
	  logWrite(MAIN_LOOP, NULL, "Accept new monitor (%s)",
		   inet_ntoa(their_addr.sin_addr));

	  FD_CLR(sockfd_rcv_monitor, &read_fds);  //allow only one monitor connection	  
	  FD_SET(sockfd_monitor, &read_fds);  //check the monitor connection for read
//	  FD_SET(sockfd_monitor, &write_fds); //check the monitor connection for write
	  if (sockfd_monitor > maxfd) { //keep track of the maximum 
	    maxfd = sockfd_monitor; 
	  } 
	  if (flag_debug) printf("server: got connection from %s\n",inet_ntoa(their_addr.sin_addr));
	} 
      }

      //receive from the monitor
      if (sockfd_monitor!=-1 && FD_ISSET(sockfd_monitor, &read_fds_copy)) {
	logWrite(MAIN_LOOP, NULL, "Receive control message from monitor");
	if (receive_monitor(sockfd_monitor, &packet_deadline) == 0) { //socket_monitor closed by peer
	  FD_CLR(sockfd_monitor, &read_fds); //stop checking the monitor socket
//	  FD_CLR(sockfd_monitor, &write_fds);
	  sockfd_monitor = -1;
	  FD_SET(sockfd_rcv_monitor, &read_fds); //start checking the receiver control socket
	  if (sockfd_rcv_monitor > maxfd) { // keep track of the maximum 
	    maxfd = sockfd_rcv_monitor; 
	  } 	 	  
	} 
      }

      //sniff packets
      if (FD_ISSET(pcapfd, &read_fds_copy)) { 
	logWrite(MAIN_LOOP, NULL, "Sniff packet stream");
	sniff();     
      }

    } //while in quanta

    //send measurements to the monitor once in each quanta
    if (sockfd_monitor!=-1)
// && FD_ISSET(sockfd_monitor, &write_fds_copy)) {  	
    {
	logWrite(MAIN_LOOP, NULL, "Send control message to monitor");
	if (send_monitor(sockfd_monitor) == 0) { //socket_monitor closed by peer
	    logWrite(MAIN_LOOP, NULL, "Message to monitor failed");
	    FD_CLR(sockfd_monitor, &read_fds); //stop checking the monitor socket
//	  FD_CLR(sockfd_monitor, &write_fds);
	    sockfd_monitor = -1;
	    FD_SET(sockfd_rcv_monitor, &read_fds); //start checking the receiver control socket
	    if (sockfd_rcv_monitor > maxfd) { // keep track of the maximum 
		maxfd = sockfd_rcv_monitor; 
	    } 	 	  
	} else {
	    logWrite(MAIN_LOOP, NULL, "Message to monitor succeeded");
//	  flag_send_monitor=1;
	}
    }
    
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


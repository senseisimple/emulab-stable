/*
** monitor.c -- a tcp client that works as the monitor 
*/

#include "stub.h"

char addr1[]  = "155.98.70.168";  //labnix18
char addr2[] = "155.98.70.161";   //labnix11
fd_set read_fds,write_fds;

void print_header(char *buf){
  int i, j, len=12;

  printf("Buffer header len: %d \n", len);
  for (i=0; i<len; i++){
    j = buf[i] & 0x000000ff;
    printf("%d: %u \n", i, j); 
  }
}

void send_stub(int sockfd, char *addr, char *buf) {
  struct in_addr address;
  unsigned long   tmpulong;

  tmpulong = htonl(127L);
  memcpy(buf, &tmpulong, SIZEOF_LONG);
  tmpulong = htonl(1L);
  memcpy(buf+SIZEOF_LONG, &tmpulong, SIZEOF_LONG);
  inet_aton(addr, &address);
  tmpulong = address.s_addr;
  if (getenv("Debug")!=NULL) {
    printf("tmpulong: %lu \n", tmpulong);
    printf("store address: %s \n", inet_ntoa(address));
  }
  memcpy(buf+SIZEOF_LONG+SIZEOF_LONG, &tmpulong, SIZEOF_LONG);
  //should use send_all()!
  if (send(sockfd, buf, 3*SIZEOF_LONG, 0) == -1){
    perror("ERROR: send_stub() - send()");
    exit(1);
  }    
}

void receive_stub(int sockfd, char *buf) {
  int numbytes;

  if ((numbytes=recv(sockfd, buf, 3*SIZEOF_LONG, 0)) == -1) {
    perror("recv");
    exit(1);
  }
    
  if (getenv("Debug")!=NULL) {
    print_header(buf);
    printf("numbytes: %d \n", numbytes);
  }    

}

int have_time(struct timeval *start_tvp, struct timeval *left_tvp){
  struct timeval current_tv;
  long   left_usec, past_usec;

  gettimeofday(&current_tv, NULL);
  past_usec = (current_tv.tv_sec-start_tvp->tv_sec)*1000000+ 
    (current_tv.tv_usec-start_tvp->tv_usec);
  left_usec = QUANTA-past_usec;
  if (left_usec > 0) {
    left_tvp->tv_sec = left_usec/1000000;
    left_tvp->tv_usec= left_usec%1000000;
    return 1;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  int sockfd1, sockfd2;  
  fd_set read_fds_copy, write_fds_copy;
  struct timeval start_tv, left_tv;
  char buf[MAX_PAYLOAD_SIZE];
  struct sockaddr_in their_addr; // connector's address information 
  int maxfd, yes=1, flag_send_stub1=0,flag_send_stub2=0 ;
  
  if ((sockfd1 = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }
  if (setsockopt(sockfd1, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }
  their_addr.sin_family = AF_INET;    // host byte order 
  their_addr.sin_port = htons(MONITOR_PORT);  // short, network byte order 
  inet_aton(addr1, &(their_addr.sin_addr));
  memset(&(their_addr.sin_zero), '\0', 8);  // zero the rest of the struct 
  if (connect(sockfd1, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
    perror("connect1");
    exit(1);
  }
  
  if ((sockfd2 = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
	    exit(1);
  }
  if (setsockopt(sockfd2, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }
  inet_aton(addr2, &(their_addr.sin_addr));
  if (connect(sockfd2, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
    perror("connect2");
    exit(1);
  }
	
  //initialization
  FD_ZERO(&read_fds);
  FD_ZERO(&read_fds_copy);
  FD_ZERO(&write_fds);
  FD_ZERO(&write_fds_copy);
  FD_SET(sockfd1, &read_fds);
  FD_SET(sockfd2, &read_fds);
  FD_SET(sockfd1, &write_fds);
  FD_SET(sockfd2, &write_fds);
  maxfd = sockfd2; //socket order

  //main loop - the monitor runs forever
  while (1) {
    gettimeofday(&start_tv, NULL); //reset start time for each quanta
    flag_send_stub1=0;
    flag_send_stub2=0;

    //while in a quanta
    while(have_time(&start_tv, &left_tv)) {  
      read_fds_copy  = read_fds;
      write_fds_copy = write_fds;

      if (select(maxfd+1, &read_fds_copy, &write_fds_copy, NULL, &left_tv) == -1) { 
	perror("select"); 
	exit(1); 
      }
      //check write
      if (flag_send_stub1==0 && FD_ISSET(sockfd1, &write_fds_copy)) {
	send_stub(sockfd1, addr2,buf);
	flag_send_stub1=1;
      } 

      if (flag_send_stub2==0 && FD_ISSET(sockfd2, &write_fds_copy)) {
	send_stub(sockfd2, addr1,buf);
	flag_send_stub2=1;
      } 

      //check read
      if (FD_ISSET(sockfd1, &read_fds_copy)) {
	receive_stub(sockfd1,buf);
      } 

      if (FD_ISSET(sockfd2, &read_fds_copy)) {
	receive_stub(sockfd2,buf);
      } 

    } //while in a quanta
  } //while forever

  close(sockfd1);
  close(sockfd2);
  return 0;
}

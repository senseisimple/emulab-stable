/*
** monitor.c -- a tcp client that works as the monitor 
*/

#include "stub.h"

char public_hostname0[128]; //= "planetlab1.cs.dartmouth.edu"; // "planet0.measure.tbres.emulab.net"; 
char public_hostname1[128]; //= "pl1.cs.utk.edu"; // "planet1.measure.tbres.emulab.net";
char public_addr0[16];
char public_addr1[16];
char private_addr0[]="10.1.0.1"; //for testing on emulab only
char private_addr1[]="10.1.0.2";
short  flag_debug;
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

  tmpulong = htonl(127L); //no use for now
  memcpy(buf, &tmpulong, SIZEOF_LONG);
  tmpulong = htonl(1L); //1 destinate
  memcpy(buf+SIZEOF_LONG, &tmpulong, SIZEOF_LONG);

  inet_aton(addr, &address);
  tmpulong = address.s_addr;
  if (flag_debug) {
    //printf("tmpulong: %lu \n", tmpulong);
    printf("send the stub a probing address: %s \n", inet_ntoa(address));
  }
  memcpy(buf+SIZEOF_LONG+SIZEOF_LONG, &tmpulong, SIZEOF_LONG);
  tmpulong = htonl(5L); //interdeparture 5 ms
  memcpy(buf+3*SIZEOF_LONG, &tmpulong, SIZEOF_LONG);
  tmpulong = htonl(80L); //packet size 80
  memcpy(buf+4*SIZEOF_LONG, &tmpulong, SIZEOF_LONG);

  //should use send_all()!
  if (send(sockfd, buf, 5*SIZEOF_LONG, 0) == -1){
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
    
  if (flag_debug) {
    print_header(buf);
    printf("numbytes: %d \n", numbytes);
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

int main(int argc, char *argv[])
{
  int sockfd0, sockfd1;  
  fd_set read_fds_copy, write_fds_copy;
  struct timeval start_tv, left_tv;
  char buf[MAX_PAYLOAD_SIZE];
  struct sockaddr_in their_addr; // connector's address information 
  int maxfd, yes=1, flag_send_stub0=0,flag_send_stub1=0 ;
  struct hostent *hp;
  struct in_addr addr;
  char *ip;

  //set up debug flag
  if (getenv("Debug")!=NULL) 
    flag_debug=1;
  else 
    flag_debug=0;

  if (argc != 3) {
    fprintf(stderr,"Usage: stub-monitor <hostname1> <hostname2>\n");
    exit(1);
  }
  strcpy(public_hostname0, argv[1]);
  strcpy(public_hostname1, argv[2]);
  if (flag_debug) {
    printf("hostname1: %s, hostname2: %s\n", public_hostname0, public_hostname1);
  }

  hp = gethostbyname(public_hostname0);
  bcopy(hp->h_addr, &addr, hp->h_length);
  ip = inet_ntoa(addr);
  strcpy(public_addr0, ip);
  if (flag_debug) {
    printf("public_addr0: %s \n", public_addr0);
  }
  hp = gethostbyname(public_hostname1);
  bcopy(hp->h_addr, &addr, hp->h_length);
  ip = inet_ntoa(addr);
  strcpy(public_addr1, ip);
  if (flag_debug) {
    printf("public_addr1: %s \n", public_addr1);
  }


  if ((sockfd0 = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }
  if (setsockopt(sockfd0, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }
  their_addr.sin_family = AF_INET;    // host byte order 
  their_addr.sin_port = htons(MONITOR_PORT);  // short, network byte order 
  inet_aton(public_addr0, &(their_addr.sin_addr)); //contact the stub through the control network!
  memset(&(their_addr.sin_zero), '\0', 8);  // zero the rest of the struct 
  if (connect(sockfd0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
    perror("connect1");
    exit(1);
  }
  
  if ((sockfd1 = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
	    exit(1);
  }
  if (setsockopt(sockfd1, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }
  inet_aton(public_addr1, &(their_addr.sin_addr));
  if (connect(sockfd1, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
    perror("connect2");
    exit(1);
  } 

  //initialization
  FD_ZERO(&read_fds);
  FD_ZERO(&read_fds_copy);
  FD_ZERO(&write_fds);
  FD_ZERO(&write_fds_copy);
  FD_SET(sockfd0, &read_fds);
  FD_SET(sockfd1, &read_fds);
  FD_SET(sockfd0, &write_fds);
  FD_SET(sockfd1, &write_fds);
  maxfd = sockfd1; //socket order

  //main loop - the monitor runs forever
  while (1) {
    gettimeofday(&start_tv, NULL); //reset start time for each quanta
    flag_send_stub0=0;
    flag_send_stub1=0;

    //while in a quanta
    while(have_time(&start_tv, &left_tv)) {  
      read_fds_copy  = read_fds;
      write_fds_copy = write_fds;

      if (select(maxfd+1, &read_fds_copy, &write_fds_copy, NULL, &left_tv) == -1) { 
	perror("select"); 
	exit(1); 
      }
      //check write
      if (flag_send_stub0==0 && FD_ISSET(sockfd0, &write_fds_copy)) {
	if (flag_debug) {
	  printf("send to: %s \n", public_addr0);
	}
	send_stub(sockfd0, private_addr1, buf); //feed the stub with the private or public address
	flag_send_stub0=1;
      } 

      if (flag_send_stub1==0 && FD_ISSET(sockfd1, &write_fds_copy)) {
	if (flag_debug) {
	  printf("send to: %s \n", public_addr1);
	}
	send_stub(sockfd1, private_addr0, buf); //feed the stub with the private or public address
	flag_send_stub1=1;
      } 

      //check read
      if (FD_ISSET(sockfd0, &read_fds_copy)) {
	if (flag_debug) {
	  printf("received from %s \n", public_addr0);
	}
	receive_stub(sockfd0,buf);
      } 

      if (FD_ISSET(sockfd1, &read_fds_copy)) {
	if (flag_debug) {
	  printf("received from %s \n", public_addr1);
	}
	receive_stub(sockfd1,buf);
      } 

    } //while in a quanta
  } //while forever

  close(sockfd0);
  close(sockfd1);
  return 0;
}

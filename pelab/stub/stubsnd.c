/*
** stubsnd.c 
*/

#include "stub.h"

connection db[CONCURRENT_RECEIVERS];

void init_db(void) {
  int i;
  
  for (i=0; i<CONCURRENT_RECEIVERS; i++){
    db[i].valid = 0;
  }
}

void insert_db(unsigned long ip, int sockfd) {
  int i, next = -1;
  time_t now  = time(NULL); 
  double thisdiff, maxdiff = 0;

  //find an unused entry or LRU entry
  for (i=0; i<CONCURRENT_RECEIVERS; i++){
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
  }
  db[next].valid = 1;
  db[next].ip    = ip;
  db[next].sockfd= sockfd;
  db[next].last_usetime = now;
}

int search_db(unsigned long indexip){
  int i;

  for (i=0; i<CONCURRENT_RECEIVERS; i++){
    if (db[i].valid==1 && db[i].ip == indexip) {
      db[i].last_usetime = time(NULL);
      return db[i].sockfd  ;
    } 
  }
  return -1; //no sockfd is -1
}

void clean_exit(int code){
  int i;

  for (i=0; i<CONCURRENT_RECEIVERS; i++){
    if (db[i].valid == 1){
      close(db[i].sockfd); 
    } 
  }
  exit(code);
}

/* -------------------------------------------------------------------
 * returns the TCP window size (on the sending buffer, SO_SNDBUF),
 * or -1 on error.
 * ------------------------------------------------------------------- */
int getsock_tcp_windowsize( int inSock )
{
  int rc;
  int theTCPWin = 0;
  socklen_t len;
  int mySock = inSock;

#ifdef SO_SNDBUF
  if ( inSock < 0 ) {
    /* no socket given, return system default
     * allocate our own new socket */
    mySock = socket( AF_INET, SOCK_STREAM, 0 );
  }

  /* send buffer -- query for buffer size */
  len = sizeof( theTCPWin );
  rc = getsockopt( mySock, SOL_SOCKET, SO_SNDBUF,
                   (char*) &theTCPWin, &len );
  if ( rc < 0 ) {
    return rc;
  }

  if ( inSock < 0 ) {
    /* we allocated our own socket, so deallocate it */
    close( mySock );
  }
#endif

  return theTCPWin;
} /* end getsock_tcp_windowsize */

void print_header(char *buf){
  int i, j, len=14;

  printf("Buffer header len: %d \n", len);
  for (i=0; i<len; i++){
    j = buf[i] & 0x000000ff;
    printf("%d: %u \n", i, j); 
  }
}

int receive_message(int sockfd_monitor, char *buf) {
  int numbytes;
  struct timeval tv;
  fd_set readset;
  unsigned long timeout;

  timeout = QUANTA/2;  //QUANTA should be even   
  tv.tv_sec = timeout/1000000;
  tv.tv_usec= timeout%1000000;
  FD_ZERO(&readset);
  FD_SET(sockfd_monitor, &readset);
  bzero(buf, MAX_PKTSIZE);
  //poll for feed-in
  if (select(sockfd_monitor+1, &readset, NULL, NULL, &tv) >0) {
    if ((numbytes=recv(sockfd_monitor, buf, MAX_PKTSIZE, 0)) == -1) {
      return -1; //no data
    }
    if (numbytes == 0){
      return 0;  //socket closed
    }   
    if (getenv("Debug")!=NULL) {
      print_header(buf);
      printf("numbytes: %d \n", numbytes);
    }
    return 1;   //received message
    
  } //if
  return -1; //no data
}

void send_message(int sockfd_monitor, char *buf) {
  char feedback[] = "test";
  struct timeval tv;
  fd_set writeset;
  unsigned long timeout;

  timeout = QUANTA/2;  //QUANTA should be even   
  tv.tv_sec = timeout/1000000;
  tv.tv_usec= timeout%1000000;
  FD_ZERO(&writeset);
  FD_SET(sockfd_monitor, &writeset);

  //poll for feed-back
  if (select(sockfd_monitor+1, NULL, &writeset, NULL, &tv) >0) {
    printf("poll receivers. \n"); //poll func
    memcpy(buf, feedback, sizeof(feedback));  
    if (send(sockfd_monitor, buf, MAX_PKTSIZE, 0) == -1){
      perror("ERROR: send_messages() - send()");
      clean_exit(1);
    }    
  } //if
}

void send_packets(unsigned long destaddr, char *packet_buffer){
  int sockfd_traffic, i;
  struct sockaddr_in their_addr;  // connector's address information 
  struct timeval application_sendtime;
  unsigned long tmpulong;

  if ((sockfd_traffic=search_db(destaddr)) == -1) {
    if ((sockfd_traffic = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("socket");
      clean_exit(1);
    }

    their_addr.sin_family = AF_INET;    // host byte order 
    their_addr.sin_port = htons(TRAFFIC_PORT);  // short, network byte order 
    their_addr.sin_addr.s_addr = destaddr;
    memset(&(their_addr.sin_zero), '\0', 8);  // zero the rest of the struct 
    if (getenv("Debug")!=NULL) printf("Try to connect to %s \n", inet_ntoa(their_addr.sin_addr));

    if (connect(sockfd_traffic, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
      perror("connect");
      clean_exit(1);
    }
    insert_db(destaddr, sockfd_traffic);
  }

  srandom(getpid());
  for (i=0; i<MAX_PKTSIZE; i++) packet_buffer[i]=(char)(random()&0x000000ff);
  //put the application send time at the first eight bytes
  gettimeofday(&application_sendtime, NULL);
  tmpulong = htonl(application_sendtime.tv_sec);
  memcpy(packet_buffer, &tmpulong, sizeof(long));
  tmpulong = htonl(application_sendtime.tv_usec);
  memcpy(packet_buffer+sizeof(long), &tmpulong, sizeof(long));

  if (send(sockfd_traffic, packet_buffer, MAX_PKTSIZE, 0) == -1){
    perror("ERROR: send_packets() - send()");
    clean_exit(1);
  }
}

void sigchld_handler(int s)
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {
  int sockfd_control, sockfd_monitor;  
  struct sigaction sa;
  char buf[MAX_PKTSIZE];
  struct sockaddr_in their_addr;  // connector's address information 
  struct sockaddr_in my_addr;	// my address information
  socklen_t sin_size;
  int   yes=1, i, longsz, rcvflag;
  unsigned long  tmpulong, destaddr, destnum;
  char  *nextptr;
  
  if ((sockfd_control = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }
  if (setsockopt(sockfd_control, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
	}
  
  my_addr.sin_family = AF_INET;		 
  my_addr.sin_port = htons(CONTROL_PORT);	
  my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
  memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct
  
  if (bind(sockfd_control, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
    perror("bind");
    exit(1);
  }
  
  if (listen(sockfd_control, 1) == -1) {
    perror("listen");
    exit(1);
  }
  sin_size = sizeof(struct sockaddr_in);
  init_db();
  
  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  longsz = sizeof(long);
  while(1) {  // main accept() loop		
    if ((sockfd_monitor = accept(sockfd_control, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
      perror("accept");
      continue;
    }
    if (getenv("Debug")!=NULL) printf("sender: got connection from %s\n",inet_ntoa(their_addr.sin_addr));		
    
    //Make the monitor socket non-blocking
    if (fcntl(sockfd_monitor, F_SETFL, O_NONBLOCK)<0){
      perror("fcntl(sockfd_monitor, F_SETFL, O_NONBLOCK):");
      exit(-1);
    }
    
    while (1) {
      rcvflag = receive_message(sockfd_monitor, buf);
      if (rcvflag == 0) break; //socket closed by the peer
      if (rcvflag > 0) {
	nextptr = buf+longsz;
	memcpy(&tmpulong, nextptr, longsz);
	destnum = ntohl(tmpulong);
	nextptr += longsz;
	for (i=0; i<destnum; i++){
	  memcpy(&tmpulong, nextptr, longsz);
	  destaddr = tmpulong; //address should stay in Network Order!
	  nextptr += longsz;
	  if (!fork()) { // this is the child process
	    close(sockfd_control); // child doesn't need the listener					 
	    close(sockfd_monitor);
	    send_packets(destaddr, buf);
	    clean_exit(0);
	  } 	    
	} //for
      } //if
      send_message(sockfd_monitor, buf);	                       
    } //while receive
    
    close(sockfd_monitor);
  }
  close(sockfd_control);
  
  return 0;
}


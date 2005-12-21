/*
** stubrcv.c 
*/

#include "stub.h"

//Global  

void print_header(char *buf){
  int i, j, len=14;

  printf("Buffer header len: %d \n", len);
  for (i=0; i<len; i++){
    j = buf[i] & 0x000000ff;
    printf("%d: %u \n", i, buf[i]);
  }
}

void receive_packets(int sockfd_snd) {
  unsigned long delay; //delay in usec
  //  unsigned long loss;  //loss*1000000
  //  unsigned long abw;   //abw in kbps
  char inbuf[MAX_PKTSIZE], outbuf[3*SIZEOF_LONG];
  int numbytes, longsz, nleft;
  unsigned long tmpulong, sndsec, sndusec, pkttype;
  struct timeval application_sendtime; 
  struct hostent *my_hostent;
  char hostname[128];
  char *next;

  longsz = sizeof(long);

  //blocking until the sender closes the socket
  while ((numbytes=recv(sockfd_snd, inbuf, longsz, 0)) != 0) {
    //read packet type first
    if ( numbytes == -1) {
      perror("recv");
      exit(1);
    }

    //get the rcv timestamp in case it is traffic
    gettimeofday(&application_sendtime, NULL);

    //check the packet type
    memcpy(&tmpulong, inbuf, longsz);
    pkttype = ntohl(tmpulong);
    if (pkttype == CODE_TRAFFIC) {
    //read the rest of the traffic packet
      next = inbuf;
      nleft= MAX_PKTSIZE-longsz;
      while (nleft > 0) {
	if ((numbytes=recv(sockfd_snd, next, nleft, 0)) == -1) {
	  perror("recv");
	  exit(1);
	}      
	nleft -= numbytes;
	next  += numbytes;
      }
      //get the traffic send time
      memcpy(&tmpulong, inbuf, longsz);
      sndsec = ntohl(tmpulong);
      memcpy(&tmpulong, inbuf+longsz, longsz);
      sndusec = ntohl(tmpulong);
      delay = (application_sendtime.tv_sec-sndsec)*1000000+
	(application_sendtime.tv_usec-sndusec);
      if (getenv("Debug")!=NULL) printf("One Way Delay (usec): %ld \n",delay);
    } else { 
      //inquiry
      gethostname(hostname, sizeof(hostname));
      if ((my_hostent=gethostbyname(hostname)) == NULL) { // get the host info 
	herror("gethostbyname"); 
	exit(1); 
      }
      memcpy(outbuf, my_hostent->h_addr, SIZEOF_LONG);
      tmpulong = htonl(CODE_DELAY);
      memcpy(outbuf+SIZEOF_LONG, &tmpulong, SIZEOF_LONG);      
      tmpulong = htonl(delay);
      memcpy(outbuf+SIZEOF_LONG+SIZEOF_LONG, &tmpulong, SIZEOF_LONG);
      if (send(sockfd_snd, outbuf, 3*SIZEOF_LONG, 0) == -1){
	perror("ERROR: send_packets() - send()");
	exit(1);
      }    
      //loss and bw come here

    } //if
  } //while
}

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(void)
{
  int sockfd_snd, sockfd_rcv;  
  struct sockaddr_in my_addr;	// my address information
  struct sockaddr_in their_addr; // connector's address information
  socklen_t sin_size;
  struct sigaction sa;
  int yes=1;

  if ((sockfd_rcv = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

  if (setsockopt(sockfd_rcv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }
  
  my_addr.sin_family = AF_INET;		 // host byte order
  my_addr.sin_port = htons(TRAFFIC_PORT);	 // short, network byte order
  my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
  memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct
  if (getenv("Debug")!=NULL) printf("Listen on %s\n",inet_ntoa(my_addr.sin_addr));
  
  if (bind(sockfd_rcv, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
    perror("bind");
    exit(1);
  }
  
  if (listen(sockfd_rcv, CONCURRENT_RECEIVERS) == -1) {
    perror("listen");
    exit(1);
  }

  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  sin_size = sizeof(struct sockaddr_in);
  while(1) {  // main accept() loop		
    if ((sockfd_snd = accept(sockfd_rcv, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
      perror("accept");
      continue;
    }
    if (getenv("Debug")!=NULL) printf("server: got connection from %s\n",inet_ntoa(their_addr.sin_addr));
    if (!fork()) { // this is the child process
      close(sockfd_rcv); // child doesn't need the listener
      receive_packets(sockfd_snd);	    
      close(sockfd_snd);
      exit(0);
    }
    close(sockfd_snd);  // parent doesn't need this
  }
  close(sockfd_rcv);
  return 0;
}


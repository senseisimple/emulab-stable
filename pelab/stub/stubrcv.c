/*
** stubrcv.c 
*/

#include "stub.h"

void print_header(char *buf){
  int i, j, len=14;

  printf("Buffer header len: %d \n", len);
  for (i=0; i<len; i++){
    j = buf[i] & 0x000000ff;
    printf("%d: %u \n", i, buf[i]);
  }
}

void receive_packets(int sockfd_snd) {
  char buf[MAX_PKTSIZE];
  int numbytes;
  unsigned long tmpulong, sndsec, sndusec, delayusec;
  struct timeval application_sendtime;

  if ((numbytes=recv(sockfd_snd, buf, MAX_PKTSIZE, 0)) == -1) {
    perror("recv");
    exit(1);
  }
  if (getenv("Debug")!=NULL) print_header(buf);

  //get the application send time at the first eight bytes
  gettimeofday(&application_sendtime, NULL);
  memcpy(&tmpulong, buf, sizeof(long));
  sndsec = ntohl(tmpulong);
  memcpy(&tmpulong, buf+sizeof(long), sizeof(long));
  sndusec = ntohl(tmpulong);
  delayusec = (application_sendtime.tv_sec-sndsec)*1000000+
    (application_sendtime.tv_usec-sndusec);
  printf("One Way Delay (usec): %ld \n",delayusec);
 
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


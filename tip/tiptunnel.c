#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


#include "capdecls.h"

int ssl   = 0;
int debug = 0;
int allowRemote = 0;

unsigned short port = 0;
int sock = 0;

unsigned short tunnelPort = 0;
int tunnelSock = 0;

char * programToLaunch = NULL;
char * hostname = NULL;

secretkey_t key;

void usage();
void loadAcl( const char * filename );
void doConnect();
void doAuthenticate();
void doCreateTunnel();
void doTunnelConnection();


int main( int argc, char ** argv )
{
  int op;

  while ((op = getopt( argc, argv, "sp:rd" )) != -1) {
    switch (op) {
      case 's': 
	ssl++; 
	break;
      case 'p':
        tunnelPort = atoi( optarg );
	break;
      case 'd':
	debug++;
	break;
      case 'r':
	allowRemote++;
	break;
    }
  }

  argc -= optind;
  argv += optind;

  if (argc < 1) {
    usage();
  }

  if (argc > 1) {
    programToLaunch = strdup( argv[1] );    
  }

  loadAcl( argv[0] );

  doConnect();
  doAuthenticate();
  doCreateTunnel();
  if (programToLaunch) {
    char portString[12];
    sprintf( portString, "%i", tunnelPort );
    if (debug) printf("Launching %s with args localhost, %s.\n",
		      programToLaunch, portString );
    if (!fork()) {
      execlp( programToLaunch, 
	      programToLaunch, "localhost", portString, NULL );
    }
  }
  doTunnelConnection();
  if (debug) { printf("tiptunnel closing.\n"); }
}

void usage()
{
  printf("Usage:\n"
	 "tiptunnel [-p <portnum>] [-d] [-r] aclfile [program]\n"
	 "-p <portnum>     specifies tunnel port number\n"
	 "-d               turns on more verbose messages\n"
	 "-r               allows connections to tunnel from non-localhost\n"
	 "\n"
	 "[program]        path of program to launch with 'localhost'\n"
	 "                 and tunnel port as arguments.\n"
	 //	 "-s\tturns on SSL (not implemented)\n"
	 //"-e <program>\tlaunches program to connect to tunnel\n"
	 //"-k\tkeeps accepting connections until killed"
	 );
  exit(-1);
}

void loadAcl( const char * filename )
{
  FILE * aclFile = fopen( filename, "r" );
  char b1[256];
  char b2[256];

  if (!aclFile) {
    perror("open");
    fprintf( stderr, "Error opening ACL file '%s'\n", filename );
    exit(-1);
  }

  if (debug) { printf("Opened ACL file '%s'\n", filename ); }
  
  bzero( &key, sizeof( key ) );

  while (fscanf(aclFile, "%s %s\n", &b1, &b2) != EOF) {
    if ( strcmp(b1, "host:") == 0 ) {
      hostname = strdup( b2 );
    } else if ( strcmp(b1, "port:") == 0 ) {
      port = atoi( b2 );
    } else if ( strcmp(b1, "keylen:") == 0 ) {
      key.keylen = atoi( b2 );
    } else if ( strcmp(b1, "key:") == 0 ) {
      strcpy( key.key, b2 );
    } else {
      fprintf(stderr, "Ignored unknown ACL: %s %s\n", b1, b2);
    }
  }

  if (!port || !hostname || !key.keylen || !strlen(key.key)) {
    fprintf(stderr, "Incomplete ACL\n");
    exit(-1);
  }

}

void doConnect()
{
  struct sockaddr_in name;
  struct hostent *he;

  name.sin_family = AF_INET;
  name.sin_port   = htons(port);

  he = gethostbyname(hostname);   
  if (!he) {
    fprintf(stderr, "Unknown hostname %s: %s\n",
	    hostname, hstrerror(h_errno));
    exit(-1);
  }
  
  memcpy ((char *)&name.sin_addr, he->h_addr, he->h_length);
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("socket");
    exit(-1);
  }

  if (connect( sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
    close( sock );
    perror("connect");
    exit(-1);
  }
}
  
void doAuthenticate()
{
  capret_t capret;

  if ( write( sock, &key, sizeof(key) ) == sizeof(key) &&
       read(sock, &capret, sizeof(capret)) > 0 ) {
    if (capret == CAPOK) {
      return;
    } else if (capret == CAPBUSY) {
      fprintf(stderr, "Tip line busy.\n");
    } else if (capret == CAPNOPERM) {
      fprintf(stderr, "You do not have permission! No TIP for you!\n");
    }
  } else {
    // write or read hosed.
    fprintf(stderr, "Error authenticating\n");
  }

  close( sock );
  exit(-1);
}


void doCreateTunnel()
{
  int i;
  struct sockaddr_in name;

  tunnelSock = socket(AF_INET, SOCK_STREAM, 0);
  if (tunnelSock < 0) { 
    perror("socket");
    exit(-1);
  }

  i = 1;
  if (setsockopt(tunnelSock, SOL_SOCKET, SO_REUSEADDR,
		 (char *)&i, sizeof(i)) < 0) {
    perror("setsockopt: SO_REUSEADDR");
    exit(-1);
  }

  name.sin_family = AF_INET;
  name.sin_addr.s_addr = INADDR_ANY;
  name.sin_port = htons( tunnelPort );
  if (bind(tunnelSock, (struct sockaddr *) &name, sizeof(name))) {
    perror("bind");
    exit(-1);
  }

  i = sizeof(name);
  if (getsockname(tunnelSock, (struct sockaddr *)&name, &i)) {
    perror("getsockname");
    exit(-1);
  }

  tunnelPort = ntohs(name.sin_port);

  printf("Listening on port %i.\n", tunnelPort);

  if (listen(tunnelSock, 1) < 0) {
    perror("listen");
    exit(-1);
  }
}

void doTunnelConnection()
{
  int conSock;
  struct sockaddr_in name;
  socklen_t namelen = sizeof( name );

acceptor:
  conSock = accept( tunnelSock, (struct sockaddr *)&name, &namelen );

  if (name.sin_family != AF_INET) {
    fprintf(stderr,"Error: non AF_INET connection.\n");
    exit(-1);
  }

  if (!allowRemote && 
      ntohl( name.sin_addr.s_addr ) != INADDR_LOOPBACK) {
    const char reject[] = 
      "Connection attempted from non-local machine (ignored.)\n"
      "Use -r switch to allow non-local connections.\n";
    write( conSock, reject, sizeof( reject ) );
    fprintf( stderr, "%s", reject );
    close( conSock );
    goto acceptor;
  }

  if (conSock < 0 ) {
    perror("accept");
    exit(-1);
  }

  while (1) {
    fd_set fds;
    char buf[4096];
    
    FD_ZERO( &fds );
    FD_SET( conSock, &fds );
    FD_SET( sock, &fds );
    
    if (select( MAX( conSock, sock ) + 1, &fds, NULL, NULL, NULL ) < 0) {
      perror("select");
      exit(-1);
    }

    if ( FD_ISSET( conSock, &fds ) ) {
      int got = read( conSock, buf, 4096);
      if (got <= 0) {
	if (got < 0) { perror("read conSock"); }
	if (debug) { printf("conSock read %i; exiting.\n", got); }
	return;
      }

      if (write( sock, buf, got ) < 0) {
	perror("write sock");
	return;
      }
    }

    if ( FD_ISSET( sock, &fds ) ) {
      int got = read( sock, buf, 4096);
      if (got <= 0) {
	if (got < 0) { perror("read sock"); }
	if (debug) { printf("sock read %i; exiting.\n", got); }
	return;
      }

      if (write( conSock, buf, got ) < 0) {
	perror("write conSock");
	return;
      }
    }
  }
}

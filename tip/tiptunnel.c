#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef WITHSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif /* WITHSSL */

#include "capdecls.h"


#define DEFAULT_PROGRAM "xterm -T TIP -e telnet %s %s"

int debug = 0;
int allowRemote = 0;

unsigned short port = 0;
int sock = 0;

unsigned short tunnelPort = 0;
int tunnelSock = 0;

char * programToLaunch = NULL;
char * hostname = NULL;

secretkey_t key;

typedef int WriteFunc( void * data, int size );
typedef int ReadFunc( void * data, int size );

WriteFunc * writeFunc;
ReadFunc  * readFunc;

void usage();
void loadAcl( const char * filename );
void doConnect();
void doAuthenticate();
void doCreateTunnel();
void doTunnelConnection();

int writeNormal( void * data, int size );
int readNormal( void * data, int size );

#ifdef WITHSSL

void initSSL();
void sslConnect();

int writeSSL( void * data, int size );
int readSSL( void * data, int size );

SSL * ssl;
SSL_CTX * ctx;

int usingSSL = 0;
char * certString = NULL;

#endif /* WITHSSL */

int main( int argc, char ** argv )
{
  int op;

  while ((op = getopt( argc, argv, "sp:rd" )) != -1) {
    switch (op) {
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
  } else {
    programToLaunch = DEFAULT_PROGRAM;
  }

  loadAcl( argv[0] );

#ifdef WITHSSL
  if (usingSSL) { 
    writeFunc = writeSSL;
    readFunc  = readSSL;
    initSSL();
    doConnect();
    sslConnect();
  } else
#endif /* WITHSSL */
  {
    /* either there is no SSL, or they're not using it. */
    writeFunc = writeNormal;
    readFunc  = readNormal;
    doConnect();
  }

  doAuthenticate();
  doCreateTunnel();

  if (programToLaunch) {
    char portString[12];
    char runString[1024];
    char * foo;

    sprintf( portString, "%i", tunnelPort );

    /*
      if (debug) printf("Launching %s with args localhost, %s.\n",
		      programToLaunch, portString );
    */

    sprintf(runString, programToLaunch, "localhost", portString);
    for (foo = runString; *foo; foo++);
    foo[0] = ' ';
    foo[1] = '&';
    foo[2] = '\0';
    if (debug) printf("Running '%s'\n", runString);
    system( runString );
    /*
    if (!fork()) {
      execlp( programToLaunch, 
	      programToLaunch, "localhost", portString, NULL );
    }
    */
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
	 "[program]        path of program to launch; default is\n"
	 "                 \"%s\"\n",
         DEFAULT_PROGRAM
	 //"-k keeps accepting connections until killed"
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
#ifdef WITHSSL
    } else if ( strcmp(b1, "ssl-server-cert:") == 0 ) {
      if (debug) { printf("Using SSL to connect to capture.\n"); }
      certString = strdup( b2 );
      usingSSL++;
#endif /* WITHSSL */
    } else {
      fprintf(stderr, "Ignored unknown ACL: %s %s\n", b1, b2);
    }
  }

  if (!port || !hostname || !key.keylen || !strlen(key.key)) {
    fprintf(stderr, "Incomplete ACL\n");
    exit(-1);
  }

}

int writeNormal( void * data, int size )
{
  return write( sock, data, size );
}

int readNormal( void * data, int size )
{
  return read( sock, data, size );
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

  if ( writeFunc( /*sock,*/ &key, sizeof(key) ) == sizeof(key) &&
       readFunc( /*sock,*/ &capret, sizeof(capret)) > 0 ) {
    switch( capret ) {
    case CAPOK: 
      return;
    case CAPBUSY:       
      fprintf(stderr, "Tip line busy.\n");
      break;
    case CAPNOPERM:
      fprintf(stderr, "You do not have permission. No TIP for you!\n");
      break;
    default:
      fprintf(stderr, "Unknown return code 0x%02x.\n", (unsigned int)capret);
      return; // XXX Final version should break here. 
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
  int i;
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

  { 
    /*
      lets pretend that we are a telnetd (RFC854).. 
      It will be crackling good fun! 

      we enable "SUPPRESS GO AHEAD" and "ECHO",
      thus (through something commonly called "kludge line mode")
      telling the client _not_ to send a line at a time (and not to echo locally.) 

      I'm actually not totally sure why this works, but I'm afraid if I 
      think too hard about it, it will stop.
    */

    const char telnet_commands[] = 
    { 
      // IAC WILL SUPPRESS GO AHEAD:
      255, 251, 3,
      // IAC WILL ECHO:
      255, 251, 1   
    };

    write( conSock, telnet_commands, sizeof( telnet_commands ) );
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

      /*  
	  we could implement some kind of search to remove 
	  "IAC DO SUPPRESS GO AHEAD" and "IAC DO ECHO" bytes from the stream,
	  or just trash any packet starting in "IAC" (255).
	  Yeah.
      */

      if ((unsigned char)buf[0] != 255) {
	if (writeFunc( /*sock,*/ buf, got ) < 0) {
	  perror("write sock");
	  return;
	}
      } 
    }

    if ( FD_ISSET( sock, &fds ) ) {
      int got = readFunc( /*sock,*/ buf, 4096);
      if (got <= 0) {
	if (got < 0) { perror("read sock"); }
	if (debug) { printf("sock read %i; exiting.\n", got); }
	return;
      }

      /*
      for(i = 0; i < got; i++ ) {
	if ((unsigned char)buf[i] > 127) {
	  printf("Special %i\n", (unsigned int)(unsigned char)buf[i] );
	}
      }
      printf("%i server->consock\n", got );
      */

      if (write( conSock, buf, got ) < 0) {
	perror("write conSock");
	return;
      }
    }
  }
}


#ifdef WITHSSL

void initSSL()
{
  SSL_library_init();
  ctx = SSL_CTX_new( SSLv23_method() );
  SSL_load_error_strings();
  
  //  if (!(SSL_CTX_load_verify_location( ctx, CA_LIST, 0 ))
}

void sslConnect()
{
  unsigned char digest[256];
  unsigned int len = sizeof( digest );
  unsigned char digestHex[512];
  X509 * peer;
  BIO * sbio;
  int i;
  
  // inform server of desire to use SSL
  {
    secretkey_t sslHintKey;
    char ret[4];

    sslHintKey.keylen = 7;
    strncpy( sslHintKey.key, "USESSL", 7 );
    write( sock, &sslHintKey, sizeof( sslHintKey ) );
    /*
    if (4 != read( sock, ret, 4 ) || 
	0 != strncmp( ret, "OKAY", 4) ) {
      fprintf( stderr, "Didnt get SSL OKAY from server.\n");
      exit(-1);
    }
    */
  } 

  ssl = SSL_new( ctx );
  SSL_set_fd( ssl, sock );

  /*
  i = 0;
  while (1) {
    char * n = SSL_get_cipher_list( ssl, i );
    if (!n) { break;}
    printf("Cipher #%i is %s\n", i, n );
    i++;
  }
  */

  // sbio = BIO_new_socket( sock, BIO_NOCLOSE );
  // SSL_set_bio( ssl, sbio, sbio );
  sleep(1);
  
  if (SSL_connect( ssl ) <= 0) {
    fprintf(stderr, "SSL Connect error.\n");
    ERR_print_errors_fp( stderr );
    exit(-1);
  }

  peer = SSL_get_peer_certificate( ssl );

  // X509_print_fp( stdout );
  // X509_digest( &peer, EVP_sha() , digest, &len );

  //X509_digest( peer, EVP_md5(), digest, &len );
  
  X509_digest( peer, EVP_sha(), digest, &len );

  for (i = 0; i < len; i++) {
    sprintf( digestHex + (i * 2), "%02x", (unsigned int) digest[i] );
  }

  if (debug) {
    printf("ACL's  cert digest: %s\n"
	   "Server cert digest: %s\n",
	   certString, 
	   digestHex );
  }

  if (0 != strcmp( certString, digestHex )) {
    fprintf(stderr, 
	    "Server does not have certificate described in ACL:\n"
	    "ACL's  cert digest: %s\n" 
	    "Server cert digest: %s\n"
	    "Possible man-in-the-middle attack. Aborting.\n\n",
	    certString,
	    digestHex );

    exit(-1);
  }
}


int writeSSL( void * data, int size )
{
  return SSL_write( ssl, data, size );
}

int readSSL( void * data, int size )
{
  return SSL_read( ssl, data, size );
}

#endif /* WITHSSL */

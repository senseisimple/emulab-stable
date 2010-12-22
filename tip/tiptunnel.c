/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2005, 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <assert.h>
#include <paths.h>

#include <sys/time.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef WITHSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif /* WITHSSL */

#include "capdecls.h"

int localmode = 0;
int uploadmode = 0;
int optionsmode = 0;
int speed = -1;

#define ACLDIR "/var/log/tiplogs"
#define DEFAULT_PROGRAM "xterm -T TIP -e telnet localhost @s"

int debug = 0;
int allowRemote = 0;

unsigned short port = 0;
int sock = 0;

unsigned short tunnelPort = 0;
int tunnelSock = 0;

char * programToLaunch = NULL;
char * hostname = NULL;

char * certfile = NULL;
char * user = NULL;

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

static void dotippipe(int capsock,int localin,int localout);

#if defined(TIPPTY)

#if defined(__FreeBSD__)
# include <libutil.h>
#endif

#if defined(linux)
# if !defined(INFTIM)
#  define INFTIM -1
# endif
# include <pty.h>
# include <utmp.h>
#endif

#include <poll.h>
#include <termios.h>

#define TIPDIR "/dev/tip"
#define POLL_HUP_INTERVAL (250) /* ms */

typedef struct {
    char data[4096];
    size_t inuse;
} buffer_t;

static void pack_buffer(buffer_t *buffer, int amount);
static void dotippty(char *nodename);

static char pidfilename[1024] = "";
static char linkpath[128] = "";

static void cleanup_atexit(void)
{
  if (strlen(pidfilename) > 0)
    unlink(pidfilename);
  if (strlen(linkpath) > 0)
    unlink(linkpath);
}

static void sigquit(int sig)
{
  exit(0);
}
#endif

int main( int argc, char ** argv )
{
  const char * name = argv[0];
  char * aclfile = (char *) NULL;
  int op;
  int oldflags;
  struct termios tios;

#if defined(LOCALBYDEFAULT) || defined(TIPPTY)
  localmode++;
#if defined(TIPPTY)
  atexit(cleanup_atexit);
#endif
#endif

  while ((op = getopt( argc, argv, "hlp:rdu:c:a:os:" )) != -1) {
    switch (op) {
      case 'h':
        usage(name);
        break;
      case 'a':
	aclfile = optarg;
        break;
      case 'l':
	localmode++;
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
#ifdef WITHSSL
      case 'u':
	user = optarg;
	uploadmode++;
	usingSSL++;
	break;
      case 's':
	speed = atoi(optarg);
	if (speed <= 0) {
	    fprintf(stderr,"Speed option must be greater than 0\n");
	    usage(name);
	}
	optionsmode++;
	usingSSL++;
	break;
      case 'c':
	certfile = optarg;
	break;
#endif
    }
  }

  argc -= optind;
  argv += optind;

  if (argc < 1) {
    usage( name );
  }

  if (argc > 1) {
    programToLaunch = strdup( argv[1] );    
  } else {
    programToLaunch = strdup( DEFAULT_PROGRAM );
  }
  if (debug) printf("Will launch program '%s'\n",programToLaunch);

  if (uploadmode && optionsmode) {
      fprintf(stderr,
	      "You cannot specify both an upload and options; "
	      "  they must be done with multiple invocations of %s",name);
      usage(name);
  }

  if (localmode) {
    if (aclfile)
      loadAcl( aclfile );
    else {
      char localAclName[1024];
      sprintf( localAclName, "%s/%s.acl", ACLDIR, argv[0] );
      loadAcl( localAclName );
    }
  } else {
    loadAcl( argv[0] );
  }

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

  if (user && (getuid() == 0)) {
    struct passwd *pw;
    struct group *gr;
    uid_t uid;
    int rc;
    
    if (sscanf(user, "%d", &uid) == 1)
      pw = getpwuid(uid);
    else
      pw = getpwnam(user);

    if (pw == NULL) {
      fprintf(stderr, "invalid user: %s %d\n", user, uid);
      exit(1);
    }
    
    if ((gr = getgrgid(pw->pw_gid)) == NULL) {
      fprintf(stderr, "invalid group: %d\n", pw->pw_gid);
      exit(1);
    }
    
    /*
     * Initialize the group list, and then flip to uid.
     */
    if (setgid(pw->pw_gid) ||
	initgroups(user, pw->pw_gid) ||
	setuid(pw->pw_uid)) {
      fprintf(stderr, "Could not become user: %s\n", user);
      exit(1);
    }
  }

  if (optionsmode)
      exit(0);

  if (uploadmode) {
    int fd = STDIN_FILENO;

    if ((strcmp(argv[1], "-") != 0) && (fd = open(argv[1], O_RDONLY)) < 0) {
      fprintf(stderr, "Cannot open file: %s\n", argv[1]);
      exit(1);
    }
    else {
      char buf[4096];
      int rc;

      while ((rc = read(fd, buf, sizeof(buf))) > 0) {
	writeFunc(buf, rc);
      }
      close(fd);
      fd = -1;
    }
    exit(0);
  }
  
  doAuthenticate();

#if defined(TIPPTY)
  if (!debug) {
    FILE *file;
    
    daemon(0, 0);
    signal(SIGINT, sigquit);
    signal(SIGTERM, sigquit);
    signal(SIGQUIT, sigquit);
    snprintf(pidfilename, sizeof(pidfilename),
	     "%s/tippty.%s.pid",
	     _PATH_VARRUN, argv[0]);
    if ((file = fopen(pidfilename, "w")) != NULL) {
      fprintf(file, "%d\n", getpid());
      fclose(file);
    }
  }

  dotippty(argv[0]);
#else
  if (localmode && strcmp(programToLaunch,"-") == 0) {
      // just hook stdin/out to connection ;)
      oldflags = fcntl(STDIN_FILENO,F_GETFL);
      oldflags |= O_NONBLOCK;
      fcntl(STDIN_FILENO,F_SETFL,oldflags);

      // also get rid of line buffering -- we do enough buffering
      // internally!
      setvbuf(stdin,NULL,_IONBF,0);
      setvbuf(stdout,NULL,_IONBF,0);

      if (isatty(STDOUT_FILENO)) {
	  tcgetattr(STDOUT_FILENO,&tios);
	  tios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	  tios.c_oflag &= ~OPOST;
	  tcsetattr(STDOUT_FILENO,TCSANOW,&tios);
      }

      oldflags = fcntl(STDOUT_FILENO,F_GETFL);
      oldflags |= O_NONBLOCK;
      fcntl(STDOUT_FILENO,F_SETFL,oldflags);

      oldflags = fcntl(sock,F_GETFL);
      oldflags |= O_NONBLOCK;
      fcntl(sock,F_SETFL,oldflags);

      dotippipe(sock,STDIN_FILENO,STDOUT_FILENO);
      exit(0);
  }

  doCreateTunnel();

  if (programToLaunch) {
    char portString[12];

    sprintf( portString, "%i", tunnelPort );

    if (localmode) {
      if (!fork()) {
	char * foo[4];
	foo[0] = "telnet";
	foo[1] = "localhost";
	foo[2] = portString;
	foo[3] = NULL;
	execvp( "telnet", foo );
	exit(666);
      }
    } else {
      char runString[1024];
      char * foo;

      for (foo = programToLaunch; *foo; foo++) {
	if (*foo == '@') { *foo = '%'; break; }
      }

      sprintf(runString, programToLaunch, portString);

      if (debug) printf("Running '%s'\n", runString);
      
      if (! fork()) {
        exit(system( runString ));
      }
    }
   
    /*
    if (!fork()) {
      execlp( programToLaunch, 
	      programToLaunch, "localhost", portString, NULL );
    }
    */
  }
  //if (localmode) { sleep(3); }
  doTunnelConnection();
  if (debug) { printf("tiptunnel closing.\n"); }
#endif
}

void usage(const char * name)
{
#if defined(LOCALBYDEFAULT)

  printf("Usage:\n"
	 "%s [-d] <pcname>\n"
	 "-d               turns on more verbose debug messages\n"
	 "<pcname>         name of machine to connect to (e.g. \"pc42\")\n",
	 name
	 );

#elif defined(TIPPTY)

  printf("Usage:\n"
	 "%s [-d] <node>\n",
	 name);
  
#else

  printf("No aclfile specified.\n"
         "If you are using a web browser, perhaps\n"
         "it is not properly configured to pass\n"
         "an argument to tiptunnel.\n\n");

  printf("Usage:\n"
	 "%s [-l] [-p <portnum>] [-d] [-r] aclfile [<program>]\n"
	 "-l               enables local mode.\n"
	 "                 acl file will be looked for in:\n"
	 "                 /var/log/tiplogs/<aclfile>.acl\n"
	 "                 (also, server certificate will not be checked)\n"
	 "-p <portnum>     specifies tunnel port number\n"
	 "-d               turns on more verbose debug messages\n"
	 "-r               allows connections to tunnel from non-localhost\n"
	 "-u <user>        upload mode\n"
	 "\n"
	 "<program>        (non-local-mode only)\n"
	 "                 path of program to launch; default is\n"
	 "                 \"%s\"\n"
         "                 ('@s' indicates where to put port number.)\n",
	 name,
         DEFAULT_PROGRAM
	 //"-k keeps accepting connections until killed"
	 );

#endif
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
    if ( strcmp(b1, "host:") == 0 || strcmp(b1, "server:") == 0 ) {
      if (!uploadmode)
	hostname = strdup( b2 );
    } else if ( strcmp(b1, "port:") == 0 || strcmp(b1, "portnum:") == 0 ) {
      if (!uploadmode)
	port = atoi( b2 );
    } else if ( strcmp(b1, "keylen:") == 0 ) {
      key.keylen = atoi( b2 );
    } else if ( strcmp(b1, "key:") == 0 || strcmp(b1, "keydata:") == 0) {
      strcpy( key.key, b2 );
#ifdef WITHSSL
    } else if ( strcmp(b1, "uphost:") == 0 ) {
      if (uploadmode)
	hostname = strdup( b2 );
    } else if ( strcmp(b1, "upport:") == 0 ) {
      if (uploadmode)
	port = atoi( b2 );
    } else if ( strcmp(b1, "ssl-server-cert:") == 0 ) {
      if (debug) { printf("Using SSL to connect to capture.\n"); }
      certString = strdup( b2 );
      usingSSL++;
#endif /* WITHSSL */
    } else {
      /* fprintf(stderr, "Ignored unknown ACL: %s %s\n", b1, b2); */
    }
  }
  fclose(aclFile);

  if (!key.keylen)
    key.keylen = strlen(key.key);

  if (!port || !hostname || !strlen(key.key)) {
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

  if (debug) { printf("Listening on port %i.\n", tunnelPort); }

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

  if (conSock == 0) {
    if (debug) { printf("Zero consock. Retrying.\n" ); }
    goto acceptor;
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

	if (buf[got - 1] == '\0') { got--; }

	/*
	for(i = 0; i < got; i++ ) {
	  if ((unsigned char)buf[i] > 127 || 
	      (unsigned char)buf[i] < 32) {
	    printf("Special to server %i\n", 
		   (unsigned int)(unsigned char)buf[i] );
	    //buf[i] = '#';
	  }
	}
	*/

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

      //printf("%i server->consock\n", got );

      if ((unsigned char)buf[0] != 255) {
	/*
	for(i = 0; i < got; i++ ) {
	  if (buf[i] == '^') { buf[i] = '!'; }
	  if ((unsigned char)buf[i] > 127 || 
	      (unsigned char)buf[i] < 32) {
	    printf("Special from server %i\n", 
		   (unsigned int)(unsigned char)buf[i] );
	    //buf[i] = '#';
	  }
	}
	*/
	if (write( conSock, buf, got ) < 0) {
	  perror("write conSock");
	  return;
	}
      }
    }
  }
}


#ifdef WITHSSL

#define DEFAULT_CERTFILE TBROOT"/etc/capture.pem"

void initSSL()
{
  SSL_library_init();
  ctx = SSL_CTX_new( SSLv23_method() );
  SSL_load_error_strings();

  if (uploadmode) {
    if (!certfile) { certfile = DEFAULT_CERTFILE; }
    
    if (SSL_CTX_use_certificate_file( ctx, certfile, SSL_FILETYPE_PEM ) <= 0) {
      fprintf( stderr, "Could not load %s as certificate file.", certfile );
      exit(1);
    }
    
    if (SSL_CTX_use_PrivateKey_file( ctx, certfile, SSL_FILETYPE_PEM ) <= 0) {
      fprintf( stderr, "Could not load %s as key file.", certfile );
      exit(1);
    }
  }
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
    if (uploadmode)
      strcpy( sslHintKey.key, "UPLOAD" );
    else if (optionsmode) {
	strcpy(sslHintKey.key,"OPTIONS ");
	if (speed > 0) {
	    // copy in the speed option safely
	    snprintf((char *)(sslHintKey.key+strlen(sslHintKey.key)),
		     sizeof(sslHintKey.key)-strlen(sslHintKey.key)-1,
		     "SPEED=%d ",speed);
	}
    }
    else
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
  // sleep(1);
  
  if (SSL_connect( ssl ) <= 0) {
    fprintf(stderr, "SSL Connect error.\n");
    ERR_print_errors_fp( stderr );
    exit(-1);
  }

  if (uploadmode || optionsmode)
    return;

  peer = SSL_get_peer_certificate( ssl );

  // X509_print_fp( stdout );
  // X509_digest( &peer, EVP_sha() , digest, &len );

  //X509_digest( peer, EVP_md5(), digest, &len );
  
  X509_digest( peer, EVP_sha(), digest, &len );

  X509_free( peer );

  for (i = 0; i < len; i++) {
    sprintf( digestHex + (i * 2), "%02x", (unsigned int) digest[i] );
  }

  if (debug) {
    printf("ACL's  cert digest: %s\n"
	   "Server cert digest: %s\n",
	   certString, 
	   digestHex );
  }

  if (/*!localmode && */0 != strcmp( certString, digestHex )) {
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

// Just do a very simple select loop that ties the remote capture
// to a pair of fds -- probably stdin/out.
static void dotippipe(int capsock,int localin,int localout) {
    fd_set rfds,rfds_master,wfds,wfds_master;
    int nfds = 0;
    struct timeval tv = { 1,0 };
    int retval, rc;
    char fromsock[1024];
    char fromlocal[1024];
    int sockrc = 0,localrc = 0,len = 0;
    char *sockhptr,*socktptr,*localhptr,*localtptr;
    sockhptr = socktptr = fromsock;
    localhptr = localtptr = fromlocal;


    nfds = (capsock > localin) ? capsock + 1 : localin + 1;
    nfds = (nfds > localout) ? nfds : localout + 1;

    FD_ZERO(&rfds_master);
    FD_ZERO(&wfds_master);
    FD_SET(capsock,&rfds_master);
    FD_SET(localin,&rfds_master);
    FD_SET(capsock,&wfds_master);
    FD_SET(localout,&wfds_master);

    while (1) {
	memcpy(&rfds,&rfds_master,sizeof(rfds));
	memcpy(&wfds,&wfds_master,sizeof(wfds));

	retval = select(nfds,&rfds,NULL,NULL,NULL);
	if (retval < 0) {
	    printf("dotippipe select: %s\n",strerror(retval));
	    exit(retval);
	}

	if (FD_ISSET(capsock,&rfds)) {
	    if (sockrc == sizeof(fromsock)) {
		// we're full... let it buffer elsewhere
		if (debug) fprintf(stderr,"sock read buf full\n");
	    }
	    else {
		// simple, non-optimal ring buffering
		if (socktptr > sockhptr || sockrc == 0) 
		    len = (fromsock + sizeof(fromsock)) - socktptr;
		else
		    len = sockhptr - socktptr;

		if (len > 0) {
		    retval = readFunc(socktptr,len);
		    if (retval < 0) {
			if (retval == ECONNRESET) {
			    if (debug) fprintf(stderr,"remote hung up\n");
			    FD_CLR(capsock,&rfds_master);
			    FD_CLR(capsock,&wfds_master);
			}
			else {
			    fprintf(stderr,"dotippipe sock read: %s\n",
				    strerror(retval));
			    exit(retval);
			}
		    }
		    else if (retval > 0) {
			if (debug) fprintf(stderr,"read %d sock\n",retval);
			sockrc += retval;
			socktptr += retval;
			if (socktptr == fromsock + sizeof(fromsock)) {
			    // move from end to head of buffer
			    socktptr = fromsock;
			}
		    }
		    else {
			if (debug) fprintf(stderr,"sock EOF\n");
			FD_CLR(capsock,&rfds_master);
		    }
		}
	    }
	}

	if (FD_ISSET(localin,&rfds)) {
	    if (localrc == sizeof(fromlocal)) {
		// we're full... let it buffer elsewhere
	    }
	    else {
		// simple, non-optimal ring buffering
		if (localtptr > localhptr || localrc == 0) 
		    len = (fromlocal + sizeof(fromlocal)) - localtptr;
		else 
		    len = localhptr - localtptr;

		if (len > 0) {
		    retval = read(localin,localtptr,len);
		    if (retval < 0) {
			if (retval == ECONNRESET) {
			    FD_CLR(localin,&rfds_master);
			    FD_CLR(localin,&wfds_master);
			}
			else {
			    printf("dotippipe localin read: %s\n",
				    strerror(retval));
			    exit(retval);
			}
		    }
		    else if (retval > 0) {
			if (debug) fprintf(stderr,"read %d local\n",retval);
			localrc += retval;
			localtptr += retval;
			if (localtptr == fromlocal + sizeof(fromlocal)) {
			    // move from end to head of buffer
			    localtptr = fromlocal;
			}
		    }
		    else {
			FD_CLR(localin,&rfds_master);
		    }
		}
	    }
	}

	if (sockrc && FD_ISSET(localout,&wfds)) {
	    // write what we can
	    if (sockhptr > socktptr || sockrc == sizeof(fromsock)) 
		len = (fromsock + sizeof(fromsock)) - sockhptr;
	    else
		len = socktptr - sockhptr;

	    if (debug) fprintf(stderr,"trying write %d local\n",len);
	    retval = write(localout,sockhptr,len);
	    if (retval < 0) {
		if (retval == EPIPE) {
		    FD_CLR(localout,&wfds_master);
		    FD_CLR(capsock,&rfds_master);
		    sockrc = 0;
		}
		else if (retval == EAGAIN) {
		    ;
		}
		else {
		    printf("dotippipe local write: %s\n",
			    strerror(retval));
		    exit(retval);
		}
	    }
	    else {
		if (debug) fprintf(stderr,"wrote %d local\n",retval);
		sockrc -= retval;
		sockhptr += retval;
		if (sockhptr == fromsock + sizeof(fromsock)) {
		    // move from end to head of buffer
		    sockhptr = fromsock;
		}
	    }
	}

	if (localrc && FD_ISSET(capsock,&wfds)) {
	    // write what we can
	    if (localhptr > localtptr || localrc == sizeof(fromlocal)) 
		len = (fromlocal + sizeof(fromlocal)) - localhptr;
	    else
		len = localtptr - localhptr;

	    retval = writeFunc(localhptr,len);
	    if (retval < 0) {
		if (retval == EPIPE) {
		    FD_CLR(capsock,&wfds_master);
		    FD_CLR(localin,&rfds_master);
		    localrc = 0;
		}
		else if (retval == EAGAIN) {
		    ;
		}
		else {
		    printf("dotippipe sock write: %s\n",
			    strerror(retval));
		    exit(retval);
		}
	    }
	    else {
		if (debug) fprintf(stderr,"wrote %d sock\n",retval);
		localrc -= retval;
		localhptr += retval;
		if (localhptr == fromlocal + sizeof(fromlocal)) {
		    // move from end to head of buffer
		    localhptr = fromlocal;
		}
	    }
	}

        if ((!FD_ISSET(capsock,&rfds_master) 
	     || !FD_ISSET(localin,&rfds_master))
	    && sockrc == 0 && localrc == 0) {
	    // if one of the src ends has closed, and we have nothing 
	    // queued, we're done
	    if (debug) fprintf(stderr,"Exiting sanely.\n");
	    exit(0);
	}
    }
    
}

#ifdef TIPPTY

static void pack_buffer(buffer_t *buffer, int amount)
{
    assert(buffer != NULL);
    assert((amount == -1) ||
	   ((amount >= 0) && (amount < sizeof(buffer->data))));
    
    if (amount > 0) {
	buffer->inuse -= amount;
	memmove(buffer->data, &buffer->data[amount], buffer->inuse);
    }
}

static void dotippty(char *nodename)
{
  int rc, master, slave;
  char path[64];

  assert(nodename != NULL);
  assert(strlen(nodename) > 0);
  
  if ((mkdir(TIPDIR, 0755) < 0) && (errno != EEXIST))
    perror("unable to make " TIPDIR);
  
  if ((rc = openpty(&master, &slave, path, NULL, NULL)) < 0) {
    perror("openpty");
    exit(1);
  }
  else {
    struct pollfd pf[2] = {
      [0] = { .fd = sock, .events = POLLIN },
      [1] = { .fd = master, .events = POLLHUP },
    };
    
    buffer_t from_pty = { .inuse = 0 }, to_pty = { .inuse = 0 };
    int tweaked = 0, fd_count = 2;
    struct timeval now, before;
    
    fcntl(master, F_SETFL, O_NONBLOCK);
    fcntl(sock, F_SETFL, O_NONBLOCK);
    close(slave);
    slave = -1;

    if (chmod(path, 0666) < 0)
      perror("unable to change permissions");

    snprintf(linkpath, sizeof(linkpath), "%s/%s", TIPDIR, nodename);
    if ((symlink(path, linkpath) < 0) && (errno != EEXIST))
      perror("unable to create symlink");

    gettimeofday(&now, NULL);
    before = now;
    while ((rc = poll(pf,
		      fd_count,
		      fd_count == 1 ? POLL_HUP_INTERVAL : INFTIM)) >= 0) {
      if (rc == 0) { // Timeout
	/* ... check for a slave connection on the next loop. */
	fd_count = 2;
      }
      else if (pf[1].revents & POLLHUP) {
	struct timeval diff;
	
	/* No slave connection. */
	if (pf[0].revents & POLLIN) // Drain the input side.
	  rc = readFunc(to_pty.data, sizeof(to_pty.data));
	if (rc <= 0)
	  exit(0);
	if ((pf[0].revents & POLLOUT) && from_pty.inuse > 0) {
	  // Drain our buffer to the output.
	  rc = writeFunc(from_pty.data, from_pty.inuse);
	  if (rc < 0)
	    exit(0);
	  pack_buffer(&from_pty, rc);
	}
	to_pty.inuse = 0; // Nowhere to send.
	gettimeofday(&now, NULL);
	timersub(&now, &before, &diff);
	if (diff.tv_sec > 0 || diff.tv_usec > (POLL_HUP_INTERVAL * 100)) {
	  before = now;
	  fd_count = 2;
	}
	else {
	  fd_count = 1; // Don't poll the master and turn on timeout.
	}
	tweaked = 0;
      }
      else {
	if (!tweaked) {
	  /*
	   * XXX This next bit is a brutal hack to forcefully turn off echo on
	   * the pseudo terminal.  Otherwise we get a nasty loop of data
	   * echoing back and forth.
	   */
	  if ((slave = open(path, O_RDONLY)) >= 0) {
	    struct termios tio;
	    
	    tcgetattr(slave, &tio);
	    cfmakeraw(&tio);
	    tio.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|ECHOCTL|ECHOKE);
	    tcsetattr(slave, TCSANOW, &tio);
	    close(slave);
	    slave = -1;
	    
	    tweaked = 1;
	  }
	}
	if (pf[1].revents & POLLIN) {
	  rc = read(pf[1].fd,
		    &from_pty.data[from_pty.inuse],
		    sizeof(from_pty.data) - from_pty.inuse);
	  if (rc > 0)
	    from_pty.inuse += rc;
	}
	if (pf[1].revents & POLLOUT) {
	  rc = write(pf[1].fd, to_pty.data, to_pty.inuse);
	  pack_buffer(&to_pty, rc);
	}
	if (pf[0].revents & POLLIN) {
	  rc = readFunc(&to_pty.data[to_pty.inuse],
			sizeof(to_pty.data) - to_pty.inuse);
	  if (rc >= 0)
	    to_pty.inuse += rc;
	  else
	    exit(0);
	}
	if (pf[0].revents & POLLOUT) {
	  rc = writeFunc(from_pty.data, from_pty.inuse);
	  if (rc < 0)
	    exit(0);
	  pack_buffer(&from_pty, rc);
	}
      }
      
      pf[0].events = 0;
      if (to_pty.inuse < sizeof(to_pty.data))
	pf[0].events |= POLLIN;
      if (from_pty.inuse > 0)
	pf[0].events |= POLLOUT;
      
      pf[1].events = POLLHUP;
      if (from_pty.inuse < sizeof(from_pty.data))
	pf[1].events |= POLLIN;
      if (to_pty.inuse > 0)
	pf[1].events |= POLLOUT;
    }
  }
}

#endif

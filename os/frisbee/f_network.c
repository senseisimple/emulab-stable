#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <assert.h>
#include <poll.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <net/if.h>
#include <net/route.h>
#include <sys/ioctl.h>

#ifdef SYSLOG
#include <syslog.h>
#endif

#include "f_common.h"
#include "f_network.h"

/* define FAKELOSS 100 */
#define OSKIT_POLL_READS

#define MCAST
#define MCAST_TTL 2
#ifdef OSKIT
/* Kludgey oskit fake poll madness.
   Ick. I should bury this crap further on in the file. */
      int poll( struct pollfd * x, uint y, int z ) {
	/* return 1; */
	fd_set fds;

	FD_ZERO( &fds );
	FD_SET( x->fd, &fds );

	if (x->events == POLLWRNORM) {
	  /* a write waits forever */
	  return 1; /* ???! XXX is this correct? */
	  return select( x->fd + 1, NULL, &fds, NULL, NULL );
	} else if (x->events == POLLIN) {
	  /* a read doesn't wait */
	  struct timeval tv;
#ifndef OSKIT_POLL_READS
	  tv.tv_sec = 0;
	  tv.tv_usec = 0;
#else
	  tv.tv_sec = z / 1000;
	  tv.tv_usec = (z % 1000) * 1000;
#endif
	  return select( x->fd + 1, &fds, NULL, NULL, &tv );	  
	} else {
	  assert( 0 );
	  return 0;
	}
      }
#endif

/* define VERUCA to try and get as much i/o buffer as possible. 
 * A good way to hose your friend's machine.
 */
/* #define VERUCA */

#define WANTBUFFER (128 * 1024)

static int n_sizeOfType( uchar type );

/* our socket */
static int sock;

/* which port we're sending to */
static ushort nboSendPort;
/* which address (broadcast hopefully) we're sending to */
static uint   nboSendAddress;

/* initialize to send, by looking up sendName and attaching the appropriate sockets */
void n_initLookup( ushort receivePort, ushort sendPort, const char * sendName )
{
  struct hostent * he = gethostbyname( sendName );
  if ( he == NULL ) {
#ifdef SYSLOG
    syslog( LOG_ERR, "Bad hostname \"%s\"\n", sendName );
#else
    fprintf(stderr, "Bad hostname \"%s\"\n", sendName );
#endif
    exit(-1);
  }
  assert( he->h_addr_list[0] != NULL );

  n_init( receivePort, sendPort, htonl( *((unsigned int *)he->h_addr_list[0])) ); /* XXX bad?? */
}

static uint gSendAddress;

static void initChk();

/* same initialize, only no lookup */
void n_init( ushort receivePort, ushort sendPort, uint sendAddress )
{
  struct sockaddr_in name;
  /*  int result; */

  initChk();

  gSendAddress = sendAddress;

  nboSendPort    = htons( sendPort ); 
  nboSendAddress = htonl( sendAddress );

  sock = socket( PF_INET, SOCK_DGRAM, 0 );
  if (sock < 0) { perror("n_init(): socket()"); abort(); }
  
  name.sin_family = AF_INET;
  name.sin_port   = htons( receivePort );
  name.sin_addr.s_addr = htonl( INADDR_ANY );

#ifdef SYSLOG
  syslog( LOG_INFO, 
	 "Binding to port %i, sending to 0x%08x:%i\n", 
	 receivePort, sendAddress, sendPort );
#else
  printf( "Binding to port %i, sending to 0x%08x:%i\n", 
	  receivePort, sendAddress, sendPort );
#endif  

  while ( bind( sock, (struct sockaddr *)&name, sizeof( name )) < 0 ) {
    perror("n_init(): bind()");
    sleep(1);
  }
  
  /*
  {
    int result = fcntl( sock, F_SETFL, O_NONBLOCK );
    if (result < 0) { perror("n_init(): fcntl()"); abort(); }
  }
  */

#ifdef MCAST
  if ((sendAddress >> 28) == 14) {
    unsigned char ttl = MCAST_TTL;
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = nboSendAddress;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    
#ifdef SYSLOG
    syslog( LOG_INFO, "Using multicast.\n");
#else
    printf("Using multicast...\n");
#endif
    if (setsockopt(sock,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
      perror("setsockopt");
      exit(1);
    }
    setsockopt(sock,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl));
    
    { /* disable local echo */
      u_char loop = 0;
      setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    }
  } else
#endif /* MCAST */
  { 
    /* get broadcast perms */
    /* int ret = 0; */
    /* int retsize = sizeof( int ); */
    
    int foo = 1;
    
    setsockopt( sock, SOL_SOCKET, SO_BROADCAST, &foo, sizeof( foo ) );
  }

#ifdef VERUCA
  {
    /* may as well try to get some headroom */
    int ret = 0;
    int retsize = sizeof( int );
    
    int foo = WANTBUFFER;
    
    setsockopt( sock, SOL_SOCKET, SO_SNDBUF, &foo, sizeof( foo ) );
    
    foo = WANTBUFFER;
  
    setsockopt( sock, SOL_SOCKET, SO_RECVBUF, &foo, sizeof( foo ) );
  }
#endif
}


void n_finish()
{
#ifdef MCAST
  if ((gSendAddress >> 28) == 14) {
    unsigned char ttl = MCAST_TTL;
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = nboSendAddress;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    
#ifdef SYSLOG
    syslog( LOG_INFO, "Dropping multicast.\n");
#else
    printf("Dropping multicast...\n");
#endif
    if (setsockopt(sock,IPPROTO_IP,IP_DROP_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
      perror("setsockopt [error ignored]");
      /*      exit(1); */
    }
  }
#endif /* MCAST */
  close( sock );
} 

/* crc code from http://www.createwindow.com/programming/crc32/ */ 

static unsigned long crc32_table[256];

static unsigned long Reflect( unsigned long ref, char ch )
{
  int i;
  unsigned long value = 0;

  for (i = 1; i < (ch + 1); i++) {
    if (ref & 1) {
      value |= 1 << (ch - i);
    }
    ref >>= 1;
  }

  return value; 
}

static void initChk()
{
  int i,j;
  unsigned long ulPolynomial = 0x04c11db7;

  for (i = 0; i < 0xff; i++) {
    crc32_table[i] = Reflect(i, 8) << 24;
    for (j = 0; j < 8; j++) {
      crc32_table[i] = (crc32_table[i] << 1) ^ 
        (crc32_table[i] & (1 << 31) ? ulPolynomial : 0);
    }
    crc32_table[i] = Reflect( crc32_table[i], 32 );
  }
}

static unsigned int doChk( unsigned char * data, int len )
{
  unsigned long ulCRC = 0xFFffFFff;

  while( len-- ) {
    ulCRC = (ulCRC >> 8) ^ crc32_table[(ulCRC & 0xFF) ^ *data++];
  }

  return ulCRC ^ 0xFFffFFff;

  /*
  unsigned int accum = 0;
  unsigned int i;
  for (i = 0; i < len; i++) {
    accum += data[i];
  }
  */
}

static int verifyChk( PacketData * pd )
{ 
  uint chk2 = 0;
  uint chk = pd->checksum;

  pd->checksum = 0;  
  chk2 = doChk( (unsigned char *)pd, sizeof( PacketData ) );
  pd->checksum = chk;

  {
    uint rv = (chk == chk2);
    if (!rv) {
      printf("Warning! Checksum failed, tossing packet.\n" );
    }
    return rv;
 }
}

static void genChk( PacketData * pd )
{
  pd->checksum = 0;
  pd->checksum = doChk( (unsigned char *)pd, sizeof( PacketData ));
}

void n_packetSend(Packet * p)
{
  int length = n_sizeOfType( p->type );
  
  if (length != 0) {
    /* valid packet type */
    struct sockaddr_in toName;
    struct pollfd fds;

    if (p->type == NPT_DATA) {
      genChk( (PacketData *)p );
    }

    toName.sin_family      = AF_INET;
    toName.sin_port        = nboSendPort;
    toName.sin_addr.s_addr = nboSendAddress;

    fds.fd      = sock;
    fds.events  = POLLWRNORM;
    fds.revents = POLLWRNORM;    
  
    if (poll( &fds, 1, /* wait forever */ -1 ) > 0) {
      int count = 0;
#ifdef FAKELOSS
      if ((rand() % FAKELOSS) != 0)
#endif 
      while (-1 == sendto( sock, (void *)p, length,
			   0 /* flags */, 
			   (struct sockaddr *)&toName, sizeof( toName ))) {
	perror("n_packetSend(): sendto()");
	usleep(50);
	if (count++ == 8) { 
	  /*
	  traceprintf("n_packetSend(): aborted send\n");
	  */
	  break;
	}
      }
    } else {
      perror("n_packetSend(): poll()");
    }
  }
}

static int n_sizeOfType( uchar type )
{
  switch (type) {
    case NPT_DATA:    return sizeof( PacketData );
    case NPT_REQUEST: return sizeof( PacketRequest );
    case NPT_POLL:    return sizeof( PacketPoll );
    case NPT_ALIVE:   return sizeof( PacketAlive );
    case NPT_GONE:    return sizeof( PacketGone );
    default:          return 0;
  }
}


/* wont timeout if it keeps getting packets it doesnt care about */
int n_packetRecv( Packet * p, uint timeout, uchar mask )
{
  int got;
    
  struct pollfd fds;
#ifdef OSKIT
  {
    /* oskit version can't block, so try once */
#else
  while (1) {
    /* keep trying until counter has run out */
#endif

    /*ifndef OSKIT*/
    fds.fd      = sock;
    fds.events  = POLLIN;
    fds.revents = POLLIN;
  
    {
      int pollReturn = poll( &fds, 1, timeout );
      if (pollReturn < 0) {
	if (errno != EINTR) {
	  perror("n_packetRecv: poll");
	}
      }
      if (pollReturn < 1) {
	/* traceprintf("nuthin'\n"); */
	return 0;
      }
      /*traceprintf("somethin'\n");*/
    }
    /*endif*/

    got = recv( sock, (void *)p, sizeof( Packet ), 0 );

    if ((got != 0) &&
	(mask & p->type) && 
        (got == n_sizeOfType( p->type )) &&
        (p->type != NPT_DATA || verifyChk( (PacketData *) p))) {
      return 1;
    }
  }
  /*
  traceprintf("Something wrong with received packet.\n");
  */
  return 0;
}
		  
/*
  fd_set fds;
  struct timeval tv;

  FD_ZERO( &fds );
  FD_SET( sock, &fds );
  tv.tv_sec = 0;
  tv.tv_usec = 1000 * timeout;

  if (select( 1, &fds, NULL, NULL, &tv ) <= 0) {
    perror("n_packetRecvCore: select");
    return 0;
  }
*/

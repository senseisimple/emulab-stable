#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <syslog.h>

#include "f_common.h"
#include "f_timer.h"
#include "f_network.h"

#define POLL_WAIT 100 
/* was 50 */

#define PACKETS_PER_CHUNK 16

int verbose = 0;
int timeout = 0;
int msecleft = 0;

int  inFile;
uint packetTotal;
uint pollSequence;
uint pollSequenceValid;

#define MAX_CLIENTS 128

/* if they drop 5 polls in a row,
   they're out of the family. */
#define NAUGHTY_THRESHOLD 5

/* how many dropped pollis in a row */
uint clientNaughtiness[ MAX_CLIENTS ];
/* list of client ids */
uint clientList[ MAX_CLIENTS ];
uint clientListPos;
uint clientCount;

/* id kept of the last client we polled..
   so if the poll times out, we know who to blame. */
uint lastAddress;

/* returns next client to send poll to */
uint getNextClient()
{
  int oldPos = clientListPos;
  do {
    clientListPos = (clientListPos + 1) % MAX_CLIENTS;
    if (clientList[ clientListPos ] != 0) { break; }
  } while (clientListPos != oldPos );

  return clientList[ clientListPos ];
}

/* records a given client as being "good" (zeros dropcount)*/
void goodClient( uint id )
{
  int i;
  for (i = 0; i < MAX_CLIENTS; i++) {
    if (clientList[i] == id ) {
      clientNaughtiness[i] = 0;
      return;
    }
  }
  if (verbose) syslog( LOG_WARNING, 
          "goodClient() called on non-existent id 0x%08x\n", id);
}

/* sets a client as being "naughty" (increments dropcount.)
   returns new dropcount */
int naughtyClient( uint id )
{
  int i;
  for (i = 0; i < MAX_CLIENTS; i++) {
    if (clientList[i] == id ) {
      return (++clientNaughtiness[i]) > NAUGHTY_THRESHOLD ? 1 : 0;
    }
  }
  if (verbose) syslog( LOG_WARNING, "naughtyClient() called on unknown id 0x%08x\n", id);
  return 0;
}

/* removes a client from clientlist */
void removeClient( uint id )
{
  int i;
  for (i = 0; i < MAX_CLIENTS; i++) {
    if (clientList[i] == id ) {
      if (verbose) syslog( LOG_INFO, "Removing client (id=0x%08x)\n", id );
      clientList[i] = 0;
      clientCount++;
      return;
    }
  }
}

/* adds a client to client list, if it is not there already */
void addClient( uint id )
{
  int i;
  int place = -1;
  for (i = 0; i < MAX_CLIENTS; i++) {
    if (clientList[i] == id) {
      /* printf("Already in list.\n"); */
      return;
    }
    if ((place == -1) && (clientList[i] == 0)) {
      place = i;
      clientCount++;
    }
  }

  if (place == -1) {
    syslog(LOG_WARNING, "Too many clients-- (up the constant in server.c)\n", MAX_CLIENTS );
  } else { 
    if (verbose) syslog(LOG_INFO, "Adding client (id=0x%08x)\n", id );
    clientList[ place ] = id;
    clientNaughtiness[ place ] = 0;
  }
}


char filename[256];
char sendAddress[256];
int  port;

void usage(char * appname)
{
  fprintf(stderr, 
	  "%s: Usage:\n"
	  "%s: %s [-v] [-t seconds] <filename> [<serveraddress> [<port>]]\n"
	  "%s: -v is verbose mode\n"
	  "%s: -t specifies idle seconds before server exits\n"
	  "%s: (default is 234.5.6.7, port 3564.)\n",
	  appname, appname, appname, appname, appname, appname );
}

void handleArgs( int argc, char ** argv )
{
  int i;
  char ch;

  verbose = 0;
  timeout = 0;

  while ((ch = getopt(argc, argv, "vht:")) != -1) {
    switch(ch) {
    case 'v':
      verbose++;
      syslog(LOG_INFO, "Verbose (-v) mode.\n");
      break;
      
    case 't':
      timeout = atoi(optarg);
      break;
      
    case 'h':
    case '?':
    default:
      syslog(LOG_ERR, "Fatal - Bad arguments." );
      usage("frisbeed" /* argv[0] */);
      exit(1);
    }
  }

  argc -= optind;
  argv += optind;
  
  port = 3564;
  strcpy( sendAddress, "234.5.6.7" );

  if (argc > 0) {
    strcpy( filename, argv[0] );
    if (argc > 1) {
      strcpy( sendAddress, argv[1] );
      if (argc > 2) {
	port = atoi( argv[2] );
      }
    }
  } else {
    syslog(LOG_ERR, "Fatal - Insufficient arguments." );
    usage("frisbeed" /* argv[0] */);
    exit(1);
  }
}

int main( int argc, char ** argv )
{
  int gotNonRequestYet;

  clientCount = 0;

  openlog("frisbeed", LOG_PERROR | LOG_PID, LOG_USER );
  atexit( closelog );

  memset( clientList, 0, sizeof( clientList ) );
  clientListPos = 0;
  lastAddress = 0;

  pollSequence = 0;
  pollSequenceValid = 0;

  handleArgs( argc, argv );

  syslog(LOG_INFO, "Using file \"%s\" for input.", filename );
  syslog(LOG_INFO, "Sending to net address %s, port %i", sendAddress, port );
 
  inFile = open( filename, O_RDONLY );

  if ( inFile == -1) {
    syslog(LOG_ERR, "Fatal - Couldn't open file \"%s\" for reading: %m\n", filename);
    exit(1);
  }

  t_init();

  packetTotal = lseek( inFile, 0, SEEK_END ) / 1024;

  syslog(LOG_INFO, "File \"%s\" opened; contains %ikB.\n", filename, packetTotal );

  n_initLookup( port, port, sendAddress );

  msecleft = 1000 * timeout;
  t_setTimer( POLL_WAIT );
  gotNonRequestYet = 0;

  while (1) {
    uint puntCount = 0;
    Packet p;

    if ((!gotNonRequestYet || !t_getWentOff()) && 
	(n_packetRecv( &p,
		       gotNonRequestYet ? t_getRemainingTime() : POLL_WAIT,
		       NPT_REQUEST | NPT_ALIVE | NPT_GONE ))) {
      /* got a packet.. */
      msecleft = 1000 * timeout;
      if (p.type == NPT_REQUEST) {
	/* it was a request for data, so service this request */
	PacketRequest * pr = (PacketRequest *)&p;

	goodClient( lastAddress );
	lastAddress = 0; /* dont trash the client who we sent the 
			    request to */


	if (pollSequenceValid &&
	    (pr->pollSequence == pollSequence) &&
	    ((pr->packetId < packetTotal) || (pr->packetId == MAXUINT))) {
	  if (pr->packetId != MAXUINT) {
	    int i;
	    puntCount = 0;
	    /* 
	     * only actually do anything if this is a real request.
	     * if this is a punt (pr->packetId == MAXUINT) then
	     * send another POLL 
	     */
	    pollSequenceValid = 0;
	    /* printf("Servicing request for %i (pollSeq %i)\n", pr->packetId, pollSequence ); */
	    lseek( inFile, pr->packetId * 1024, SEEK_SET );
	    for( i = 0; i < PACKETS_PER_CHUNK; i++ ) {
	      PacketData pd;
	      pd.type = NPT_DATA;
	      pd.packetId = i + pr->packetId;
              if (pd.packetId < packetTotal) {
                int tries = 8;
                while (--tries && (1024 != read( inFile, &(pd.data), 1024))) ;
		if (tries) { 
		  n_packetSend( (Packet *)&pd );
		} else {
		  perror("read call [non-fatal]");
		}
	      }
	    }
	  } else {
	    /* was a punt */
	    puntCount++;
	    if (puntCount > clientCount) {
	      usleep( 1000 * 333 ); /* sleep a third of a second. */
	      puntCount = 0;
	    }
	  }
	  /* "fall through" to polling code */
	} else {
	  if (verbose) syslog( LOG_WARNING, 
                  "Bad or duplicate request:"
                  "psv = %i; p.ps = %i; ps = %i; p.pi = %i; pt = %i;\n ", 
		  pollSequenceValid, 
		  pr->pollSequence, 
		  pollSequence,
		  pr->packetId,
		  packetTotal );
	  /* don't poll again.. a viscious cycle.. */
	  gotNonRequestYet = 1;
	  continue; /* continue with while as long as poll timer is > 0*/
	}
      } else if (p.type == NPT_ALIVE) {
	PacketAlive * pa = (PacketAlive *)&p;
	if (verbose) syslog( LOG_DEBUG, "Alive hint received\n");
	addClient( pa->clientId );
	gotNonRequestYet = 1;
	continue; /* continue with while as long as poll timer is > 0*/
      } else if (p.type == NPT_GONE) {
	PacketGone * pg = (PacketGone *)&p;
	if (verbose) syslog( LOG_DEBUG, "Gone hint received\n");
	removeClient( pg->clientId );
	if (pg->clientId == lastAddress) {
	  /* we are in the process of polling them, so poll someone else */
	  /* fall through to poll code */
	} else {
	  gotNonRequestYet = 1;
	  continue; /* continue with while as long as poll timer is > 0*/
	}
      }

    } /* if */
          
    { /* timeout elapsed, or we successfully sent data */
      PacketPoll pp;

      if (lastAddress != 0) {
        /* client didn't seem to do its job */
	if (verbose) syslog( LOG_WARNING, "Poll failed to produce results.\n");
	if (naughtyClient( lastAddress )) {
          /* too many dropped polls in a row. */
	  if (verbose) syslog( LOG_WARNING,
                  "Removing client 0x%08x for being unresponsive.\n", lastAddress);
	  removeClient( lastAddress ); 
	}
	/* we timed out.. no request received. */
        /* removeClient( lastAddress ); XXX*/
      }

      pp.address      = getNextClient(); 
      
      if (pp.address != 0) {
        /* there is a client who cares, 
           so lovingly hand-craft a new poll packet */
	lastAddress     = pp.address;
	pp.type         = NPT_POLL;
	pp.packetTotal  = packetTotal;
	pollSequence++;
	pollSequenceValid = 1;
	pp.pollSequence = pollSequence;
     
	/*      
      printf("*** Sending poll for client 0x%08x, mask 0x%08x.\n",
	     pp.address,
	     pp.addressMask );
	*/
	n_packetSend( (Packet *)&pp );
      } else {
	/* printf("No clients to poll *twiddle* *twiddle* *twiddle*.\n"); */
        if (msecleft > 0) { 
	  msecleft -= POLL_WAIT; 
	  if (msecleft <= 0) {
	    /* printf("TIMEOUT ELAPSED.\n"); */
	  syslog( LOG_INFO,
		  "Timeout of %i seconds elapsed with no clients - exiting.\n",
		  timeout );
	    exit( 0 );
	  }
	}

	lastAddress = 0;
      }
    }

    t_setTimer( POLL_WAIT );
    gotNonRequestYet = 0;
  }
}

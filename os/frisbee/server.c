#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>

#include "f_common.h"
#include "f_timer.h"
#include "f_network.h"

#define POLL_WAIT 50

#define PACKETS_PER_CHUNK 16

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
  printf("goodClient() called on non-existent id 0x%08x\n", id);
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
  printf("naughtyClient() called on non-existent id 0x%08x\n", id);
  return 0;
}

/* removes a client from clientlist */
void removeClient( uint id )
{
  int i;
  for (i = 0; i < MAX_CLIENTS; i++) {
    if (clientList[i] == id ) {
      printf("* Removing client (id=0x%08x)\n", id );
      clientList[i] = 0;
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
    }
  }

  if (place == -1) {
    printf("!! TOO MANY DAMN CLIENTS. (%i max)\n", MAX_CLIENTS );
  } else { 
    printf("* Adding client (id=0x%08x)\n", id );
    clientList[ place ] = id;
    clientNaughtiness[ place ] = 0;
  }
}

int main( int argc, char ** argv )
{
  int gotNonRequestYet;

  memset( clientList, 0, sizeof( clientList ) );
  clientListPos = 0;
  lastAddress = 0;

  pollSequence = 0;
  pollSequenceValid = 0;

  if ( argc != 5 ) {
    printf("!!! Wrong number of arguments.\n"
	   "Usage:\n"
           "server <filename> <myport> <serveraddress> <serverport>\n\n"
           "Broadcast example:\n"
           "server foo.image 7000 168.192.0.255 7000\n\n"
           "Other example:\n"
	   "server bar.image 3447 biff.foo.org 3447\n\n" );
    exit( -1 );
  }  
 
  printf( "* Opening file \"%s\" for input.\n", argv[1]);

  inFile = open( argv[1], O_RDONLY );

  if ( inFile == -1) {
    perror("!!! Error opening file");
    exit(-1);
  }

  t_init();

  packetTotal = lseek( inFile, 0, SEEK_END ) / 1024;

  printf( "* %ik in file.\n", packetTotal );

  n_initLookup( atoi( argv[2] ),
	        atoi( argv[4] ),
		argv[3] );

  t_setTimer( POLL_WAIT );
  gotNonRequestYet = 0;

  while (1) {
    Packet p;

    if ((!gotNonRequestYet || !t_getWentOff()) && 
	(n_packetRecv( &p,
		       gotNonRequestYet ? t_getRemainingTime() : POLL_WAIT,
		       NPT_REQUEST | NPT_ALIVE | NPT_GONE ))) {
      /* got a packet.. */
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
	      pd.packetId = (i + pr->packetId) % packetTotal;
	      read( inFile, &(pd.data), 1024 );
	      n_packetSend( (Packet *)&pd );
	    }
	  }
	  /* "fall through" to polling code */
	} else {
	  printf("! Bad or duplicate request: psv = %i; p.ps = %i; ps = %i; p.pi = %i; pt = %i;\n ", 
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
	/* printf("Alive hint received\n"); */
	addClient( pa->clientId );
	gotNonRequestYet = 1;
	continue; /* continue with while as long as poll timer is > 0*/
      } else if (p.type == NPT_GONE) {
	PacketGone * pg = (PacketGone *)&p;
	/* printf("Gone hint received\n"); */
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
	printf("! Poll failed to produce results.\n");
	if (naughtyClient( lastAddress )) {
          /* too many dropped polls in a row. */
	  printf("Removing client 0x%08x for being naughty.\n", lastAddress);
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
       
	lastAddress = 0;
      }
    }

    t_setTimer( POLL_WAIT );
    gotNonRequestYet = 0;
  }
}

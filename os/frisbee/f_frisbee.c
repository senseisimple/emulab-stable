#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <poll.h>
#include <stdio.h>

#ifdef OSKIT
#include <oskit/startup.h>
#include <oskit/clientos.h>
#endif

#include "f_common.h"

#include "f_network.h"
#include "f_timer.h"
#include "f_chunker.h"

/* implements the following API: */
#include "frisbee.h"

#define FRISBEE_PORT     3564

/* timeout before it decides the server is dead */
#define TIMEOUT          3000

/* the minimum number of msecs between alive hint attempts */
#define ALIVE_HINT_MIN   1000

/* when added to ALIVE_HINT_MIN, the maximum number of msecs between alive hint attempts */
#define ALIVE_HINT_RANGE 2000

/* define GUNS_ABLAZING to make the client send
   an alive hint immediately after it starts, rather
   than holding back a while. */
#define GUNS_ABLAZING

int packetTotal;
int nextExpectedPacket;
int packetsReceivedSoFar;
int skipNextAlive;
int finishedMarker;

uint myId;

extern char version[];
extern char build_info[];

static void sendAliveHint();
static void sendGoneHint();


int frisbeeInit( const char * imageName,
		 unsigned int broadcastAddr )
{
#ifdef OSKIT
  /* redundant calls removed.
  oskit_clientos_init();
 
  start_clock();
  start_network();
  */
#endif
  printf("frisbeeInit: called.\n");
  /* setup my unique id */
  srandomdev();
  myId = random();

  packetTotal = -1;
  nextExpectedPacket = -1;
  packetsReceivedSoFar = 0;

  printf("frisbeeInit: calling n_init.\n");
  n_init( FRISBEE_PORT, FRISBEE_PORT, broadcastAddr );

  printf("frisbeeInit: calling t_init.\n");
  t_init();

  printf("frisbeeInit: calling t_setTimer.\n");
  t_setTimer( ALIVE_HINT_MIN + (random() % ALIVE_HINT_RANGE) );
  skipNextAlive = 0;
  finishedMarker = -1;

#ifdef GUNS_ABLAZING
  printf("frisbeeInit: sending alive hint.\n");
  sendAliveHint();
#endif
  /* XXX detect network errors, etc, and return appropriate errorcodes. */
  printf("frisbeeInit: returning FRISBEE_OK\n");
  return FRISBEE_OK;
}

int frisbeeInit2( const char * imageName,
		  const char * broadcastAddr,
                  int port )
{
#ifdef OSKIT
  /* redundant calls removed.
  oskit_clientos_init();
 
  start_clock();
  start_network();
  */
#endif
  printf("frisbeeInit: called.\n");
  /* setup my unique id */
  srandomdev();
  myId = random();

  packetTotal = -1;
  nextExpectedPacket = -1;
  packetsReceivedSoFar = 0;

  printf("frisbeeInit: calling n_init.\n");
  n_initLookup( port, port, broadcastAddr );

  printf("frisbeeInit: calling t_init.\n");
  t_init();

  printf("frisbeeInit: calling t_setTimer.\n");
  t_setTimer( ALIVE_HINT_MIN + (random() % ALIVE_HINT_RANGE) );
  skipNextAlive = 0;
  finishedMarker = -1;

#ifdef GUNS_ABLAZING
  printf("frisbeeInit: sending alive hint.\n");
  sendAliveHint();
#endif
  /* XXX detect network errors, etc, and return appropriate errorcodes. */
  printf("frisbeeInit: returning FRISBEE_OK\n");
  return FRISBEE_OK;
}

int frisbeeInitOld( const char * imageName, 
                    unsigned short localPort, 
	            const char * remoteAddr, 
                    unsigned short remotePort )
{
#ifdef OSKIT
  oskit_clientos_init();
  start_clock();
  start_network();
#endif
  printf("frisbeeInitOld: called. =(\n");

  /* setup my unique id */
  srandomdev();
  myId = random();

  packetTotal = -1;
  nextExpectedPacket = -1;
  packetsReceivedSoFar = 0;

  n_initLookup( localPort, remotePort, remoteAddr );

  t_init();

  t_setTimer( ALIVE_HINT_MIN + (random() % ALIVE_HINT_RANGE) );

  skipNextAlive = 0;
  finishedMarker = -1;

#ifdef GUNS_ABLAZING
  sendAliveHint();
#endif
  /* XXX detect network errors, etc, and return appropriate errorcodes. */
  return FRISBEE_OK;
}

int frisbeeLockReadyChunk( uint * chunkId, uchar ** data )
{
  return c_consumeM( chunkId, data ) ? FRISBEE_OK : FRISBEE_NO_CHUNK;
}

int frisbeeUnlockReadyChunk( uint chunkId )
{
  /* XXX should check to make sure that chunkId is valid. */
  c_finishedM( chunkId );
  return FRISBEE_OK;
}

int frisbeeLoop()
{
  Packet p;
  static int done = 0;  
  
  if (done) { 
    return FRISBEE_DONE;
  }

  if (n_packetRecv( &p, TIMEOUT, NPT_DATA | NPT_POLL)) {
    if ((p.type == NPT_DATA) && (packetTotal != -1)) {
      /* we got data, and we already know how many packets there are */
      PacketData * pd = (PacketData *)&p;
      
      /* c_addKtoM( pd->packetId, &(pd->data)); */
      c_addKtoM( pd->packetId, pd->data);
    } else if (p.type == NPT_POLL) {
      /* this was a poll*/
      PacketPoll * pp = (PacketPoll *)&p;
      /* NPT_POLL */
      
      if (packetTotal == -1) {
	/* dont know if this poll goes to us, but 
         * we dont already know how many packets there are,
         * so there is useful information to be learned */
	packetTotal = pp->packetTotal;
	traceprintf("%iM to get...\n", packetTotal / 1024 );
	c_init( packetTotal / 1024, 32 );
      }
      if (pp->address == myId) { /* XXX fix to test addressMask/address */
	/* this is a poll to us */
	PacketRequest pr;

        /* we know the server knows about us, so don't bother sending next alive hint. */
	skipNextAlive = 1;
	pr.type = NPT_REQUEST;
	pr.pollSequence = pp->pollSequence;
	pr.packetId = c_suggestK();
	if (pr.packetId == -2) { 
	  /* we're done with the file 
	   * XXX - shouldn't wait for a poll to decide we're done. */
	  sendGoneHint();
	  n_finish();
	  done = 1;
	  return FRISBEE_OK; /* next time we're called, we'll return a FRISBEE_DONE. */ }
	if (pr.packetId == -1) {
          /* there is no buffer space, so punt. */
	  pr.packetId = MAXUINT;
	}
	n_packetSend( (Packet *)&pr );
      }
    }
  } else {
    /* printf("! Couldn't hear any server.\n"); */
  }
  
  /* traceprintf("Checking if alarm went off..\n"); */
  if (t_getWentOff()) {
/*
    traceprintf("Got that alarm went off...\n");
*/
    t_setTimer( ALIVE_HINT_MIN + (random() % ALIVE_HINT_RANGE) );
    if (skipNextAlive == 0) {
/*
      traceprintf("Sending alive hint...\n");
*/
      sendAliveHint();
    } else {
/*
      traceprintf("Skipping send of alive hint...\n");
*/
    }
    skipNextAlive = 0;
  }
  
  return FRISBEE_OK;
}

int frisbeeFinish()
{
  c_finish();
  return FRISBEE_OK;
}

/* statics */
void sendAliveHint()
{
  PacketAlive pa;

  traceprintf("Sending alive hint.\n");
  pa.type = NPT_ALIVE;
  pa.clientId = myId;
  n_packetSend( (Packet *)&pa );
}

void sendGoneHint()
{
  PacketGone pg;
  pg.type = NPT_GONE;
  pg.clientId = myId;
  n_packetSend( (Packet *)&pg );
}

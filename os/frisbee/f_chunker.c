#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "f_common.h"
#include "f_chunker.h"

#define BufferSlotCount 32

/* the number of empty bufferslots there must be
   for the system to request more data. 
   Originally, this was 1 (any slots), but
   having more means potentially better synchronization. */ 
#define BUFFER_HEADROOM 7

typedef struct {
  int mb; /* -1 = none. */
  int gotCount;
  uchar gotBitmap[ 1024 ];
  uchar data[ 1024 * 1024 ];
} BufferSlot;

BufferSlot slots[BufferSlotCount];

int totalMB;
uchar * finishedMBBitmap;

unsigned int redundantKBs;  /* intraMB redundant */
unsigned int redundantKBs2; /* MB redundant */

void c_init( uint sizeM, uint bufferCount )
{
  int i;

  redundantKBs = 0;
  redundantKBs2 = 0;
  /* ignoring BufferCount. */
  totalMB = sizeM;

  finishedMBBitmap = malloc( totalMB );

  assert( finishedMBBitmap );

  bzero( finishedMBBitmap, totalMB );

  for (i = 0; i < BufferSlotCount; i++) {
    slots[i].mb = -1;
  }
}

void c_finish()
{
  int i;

  assert( finishedMBBitmap );

  for (i = 0; i < totalMB; i++) {
    if (!finishedMBBitmap[i]) {
      printf("Chunker: Warning! finish called without all MBs being finished.\n");
    }
  }

  free( finishedMBBitmap );
}

static int inProgressMB( int mb )
{
  int i;

  assert( mb >= 0 );
  assert( mb < totalMB );

  for (i = 0; i < BufferSlotCount; i++) {
    if (slots[i].mb == mb) {
      return 1;
    }
  }

  return 0;
}

static int getOrAllocBufferSlotForMB( BufferSlot ** returnPointer, int mb )
{ 
  int i;

  assert( mb >= 0 );
  assert( mb < totalMB );

  for (i = 0; i < BufferSlotCount; i++) {
    if (slots[i].mb == mb) {
      *returnPointer = &(slots[i]);
      return 1;
    }
  }

  for (i = 0; i < BufferSlotCount; i++) {
    if (slots[i].mb == -1) {
      slots[i].mb = mb;
      slots[i].gotCount = 0;
      bzero( slots[i].gotBitmap, 1024 );
      *returnPointer = &(slots[i]);
      /*
      printf("Chunker: Starting MB %i (in slot %i)\n", mb, i ); 
      */
      return 1;
    }
  }

  /* no slots */
  return 0;
}

void c_addKtoM( uint kb, uchar * data ) 
{
  int mb = kb / 1024;
  int kbOffset = kb % 1024;
  int byteOffset = kbOffset * 1024;

  BufferSlot * bsp;

  assert( mb >= 0 );
  assert( mb < totalMB );

  if (!finishedMBBitmap[ mb ]) {
    if (getOrAllocBufferSlotForMB( &bsp, mb )) {
      if (!bsp->gotBitmap[ kbOffset ]) {
	bsp->gotBitmap[ kbOffset ] = 1;
	memcpy( bsp->data + byteOffset, data, 1024 );
	bsp->gotCount++;
      } else {
	redundantKBs++;
      }
    }  
  } else {
    redundantKBs2++;
  }
}

int minIncompleteSlot()
{
  int min = -1;
  int minCount, i;
  for (i = 0; i < BufferSlotCount; i++) {
    if (slots[i].mb != -1 && slots[i].gotCount != 1024) {
      if (min == -1 || slots[i].gotCount < minCount) {
        min = i;
	minCount = slots[i].gotCount;
      }
    }
  }
  return min;
}

int maxIncompleteSlot()
{
  int max = -1;
  int maxCount, i;
  for (i = 0; i < BufferSlotCount; i++) {
    if (slots[i].mb != -1 && slots[i].gotCount != 1024) {
      if (max == -1 || slots[i].gotCount > maxCount) {
        max = i;
	maxCount = slots[i].gotCount;
      }
    }
  }
  return max;
}

int getKForIncompleteBufferSlot( int slot )
{
  int i;
  assert( slots[slot].mb != -1 && slots[slot].gotCount != 1024 );
  for ( i = 0; i < 1024; i++) {
    if (!slots[slot].gotBitmap[i]) {
      return i;
    }
  }
}
 
/* this is the heart of Chunker, and indeed, Frisbee.
   Could possibly be broken into two or three subfunctions,
   though it is fairly linear. */

int c_suggestK()
{
  static int lastMessage = 0;
  static int lastMessageCount = 0;
  int allDone;
  int i, ii, j, ei;
  int freeCount = 0;
  int highestWeHave;

  int m = maxIncompleteSlot();

  if (m != -1) {
    return slots[m].mb * 1024 + getKForIncompleteBufferSlot( m );
  }

  for (i = 0; i < BufferSlotCount; i++) {
    if (slots[i].mb == -1) {
      freeCount++;
    }
  }

#if 0
  int foo = rand() % BufferSlotCount;

  /* find an incompletely full buffer slot --
     but start from a random location, as not to give
     bucket #0 preferential treatment. */
  for (ii = 0; ii < BufferSlotCount; ii++) {
    i = (ii + foo) % BufferSlotCount; 
    if (slots[i].mb != -1) {
      if (slots[i].gotCount != 1024) {
	for (j = 0; j < 1024; j++) {
	  if (!slots[i].gotBitmap[j]) {
	    if (lastMessage != 1) {
	      if (lastMessageCount) {
		/*
		printf("ChunkerSK: Last ChunkerSK message repeated %i times.\n", 
		       lastMessageCount );
 		*/
		lastMessageCount = 0;
	      }
		/*
	      printf("ChunkerSK: Suggesting kb from existing MB...\n");
		*/
	      lastMessage = 1;
	    } else {
	      lastMessageCount++;
	    }
	    return (slots[i].mb * 1024 + j);
	  }
	}
        assert( 0 ); /* shouldn't happen */
      }
    } else {
      /* slots[i].mb == -1, ergo it's free. */
      freeCount++;
    }
  }  
#endif
  /* since all 'inProgress' MBs were shown to be complete, 
     at this point "inProgressMB" means complete, but not consumed. 
     Establish whether there are MB's that arent either complete and consumed, 
     or sitting complete (but not consumed) in our buffer. */ 

  allDone = 1;
  for (i = 0; i < totalMB; i++) {
    if (!finishedMBBitmap[i] && !inProgressMB(i)) {
      allDone = 0;
      break;
    }
  }

  /* if there weren't incomplete MBs, craft a suitable reply. */

  if (allDone) {
    if (lastMessageCount) {
	/*
      printf("ChunkerSK: Last chunkerSK message repeated %i times.\n", 
	     lastMessageCount );
	*/
      lastMessageCount = 0;
    }
    printf("ChunkerSK: No incomplete MB's - done.\n");
    printf("ChunkerSK: %i redundant  KBs (MB needed, KB not)\n", redundantKBs);
    printf("ChunkerSK: %i redundant2 KBs (MB not needed)\n", redundantKBs2);
    /* tell caller we're done. */
    return -2;
  }
  
  if (freeCount < BUFFER_HEADROOM) {
    /* there exists one or more incomplete MBs, 
       but we cannot (or choose not to) deal with data. */
    if (lastMessage != 3) {
      if (lastMessageCount) {
/*
	printf("ChunkerSK: Last chunkerSK message repeated %i times.\n", 
	       lastMessageCount );
*/
	lastMessageCount = 0;
      }
/*
      printf("ChunkerSK: Buffer too full to recommend chunks.\n", i);
*/
      lastMessage = 3;
    } else {
      lastMessageCount++;
    }
    /* tell caller to punt. */
    return -1;
  } else {
    int beginEqualsICount = 0;
    int oneDoneYet = 0;
    int begin = rand() % totalMB; 
    i = begin;

    /* okay.. starting at a random MB, go through list of complete MBs;
       first find a MB which _is_ finished, then find the next MB which is _not_,
       and request that.
       If there are no finished MBs, request MB #1. 
       Note this loop must be processed twice for i == begin, 
       for the (rare) case that MB[begin] is the only incomplete MB in the list.
    */

    while (1) {
      /* this loop must get processed twice for i == begin, once for
         each other MB. */
      if (begin == i) { 
	beginEqualsICount++;
      } 

      if (oneDoneYet) {
	if (!finishedMBBitmap[i] && !inProgressMB(i)) {
	  /* found it. */
	  if (lastMessage != 2) {
	    if (lastMessageCount) {
/*
	      printf("ChunkerSK: Last chunkerSK message repeated %i times.\n", 
		     lastMessageCount );
*/
	      lastMessageCount = 0;
	    }
/*
	    printf("ChunkerSK: Suggesting a new MB (%i)...\n", i);
*/
	    lastMessage = 2;
	  } else {
	    lastMessageCount++;
	  }
	  return i * 1024;
	}
      } else {
	if (finishedMBBitmap[i] || inProgressMB(i)) { 
	  oneDoneYet = 1;
	}
      }  

      if (beginEqualsICount == 2) {
	/* we've now handled begin == i twice, so break. */ 
	break;
      }

      i = (i + 1) % totalMB;
    } /* while (1) */
    
    if (oneDoneYet == 0) {
      /* there were no complete MBs, so request beginning. */
      return 0;
    } else {
      /* there were no incomplete MBs-- this should've been caught. */ 
      assert( 0 ); /* there should be an incomplete. */
    }
  }
}

int c_consumeM( uint * mb, uchar ** data )
{
  int i;

  assert( mb );
  assert( data );

  for (i = 0; i < BufferSlotCount; i++) {
    if ((slots[i].mb != -1) && (slots[i].gotCount == 1024)) {
      *mb = slots[i].mb;
      *data = slots[i].data;
      return 1;
    }
  }

  return 0;
}

void c_finishedM( uint mb )
{
  int i;

  assert( mb >= 0 );
  assert( mb < totalMB );

  finishedMBBitmap[mb] = 1;

  for (i = 0; i < BufferSlotCount; i++) {
    if (slots[i].mb == mb) {
/*
      printf("Chunker: Finished MB %i (in slot %i)\n", mb, i ); 
*/
      slots[i].mb = -1;
      return;
    }
  }

  assert( 0 );
}

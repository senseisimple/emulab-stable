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
      printf("Chunker: Starting MB %i (in slot %i)\n", mb, i ); 
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

int c_suggestK()
{
  static int lastMessage = 0;
  static int lastMessageCount = 0;
  int i, j, ei;
  int freeCount = 0;
  int highestWeHave;

  for (i = 0; i < BufferSlotCount; i++) {
    if (slots[i].mb != -1) {
      if (slots[i].gotCount != 1024) {
	for (j = 0; j < 1024; j++) {
	  if (!slots[i].gotBitmap[j]) {
	    if (lastMessage != 1) {
	      if (lastMessageCount) {
		printf("ChunkerSK: Last ChunkerSK message repeated %i times.\n", 
		       lastMessageCount );
		lastMessageCount = 0;
	      }
	      printf("ChunkerSK: Suggesting kb from existing MB...\n");
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

  highestWeHave = 0;
  
  for (i = 1; i < totalMB; i++) {
    if (finishedMBBitmap[i]) {
      highestWeHave = i;
    }
  }

  for (ei = 0; ei < totalMB; ei++) {
    /* kind of a hack, so we start at the highest mb we have and
       swing around. */ 
    i = (highestWeHave + ei) % totalMB;
    if (!finishedMBBitmap[i] && !inProgressMB(i)) {
      if (freeCount >= BUFFER_HEADROOM ) {
	if (lastMessage != 2) {
	  if (lastMessageCount) {
	    printf("ChunkerSK: Last chunkerSK message repeated %i times.\n", 
		   lastMessageCount );
	    lastMessageCount = 0;
	  }
	  printf("ChunkerSK: Suggesting a new MB (%i)...\n", i);
	  lastMessage = 2;
	} else {
	  lastMessageCount++;
	}
	/* incomplete MB, and we *can* deal with data */
	return i * 1024;
      } else {
	/* incomplete MB, but we cannot deal with data. */
	if (lastMessage != 3) {
	  if (lastMessageCount) {
	    printf("ChunkerSK: Last chunkerSK message repeated %i times.\n", 
		   lastMessageCount );
	    lastMessageCount = 0;
	  }
	  printf("ChunkerSK: Buffer too full to recommend chunks.\n", i);
	  lastMessage = 3;
	} else {
	  lastMessageCount++;
	}
	return -1;
      }
    }
  }

  /* no incomplete MB -> done! */
  if (lastMessageCount) {
    printf("ChunkerSK: Last chunkerSK message repeated %i times.\n", 
	   lastMessageCount );
    lastMessageCount = 0;
  }
  printf("ChunkerSK: No incomplete MB's - done.\n");
  printf("ChunkerSK: %i redundant  KBs (MB needed, KB not)\n", redundantKBs);
  printf("ChunkerSK: %i redundant2 KBs (MB not needed)\n", redundantKBs2);
  return -2;
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
      printf("Chunker: Finishing MB %i (in slot %i)\n", mb, i ); 
      slots[i].mb = -1;
      return;
    }
  }

  assert( 0 );
}

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "f_common.h"
#include "f_chunker.h"

#define BufferSlotCount 32

typedef struct {
  int mb; /* -1 = none. */
  int gotCount;
  uchar gotBitmap[ 1024 ];
  uchar data[ 1024 * 1024 ];
} BufferSlot;

BufferSlot slots[BufferSlotCount];

int totalMB;
uchar * finishedMBBitmap;

void c_init( uint sizeM, uint bufferCount )
{
  int i;

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
      }
    }  
  }
}

int c_suggestK()
{
  int i, j;
  int wasAtLeastOneFree = 0;

  for (i = 0; i < BufferSlotCount; i++) {
    if (slots[i].mb != -1) {
      if (slots[i].gotCount != 1024) {
	for (j = 0; j < 1024; j++) {
	  if (!slots[i].gotBitmap[j]) {
	    return (slots[i].mb * 1024 + j);
	  }
	}
        assert( 0 ); /* shouldn't happen */
      }
    } else {
      /* slots[i].mb == -1, ergo it's free. */
      wasAtLeastOneFree = 1;
    }
  }  

  for (i = 0; i < totalMB; i++) {
    if (!finishedMBBitmap[i] && !inProgressMB(i)) {
      if (wasAtLeastOneFree) {
	/* incomplete MB, and we *can* deal with data */
	return i * 1024;
      } else {
	/* incomplete MB, but we cannot deal with data. */
	return -1;
      }
    }
  }

  /* no incomplete MB -> done! */
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

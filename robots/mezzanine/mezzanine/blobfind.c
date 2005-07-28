/* 
 *  Mezzanine - an overhead visual object tracker.
 *  Copyright (C) Andrew Howard 2002
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
/***************************************************************************
 * Desc: Blob finder
 * Author: Andrew Howard
 * Date: 28 Mar 2002
 * CVS: $Id: blobfind.c,v 1.2 2005-07-28 20:54:19 stack Exp $
 ***************************************************************************/

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "opt.h"
#include "mezzanine.h"


// Do a flood-fill to form a region
void blobfind_fill(uint8_t *class, uint8_t nclass, int ox, int oy);


// Information pertaining to the blob finder
typedef struct
{
  mezz_mmap_t *mmap;         // Pointer to the vision mmap area
  uint8_t blobmap[MEZZ_MAX_AREA];  // An image for forming blobs in-place.
  mezz_bloblist_t bloblist;        // List of extracted blobs.
} blobfind_t;


// The one and only instance of the blob finder
static blobfind_t *blobfind;


#define MIN(x, y) ((x) < (y) ? x : y)
#define MAX(x, y) ((x) > (y) ? x : y)


// Initialise the blob finder
int blobfind_init(mezz_mmap_t *mmap)
{
  mezz_blobdef_t *blobdef;

  blobfind = malloc(sizeof(blobfind_t));
  memset(blobfind, 0, sizeof(blobfind_t));

  blobfind->mmap = mmap;
  
  // Initialise the blob options
  blobdef = &blobfind->mmap->blobdef;
  blobdef->min_area = opt_get_int("blobfind", "min_area", 10);
  blobdef->max_area = opt_get_int("blobfind", "max_area", 400);
  blobdef->min_sizex = opt_get_int("blobfind", "min_sizex", 4);
  blobdef->max_sizex = opt_get_int("blobfind", "max_sizex", 20);
  blobdef->min_sizey = opt_get_int("blobfind", "min_sizey", 4);
  blobdef->max_sizey = opt_get_int("blobfind", "max_sizey", 20);
  
  return 0;
}


// Close the blob finder
void blobfind_term()
{
  mezz_blobdef_t *blobdef;
  
  // Save back some settings
  blobdef = &blobfind->mmap->blobdef;
  opt_set_int("blobfind", "min_area", blobdef->min_area);
  opt_set_int("blobfind", "max_area", blobdef->max_area);
  opt_set_int("blobfind", "min_sizex", blobdef->min_sizex);
  opt_set_int("blobfind", "max_sizex", blobdef->max_sizex);
  opt_set_int("blobfind", "min_sizey", blobdef->min_sizey);
  opt_set_int("blobfind", "max_sizey", blobdef->max_sizey);
  
  // Clean up
  free(blobfind);
}


// Extract blobs from image
// We look for pixels with valid classifications that have not
// yet been added to a blob, then build a new blob around them.
// This will segfault if it starts from the border of the image,
// so we rely on the classifier to generate null classifications
// for these pixels.  We dont do bounds-checking here since it
// slows thigs down too much.
mezz_bloblist_t *blobfind_update(uint8_t *class)
{
  register int i, area;
  register uint8_t *pclass;
  register uint8_t *pblob;
  int x, y;

  blobfind->bloblist.count = 0;
  memset(blobfind->blobmap, 0, sizeof(blobfind->blobmap));

  i = 0;
  area = blobfind->mmap->area;
  pclass = class;
  pblob = blobfind->blobmap;
  while (i < area)
  {
    if (*pclass != 0xFF && *pblob == 0)
    {
      x = i % blobfind->mmap->width;
      y = i / blobfind->mmap->width;
      blobfind_fill(class, *pclass, x, y);
    }
    
    i++;
    pclass++;
    pblob++;
  }

  // If the gui is enabled, copy our results there
  //  if (blobfind->mmap->calibrate)
  //  {
    memcpy(&blobfind->mmap->bloblist, &blobfind->bloblist,
           sizeof(blobfind->mmap->bloblist));
    //  }

  return &blobfind->bloblist;

}


// Do a flood-fill to form a blob.
// Returns the blobs stats.
void blobfind_fill(uint8_t *class, uint8_t nclass, int ox, int oy)
{
  int x, y, i, j;
  double mx, my;
  int sn, sl, sr, st, sb;
  int hood[8];
  uint8_t *pclass, *pblob;
  int top;
  uint8_t *stack[0x10000][2];
  mezz_blob_t *blob;
  mezz_blobdef_t *blobdef;

  // Initialise region stats
  sn = 0;
  sl = st = +10000;
  sr = sb = -10000;
  mx = my = 0;

  // Initialise neighborhood LUT
  hood[0] = -blobfind->mmap->width - 1;
  hood[1] = -blobfind->mmap->width;
  hood[2] = -blobfind->mmap->width + 1;
  hood[3] = -1;
  hood[4] = +1;
  hood[5] = +blobfind->mmap->width - 1;
  hood[6] = +blobfind->mmap->width;
  hood[7] = +blobfind->mmap->width + 1;
  
  // Push the first pixel on the stack
  top = 0;
  stack[top][0] = class + (ox + oy * blobfind->mmap->width);
  stack[top][1] = blobfind->blobmap + (ox + oy * blobfind->mmap->width);
  top++;

  // Now fill until the stack is empty
  while (top > 0)
  {
    top--;
    pclass = stack[top][0];
    pblob = stack[top][1];

    for (i = 0; i < 8; i++)
    {
      if (*(pclass + hood[i]) != nclass)
        continue;
      if (*(pblob + hood[i]) != 0)
        continue;
      if (top >= ARRAYSIZE(stack))
        continue;

      // Add this pixel to the region, and
      // place it on the end of the stack.
      *(pblob + hood[i]) = 1;
      stack[top][0] = pclass + hood[i];
      stack[top][1] = pblob + hood[i];
      top++;

      // Compute the x, y coords of this pixel
      j = (int) (pclass + hood[i] - class);
      x = j % blobfind->mmap->width;
      y = j / blobfind->mmap->width;

      // Update the region stats
      sn += 1;
      mx += x;
      my += y;
      sl = MIN(sl, x);
      sr = MAX(sr, x);
      st = MIN(st, y);
      sb = MAX(sb, y);
    }
  }

  //printf("blob %d %d %d\n", sn, sl, sr);
  
  // Check for overflow
  if (blobfind->bloblist.count >= ARRAYSIZE(blobfind->bloblist.blobs))
    return;
  
  // Now fill out stats
  blob = blobfind->bloblist.blobs + blobfind->bloblist.count;
  blob->class = nclass;
  blob->min_x = sl;
  blob->max_x = sr;
  blob->min_y = st;
  blob->max_y = sb;
  blob->area = sn;
  blob->ox = mx / sn;
  blob->oy = my / sn;
  blob->wox = 0;
  blob->woy = 0;
  blob->object = -1;

  // See if we like the look of this blob
  blobdef = &blobfind->mmap->blobdef;
  if (blob->area < blobdef->min_area || blob->area > blobdef->max_area)
    return;
  if (blob->max_x - blob->min_x < blobdef->min_sizex ||
      blob->max_x - blob->min_x > blobdef->max_sizex)
    return;
  if (blob->max_y - blob->min_y < blobdef->min_sizey ||
      blob->max_y - blob->min_y > blobdef->max_sizey)
    return;

  blobfind->bloblist.count++;
}




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
 * Desc: Assign identities to blobs
 * Author: Andrew Howard
 * Date: 21 Apr 2002
 * CVS: $Id: ident.c,v 1.4 2005-07-28 20:54:19 stack Exp $
 ***************************************************************************/

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "geom.h"
#include "opt.h"
#include "mezzanine.h"
#include <stdio.h>

// Find the nearest blob
mezz_blob_t *ident_get_nearest(mezz_bloblist_t *bloblist, int class,
                                double max_r, double ox, double oy);

// Ident information
typedef struct
{
    mezz_mmap_t *mmap;
  mezz_objectlist_t *objectlist;     // List of objects in the current frame
} ident_t;

// The one and only instance of the identer
static ident_t *ident;


// Initialise the identer
int ident_init(mezz_mmap_t *mmap)
{
  int i;
  //char key[64];
  //const char *value;
  mezz_object_t *object;

  ident = malloc(sizeof(ident_t));
  memset(ident, 0, sizeof(ident_t));

  ident->mmap = mmap;

  // Use the mmap'ed object list
  ident->objectlist = &mmap->objectlist;

  // Load the object descriptions
  ident->objectlist->count = opt_get_int("ident", "object_count", 0);
  for (i = 0; i < ident->objectlist->count; i++)
  {
    object = ident->objectlist->objects + i;
    object->class[0] = opt_get_int("ident", "class[0]", 0);
    object->class[1] = opt_get_int("ident", "class[1]", 1);
    object->max_disp = opt_get_double("ident", "max_disp", 0.10);
    object->max_sep = opt_get_double("ident", "max_sep", 0.15);
    object->max_missed = opt_get_int("ident", "max_missed", 30);
    object->missed = 1000;
	object->valid = 0;
  }
  
  return 0;
}


// Close the identer
void ident_term()
{
  // Clean up
  free(ident);
}


// Locate and identify objects from the list of blobs.
// This doesnt do any filtering on the pose.
mezz_objectlist_t *ident_update(mezz_bloblist_t *bloblist)
{
  int i;
  mezz_object_t *object;
  mezz_blob_t *ablob, *bblob;
  double mx, my, dx, dy;

  //printf("b\n");

  for (i = 0; i < ident->objectlist->count; i++)
  {
      object = ident->objectlist->objects + i;
      object->missed++;
      object->valid = 0;
      ablob = bblob = NULL;

      if (object->missed <= object->max_missed) {
/* 	  printf("x: %d\n",i); */
	  ablob = ident_get_nearest(bloblist, object->class[0],
				    object->max_disp, object->px, object->py);
      }
      else {
	  ablob = ident_get_nearest(bloblist, object->class[0],
				    1e16, object->px, object->py);
      }
      if (ablob == NULL) {
	  continue;
      }
      bblob = ident_get_nearest(bloblist, object->class[1],
				object->max_sep, ablob->wox, ablob->woy);
      if (bblob == NULL) {
/* 	  printf("y\n"); */
	  continue;
      }
      
      mx = (bblob->wox + ablob->wox)/ 2;
      my = (bblob->woy + ablob->woy)/ 2;
      dx = (bblob->wox - ablob->wox);
      dy = (bblob->woy - ablob->woy);
      
#if 0
      printf("a(%f,%f) b(%f,%f) -- %f %f\n",
 	     ablob->ox, ablob->oy,
 	     bblob->ox, bblob->oy,
 	     mx,my);
#endif
      
      object->valid = 1;
/*       printf("went valid on %d\n",i); */
      object->ablob = *ablob;
      object->bblob = *bblob;
      
      object->missed = 0;
      object->px = mx;
      object->py = my;
      object->pa = atan2(dy, dx);
      // we're seeing 0 to 180 and -1 to -179: fix:
      if (object->pa < 0.0) {
	  object->pa += 2.0*M_PI;
      }
      
      // Assign the blobs to this object
      //printf("assigned %d to a/b blobs; bloblist == 0x%x\n",i,bloblist);
      ablob->object = i;
      bblob->object = i;
  }

  // this code is no longer necessary
  
  // copy the bloblist to ipc seg
  // this saves the blobs
  /*   if (bloblist == NULL) { */
  /*       printf("bloblist == null\n"); */
  /*   } */
  /*   else { */
  /*       ident->mmap->bloblist = *bloblist; */
  /*   } */
  
  return ident->objectlist;
}


// Find the nearest blob
mezz_blob_t *ident_get_nearest(mezz_bloblist_t *bloblist, int class,
                                double max_r, double ox, double oy)
{
  int i;
  double dx, dy, dr;
  double min_dr;
  mezz_blob_t *blob, *min_blob;

  min_dr = max_r;
  min_blob = NULL;
  
  for (i = 0; i < bloblist->count; i++)
  {
    blob = bloblist->blobs + i;
    if (blob->class != class)
      continue;
    if (blob->object >= 0)
      continue;

    dx = blob->wox - ox;
    dy = blob->woy - oy;
    dr = sqrt(dx * dx + dy * dy);

    if (dr < min_dr)
    {
      min_blob = blob;
      min_dr = dr;
    }
/*     else { */
/* 	printf("not good: %f %f %f, on %f\n",dx,dy,dr,min_dr); */
/*     } */
  }
  
  return min_blob;
}




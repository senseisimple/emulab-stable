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
 * Desc: Functions for set blob parameters
 * Author: Andrew Howard
 * Date: 11 Apr 2002
 * CVS: $Id: blobfind.c,v 1.1 2004-12-12 23:36:34 johnsond Exp $
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "mezzcal.h"


// Information about all the definitions
typedef struct
{
  imagewnd_t *imagewnd;       // The window with the image
  tablewnd_t *tablewnd;       // The window with the table
  mezz_mmap_t *mmap;         // Pointer to mmap
  mezz_blobdef_t *blobdef;   // Pointer to mmap blob definition
  rtk_tableitem_t *area[2];   // Blob area settings
  rtk_tableitem_t *sizex[2];  // Blob width
  rtk_tableitem_t *sizey[2];  // Blob height
  rtk_fig_t *blobfig;         // Figure for drawing blobs
} blobfind_t;


// The one and only blobfind set
static blobfind_t *blobfind;


// Initialise the blobfind interface
int blobfind_init(imagewnd_t *imagewnd, tablewnd_t *tablewnd, mezz_mmap_t *mmap)
{
  blobfind = malloc(sizeof(blobfind_t));

  blobfind->imagewnd = imagewnd;
  blobfind->tablewnd = tablewnd;
  
  // Store pointer to the ipc mmap
  blobfind->mmap = mmap;
  blobfind->blobdef = &mmap->blobdef;

  blobfind->blobfig = rtk_fig_create(imagewnd->canvas, imagewnd->imagefig, 1);
  
  blobfind->area[0] = rtk_tableitem_create_int(tablewnd->table, "blob min area", 0, 10000);
  rtk_tableitem_set_int(blobfind->area[0], blobfind->blobdef->min_area);
  blobfind->area[1] = rtk_tableitem_create_int(tablewnd->table, "blob max area", 0, 10000);
  rtk_tableitem_set_int(blobfind->area[1], blobfind->blobdef->max_area);

  blobfind->sizex[0] = rtk_tableitem_create_int(tablewnd->table, "blob min width", 0, 10000);
  rtk_tableitem_set_int(blobfind->sizex[0], blobfind->blobdef->min_sizex);
  blobfind->sizex[1] = rtk_tableitem_create_int(tablewnd->table, "blob max width", 0, 10000);
  rtk_tableitem_set_int(blobfind->sizex[1], blobfind->blobdef->max_sizex);

  blobfind->sizey[0] = rtk_tableitem_create_int(tablewnd->table, "blob min height", 0, 10000);
  rtk_tableitem_set_int(blobfind->sizey[0], blobfind->blobdef->min_sizey);
  blobfind->sizey[1] = rtk_tableitem_create_int(tablewnd->table, "blob max height", 0, 10000);
  rtk_tableitem_set_int(blobfind->sizey[1], blobfind->blobdef->max_sizey);
  
  return 0;
}


// Update the blobfind interface
void blobfind_update()
{
  int i;
  int color;
  mezz_blob_t *blob;
  
  // Draw in the extracted blobs
  rtk_fig_clear(blobfind->blobfig);
  if (rtk_menuitem_ischecked(blobfind->imagewnd->blobitem))
  {
    for (i = 0; i < blobfind->mmap->bloblist.count; i++)
    {
      blob = blobfind->mmap->bloblist.blobs + i;
      if (blob->class < 0)
        continue;
      color = blobfind->mmap->classdeflist.classdefs[blob->class].color;
      rtk_fig_color_rgb32(blobfind->blobfig, color);
      rtk_fig_rectangle(blobfind->blobfig,
                        (blob->max_x + blob->min_x)/2,
                        (blob->max_y + blob->min_y)/2, 0,
                        (blob->max_x - blob->min_x),
                        (blob->max_y - blob->min_y), 0);
    }
  }

  // Update blob settings from the table
  blobfind->blobdef->min_area = rtk_tableitem_get_int(blobfind->area[0]);
  blobfind->blobdef->max_area = rtk_tableitem_get_int(blobfind->area[1]);
  blobfind->blobdef->min_sizex = rtk_tableitem_get_int(blobfind->sizex[0]);
  blobfind->blobdef->max_sizex = rtk_tableitem_get_int(blobfind->sizex[1]);
  blobfind->blobdef->min_sizey = rtk_tableitem_get_int(blobfind->sizey[0]);
  blobfind->blobdef->max_sizey = rtk_tableitem_get_int(blobfind->sizey[1]);
}


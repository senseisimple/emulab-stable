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
 * Desc: Manipulate the image mask
 * Author: Andrew Howard
 * Date: 11 Apr 2002
 * CVS: $Id: mask.c,v 1.1 2004-12-12 23:36:34 johnsond Exp $
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "mezzcal.h"


// Image mask info.
typedef struct
{
  imagewnd_t *imagewnd;   // Window containing raw image data.
  mezz_maskdef_t *mmap;  // Pointer to mmap
  rtk_fig_t *polyfig;     // Mask polygon edges
  rtk_fig_t *fig[MEZZ_MAX_MASK]; // Mask polygon vertices
} mask_t;


// The one and only mask
static mask_t *mask;


// Initialise the mask interface
int mask_init(imagewnd_t *imagewnd, mezz_mmap_t *mmap)
{
  int i;

  mask = malloc(sizeof(mask_t));

  // Get the mask part of the mmap
  mask->imagewnd = imagewnd;
  mask->mmap = &mmap->maskdef;

  // Generate figs describing the mask polygon
  for (i = 0; i < MEZZ_MAX_MASK; i++)
  {
    mask->fig[i] = rtk_fig_create(imagewnd->canvas, imagewnd->imagefig, 1);
    rtk_fig_movemask(mask->fig[i], RTK_MOVE_TRANS);
    rtk_fig_origin(mask->fig[i], mask->mmap->poly[i][0], mask->mmap->poly[i][1], 0);
    rtk_fig_color(mask->fig[i], COLOR_MASK);
    rtk_fig_rectangle(mask->fig[i], 0, 0, 0, 10, 10, 0);
  }
  mask->polyfig = rtk_fig_create(imagewnd->canvas, imagewnd->imagefig, 1);
  
  return 0;
}


// Update the mask interface
void mask_update()
{
  int i;
  double ax, ay, aa;
  double bx, by, ba;
  int showmask;

  // Either show or hide the mask figures
  showmask = rtk_menuitem_ischecked(mask->imagewnd->maskitem);
  for (i = 0; i < MEZZ_MAX_MASK; i++)
    rtk_fig_show(mask->fig[i], showmask);
  rtk_fig_show(mask->polyfig, showmask);
  
  // Re-draw the polygon edges so they match the
  // vertices (which may have been moved by the user).
  // At the same time, update the mmap.
  rtk_fig_clear(mask->polyfig);
  rtk_fig_color(mask->polyfig, COLOR_MASK);
  for (i = 0; i < MEZZ_MAX_MASK; i++)
  {    
    rtk_fig_get_origin(mask->fig[i], &ax, &ay, &aa);
    rtk_fig_get_origin(mask->fig[(i + 1) % MEZZ_MAX_MASK], &bx, &by, &ba);
    rtk_fig_line(mask->polyfig, ax, ay, bx, by);
    mask->mmap->poly[i][0] = ax;
    mask->mmap->poly[i][1] = ay;
  }
}


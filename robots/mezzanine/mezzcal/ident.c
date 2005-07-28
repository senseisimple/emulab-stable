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
 * Desc: Display identified objects and set object definitions.
 * Author: Andrew Howard
 * Date: 11 Apr 2002
 * CVS: $Id: ident.c,v 1.2 2005-07-28 20:54:20 stack Exp $
 ***************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "color.h"
#include "mezzcal.h"


// Information pertaining the object identification
typedef struct
{
  imagewnd_t *imagewnd;           // The image window
  mezz_objectlist_t *objectlist; // The object list
  rtk_fig_t *fig;                 // Figure for drawing objects
} ident_t;


// The one and only interface
static ident_t *ident;


// Initialise the ident interface
int ident_init(imagewnd_t *imagewnd, mezz_mmap_t *mmap)
{
  ident = malloc(sizeof(ident_t));

  ident->imagewnd = imagewnd;
  ident->objectlist = &mmap->objectlist;

  // Create figure for drawing objects
  ident->fig = rtk_fig_create(imagewnd->canvas, imagewnd->imagefig, 10);

  return 0;
}


// Update the objects
void ident_update()
{
  int i, j;
  double r;
  char text[64];
  double ax, ay, aa, bx, by, ba;
  mezz_object_t *object;
  
  rtk_fig_clear(ident->fig);

  for (i = 0; i < ident->objectlist->count; i++)
  {
    object = ident->objectlist->objects + i;

    if (!object->valid)
      continue;
    
    r = object->max_sep;
    
    dewarp_world2image(object->px, object->py, &ax, &ay);
    dewarp_world2image(object->px + r * cos(object->pa+M_PI_2),
                       object->py + r * sin(object->pa+M_PI_2), &bx, &by);

    rtk_fig_color(ident->fig, COLOR_IDENT);
    rtk_fig_arrow_ex(ident->fig, ax, ay, bx, by, 5);

    snprintf(text, sizeof(text), "obj [%d] (%.2f, %.2f)\n    (%.0f, %.0f)",
	     i, object->px, object->py,
	     (object->ablob.ox + object->bblob.ox) / 2.0,
	     (object->ablob.oy + object->bblob.oy) / 2.0);
    rtk_fig_text(ident->fig, ax + 10, ay - 20, 0, text);

    for (j = 0; j < 4; j++)
    {
      aa = object->pa + M_PI/4 + j * M_PI/2;
      ba = object->pa + M_PI/4 + (j + 1) * M_PI/2;
      
      dewarp_world2image(object->px + r * cos(aa),
                         object->py + r * sin(aa), &ax, &ay);
      dewarp_world2image(object->px + r * cos(ba),
                         object->py + r * sin(ba), &bx, &by);
      rtk_fig_line(ident->fig, ax, ay, bx, by);
    }
  }
}

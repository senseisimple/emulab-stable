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
 * Desc: Functions for maintaining the color classification.
 * Author: Andrew Howard
 * Date: 11 Apr 2002
 * CVS: $Id: classify.c,v 1.1 2004-12-12 23:36:34 johnsond Exp $
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "mezzcal.h"


// Information about one color definition
typedef struct
{
  mezz_classdef_t *mmap; // Pointer to mmap color class definitions
  rtk_menuitem_t *menuitem;   // Menu item that allows us to select this color.
  rtk_fig_t *vufigs[4];   // Polygon vertices in VU space
  rtk_fig_t *vupolyfig;   // Polygon edges in VU space
  rtk_fig_t *yufigs[4];   // Polygon vertices in YU space
  rtk_fig_t *yupolyfig;   // Polygon edges in YU space
  rtk_fig_t *vyfigs[4];   // Polygon vertices in VY space
  rtk_fig_t *vypolyfig;   // Polygon edges in VY space
} classdef_t;


// Information about all the definitions
typedef struct
{
  mezz_mmap_t *mmap;   // Pointer to vision calibration mmap.
  classdef_t classdefs[MEZZ_MAX_CLASSES];  // Local  color class definitions
  rtk_menu_t *menu;     // A menu to put our options
} classify_t;


// The one and only classify set
static classify_t *classify;


// Initialise the classify interface
int classify_init(colorwnd_t *colorwnd, mezz_mmap_t *mmap)
{
  int i, j;
  classdef_t *classdef;
  
  classify = malloc(sizeof(classify_t));

  // Store pointer to the ipc mmap
  classify->mmap = mmap;

  // Create a menu to put our menu options
  classify->menu = rtk_menu_create(colorwnd->canvas, "Classes");
  
  // Initialise all of the color definitions
  for (j = 0; j < classify->mmap->classdeflist.count; j++)
  {
    classdef = classify->classdefs + j;
    classdef->mmap = classify->mmap->classdeflist.classdefs + j;

    // Add a menu item so we can select this color
    classdef->menuitem = rtk_menuitem_create(classify->menu, classdef->mmap->name, 1);
    rtk_menuitem_check(classdef->menuitem, 1);    
    
    // Create the VU figs
    classdef->vupolyfig = rtk_fig_create(colorwnd->canvas, colorwnd->vufig, 3);
    for (i = 0; i < 4; i++)
    {
      classdef->vufigs[i] = rtk_fig_create(colorwnd->canvas, colorwnd->vufig, 3);
      rtk_fig_movemask(classdef->vufigs[i], RTK_MOVE_TRANS);
      rtk_fig_origin(classdef->vufigs[i],
                     classdef->mmap->vupoly[i][0],
                     classdef->mmap->vupoly[i][1], 0);
      rtk_fig_color_rgb32(classdef->vufigs[i], classdef->mmap->color);
      rtk_fig_rectangle(classdef->vufigs[i], 0, 0, 0, 10, 10, 0);
    }

    // Create the YU figs
    classdef->yupolyfig = rtk_fig_create(colorwnd->canvas, colorwnd->yufig, 3);
    for (i = 0; i < 4; i++)
    {
      classdef->yufigs[i] = rtk_fig_create(colorwnd->canvas, colorwnd->yufig, 3);
      rtk_fig_movemask(classdef->yufigs[i], RTK_MOVE_TRANS);
      rtk_fig_origin(classdef->yufigs[i],
                     classdef->mmap->yupoly[i][0],
                     classdef->mmap->yupoly[i][1], 0);
      rtk_fig_color_rgb32(classdef->yufigs[i], classdef->mmap->color);
      rtk_fig_rectangle(classdef->yufigs[i], 0, 0, 0, 10, 10, 0);
    }

    // Create the VY figs
    classdef->vypolyfig = rtk_fig_create(colorwnd->canvas, colorwnd->vyfig, 3);
    for (i = 0; i < 4; i++)
    {
      classdef->vyfigs[i] = rtk_fig_create(colorwnd->canvas, colorwnd->vyfig, 3);
      rtk_fig_movemask(classdef->vyfigs[i], RTK_MOVE_TRANS);
      rtk_fig_origin(classdef->vyfigs[i],
                     classdef->mmap->vypoly[i][0],
                     classdef->mmap->vypoly[i][1], 0);
      rtk_fig_color_rgb32(classdef->vyfigs[i], classdef->mmap->color);
      rtk_fig_rectangle(classdef->vyfigs[i], 0, 0, 0, 10, 10, 0);
    }
  }
  
  return 0;
}


// Update the classify interface
void classify_update()
{
  int i, j;
  int show;
  double ax, ay, aa;
  double bx, by, ba;
  classdef_t *classdef;

  // Initialise all of the color definitions
  for (j = 0; j < classify->mmap->classdeflist.count; j++)
  {
    classdef = classify->classdefs + j;

    // Either show or hide the figs for this color
    show = rtk_menuitem_ischecked(classdef->menuitem);
    rtk_fig_show(classdef->vupolyfig, show);
    rtk_fig_show(classdef->yupolyfig, show);
    rtk_fig_show(classdef->vypolyfig, show);
    for (i = 0; i < 4; i++)
    {
      rtk_fig_show(classdef->vufigs[i], show);
      rtk_fig_show(classdef->yufigs[i], show);
      rtk_fig_show(classdef->vyfigs[i], show);
    }

    // Update the VU polygon
    rtk_fig_clear(classdef->vupolyfig);
    rtk_fig_color_rgb32(classdef->vupolyfig, classdef->mmap->color);
    for (i = 0; i < 4; i++)
    {
      rtk_fig_get_origin(classdef->vufigs[i], &ax, &ay, &aa);
      rtk_fig_get_origin(classdef->vufigs[(i + 1) % 4], &bx, &by, &ba);
      rtk_fig_line(classdef->vupolyfig, ax, ay, bx, by);
      classdef->mmap->vupoly[i][0] = ax;
      classdef->mmap->vupoly[i][1] = ay;
    }

    // Update the YU polygon
    rtk_fig_clear(classdef->yupolyfig);
    rtk_fig_color_rgb32(classdef->yupolyfig, classdef->mmap->color);
    for (i = 0; i < 4; i++)
    {
      rtk_fig_get_origin(classdef->yufigs[i], &ax, &ay, &aa);
      rtk_fig_get_origin(classdef->yufigs[(i + 1) % 4], &bx, &by, &ba);
      rtk_fig_line(classdef->yupolyfig, ax, ay, bx, by);
      classdef->mmap->yupoly[i][0] = ax;
      classdef->mmap->yupoly[i][1] = ay;
    }

    // Update the VY polygon
    rtk_fig_clear(classdef->vypolyfig);
    rtk_fig_color_rgb32(classdef->vypolyfig, classdef->mmap->color);
    for (i = 0; i < 4; i++)
    {
      rtk_fig_get_origin(classdef->vyfigs[i], &ax, &ay, &aa);
      rtk_fig_get_origin(classdef->vyfigs[(i + 1) % 4], &bx, &by, &ba);
      rtk_fig_line(classdef->vypolyfig, ax, ay, bx, by);
      classdef->mmap->vypoly[i][0] = ax;
      classdef->mmap->vypoly[i][1] = ay;
    }
  }
}


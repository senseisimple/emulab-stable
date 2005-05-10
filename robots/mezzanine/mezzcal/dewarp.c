/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
 * Desc: Dewarping interface
 * Author: Andrew Howard
 * Date: 11 Apr 2002
 * CVS: $Id: dewarp.c,v 1.2 2005-05-10 15:25:17 johnsond Exp $
 ***************************************************************************/

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "mezzcal.h"


// Generate a warped grid over the field
void dewarp_update_grid();


// Dewarping info
typedef struct
{
  imagewnd_t *imagewnd;     // Window with image figure
  mezz_dewarpdef_t *mmap;  // Pointer to dewarp mmap
  rtk_fig_t *figs[MEZZ_MAX_DEWARP];  // Figures denoting the calibration points.
  rtk_fig_t *gridfig;     // Figure for displaying the warped grid.
} dewarp_t;


// The one and only dewarping interface
static dewarp_t *dewarp;

// Initialise the dewarping interface
int dewarp_init(imagewnd_t *imagewnd, mezz_mmap_t *mmap)
{
  int i;
  char text[64];
  
  dewarp = malloc(sizeof(dewarp_t));
  dewarp->imagewnd = imagewnd;
  dewarp->mmap = &mmap->dewarpdef;

  // Create the figures we will use to calibrate the dewarp.
  for (i = 0; i < dewarp->mmap->points; i++)
  {
    dewarp->figs[i] = rtk_fig_create(imagewnd->canvas, imagewnd->imagefig, 2);
    rtk_fig_origin(dewarp->figs[i],
                   dewarp->mmap->ipos[i][0], dewarp->mmap->ipos[i][1], 0);
    rtk_fig_movemask(dewarp->figs[i], RTK_MOVE_TRANS);
    rtk_fig_color(dewarp->figs[i], COLOR_DEWARP);
    rtk_fig_rectangle(dewarp->figs[i], 0, 0, 0, 10, 10, 0);
    snprintf(text, sizeof(text), "(%.3f, %0.3f)",
             dewarp->mmap->wpos[i][0], dewarp->mmap->wpos[i][1]);
    rtk_fig_text(dewarp->figs[i], 20, 0, 0, text);
  }
  dewarp->gridfig = rtk_fig_create(imagewnd->canvas, NULL, 1);

  return 0;
}


// Update the dewarp interface
void dewarp_update()
{
  int i;
  double ox, oy, oa;
  
  // Get the image coords of the calibration points
  for (i = 0; i < dewarp->mmap->points; i++)
  {
    rtk_fig_get_origin(dewarp->figs[i], &ox, &oy, &oa);
    dewarp->mmap->ipos[i][0] = ox;
    dewarp->mmap->ipos[i][1] = oy;
  }

  // Now draw the grid
  dewarp_update_grid();
}


// Generate a warped grid over the field
void dewarp_update_grid()
{
  int i;
  int showdewarp;
  double ax, ay;
  double ai, aj, bi, bj;
  double sx, sy;
  double dx, dy;

  // Grid size and spacing
  sx = 4.0; sy = 4.0;
  dx = 0.25; dy = 0.25;

  // Either show or hide the dewarping figures
  showdewarp = rtk_menuitem_ischecked(dewarp->imagewnd->dewarpitem);
  for (i = 0; i < dewarp->mmap->points; i++)
    rtk_fig_show(dewarp->figs[i], showdewarp);
  rtk_fig_show(dewarp->gridfig, showdewarp);
  
  rtk_fig_clear(dewarp->gridfig);
  rtk_fig_color(dewarp->gridfig, COLOR_DEWARP_GRID);
    
  // Draw the horizontal grid lines
  for (ay = -sy / 2; ay <= +sy / 2; ay += dy)
  {
    for (ax = -sx / 2; ax < +sx / 2; ax += dx)
    {
      dewarp_world2image(ax, ay, &ai, &aj);
      dewarp_world2image(ax + dx, ay, &bi, &bj);
      rtk_fig_line(dewarp->gridfig, ai, aj, bi, bj);
    }
  }

  // Draw the vertical  grid lines
  for (ax = -sx / 2; ax <= +sx / 2; ax += dx)
  {
    for (ay = -sy / 2; ay < +sy / 2; ay += dy)
    {
      dewarp_world2image(ax, ay, &ai, &aj);
      dewarp_world2image(ax, ay + dy, &bi, &bj);
      rtk_fig_line(dewarp->gridfig, ai, aj, bi, bj);
    }
  }
}


// Convert point from world to image coords
void dewarp_world2image(double x, double y, double *i, double *j)
{
  *i = dewarp->mmap->witrans[0][0] + dewarp->mmap->witrans[0][1] * x +
    + dewarp->mmap->witrans[0][2] * y + dewarp->mmap->witrans[0][3] * x * x
    + dewarp->mmap->witrans[0][4] * y * y + dewarp->mmap->witrans[0][5] * x * y
    + dewarp->mmap->witrans[0][6] * x * fabs(x) + dewarp->mmap->witrans[0][7] * x * y * y;

  *j = dewarp->mmap->witrans[1][0] + dewarp->mmap->witrans[1][1] * y
    + dewarp->mmap->witrans[1][2] * x + dewarp->mmap->witrans[1][3] * y * y
    + dewarp->mmap->witrans[1][4] * x * x + dewarp->mmap->witrans[1][5] * y * x
    + dewarp->mmap->witrans[1][6] * y * fabs(y) + dewarp->mmap->witrans[1][7] * y * x * x;
}



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
 * CVS: $Id: dewarp.c,v 1.3 2005-07-28 20:54:20 stack Exp $
 ***************************************************************************/

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "mezzcal.h"
#include "opt.h"

#define WARP_SCALE
#define WARP_COS

// Generate a warped grid over the field
void dewarp_update_grid();


// Convert point from image to world coords
void dewarp_image2world(double i, double j, double *x, double *y);

// Convert point from world to image coords
void dewarp_world2image(double x, double y, double *i, double *j);

// Dewarping info
typedef struct
{
  imagewnd_t *imagewnd;     // Window with image figure
  mezz_dewarpdef_t *mmap;  // Pointer to dewarp mmap
  rtk_fig_t *figs[MEZZ_MAX_DEWARP];  // Figures denoting the calibration points.
  rtk_fig_t *gridfig;     // Figure for displaying the warped grid.

  rtk_tableitem_t *oc[3];
  rtk_tableitem_t *scalers[2];
  rtk_tableitem_t *warp;
  
  rtk_tableitem_t *grid[2];
} dewarp_t;


// The one and only dewarping interface
static dewarp_t *dewarp;

// Initialise the dewarping interface
int dewarp_init(imagewnd_t *imagewnd, tablewnd_t *tablewnd, mezz_mmap_t *mmap)
{
  int i;
  char text[64];

  dewarp = malloc(sizeof(dewarp_t));
  dewarp->imagewnd = imagewnd;
  dewarp->mmap = &mmap->dewarpdef;

  dewarp->oc[0] = rtk_tableitem_create_int(tablewnd->table, "Opt. Center X", 0, mmap->width);
  rtk_tableitem_set_int(dewarp->oc[0], dewarp->mmap->ocX);
  dewarp->oc[1] = rtk_tableitem_create_int(tablewnd->table, "Opt. Center Y", 0, mmap->height);
  rtk_tableitem_set_int(dewarp->oc[1], dewarp->mmap->ocY);
  dewarp->oc[2] = rtk_tableitem_create_float(tablewnd->table, "Opt. Center Z", 0, 1000, 0.10);
  rtk_tableitem_set_float(dewarp->oc[2], dewarp->mmap->ocHeight);
  
  dewarp->scalers[0] = rtk_tableitem_create_float(tablewnd->table, "Scale X", 0, 1000, 1.0);
  rtk_tableitem_set_float(dewarp->scalers[0], dewarp->mmap->scaleFactorX);
  dewarp->scalers[1] = rtk_tableitem_create_float(tablewnd->table, "Scale Y", 0, 1000, 1.0);
  rtk_tableitem_set_float(dewarp->scalers[1], dewarp->mmap->scaleFactorY);
  
  dewarp->warp = rtk_tableitem_create_float(tablewnd->table, "Warp", 0, 10, 0.01);
  rtk_tableitem_set_float(dewarp->warp, dewarp->mmap->warpFactor);

  dewarp->grid[0] = rtk_tableitem_create_float(tablewnd->table, "Grid X", -5, 5, 0.01);
  rtk_tableitem_set_float(dewarp->grid[0], dewarp->mmap->gridX);
  dewarp->grid[1] = rtk_tableitem_create_float(tablewnd->table, "Grid Y", -5, 5, 0.01);
  rtk_tableitem_set_float(dewarp->grid[1], dewarp->mmap->gridY);
  
  // Create the figures we will use to calibrate the dewarp.
  for (i = 0; i < dewarp->mmap->points; i++)
  {
    dewarp->figs[i] = rtk_fig_create(imagewnd->canvas, imagewnd->imagefig, 2);
    rtk_fig_origin(dewarp->figs[i],
                   dewarp->mmap->ipos[i][0], dewarp->mmap->ipos[i][1], 0);
    rtk_fig_movemask(dewarp->figs[i], RTK_MOVE_TRANS);
    rtk_fig_color(dewarp->figs[i], COLOR_DEWARP);
    rtk_fig_rectangle(dewarp->figs[i], 0, 0, 0, 10, 10, 0);
    snprintf(text, sizeof(text), "(%.1f, %0.1f)",
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

  dewarp->mmap->ocX = rtk_tableitem_get_int(dewarp->oc[0]);
  dewarp->mmap->ocY = rtk_tableitem_get_int(dewarp->oc[1]);
  dewarp->mmap->ocHeight = rtk_tableitem_get_float(dewarp->oc[2]);

  dewarp->mmap->scaleFactorX = rtk_tableitem_get_float(dewarp->scalers[0]);
  dewarp->mmap->scaleFactorY = rtk_tableitem_get_float(dewarp->scalers[1]);
  
  dewarp->mmap->warpFactor = rtk_tableitem_get_float(dewarp->warp);

  dewarp->mmap->gridX = rtk_tableitem_get_float(dewarp->grid[0]);
  dewarp->mmap->gridY = rtk_tableitem_get_float(dewarp->grid[1]);
  
  // Now draw the grid
  dewarp_update_grid();
}

#define WASH_POINTS

// Generate a warped grid over the field
void dewarp_update_grid()
{
  int i;
  int showdewarp;
  double ax, ay, tx, ty;
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
      dewarp_world2image(dewarp->mmap->gridX + ax,
			 dewarp->mmap->gridY + ay, &ai, &aj);
#ifdef WASH_POINTS
      dewarp_image2world(ai, aj, &tx, &ty);
      dewarp_world2image(tx, ty, &ai, &aj);
#endif
      
      dewarp_world2image(dewarp->mmap->gridX + ax + dx,
			 dewarp->mmap->gridY + ay, &bi, &bj);
#ifdef WASH_POINTS
      dewarp_image2world(bi, bj, &tx, &ty);
      dewarp_world2image(tx, ty, &bi, &bj);
#endif
      
      rtk_fig_line(dewarp->gridfig, ai, aj, bi, bj);
    }
  }

  // Draw the vertical  grid lines
  for (ax = -sx / 2; ax <= +sx / 2; ax += dx)
  {
    for (ay = -sy / 2; ay < +sy / 2; ay += dy)
    {
      dewarp_world2image(dewarp->mmap->gridX + ax,
			 dewarp->mmap->gridY + ay, &ai, &aj);
#ifdef WASH_POINTS
      dewarp_image2world(ai, aj, &tx, &ty);
      dewarp_world2image(tx, ty, &ai, &aj);
#endif
      
      dewarp_world2image(dewarp->mmap->gridX + ax,
			 dewarp->mmap->gridY + ay + dy, &bi, &bj);
#ifdef WASH_POINTS
      dewarp_image2world(bi, bj, &tx, &ty);
      dewarp_world2image(tx, ty, &bi, &bj);
#endif
      
      rtk_fig_line(dewarp->gridfig, ai, aj, bi, bj);
    }
  }
}

double dewarp_cos(double x, double y, mezz_dewarpdef_t *mmap)
{
    return cos(mmap->warpFactor * atan2(hypot(x,y), mmap->ocHeight));
}

// Convert point from world to image coords
void dewarp_world2image(double x, double y, double *i, double *j)
{

#ifdef WARP_IDENTITY
    *i = x;
    *j = y;

#elif defined(WARP_SCALE) || defined(WARP_COS)

# if defined(WARP_COS)
    // Warp by the cosine of the off-axis angle.
    double f = dewarp_cos(x, y, dewarp->mmap);
# else
    const double f = 1.0;
# endif

    // World coordinates are [-1 ... 1, -1 ... 1], with the origin in the
    // center.
    // Pixels are  to [0 ... 639, 0 ... 479], with the origin in the
    // upper-left.
    *i = ( (x * f * dewarp->mmap->scaleFactorX) + dewarp->mmap->ocX );
    *j = ( -(y * f * dewarp->mmap->scaleFactorY) + dewarp->mmap->ocY );

#else
  *i = dewarp->def->witrans[0][0] + dewarp->def->witrans[0][1] * x +
    + dewarp->def->witrans[0][2] * y + dewarp->def->witrans[0][3] * x * x
    + dewarp->def->witrans[0][4] * y * y + dewarp->def->witrans[0][5] * x * y
      + dewarp->def->witrans[0][6] * x * fabs(x) + dewarp->def->witrans[0][7] * x\
      * y * y;

  *j = dewarp->def->witrans[1][0] + dewarp->def->witrans[1][1] * y
    + dewarp->def->witrans[1][2] * x + dewarp->def->witrans[1][3] * y * y
    + dewarp->def->witrans[1][4] * x * x + dewarp->def->witrans[1][5] * y * x
      + dewarp->def->witrans[1][6] * y * fabs(y) + dewarp->def->witrans[1][7] * y\
      * x * x;
#endif

  /*   *i = dewarp->def->witrans[0][0] + dewarp->def->witrans[0][1] * x + */
  /*     + dewarp->def->witrans[0][2] * y + dewarp->def->witrans[0][3] * x * x
	 */
  /*     + dewarp->def->witrans[0][4] * y * y + dewarp->def->witrans[0][5] * x
   * y */
  /*     + dewarp->def->witrans[0][6] * x * fabs(x) +
	 dewarp->def->witrans[0][7] * x * y * y; */

  /*   *j = dewarp->def->witrans[1][0] + dewarp->def->witrans[1][1] * y */
  /*     + dewarp->def->witrans[1][2] * x + dewarp->def->witrans[1][3] * y * y
	 */
  /*     + dewarp->def->witrans[1][4] * x * x + dewarp->def->witrans[1][5] * y
   * x */
  /*     + dewarp->def->witrans[1][6] * y * fabs(y) +
	 dewarp->def->witrans[1][7]
	 * y * x * x; */

  //    *i = x;
  //    *j = y;

}

// Convert point from image to world coords
void dewarp_image2world(double i, double j, double *x, double *y)
{

#ifdef WARP_IDENTITY
  // World coords are the same as pixel coords.
  *x = i;
  *y = j;

#elif defined(WARP_SCALE)
  // Pixels are  to [0 ... 639, 0 ... 479], with the origin in the upper-left.
  // World coordinates are [-1 ... 1, -1 ... 1], with the origin in the center.
  *x = (i - dewarp->mmap->ocX) / dewarp->mmap->scaleFactorX;
  *y = -(j - dewarp->mmap->ocY) / dewarp->mmap->scaleFactorY;

# if defined(WARP_COS)
  double f = 1.0;
  int lpc;
  
  for (lpc = 0; lpc < 8; lpc++) {
      f = dewarp_cos(*x / f, *y / f, dewarp->mmap);
  }

  *x /= f;
  *y /= f;
# endif

#else
  *x = dewarp->mmap->iwtrans[0][0] + dewarp->mmap->iwtrans[0][1] * i +
    + dewarp->mmap->iwtrans[0][2] * j + dewarp->mmap->iwtrans[0][3] * i * i
    + dewarp->mmap->iwtrans[0][4] * j * j + dewarp->mmap->iwtrans[0][5] * i * j
    + dewarp->mmap->iwtrans[0][6] * i * fabs(i) + dewarp->mmap->iwtrans[0][7] * i\
 * j * j;

  *y = dewarp->mmap->iwtrans[1][0] + dewarp->mmap->iwtrans[1][1] * j
    + dewarp->mmap->iwtrans[1][2] * i + dewarp->mmap->iwtrans[1][3] * j * j
    + dewarp->mmap->iwtrans[1][4] * i * i + dewarp->mmap->iwtrans[1][5] * j * i
    + dewarp->mmap->iwtrans[1][6] * j * fabs(j) + dewarp->mmap->iwtrans[1][7] * j\
 * i * i;
#endif

/*   *x = dewarp->def->iwtrans[0][0] + dewarp->def->iwtrans[0][1] * i + */
/*     + dewarp->def->iwtrans[0][2] * j + dewarp->def->iwtrans[0][3] * i * i */
/*     + dewarp->def->iwtrans[0][4] * j * j + dewarp->def->iwtrans[0][5] * i * j */
/*     + dewarp->def->iwtrans[0][6] * i * fabs(i) + dewarp->def->iwtrans[0][7] * i * j * j; */

/*   *y = dewarp->def->iwtrans[1][0] + dewarp->def->iwtrans[1][1] * j */
/*     + dewarp->def->iwtrans[1][2] * i + dewarp->def->iwtrans[1][3] * j * j */
/*     + dewarp->def->iwtrans[1][4] * i * i + dewarp->def->iwtrans[1][5] * j * i */
/*     + dewarp->def->iwtrans[1][6] * j * fabs(j) + dewarp->def->iwtrans[1][7]
       * j * i * i; */

  //*x = i;
  //*y = j;

}

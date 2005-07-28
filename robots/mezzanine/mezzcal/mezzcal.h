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
 * Desc: Public strutures, functions
 * Author: Andrew Howard
 * Date: 28 Mar 2002
 * CVS: $Id: mezzcal.h,v 1.2 2005-07-28 20:54:20 stack Exp $
 ***************************************************************************/

#ifndef MEZZCAL_H
#define MEZZCAL_H

#include "mezz.h"
#include "rtk.h"

/***************************************************************************
 * Colors
 ***************************************************************************/

#define COLOR_SAMPLE 1, 0, 0
#define COLOR_MASK   1, 1, 0
#define COLOR_DEWARP 0, 0.7, 0
#define COLOR_DEWARP_GRID 0, 0.7, 0
#define COLOR_IDENT  0.7, 0.7, 1.0


/***************************************************************************
 * Top-level GUI elements
 ***************************************************************************/

// Window containing the raw image data,
// classifications, etc.
typedef struct
{
  mezz_mmap_t *mmap;       // The mmap
  rtk_canvas_t *canvas;     // The canvas
  rtk_fig_t *imagefig;      // Image figure.
  rtk_menu_t *filemenu;     // Menu containing file options
  rtk_menuitem_t *saveitem; // Save changes to configuration
  rtk_menu_t *viewmenu;     // Menu containing view options
  rtk_menuitem_t *maskitem;     // Show image mask menu option
  rtk_menuitem_t *classitem;    // Show classes menu option
  rtk_menuitem_t *blobitem;     // Show blobs menu option
  rtk_menuitem_t *dewarpitem;   // Show dewarp points menu option
} imagewnd_t;


// Create the image window
imagewnd_t *imagewnd_init(rtk_app_t *app, mezz_mmap_t *mmap);

// Update the window
void imagewnd_update(imagewnd_t *wnd);


// Window containing the YUV color space
typedef struct
{
  rtk_canvas_t *canvas;
  rtk_fig_t *vufig, *yufig, *vyfig;
} colorwnd_t;

// Create the color window
colorwnd_t *colorwnd_init(rtk_app_t *app, mezz_mmap_t *mmap);

// Information about all the definitions
typedef struct
{
  mezz_mmap_t *mmap; // Pointer to vision calibration mmap.
  rtk_table_t *table;   // The RTK table widget
} tablewnd_t;

// Create the table window
tablewnd_t *tablewnd_init(rtk_app_t *app, mezz_mmap_t *mmap);


/***************************************************************************
 * Interfaces
 ***************************************************************************/

// Initialise the image interface
int image_init(imagewnd_t *canvas, mezz_mmap_t *mmap);

// Update the image interface
void image_update();

// Initialise the mask interface
int mask_init(imagewnd_t *imagewnd, mezz_mmap_t *mmap);

// Update the mask interface
void mask_update();

// Initialise the sample interface
int sample_init(imagewnd_t *imagewnd, colorwnd_t *colorwnd, mezz_mmap_t *mmap);

// Update the sample interface
void sample_update();

// Initialise the classify interface
int classify_init(colorwnd_t *colorwnd, mezz_mmap_t *mmap);

// Update the classify interface
void classify_update();

// Initialise the blobfind interface
int blobfind_init(imagewnd_t *imagewnd, tablewnd_t *tablewnd, mezz_mmap_t *mmap);

// Update the blobfind interface
void blobfind_update();

// Initialise the dewarping interface
int dewarp_init(imagewnd_t *imagewnd, tablewnd_t *tablewnd, mezz_mmap_t *mmap);

// Update the dewarp interface
void dewarp_update();

// Convert point from world to image coords
void dewarp_world2image(double x, double y, double *i, double *j);

// Initialise the ident interface
int ident_init(imagewnd_t *imagewnd, mezz_mmap_t *mmap);

// Update the ident interface
void ident_update();


/***************************************************************************
 * Misc useful stuff
 ***************************************************************************/

// Determine number of elements in a static array
#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))


#endif

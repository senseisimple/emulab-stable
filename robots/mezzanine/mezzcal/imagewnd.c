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
 * Desc: Canvas containing the raw image, classification, etc.
 * Author: Andrew Howard
 * Date: 11 Apr 2002
 * CVS: $Id: imagewnd.c,v 1.1 2004-12-12 23:36:34 johnsond Exp $
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "mezzcal.h"


// Create the image window
imagewnd_t *imagewnd_init(rtk_app_t *app, mezz_mmap_t *mmap)
{
  imagewnd_t *wnd;

  wnd = malloc(sizeof(imagewnd_t));
  wnd->mmap = mmap;
  wnd->canvas = rtk_canvas_create(app);

  // Set up the canvas
  rtk_canvas_movemask(wnd->canvas, 0);
  rtk_canvas_size(wnd->canvas, mmap->width, mmap->height);
  rtk_canvas_scale(wnd->canvas, 1, -1);
  rtk_canvas_origin(wnd->canvas, +mmap->width/2, +mmap->height/2);

  // Create file menu
  wnd->filemenu = rtk_menu_create(wnd->canvas, "File");
  wnd->saveitem = rtk_menuitem_create(wnd->filemenu, "Save", 0);
  
  // Create view menu
  wnd->viewmenu = rtk_menu_create(wnd->canvas, "View");
  wnd->maskitem = rtk_menuitem_create(wnd->viewmenu, "Show mask", 1);
  wnd->classitem = rtk_menuitem_create(wnd->viewmenu, "Show classes", 1);
  wnd->blobitem = rtk_menuitem_create(wnd->viewmenu, "Show blobs", 1);
  wnd->dewarpitem = rtk_menuitem_create(wnd->viewmenu, "Show dewarp", 1);

  // Show classes and blobs by default
  rtk_menuitem_check(wnd->maskitem, 1);
  rtk_menuitem_check(wnd->classitem, 1);
  rtk_menuitem_check(wnd->blobitem, 1);
  rtk_menuitem_check(wnd->classitem, 1);
  rtk_menuitem_check(wnd->dewarpitem, 1);

  // Create an image to put the image
  // This fig is drawn as a background layer.
  wnd->imagefig = rtk_fig_create(wnd->canvas, NULL, -1);
  rtk_fig_origin(wnd->imagefig, 0, 0, 0);
  rtk_fig_color(wnd->imagefig, 0, 0, 0);

  return wnd;
}


// Update the window
void imagewnd_update(imagewnd_t *wnd)
{
  if (rtk_menuitem_isactivated(wnd->saveitem))
    wnd->mmap->save = 1;
}

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
 * Desc: Canvas containing the YUV color space.
 * Author: Andrew Howard
 * Date: 11 Apr 2002
 * CVS: $Id: colorwnd.c,v 1.1 2004-12-12 23:36:34 johnsond Exp $
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "color.h"
#include "mezzcal.h"


// Create the color-space window
colorwnd_t *colorwnd_init(rtk_app_t *app, mezz_mmap_t *mmap)
{
  colorwnd_t *wnd;

  wnd = malloc(sizeof(colorwnd_t));
  wnd->canvas = rtk_canvas_create(app);

  // Set up the canvas
  rtk_canvas_movemask(wnd->canvas, 0);
  rtk_canvas_size(wnd->canvas, 512, 512);
  rtk_canvas_scale(wnd->canvas, 1, 1);
  rtk_canvas_origin(wnd->canvas, 0, 0);
  
  wnd->vufig = rtk_fig_create(wnd->canvas, NULL, 0);
  rtk_fig_origin(wnd->vufig, -256, -256, 0);
  rtk_fig_color(wnd->vufig, 0, 0, 0);
  rtk_fig_rectangle(wnd->vufig, 128, 128, 0, 256, 256, 1);

  wnd->yufig = rtk_fig_create(wnd->canvas, NULL, 0);
  rtk_fig_origin(wnd->yufig, 0, -256, 0);
  rtk_fig_color(wnd->yufig, 0, 0, 0);
  rtk_fig_rectangle(wnd->yufig, 128, 128, 0, 256, 256, 1);
  
  wnd->vyfig = rtk_fig_create(wnd->canvas, NULL, 0);
  rtk_fig_origin(wnd->vyfig, -256, 0, 0);
  rtk_fig_color(wnd->vyfig, 0, 0, 0);
  rtk_fig_rectangle(wnd->vyfig, 128, 128, 0, 256, 256, 1);

  return wnd;
}


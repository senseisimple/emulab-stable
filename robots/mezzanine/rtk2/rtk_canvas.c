/*
 *  RTK2 : A GUI toolkit for robotics
 *  Copyright (C) 2001  Andrew Howard  ahoward@usc.edu
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * Desc: Rtk canvas functions
 * Author: Andrew Howard
 * CVS: $Id: rtk_canvas.c,v 1.1 2004-12-12 23:36:34 johnsond Exp $
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <gdk/gdkkeysyms.h>

#define DEBUG
#include "rtk.h"
#include "rtkprivate.h"

// Declare some local functions
static gboolean rtk_on_destroy(GtkWidget *widget, rtk_canvas_t *canvas);
static void rtk_on_configure(GtkWidget *widget, GdkEventConfigure *event, rtk_canvas_t *canvas);
static void rtk_on_expose(GtkWidget *widget, GdkEventExpose *event, rtk_canvas_t *canvas);
static void rtk_on_press(GtkWidget *widget, GdkEventButton *event, rtk_canvas_t *canvas);
static void rtk_on_motion(GtkWidget *widget, GdkEventMotion *event, rtk_canvas_t *canvas);
static void rtk_on_release(GtkWidget *widget, GdkEventButton *event, rtk_canvas_t *canvas);
static void rtk_on_key_press(GtkWidget *widget, GdkEventKey *event, rtk_canvas_t *canvas);
static void rtk_canvas_export_image_private(rtk_canvas_t *canvas);
static void rtk_canvas_mouse(rtk_canvas_t *canvas, int event, int button, int x, int y);

// Mouse modes 
enum {MOUSE_NONE, MOUSE_PAN, MOUSE_ZOOM, MOUSE_TRANS, MOUSE_ROT, MOUSE_SCALE};

// Mouse events
enum {EVENT_PRESS, EVENT_MOTION, EVENT_RELEASE};

// Convert from device to logical coords
#define LX(x) (canvas->ox + (+(x) - canvas->sizex / 2) * canvas->sx)
#define LY(y) (canvas->oy + (-(y) + canvas->sizey / 2) * canvas->sy)

// Convert from logical to device coords
#define DX(x) (canvas->sizex / 2 + ((x) - canvas->ox) / canvas->sx)
#define DY(y) (canvas->sizey / 2 - ((y) - canvas->oy) / canvas->sy)


// Create a canvas
rtk_canvas_t *rtk_canvas_create(rtk_app_t *app)
{
  rtk_canvas_t *canvas;

  // Create canvas
  canvas = malloc(sizeof(rtk_canvas_t));

  // Append canvas to linked list
  RTK_LIST_APPEND(app->canvas, canvas);
    
  canvas->app = app;
  canvas->sizex = 0;
  canvas->sizey = 0;
  canvas->ox = 0.0;
  canvas->oy = 0.0;
  canvas->sx = 0.01;
  canvas->sy = 0.01;
  canvas->destroyed = FALSE;
  canvas->bg_dirty = TRUE;
  canvas->fg_dirty = TRUE;
  canvas->bg_suppress = FALSE;
  canvas->movemask = RTK_MOVE_PAN | RTK_MOVE_ZOOM;
  canvas->export_flag = 0;
  canvas->export_filename = NULL;
  sem_init(&canvas->export_sem, 0, 0);
  canvas->fig = NULL;
  
  // Create a mutex and some TLS
  pthread_key_create(&canvas->key, NULL);
  pthread_mutex_init(&canvas->mutex, NULL);

  // Initialise mouse handling
  canvas->mouse_mode = MOUSE_NONE;
  canvas->mouse_over_fig = NULL;  
  canvas->mouse_selected_fig = NULL;
  canvas->mouse_indicator_fig = NULL;

  // Create a top-level window
	canvas->frame = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	canvas->layout = gtk_vbox_new(FALSE, 0);

  // Create gtk drawing area
  canvas->canvas = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(canvas->canvas), 100, 100);

  // Create menu bar
  canvas->menu_bar = gtk_menu_bar_new();
  
  // Put it all together
  gtk_container_add(GTK_CONTAINER(canvas->frame), canvas->layout);
  gtk_box_pack_start(GTK_BOX(canvas->layout), canvas->menu_bar, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(canvas->layout), canvas->canvas, TRUE, TRUE, 0);

  canvas->bg_pixmap = NULL;
  canvas->fg_pixmap = NULL;
  canvas->gc = NULL;
  canvas->colormap = NULL;
  canvas->fontname = strdup("-adobe-helvetica-medium-r-*-*-*-120-*-*-*-*-*-*");
  canvas->font = NULL;

  // Connect gtk signal handlers
	gtk_signal_connect(GTK_OBJECT(canvas->frame), "destroy",
                     GTK_SIGNAL_FUNC(rtk_on_destroy), canvas);
  gtk_signal_connect(GTK_OBJECT(canvas->canvas), "configure_event", 
                     GTK_SIGNAL_FUNC(rtk_on_configure), canvas);
  gtk_signal_connect(GTK_OBJECT(canvas->canvas), "expose_event", 
                     GTK_SIGNAL_FUNC(rtk_on_expose), canvas);
  gtk_signal_connect(GTK_OBJECT(canvas->canvas), "button_press_event", 
                     GTK_SIGNAL_FUNC(rtk_on_press), canvas);
  gtk_signal_connect(GTK_OBJECT(canvas->canvas), "motion_notify_event", 
                     GTK_SIGNAL_FUNC(rtk_on_motion), canvas);
  gtk_signal_connect(GTK_OBJECT(canvas->canvas), "button_release_event", 
                     GTK_SIGNAL_FUNC(rtk_on_release), canvas);
  gtk_signal_connect(GTK_OBJECT(canvas->frame), "key-press-event", 
                     GTK_SIGNAL_FUNC(rtk_on_key_press), canvas);

  // Set the event mask
	gtk_widget_set_events(canvas->frame,
                        GDK_KEY_PRESS_MASK);
	gtk_widget_set_events(canvas->canvas,
                        GDK_EXPOSURE_MASK |
                        GDK_BUTTON_PRESS_MASK |
                        GDK_BUTTON_RELEASE_MASK |
                        GDK_POINTER_MOTION_MASK);
  
  return canvas;
}


// Delete the canvas
void rtk_canvas_destroy(rtk_canvas_t *canvas)
{
  int count;
    
  // Get rid of any figures we still have
  count = 0;
  while (canvas->fig)
  {
    rtk_fig_destroy(canvas->fig);
    count++;
  }
  if (count > 0)
    PRINT_WARN1("garbage collected %d figures", count);

  // Remove ourself from the linked list in the app
  RTK_LIST_REMOVE(canvas->app->canvas, canvas);   
    
  // Free ourself
  free(canvas);
}


// Lock the canvas
// This implements recursive mutexes using TLD
void rtk_canvas_lock(rtk_canvas_t *canvas)
{
  int count;

  count = (int) pthread_getspecific(canvas->key);
  if (count == 0)
    pthread_mutex_lock(&canvas->mutex);
  count++;
  pthread_setspecific(canvas->key, (void*) count);
}


// Unlock the canvas
void rtk_canvas_unlock(rtk_canvas_t *canvas)
{
  int count;

  count = (int) pthread_getspecific(canvas->key);
  count--;
  pthread_setspecific(canvas->key, (void*) count);
  if (count == 0)
    pthread_mutex_unlock(&canvas->mutex);
}


// Set the size of a canvas
// (sizex, sizey) is the width and height of the canvas, in pixels.
void rtk_canvas_size(rtk_canvas_t *canvas, int sizex, int sizey)
{
  //gtk_window_set_default_size(GTK_WINDOW(app->frame), sizex, sizey);
  gtk_drawing_area_size(GTK_DRAWING_AREA(canvas->canvas), sizex, sizey);
}


// Get the canvas size
// (sizex, sizey) is the width and height of the canvas, in pixels.
void rtk_canvas_get_size(rtk_canvas_t *canvas, int *sizex, int *sizey)
{
  gdk_window_get_size(canvas->canvas->window, sizex, sizey);
}


// Set the origin of a canvas
// (ox, oy) specifies the logical point that maps to the center of the
// canvas.
void rtk_canvas_origin(rtk_canvas_t *canvas, double ox, double oy)
{
  rtk_fig_t *fig;
    
  canvas->ox = ox;
  canvas->oy = oy;

  // Re-calculate all figures
  for (fig = canvas->fig; fig != NULL; fig = fig->next)
  {
    rtk_fig_lock(fig);
    rtk_fig_calc(fig);
    rtk_fig_unlock(fig);
  }
}


// Get the origin of a canvas
// (ox, oy) specifies the logical point that maps to the center of the
// canvas.
void rtk_canvas_get_origin(rtk_canvas_t *canvas, double *ox, double *oy)
{
  *ox = canvas->ox;
  *oy = canvas->oy;
}


// Scale a canvas
// Sets the pixel width and height in logical units
void rtk_canvas_scale(rtk_canvas_t *canvas, double sx, double sy)
{
  rtk_fig_t *fig;
    
  canvas->sx = sx;
  canvas->sy = sy;

  // Re-calculate all figures
  for (fig = canvas->fig; fig != NULL; fig = fig->next)
  {
    rtk_fig_lock(fig);
    rtk_fig_calc(fig);
    rtk_fig_unlock(fig);
  }
}


// Get the scale of the canvas
// (sx, sy) are the pixel with and height in logical units
void rtk_canvas_get_scale(rtk_canvas_t *canvas, double *sx, double *sy)
{
  *sx = canvas->sx;
  *sy = canvas->sy;
}


// Set the movement mask
// Set the mask to a bitwise combination of RTK_MOVE_TRANS, RTK_MOVE_SCALE.
// to enable user manipulation of the canvas.
void rtk_canvas_movemask(rtk_canvas_t *canvas, int mask)
{
  canvas->movemask = mask;
}


// Set the default font for text strokes
void rtk_canvas_font(rtk_canvas_t *canvas, const char *fontname)
{
  rtk_canvas_lock(canvas);

  if (canvas->font)
  {
    gdk_font_unref(canvas->font);
    canvas->font = NULL;
  }

  if (canvas->fontname)
  {
    free(canvas->fontname);
    canvas->fontname = strdup(fontname);
  }

  rtk_canvas_unlock(canvas);
}


// Render the figures in the canvas
void rtk_canvas_render(rtk_canvas_t *canvas, int all, GdkRectangle *rect)
{
  int layer;
  int bg_count;
  rtk_fig_t *fig;
  GdkColor color;

  if (canvas->destroyed)
    return;

  rtk_canvas_lock(canvas);
    
  gdk_color_white(canvas->colormap, &color);
  gdk_gc_set_foreground(canvas->gc, &color);

  // Load the default font
  if (canvas->font == NULL)
    canvas->font = gdk_font_load(canvas->fontname);

  // See if there is anything in the background
  bg_count = 0;
  for (fig = canvas->fig; fig != NULL; fig = fig->next)
  {
    if (fig->layer < 0 && fig->show)
    {
      bg_count++;
      break;
    }
  }

  // Render the background
  if (canvas->bg_dirty && !canvas->bg_suppress)
  {
    // Clear background pixmap
    gdk_draw_rectangle(canvas->bg_pixmap, canvas->gc, TRUE,
                       0, 0, canvas->sizex, canvas->sizey);

    // Render all figures, in order of layer
    // This could be done *much* more efficienty
    for (layer = -RTK_CANVAS_LAYERS; layer < 0; layer++)
    {
      for (fig = canvas->fig; fig != NULL; fig = fig->next)
      {
        if (fig->layer == layer)
          rtk_fig_render(fig);
      }
    }
  }
  else if ((canvas->fg_dirty && bg_count == 0) || canvas->bg_suppress)
  {
    // Clear background pixmap
    gdk_draw_rectangle(canvas->bg_pixmap, canvas->gc, TRUE,
                       0, 0, canvas->sizex, canvas->sizey);
  }

  // Render the foreground
  if (canvas->bg_dirty || canvas->fg_dirty)
  {
    // Copy background pixmap to foreground pixmap
    gdk_draw_pixmap(canvas->fg_pixmap, canvas->gc, canvas->bg_pixmap,
                    0, 0, 0, 0, canvas->sizex, canvas->sizey);

    // Render all figures, in order of layer
    // This could be done *much* more efficienty
    for (layer = 0; layer < RTK_CANVAS_LAYERS; layer++)
    {
      for (fig = canvas->fig; fig != NULL; fig = fig->next)
      {
        if (fig->layer == layer)
          rtk_fig_render(fig);
      }
    }

    // Now copy foreground pixmap to screen
    gdk_draw_pixmap(canvas->canvas->window, canvas->gc, canvas->fg_pixmap,
                    0, 0, 0, 0, canvas->sizex, canvas->sizey);
  }
  
  canvas->bg_dirty = FALSE;
  canvas->fg_dirty = FALSE;

  // Grab image if we are exporting.
  // We have to a the semaphore to indicate that we are done.
  if (canvas->export_flag)
  {
    rtk_canvas_export_image_private(canvas);
    canvas->export_flag = 0;
    sem_post(&canvas->export_sem);
  }
  
  rtk_canvas_unlock(canvas);
}


// Export the canvas
void rtk_canvas_export(rtk_canvas_t *canvas, char *filename)
{
  int layer;
  int wx, wy;
  rtk_fig_t *fig;
    
  rtk_canvas_lock(canvas);

  canvas->file = fopen(filename, "w+");

  // Write header info
  fprintf(canvas->file, "#FIG 3.2\n");
  fprintf(canvas->file, "Portrait\nCenter\nInches\nLetter\n100.00\nSingle\n");
  fprintf(canvas->file, "-2\n 1200 2\n");

  // Create a bounding box
  // Box has line width of zero, so it hopefully wont be visible
  wx = 1200 * 6;
  wy = (int) (1200 * 6 * canvas->sizey / canvas->sizex);
  fprintf(canvas->file, "2 3 0 0 0 7 50 0 -1 0.000 0 0 -1 0 0 5\n");
  fprintf(canvas->file, "0 0 %d %d %d %d %d %d 0 0\n", wx, 0, wx, wy, 0, wy);

  // Render all figures, in order of layer
  // This could be done *much* more efficienty
  for (layer = -RTK_CANVAS_LAYERS; layer < RTK_CANVAS_LAYERS; layer++)
  {
    for (fig = canvas->fig; fig != NULL; fig = fig->next)
    {
      if (fig->layer == layer)
        rtk_fig_render_xfig(fig);
    }
  }

  // Clean up
  fclose(canvas->file);
  canvas->file = NULL;
    
  rtk_canvas_unlock(canvas);
}


// Export the canvas to a bitmap image
void rtk_canvas_export_image(rtk_canvas_t *canvas, const char *filename)
{
  // Tell the GUI thread to do the exporting.
  rtk_canvas_lock(canvas);
  canvas->export_flag = 1;
  if (canvas->export_filename)
    free(canvas->export_filename);
  canvas->export_filename = strdup(filename);
  rtk_canvas_unlock(canvas);

  // Wait for the export to be completed
  sem_wait(&canvas->export_sem);
}


// Export the canvas to a bitmap image
void rtk_canvas_export_image_private(rtk_canvas_t *canvas)
{
  GdkImage *im;
  int ix, iy;
  guint32 c;
  unsigned char r, g, b;
  FILE *file;
      
  // Get the image
  im = gdk_image_get((GdkDrawable*) canvas->fg_pixmap,
                     0, 0, canvas->sizex, canvas->sizey);
  if (!im)
    PRINT_ERR("image creation failed; export aborted");
    
  // Now save it to a file
  if (!file)
  {
    PRINT_ERR("export filename is NULL; ignoring");
    return;
  }
  
  file = fopen(canvas->export_filename, "w+");
  if (!file)
  {
    PRINT_ERR2("unable to open [%s] for export: [%s]",
               canvas->export_filename, strerror(errno));
    return;
  }

  // We only to 16 bpp
  if (im->depth != 16)
  {
    PRINT_ERR("only 16 bpp displays are currently supported; export abandoned");
    return;
  }
  
  fprintf(file, "P6\n%d %d\n255\n", canvas->sizex, canvas->sizey);
  fprintf(file, "# Generated by RTK2\n");
  
  for (iy = 0; iy < canvas->sizey; iy++)
  {
    for (ix = 0; ix < canvas->sizex; ix++)
    {
      c = gdk_image_get_pixel(im, ix, iy);

      // *** HACK -- assumes 16 bit (565) color
      // Byte order has to be correct, also
      r = (c << 3) & 0xF8;
      g = (c >> 7) & 0xF8;
      b = (c >> 3) & 0xFC;

      fwrite(&r, sizeof(char), 1, file);
      fwrite(&g, sizeof(char), 1, file);
      fwrite(&b, sizeof(char), 1, file);
    }
  }

  fclose(file);
  
  gdk_image_destroy(im);
}


// Pixel tolerances for moving stuff
#define TOL_MOVE 15

// See if there is a moveable figure close to the given device point
rtk_fig_t *rtk_canvas_pick_fig(rtk_canvas_t *canvas, int x, int y)
{
  double dx, dy, r;
  rtk_fig_t *fig;

  for (fig = canvas->fig; fig != NULL; fig = fig->next)
  {
    if (!fig->show)
      continue;
    if (fig->movemask == 0)
      continue;
    if (fig == canvas->mouse_indicator_fig)
      continue;
    dx = x - DX(fig->dox);
    dy = y - DY(fig->doy);
    r = sqrt(dx * dx + dy * dy);
    if (r < TOL_MOVE)
      return fig;
  }
  return NULL;
}



// Do mouse stuff
void rtk_canvas_mouse(rtk_canvas_t *canvas, int event, int button, int x, int y)
{
  double px, py, pa, rd, rl;
  rtk_fig_t *fig;
    
  if (event == EVENT_PRESS)
  {        
    // See of there are any moveable figures at this point
    fig = rtk_canvas_pick_fig(canvas, x, y);

    // If there are moveable figures...
    if (fig)
    {
      if (button == 1 && (fig->movemask & RTK_MOVE_TRANS))
      {
        canvas->mouse_mode = MOUSE_TRANS;
        canvas->mouse_start_x = LX(x) - fig->dox;
        canvas->mouse_start_y = LY(y) - fig->doy;
        canvas->mouse_selected_fig = fig;
      }
      else if (button == 3 && (fig->movemask & RTK_MOVE_ROT))
      {
        canvas->mouse_mode = MOUSE_ROT;
        px = LX(x) - fig->dox;
        py = LY(y) - fig->doy;
        canvas->mouse_start_a = atan2(py, px) - fig->doa;
        canvas->mouse_selected_fig = fig; 
      }
    }

    // Else translate and scale the canvas...
    else
    {
      if (button == 1 && (canvas->movemask & RTK_MOVE_PAN))
      {
        // Store the logical coordinate of the start point
        canvas->mouse_mode = MOUSE_PAN;
        canvas->mouse_start_x = LX(x);
        canvas->mouse_start_y = LY(y);
      }
      else if (button == 3 && (canvas->movemask & RTK_MOVE_ZOOM))
      {
        // Store the logical coordinate of the start point
        canvas->mouse_mode = MOUSE_ZOOM;
        canvas->mouse_start_x = LX(x);
        canvas->mouse_start_y = LY(y);
      }
    }
  }

  if (event == EVENT_MOTION)
  {            
    if (canvas->mouse_mode == MOUSE_TRANS)
    {
      // Translate the selected figure
      fig = canvas->mouse_selected_fig;
      px = LX(x) - canvas->mouse_start_x;
      py = LY(y) - canvas->mouse_start_y;
      rtk_fig_origin_global(fig, px, py, fig->doa);
    }
    else if (canvas->mouse_mode == MOUSE_ROT)
    {
      // Rotate the selected figure 
      fig = canvas->mouse_selected_fig;
      px = LX(x) - fig->dox;
      py = LY(y) - fig->doy;
      pa = atan2(py, px) - canvas->mouse_start_a;
      rtk_fig_origin_global(fig, fig->dox, fig->doy, pa);
    }
    else if (canvas->mouse_mode == MOUSE_PAN)
    {
      // Infer the translation that will map the current physical mouse
      // point to the original logical mouse point.        
      canvas->ox = canvas->mouse_start_x -
        (+x - canvas->sizex / 2) * canvas->sx;
      canvas->oy = canvas->mouse_start_y -
        (-y + canvas->sizey / 2) * canvas->sy;
    }
    else if (canvas->mouse_mode == MOUSE_ZOOM)
    {
      // Compute scale change that will map original logical point
      // onto circle intersecting current mouse point.
      px = x - canvas->sizex / 2;
      py = y - canvas->sizey / 2;
      rd = sqrt(px * px + py * py) + 1;
      px = canvas->mouse_start_x - canvas->ox;
      py = canvas->mouse_start_y - canvas->oy;
      rl = sqrt(px * px + py * py);
      canvas->sy = rl / rd * canvas->sy / canvas->sx;        
      canvas->sx = rl / rd;
    }
    else if (canvas->mouse_mode == MOUSE_NONE)
    {
      // See of there are any moveable figures at this point
      fig = rtk_canvas_pick_fig(canvas, x, y);
      if (fig)
      {
        // Create a indicator figure
        if (!canvas->mouse_indicator_fig)
        {
          canvas->mouse_indicator_fig = rtk_fig_create(canvas, fig, RTK_CANVAS_LAYERS-1);
          rtk_fig_ellipse(canvas->mouse_indicator_fig, 0, 0, 0,
                          2 * TOL_MOVE * canvas->sx, 2 * TOL_MOVE * canvas->sy, FALSE);
          rtk_fig_arrow(canvas->mouse_indicator_fig, 0, 0, 0,
                        TOL_MOVE * canvas->sx, 0.5 * TOL_MOVE * canvas->sx);
        }
        canvas->mouse_over_fig = fig;
      }
      else
      {
        // Destroy the indicator figure
        if (canvas->mouse_indicator_fig)
        {
          rtk_fig_destroy(canvas->mouse_indicator_fig);
          canvas->mouse_indicator_fig = NULL;
        }
        canvas->mouse_over_fig = NULL;
      }
    }

    if (canvas->mouse_mode == MOUSE_PAN ||
        canvas->mouse_mode == MOUSE_ZOOM)
    {
      // Re-compute figures
      // Ignore the background to save time
      canvas->bg_suppress = TRUE;
      for (fig = canvas->fig; fig != NULL; fig = fig->next)
      {
        if (fig->layer >= 0)
        {
          rtk_fig_lock(fig);
          rtk_fig_calc(fig);
          rtk_fig_unlock(fig);
        }
      }
    }
  }

  if (event == EVENT_RELEASE)
  {
    if (canvas->mouse_mode == MOUSE_PAN ||
        canvas->mouse_mode == MOUSE_ZOOM)
    {
      // Re-compute all figures
      canvas->bg_suppress = FALSE;
      for (fig = canvas->fig; fig != NULL; fig = fig->next)
      {
        rtk_fig_lock(fig);
        rtk_fig_calc(fig);
        rtk_fig_unlock(fig);
      }

      // Reset mouse mode
      canvas->mouse_mode = MOUSE_NONE;
      canvas->mouse_selected_fig = NULL;
    }
    else
    {
      // Reset mouse mode
      canvas->mouse_mode = MOUSE_NONE;
      canvas->mouse_selected_fig = NULL;
    }
  }
}


// Handle destroy events
gboolean rtk_on_destroy(GtkWidget *widget, rtk_canvas_t *canvas)
{
  canvas->destroyed = TRUE;
  return FALSE;
}


// Process configure events
void rtk_on_configure(GtkWidget *widget, GdkEventConfigure *event, rtk_canvas_t *canvas)
{
  rtk_fig_t *fig;

  canvas->sizex = event->width;
  canvas->sizey = event->height;

  if (canvas->gc == NULL)
    canvas->gc = gdk_gc_new(canvas->canvas->window);
  if (canvas->colormap == NULL)
    canvas->colormap = gdk_colormap_get_system();
  
  // Create offscreen pixmaps
  if (canvas->bg_pixmap != NULL)
    gdk_pixmap_unref(canvas->bg_pixmap);
  if (canvas->fg_pixmap != NULL)
    gdk_pixmap_unref(canvas->fg_pixmap);
  canvas->bg_pixmap = gdk_pixmap_new(canvas->canvas->window,
                                     canvas->sizex, canvas->sizey, -1);
  canvas->fg_pixmap = gdk_pixmap_new(canvas->canvas->window,
                                     canvas->sizex, canvas->sizey, -1);

  // Clear pixmaps
  gdk_draw_rectangle(canvas->bg_pixmap, canvas->gc, TRUE,
                     0, 0, canvas->sizex, canvas->sizey);
  gdk_draw_rectangle(canvas->fg_pixmap, canvas->gc, TRUE,
                     0, 0, canvas->sizex, canvas->sizey);
    
  // Re-calc all figures
  // since the coord transform has changed
  for (fig = canvas->fig; fig != NULL; fig = fig->next)
  {
    rtk_fig_lock(fig);
    rtk_fig_calc(fig);
    rtk_fig_unlock(fig);
  }
}


// Process expose events
void rtk_on_expose(GtkWidget *widget, GdkEventExpose *event, rtk_canvas_t *canvas)
{
  // Copy foreground pixmap to screen
  if (canvas->fg_pixmap)
    gdk_draw_pixmap(canvas->canvas->window, canvas->gc, canvas->fg_pixmap,
                    0, 0, 0, 0, canvas->sizex, canvas->sizey);
}


// Process keyboard events
void rtk_on_key_press(GtkWidget *widget, GdkEventKey *event, rtk_canvas_t *canvas)
{
  double scale, dx, dy;
  //PRINT_DEBUG3("key: %d %d %s", event->keyval, event->state, gdk_keyval_name(event->keyval));

  dx = canvas->sizex * canvas->sx;
  dy = canvas->sizey * canvas->sy;
  scale = 1.5;
  
  switch (event->keyval)
  {
    case GDK_Left:
      if (event->state == 0)
        rtk_canvas_origin(canvas, canvas->ox - 0.05 * dx, canvas->oy);
      else if (event->state == 4)
        rtk_canvas_origin(canvas, canvas->ox - 0.5 * dx, canvas->oy);
      break;
    case GDK_Right:
      if (event->state == 0)
        rtk_canvas_origin(canvas, canvas->ox + 0.05 * dx, canvas->oy);
      else if (event->state == 4)
        rtk_canvas_origin(canvas, canvas->ox + 0.5 * dx, canvas->oy);
      break;
    case GDK_Up:
      if (event->state == 0)
        rtk_canvas_origin(canvas, canvas->ox, canvas->oy + 0.05 * dy);
      else if (event->state == 4)
        rtk_canvas_origin(canvas, canvas->ox, canvas->oy + 0.5 * dy);
      else if (event->state == 1)
        rtk_canvas_scale(canvas, canvas->sx / scale, canvas->sy / scale);
      break;
    case GDK_Down:
      if (event->state == 0)
        rtk_canvas_origin(canvas, canvas->ox, canvas->oy - 0.05 * dy);
      else if (event->state == 4)
        rtk_canvas_origin(canvas, canvas->ox, canvas->oy - 0.5 * dy);
      else if (event->state == 1)
        rtk_canvas_scale(canvas, canvas->sx * scale, canvas->sy * scale);
      break;
  }
}


// Process mouse press events
void rtk_on_press(GtkWidget *widget, GdkEventButton *event, rtk_canvas_t *canvas)
{
  rtk_canvas_mouse(canvas, EVENT_PRESS, event->button, event->x, event->y);
}


// Process mouse motion events
void rtk_on_motion(GtkWidget *widget, GdkEventMotion *event, rtk_canvas_t *canvas)
{
  rtk_canvas_mouse(canvas, EVENT_MOTION, 0, event->x, event->y);
}


// Process mouse release events
void rtk_on_release(GtkWidget *widget, GdkEventButton *event, rtk_canvas_t *canvas)
{
  rtk_canvas_mouse(canvas, EVENT_RELEASE, event->button, event->x, event->y);
}




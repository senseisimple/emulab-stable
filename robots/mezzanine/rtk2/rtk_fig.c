/*
 *  RTK2 : A GUI toolkit for robotics
 *  Copyright (C) 2001, 2005  Andrew Howard  ahoward@usc.edu
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
 * Desc: Rtk fig functions
 * Author: Andrew Howard
 * CVS: $Id: rtk_fig.c,v 1.2 2005-07-28 20:54:21 stack Exp $
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "rtk.h"
#include "rtkprivate.h"


// Recompute physical coords for figure
void rtk_fig_calc(rtk_fig_t *fig);

// Mark figure as dirty
void rtk_fig_dirty(rtk_fig_t *fig);


// Convert from local to global coords
#define GX(x, y) (fig->dox + (x) * fig->dsx * fig->dcos - (y) * fig->dsy * fig->dsin)
#define GY(x, y) (fig->doy + (x) * fig->dsx * fig->dsin + (y) * fig->dsy * fig->dcos)
#define GA(a) (fig->doa + (a))

// Convert from global to local coords
#define LX(x, y) +(((x) - fig->dox) / fig->dsx * fig->dcos + \
                   ((y) - fig->doy) / fig->dsy * fig->dsin)
#define LY(x, y) -(((x) - fig->dox) / fig->dsx * fig->dsin + \
                   ((y) - fig->doy) / fig->dsy * fig->dcos)
#define LA(a) ((a) - fig->doa)

// Convert from global to device coords
#define DX(x) (fig->canvas->sizex / 2 + ((x) - fig->canvas->ox) / fig->canvas->sx)
#define DY(y) (fig->canvas->sizey / 2 - ((y) - fig->canvas->oy) / fig->canvas->sy)

// Convert from global to paper coords
#define PX(x) (DX(x) * 1200 * 6 / fig->canvas->sizex)
#define PY(y) (DY(y) * 1200 * 6 / fig->canvas->sizex)

// Convert from local to global coords (to point structure)
#define LTOG(p, fx, fy) {p.x = GX(fx, fy); p.y = GY(fx, fy);}

// Convert from global to device coords (point structure)
#define GTOD(p, q) {p.x = DX(q.x); p.y = DY(q.y);}

// Convert from local to device coords (to point structure)
#define LTOD(p, fx, fy) {p.x = DX(GX(fx, fy)); p.y = DY(GY(fx, fy));}

// Convert from local to paper coords
#define GTOP(p, fx, fy) {p.x = PX(GX(fx, fy)); p.y = PY(GY(fx, fy));}

// Set a point
#define SETP(p, fx, fy) {p.x = fx; p.y = fy;}

// See if a point in paper coords is outside the crop region
#define CROP(p) (p.x < 0 || p.x >= 1200 * 6 || \
                 p.y < 0 || p.y >= 1200 * 6)


// Create a figure
rtk_fig_t *rtk_fig_create(rtk_canvas_t *canvas, rtk_fig_t *parent, int layer)
{
  rtk_fig_t *fig;

  // Make sure the layer is valid
  if (layer <= -RTK_CANVAS_LAYERS || layer >= RTK_CANVAS_LAYERS)
    return NULL;

  rtk_canvas_lock(canvas);

  // Create fig
  fig = malloc(sizeof(rtk_fig_t));

  // Append fig to linked list
  RTK_LIST_APPEND(canvas->fig, fig);

  fig->canvas = canvas;
  fig->parent = parent;
  fig->layer = 0;
  fig->show = TRUE;
  fig->movemask = 0;
  fig->ox = fig->oy = fig->oa = 0.0;
  fig->cos = 1.0;
  fig->sin = 0.0;
  fig->sx = 1.0;
  fig->sy = 1.0;
  fig->stroke_count = 0;
  fig->stroke_size = 1024;
  fig->stroke = malloc(fig->stroke_size * sizeof(rtk_stroke_t));
  fig->dc_color.red = 0;
  fig->dc_color.green = 0;
  fig->dc_color.blue = 0;
  
  rtk_fig_layer(fig, layer);

  // Determine global coords for fig
  rtk_fig_calc(fig);
    
  rtk_canvas_unlock(canvas);

  return fig;
}


/* Destroy a figure
 */
void rtk_fig_destroy(rtk_fig_t *fig)
{
  rtk_fig_dirty(fig);

  rtk_canvas_lock(fig->canvas);

  RTK_LIST_REMOVE(fig->canvas->fig, fig);
  free(fig->stroke);
  free(fig);

  rtk_canvas_unlock(fig->canvas);
}


// Mark the figure as dirty
void rtk_fig_dirty(rtk_fig_t *fig)
{
  if (fig->layer < 0)
    fig->canvas->bg_dirty = TRUE;
  else
    fig->canvas->fg_dirty = TRUE;
}
 

// Clear all strokes from the figure
void rtk_fig_clear(rtk_fig_t *fig)
{
  rtk_fig_lock(fig);

  fig->stroke_count = 0;
  rtk_fig_dirty(fig);
    
  rtk_fig_unlock(fig);
}


// Lock the figure. i.e. get exclusive access.
void rtk_fig_lock(rtk_fig_t *fig)
{
  rtk_canvas_lock(fig->canvas);
}


/* Unlock the figure
 * i.e. release exclusive access.
 */
void rtk_fig_unlock(rtk_fig_t *fig)
{
  rtk_canvas_unlock(fig->canvas);
}


// Show or hide the figure
void rtk_fig_show(rtk_fig_t *fig, int show)
{
  if (show != fig->show)
  {
    fig->show = show;
    rtk_fig_dirty(fig);
  }
}


// Set the movement mask
void rtk_fig_movemask(rtk_fig_t *fig, int mask)
{
  fig->movemask = mask;
}


// See if the mouse is over this figure
int rtk_fig_mouse_over(rtk_fig_t *fig)
{
  if (fig->canvas->mouse_over_fig == fig)
    return TRUE;
  return FALSE;
}


// See if the figure has been selected
int rtk_fig_mouse_selected(rtk_fig_t *fig)
{
  if (fig->canvas->mouse_selected_fig == fig)
    return TRUE;
  return FALSE;
}


// Change the origin of a figure
// Coords are relative to parent
void rtk_fig_origin(rtk_fig_t *fig, double ox, double oy, double oa)
{    
  rtk_fig_lock(fig);

  if (fig->ox != ox || fig->oy != oy || fig->oa != oa)
  {
    // Set coords relative to parent
    fig->ox = ox;
    fig->oy = oy;
    fig->oa = oa;
    fig->cos = cos(oa);
    fig->sin = sin(oa);

    // Determine global coords for fig
    rtk_fig_calc(fig);
  }

  rtk_fig_unlock(fig);    
}


// Change the origin of a figure
// Coords are global
void rtk_fig_origin_global(rtk_fig_t *fig, double ox, double oy, double oa)
{    
  rtk_fig_lock(fig);
    
  if (fig->parent)
  {
    // Set coords relative to parent
    fig->ox = +(ox - fig->parent->dox) / fig->parent->dsx * fig->parent->dcos
      + (oy - fig->parent->doy) / fig->parent->dsy * fig->parent->dsin;
    fig->oy = -(ox - fig->parent->dox) / fig->parent->dsx * fig->parent->dsin
      + (oy - fig->parent->doy) / fig->parent->dsy * fig->parent->dcos;
    fig->oa = oa - fig->parent->doa;
    fig->cos = cos(fig->oa);
    fig->sin = sin(fig->oa);
  }
  else
  {
    // Top level figure -- use global coords directly
    fig->ox = ox;
    fig->oy = oy;
    fig->oa = oa;
    fig->cos = cos(fig->oa);
    fig->sin = sin(fig->oa);
  }

  // Determine global coords for fig
  rtk_fig_calc(fig);

  rtk_fig_unlock(fig);    
}


// Get the current figure origin
void rtk_fig_get_origin(rtk_fig_t *fig, double *ox, double *oy, double *oa)
{
  *ox = fig->ox;
  *oy = fig->oy;
  *oa = fig->oa;
}


// Change the scale of a figure
void rtk_fig_scale(rtk_fig_t *fig, double scale)
{
  rtk_fig_lock(fig);

  // Set scale
  fig->sy = scale * fig->sy / fig->sx;
  fig->sx = scale;

  // Recompute global coords
  // (for child figures in particular)
  rtk_fig_calc(fig);

  rtk_fig_unlock(fig); 
}


// Change the layer the figure is in
void rtk_fig_layer(rtk_fig_t *fig, int layer)
{
  // TODO: if the layer is changing from foreground to
  // background or vice-versa, we need to set a flag
  // to indicate that both are now dirty.
  
  // Constrain layers to fixed domain
  if (layer <= -RTK_CANVAS_LAYERS)
  {
    PRINT_WARN2("invalid layer [%d]; changed to [%d]", layer, -RTK_CANVAS_LAYERS + 1);
    layer = -RTK_CANVAS_LAYERS + 1;
  }
  if (layer >= +RTK_CANVAS_LAYERS)
  {
    PRINT_WARN2("invalid layer [%d]; changed to [%d]", layer, +RTK_CANVAS_LAYERS - 1);
    layer = +RTK_CANVAS_LAYERS - 1;
  }

  fig->layer = layer;
  rtk_fig_dirty(fig);
}


// Set the color
void rtk_fig_color(rtk_fig_t *fig, double r, double g, double b)
{ 
  fig->dc_color.red = (int) (r * 0xFFFF);
  fig->dc_color.green = (int) (g * 0xFFFF);
  fig->dc_color.blue = (int) (b * 0xFFFF);
}


// Set the color for strokes.
// Color is specified as an RGB32 value (8 bits per color).
void rtk_fig_color_rgb32(rtk_fig_t *fig, int color)
{
  fig->dc_color.red = ((color & 0xFF) << 8);
  fig->dc_color.green = (((color >> 8) & 0xFF) << 8);
  fig->dc_color.blue = (((color >> 16) & 0xFF) << 8);
}


// Create a generic stroke
rtk_stroke_t *rtk_fig_stroke(rtk_fig_t *fig)
{
  rtk_stroke_t *stroke;

  // Increase size of stroke list if necessary
  if (fig->stroke_count == fig->stroke_size)
  {
    fig->stroke_size *= 2;
    fig->stroke = realloc(fig->stroke, fig->stroke_size * sizeof(rtk_stroke_t));
  }
        
  stroke = fig->stroke + fig->stroke_count++;
  stroke->color = fig->dc_color;
  stroke->drawfn = NULL;
  stroke->xfigfn = NULL;
  stroke->calcfn = NULL;
  stroke->freefn = NULL;

  // The modified fig has not been rendered
  rtk_fig_dirty(fig);
  
  return stroke;
}


// Recalculate global coords for figure
void rtk_fig_calc(rtk_fig_t *fig)
{
  int i;
  rtk_fig_t *child;
  rtk_stroke_t *stroke;
    
  if (fig->parent)
  {        
    fig->dox = fig->parent->dox
      + fig->ox * fig->parent->dsx * fig->parent->dcos
      - fig->oy * fig->parent->dsy * fig->parent->dsin;
    fig->doy = fig->parent->doy
      + fig->ox * fig->parent->dsx * fig->parent->dsin
      + fig->oy * fig->parent->dsy * fig->parent->dcos;
    fig->doa = fig->parent->doa + fig->oa;
    fig->dcos = cos(fig->doa);
    fig->dsin = sin(fig->doa);
    fig->dsx = fig->parent->dsx * fig->sx;
    fig->dsy = fig->parent->dsy * fig->sy;
  }
  else
  {
    fig->dox = fig->ox;
    fig->doy = fig->oy;
    fig->doa = fig->oa;
    fig->dcos = fig->cos;
    fig->dsin = fig->sin;
    fig->dsx = fig->sx;
    fig->dsy = fig->sy;
  }

  // Recalculate strokes
  for (i = 0; i < fig->stroke_count; i++)
  {
    stroke = fig->stroke + i;
    (*stroke->calcfn) (fig, stroke);
  }
    
  // Update all our children
  for (child = fig->canvas->fig; child != NULL; child = child->next)
  {
    if (child->parent == fig)
      rtk_fig_calc(child);
  }

  // The modified fig has not been rendered
  rtk_fig_dirty(fig);
}


// Render the figure
void rtk_fig_render(rtk_fig_t *fig)
{
  int i;
  rtk_stroke_t *stroke;
  GdkDrawable *drawable;
  GdkGC *gc;
  GdkColormap *colormap;
  GdkColor color;

  // Dont render if we're not supposed to show it
  if (!fig->show)
    return;
  
  rtk_fig_lock(fig);
    
  drawable = (fig->layer < 0 ? fig->canvas->bg_pixmap : fig->canvas->fg_pixmap);
  gc = fig->canvas->gc;
  colormap = fig->canvas->colormap;

  color.red = 0x0;
  color.green = 0x0;
  color.blue = 0x0;
  gdk_color_alloc(colormap, &color);
  gdk_gc_set_foreground(gc, &color);
  gdk_gc_set_function(gc, GDK_COPY);
    
  for (i = 0; i < fig->stroke_count; i++)
  {
    stroke = fig->stroke + i;

    // Set the color
    if (stroke->color.red != color.red ||
        stroke->color.green != color.green ||
        stroke->color.blue != color.blue)
    {
      gdk_colormap_free_colors(colormap, &color, 1);
      color.red = stroke->color.red;
      color.green = stroke->color.green;
      color.blue = stroke->color.blue;
      gdk_color_alloc(colormap, &color);
      gdk_gc_set_foreground(gc, &color);
    }
        
    (*stroke->drawfn) (fig, stroke); 
  }

  // Free any colors we have allocated
  gdk_colormap_free_colors(colormap, &color, 1);

  rtk_fig_unlock(fig);
}


// Render the figure to xfig
void rtk_fig_render_xfig(rtk_fig_t *fig)
{
  int i;
  rtk_stroke_t *stroke;

  rtk_fig_lock(fig);

  for (i = 0; i < fig->stroke_count; i++)
  {
    stroke = fig->stroke + i;
    if (stroke->xfigfn)
      (*stroke->xfigfn)(fig, stroke);
  }

  rtk_fig_unlock(fig);
}


// Draw a grid
// Grid is centered on (ox, oy) with size (dx, dy)
// with spacing (sp)
void rtk_fig_grid(rtk_fig_t *fig, double ox, double oy,
                  double dx, double dy, double sp)
{
  int i, nx, ny;

  nx = (int) ceil(dx / sp);
  ny = (int) ceil(dy / sp);

  for (i = 0; i <= nx; i++)
    rtk_fig_line(fig, ox - dx/2 + i * sp, oy - dy/2,
                      ox - dx/2 + i * sp, oy - dy/2 + ny * sp);

  for (i = 0; i <= ny; i++)
    rtk_fig_line(fig, ox - dx/2, oy - dy/2 + i * sp,
                      ox - dx/2 + nx * sp, oy - dy/2 + i * sp);
}


// Update a line
void rtk_fig_line_calc(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
  rtk_fig_line_t *data;
    
  data = &stroke->data.line;

  LTOD(data->points[0], data->ax, data->ay);
  LTOD(data->points[1], data->bx, data->by);
}

// Render a line
void rtk_fig_line_draw(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
    GdkDrawable *drawable;
    drawable = (fig->layer < 0 ? fig->canvas->bg_pixmap : fig->canvas->fg_pixmap);
    gdk_draw_lines(drawable, fig->canvas->gc, stroke->data.line.points, 2);
}


// Render stroke to xfig
void rtk_fig_line_xfig(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
    /* TODO
    int i;
    double cosx, sinx, cosy, siny;
    rtk_point_t point[4];
    rtk_fig_line_t *data;

    data = &stroke->data.line;

    // Compute polygon in paper coords
    cosx = data->sx / 2 * cos(data->oa);
    sinx = data->sx / 2 * sin(data->oa);
    cosy = data->sy / 2 * cos(data->oa);
    siny = data->sy / 2 * sin(data->oa);
    GTOP(point[0], data->ox + cosx - siny, data->oy + sinx + cosy);
    GTOP(point[1], data->ox + cosx + siny, data->oy + sinx - cosy);
    GTOP(point[2], data->ox - cosx + siny, data->oy - sinx - cosy);
    GTOP(point[3], data->ox - cosx - siny, data->oy - sinx + cosy);

    // Crop figure
    for (i = 0; i < 4; i++)
        if (CROP(point[i]))
            return;
    
    fprintf(fig->canvas->file, "2 3 0 1 0 7 50 0 -1 0.000 0 0 -1 0 0 5\n");
    for (i = 0; i < 5; i++)
        fprintf(fig->canvas->file, "%d %d ", (int) point[i % 4].x, (int) point[i % 4].y);
    fprintf(fig->canvas->file, "\n");
    */
}


// Create a line
void rtk_fig_line(rtk_fig_t *fig, double ax, double ay, double bx, double by)
{
  rtk_stroke_t *stroke;
  rtk_fig_line_t *data;

  rtk_fig_lock(fig);

  stroke = rtk_fig_stroke(fig);
  stroke->drawfn = rtk_fig_line_draw;
  stroke->xfigfn = rtk_fig_line_xfig;
  stroke->calcfn = rtk_fig_line_calc;
  stroke->freefn = NULL;

  data = &stroke->data.line;
  data->ax = ax;
  data->ay = ay;
  data->bx = bx;
  data->by = by;

  (*stroke->calcfn) (fig, stroke);

  rtk_fig_unlock(fig);
}


// Update a rectangle
void rtk_fig_rectangle_calc(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
  rtk_fig_rectangle_t *data;
  double cosa, sina, cosx, sinx, cosy, siny;
    
  data = &stroke->data.rectangle;

  cosa = cos(data->oa);
  sina = sin(data->oa);
  cosx = data->sx / 2 * cosa;
  sinx = data->sx / 2 * sina;
  cosy = data->sy / 2 * cosa;
  siny = data->sy / 2 * sina;

  LTOD(data->points[0], data->ox + cosx - siny, data->oy + sinx + cosy);
  LTOD(data->points[1], data->ox + cosx + siny, data->oy + sinx - cosy);
  LTOD(data->points[2], data->ox - cosx + siny, data->oy - sinx - cosy);
  LTOD(data->points[3], data->ox - cosx - siny, data->oy - sinx + cosy);
}


// Render a rectangle
void rtk_fig_rectangle_draw(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
  GdkDrawable *drawable;
  drawable = (fig->layer < 0 ? fig->canvas->bg_pixmap : fig->canvas->fg_pixmap);

  // For filled rectangles, we draw both filled and unfilled
  // gdk rectangles to ensure that the outline gets drawn correctly.
  if (stroke->data.rectangle.filled)
      gdk_draw_polygon(drawable, fig->canvas->gc, TRUE,
                   stroke->data.rectangle.points, 4);
  gdk_draw_polygon(drawable, fig->canvas->gc, FALSE,
                   stroke->data.rectangle.points, 4);
}


// Render stroke to xfig
void rtk_fig_rectangle_xfig(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
  int i;
  double cosx, sinx, cosy, siny;
  rtk_point_t point[4];
  rtk_fig_rectangle_t *data;
  int area_fill;

  data = &stroke->data.rectangle;

  // Compute polygon in paper coords
  cosx = data->sx / 2 * cos(data->oa);
  sinx = data->sx / 2 * sin(data->oa);
  cosy = data->sy / 2 * cos(data->oa);
  siny = data->sy / 2 * sin(data->oa);
  GTOP(point[0], data->ox + cosx - siny, data->oy + sinx + cosy);
  GTOP(point[1], data->ox + cosx + siny, data->oy + sinx - cosy);
  GTOP(point[2], data->ox - cosx + siny, data->oy - sinx - cosy);
  GTOP(point[3], data->ox - cosx - siny, data->oy - sinx + cosy);

  // DOESNT WORK PROPERLY
  // Crop figure
  //for (i = 0; i < 4; i++)
  //  if (CROP(point[i]))
  //    return;

  // Compute area fill value
  if (data->filled)
    area_fill = ((20 * (unsigned int) stroke->color.green) / 0xFFFF);
  else
    area_fill = -1;

  fprintf(fig->canvas->file, "2 3 0 1 0 7 50 0 %d 0.000 0 0 -1 0 0 5\n",
          area_fill);
  for (i = 0; i < 5; i++)
    fprintf(fig->canvas->file, "%d %d ", (int) point[i % 4].x, (int) point[i % 4].y);
  fprintf(fig->canvas->file, "\n");
}


// Create a rectangle
void rtk_fig_rectangle(rtk_fig_t *fig, double ox, double oy, double oa,
                       double sx, double sy, int filled)
{
  rtk_stroke_t *stroke;
  rtk_fig_rectangle_t *data;

  rtk_fig_lock(fig);

  stroke = rtk_fig_stroke(fig);

  stroke->drawfn = rtk_fig_rectangle_draw;
  stroke->xfigfn = rtk_fig_rectangle_xfig;
  stroke->calcfn = rtk_fig_rectangle_calc;
  stroke->freefn = NULL;
    
  data = &stroke->data.rectangle;
  data->ox = ox;
  data->oy = oy;
  data->oa = oa;
  data->sx = sx;
  data->sy = sy;
  data->filled = filled;

  (*stroke->calcfn) (fig, stroke);
    
  rtk_fig_unlock(fig);
}


// Update an ellipse
void rtk_fig_ellipse_calc(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
    rtk_fig_ellipse_t *data;
    double ax, ay, bx, by;
    double c, s;
    int i;
    
    data = &stroke->data.ellipse;    
    c = cos(data->oa);
    s = sin(data->oa);
    for (i = 0; i < 32; i++)
    {
        ax = data->sx / 2 * cos(i * M_PI / 16);
        ay = data->sy / 2 * sin(i * M_PI / 16);
        bx = data->ox + ax * c - ay * s;
        by = data->oy + ax * s + ay * c;
        LTOD(data->points[i], bx, by);
    }
}


// Render an ellipse
void rtk_fig_ellipse_draw(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
  GdkDrawable *drawable;
  drawable = (fig->layer < 0 ? fig->canvas->bg_pixmap : fig->canvas->fg_pixmap);
  gdk_draw_polygon(drawable, fig->canvas->gc,
                   stroke->data.ellipse.filled,
                   stroke->data.ellipse.points, 32);
}


// Render stroke to xfig
void rtk_fig_ellipse_xfig(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
    int ox, oy, sx, sy;
    int ax, ay, bx, by;
    double px, py, oa;
    rtk_fig_ellipse_t *data;

    data = &stroke->data.ellipse;

    // Compute origin
    ox = PX(GX(data->ox, data->oy));
    oy = PY(GY(data->ox, data->oy));
    oa = GA(data->oa);

    // Compute x and y radii
    px = PX(GX(data->ox + data->sx / 2, data->oy));
    py = PY(GY(data->ox + data->sx / 2, data->oy));
    sx = sqrt((px - ox) * (px - ox) + (py - oy) * (py - oy));
    px = PX(GX(data->ox, data->oy + data->sy / 2));
    py = PY(GY(data->ox, data->oy + data->sy / 2));
    sy = sqrt((px - ox) * (px - ox) + (py - oy) * (py - oy));

    // Compute bounding box
    ax = PX(GX(data->ox - data->sx / 2, data->oy - data->sy / 2));
    ay = PY(GY(data->ox - data->sx / 2, data->oy - data->sy / 2));
    bx = PX(GX(data->ox + data->sx / 2, data->oy + data->sy / 2));
    by = PY(GY(data->ox + data->sx / 2, data->oy + data->sy / 2));
        
    fprintf(fig->canvas->file, "1 3 0 1 0 7 50 0 -1 0.000 1 ");
    fprintf(fig->canvas->file, "%f ", oa);
    fprintf(fig->canvas->file, "%d %d ", ox, oy);
    fprintf(fig->canvas->file, "%d %d ", sx, sy);
    fprintf(fig->canvas->file, "%d %d ", ax, ay);
    fprintf(fig->canvas->file, "%d %d\n", bx, by);
}


// Create an ellipse
void rtk_fig_ellipse(rtk_fig_t *fig, double ox, double oy, double oa,
                     double sx, double sy, int filled)
{
    rtk_stroke_t *stroke;
    rtk_fig_ellipse_t *data;

    rtk_fig_lock(fig);
    
    stroke = rtk_fig_stroke(fig);
    stroke->drawfn = rtk_fig_ellipse_draw;
    stroke->xfigfn = rtk_fig_ellipse_xfig;
    stroke->calcfn = rtk_fig_ellipse_calc;
    stroke->freefn = NULL;

    data = &stroke->data.ellipse;
    data->ox = ox;
    data->oy = oy;
    data->oa = oa;
    data->sx = sx;
    data->sy = sy;
    data->filled = filled;

    (*stroke->calcfn) (fig, stroke);
        
    rtk_fig_unlock(fig);
}


// Render an arrow
void rtk_fig_arrow_draw(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
    int i;
    GdkPoint points[5];
    rtk_fig_arrow_t *data;
    GdkDrawable *drawable;

    drawable = (fig->layer < 0 ? fig->canvas->bg_pixmap : fig->canvas->fg_pixmap);
    data = &stroke->data.arrow;
    for (i = 0; i < 5; i++)
        GTOD(points[i], data->points[i]);
    gdk_draw_lines(drawable, fig->canvas->gc, points, 5);
}


// Render stroke to xfig
void rtk_fig_arrow_xfig(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
    int i;
    rtk_fig_arrow_t *data;
    
    data = &stroke->data.arrow;

    /* HACK -- FIX COLORS PROPERLY */
    /* 10 - light blue */
    fprintf(fig->canvas->file, "2 1 0 1 11 7 50 0 -1 0.000 0 0 -1 0 0 5\n");
    for (i = 0; i < 5; i++)
    {
        fprintf(fig->canvas->file, "%d %d ",
                (int) PX(data->points[i].x), (int) PY(data->points[i].y));
    }
    fprintf(fig->canvas->file, "\n");
}


// Update an arrow
void rtk_fig_arrow_calc(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
    rtk_fig_arrow_t *data;
    double ax, ay, bx, by, cx, cy, dx, dy;
    
    data = &stroke->data.arrow;

    ax = data->ox;
    ay = data->oy;
    bx = ax + data->len * cos(data->oa);
    by = ay + data->len * sin(data->oa);
    cx = bx + data->head * cos(data->oa + 0.80 * M_PI);
    cy = by + data->head * sin(data->oa + 0.80 * M_PI);
    dx = bx + data->head * cos(data->oa - 0.80 * M_PI);
    dy = by + data->head * sin(data->oa - 0.80 * M_PI);

    LTOG(data->points[0], ax, ay);
    LTOG(data->points[1], bx, by);
    LTOG(data->points[2], cx, cy);
    LTOG(data->points[3], dx, dy);
    LTOG(data->points[4], bx, by);
}


// Create an arrow
void rtk_fig_arrow(rtk_fig_t *fig, double ox, double oy, double oa,
                   double len, double head)
{
    rtk_stroke_t *stroke;
    rtk_fig_arrow_t *data;

    rtk_fig_lock(fig);
    
    stroke = rtk_fig_stroke(fig);
    stroke->drawfn = rtk_fig_arrow_draw;
    stroke->xfigfn = rtk_fig_arrow_xfig;
    stroke->calcfn = rtk_fig_arrow_calc;

    data = &stroke->data.arrow;
    data->ox = ox;
    data->oy = oy;
    data->oa = oa;
    data->len = len;
    data->head = head;

    (*stroke->calcfn) (fig, stroke);
    
    rtk_fig_unlock(fig);
}


// Create an arrow
void rtk_fig_arrow_ex(rtk_fig_t *fig, double ax, double ay,
                      double bx, double by, double head)
{
  rtk_stroke_t *stroke;
  rtk_fig_arrow_t *data;

  rtk_fig_lock(fig);
    
  stroke = rtk_fig_stroke(fig);
  stroke->drawfn = rtk_fig_arrow_draw;
  stroke->xfigfn = rtk_fig_arrow_xfig;
  stroke->calcfn = rtk_fig_arrow_calc;

  data = &stroke->data.arrow;
  data->ox = ax;
  data->oy = ay;
  data->oa = atan2(by - ay, bx - ax);
  data->len = sqrt((bx - ax) * (bx - ax) + (by - ay) * (by - ay));
  data->head = head;

  (*stroke->calcfn) (fig, stroke);
    
  rtk_fig_unlock(fig);
}


/***************************************************************************
 * Text stroke
 ***************************************************************************/


// Render text
void rtk_fig_text_draw(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
    rtk_fig_text_t *data;
    GdkDrawable *drawable;
    int i, len;
    int dy, oy;

    drawable = (fig->layer < 0 ? fig->canvas->bg_pixmap : fig->canvas->fg_pixmap);
    data = &stroke->data.text;

    // Find the line-breaks and draw separate lines of text.
    i = 0;
    oy = data->point.y;
    while (i < strlen(data->text))
    {
      len = strcspn(data->text + i, "\n");
      gdk_draw_text(drawable, fig->canvas->font, fig->canvas->gc,
                    data->point.x, oy, data->text + i, len);

      i += len + 1;
      if (i < strlen(data->text)) {
	dy = gdk_text_height(fig->canvas->font, data->text + i, len);
	oy += 14 * dy / 10;
      }
    }
}


// Render stroke to xfig
void rtk_fig_text_xfig(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
    int ox, oy, sx, sy;
    int fontsize;
    rtk_fig_text_t *data;
    
    data = &stroke->data.text;

    // Compute origin
    ox = PX(GX(data->ox, data->oy));
    oy = PY(GY(data->ox, data->oy));

    // Compute extent
    sx = 0;
    sy = 0;

    // Compute font size
    // HACK
    fontsize = 12; // * 6 * 1200 / fig->canvas->sizex;

    fprintf(fig->canvas->file, "4 0 0 50 0 0 %d 0 4 ", fontsize);
    fprintf(fig->canvas->file, "%d %d ", sx, sy);
    fprintf(fig->canvas->file, "%d %d ", ox, oy);
    fprintf(fig->canvas->file, "%s\\001\n", data->text);
}


// Update text
void rtk_fig_text_calc(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
    rtk_fig_text_t *data;

    data = &stroke->data.text;
    LTOD(data->point, data->ox, data->oy);
}


// Cleanup text stroke
void rtk_fig_text_free(rtk_stroke_t *stroke)
{
    rtk_fig_text_t *data = &stroke->data.text;
    assert(data->text);
    free(data->text);
}


// Create a text stroke
void rtk_fig_text(rtk_fig_t *fig, double ox, double oy, double oa, const char *text)
{
    rtk_stroke_t *stroke;
    rtk_fig_text_t *data;

    rtk_fig_lock(fig);
    
    stroke = rtk_fig_stroke(fig);
    stroke->drawfn = rtk_fig_text_draw;
    stroke->xfigfn = rtk_fig_text_xfig;
    stroke->calcfn = rtk_fig_text_calc;
    stroke->freefn = rtk_fig_text_free;

    data = &stroke->data.text;    
    data->ox = ox;
    data->oy = oy;
    data->oa = oa;
    assert(text);
    data->text = strdup(text);

    (*stroke->calcfn) (fig, stroke);
    
    rtk_fig_unlock(fig);
}


/***************************************************************************
 * Image stroke
 ***************************************************************************/

void rtk_fig_image_free(rtk_stroke_t *stroke);
void rtk_fig_image_calc(rtk_fig_t *fig, rtk_stroke_t *stroke);
void rtk_fig_image_draw(rtk_fig_t *fig, rtk_stroke_t *stroke);
void rtk_fig_image_xfig(rtk_fig_t *fig, rtk_stroke_t *stroke);

void rtk_fig_image(rtk_fig_t *fig, double ox, double oy,
                   int width, int height, int bpp, void *image)
{
    rtk_stroke_t *stroke;
    rtk_fig_image_t *data;

    rtk_fig_lock(fig);
    
    stroke = rtk_fig_stroke(fig);
    stroke->drawfn = rtk_fig_image_draw;
    stroke->xfigfn = rtk_fig_image_xfig;
    stroke->calcfn = rtk_fig_image_calc;
    stroke->freefn = rtk_fig_image_free;

    data = &stroke->data.image;    
    data->ox = ox;
    data->oy = oy;
    data->width = width;
    data->height = height;
    data->bpp = bpp;
    data->image = image;

    (*stroke->calcfn) (fig, stroke);
    
    rtk_fig_unlock(fig);
}


void rtk_fig_image_free(rtk_stroke_t *stroke)
{
}


void rtk_fig_image_calc(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
    rtk_fig_image_t *data;

    data = &stroke->data.image;
    LTOD(data->point, data->ox, data->oy);
    data->point.x -= data->width/2;
    data->point.y -= data->height/2;
}


void rtk_fig_image_draw(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
  GdkDrawable *drawable;
  rtk_fig_image_t *data = &stroke->data.image;
  drawable = (fig->layer < 0 ? fig->canvas->bg_pixmap : fig->canvas->fg_pixmap);

  if (data->bpp == 16)
    gdk_draw_rgb_image(drawable, fig->canvas->gc,
                       data->point.x, data->point.y, data->width, data->height,
                       GDK_RGB_DITHER_NONE, data->image, data->width * 3);
  else if (data->bpp == 32)
    gdk_draw_rgb_32_image(drawable, fig->canvas->gc,
                          data->point.x, data->point.y, data->width, data->height,
                          GDK_RGB_DITHER_NONE, data->image, data->width * 4);
}


void rtk_fig_image_xfig(rtk_fig_t *fig, rtk_stroke_t *stroke)
{
}



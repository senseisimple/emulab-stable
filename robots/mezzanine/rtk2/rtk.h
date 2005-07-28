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
 * Desc: Combined Rtk functions
 * Author: Andrew Howard
 * CVS: $Id: rtk.h,v 1.2 2005-07-28 20:54:21 stack Exp $
 */

#ifndef RTK_H
#define RTK_H

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RTK_CANVAS_LAYERS 100

// Movement mask for canvases
#define RTK_MOVE_PAN   (1 << 0)
#define RTK_MOVE_ZOOM  (1 << 1)

// Movement masks for figures
#define RTK_MOVE_TRANS (1 << 0)
#define RTK_MOVE_ROT   (1 << 1)
#define RTK_MOVE_SCALE (1 << 2)
  
// Useful forward declarations
struct _rtk_canvas_t;
struct _rtk_table_t;
struct _rtk_fig_t;
struct _rtk_stroke_t;

/***************************************************************************
 * Library functions
 ***************************************************************************/

// Initialise the library.
// Pass in the program arguments through argc, argv
int rtk_init(int *argc, char ***argv);

  
/***************************************************************************
 * Application functions
 ***************************************************************************/

// Structure describing and application
typedef struct _rtk_app_t
{
  guint timer;
  int refresh_rate;
  pthread_t thread;
  int must_quit;
  int has_quit;
  struct _rtk_canvas_t *canvas;  // Linked list of canvases
  struct _rtk_table_t *table;   // Linked list of tables
} rtk_app_t;
  
// Create the application
rtk_app_t *rtk_app_create();

// Destroy the application
void rtk_app_destroy(rtk_app_t *app);

// Set the app refresh rate in Hz.
//   This function has no effect if called after the app as been started.
void rtk_app_refresh_rate(rtk_app_t *app, int rate);

// Start the app
// Starts a new thread and returns immediately.
void rtk_app_start(rtk_app_t *app);

// Stop the app
void rtk_app_stop(rtk_app_t *app);

// Main loop the the app
// Will return only when the gui is closed by the user.
unsigned int rtk_app_main(rtk_app_t *app);



/***************************************************************************
 * Canvas functions
 ***************************************************************************/
  
// Structure describing a canvas
typedef struct _rtk_canvas_t
{
  // Linked list of canvases
  struct _rtk_canvas_t *next, *prev;

  // Link to our parent app
  struct _rtk_app_t *app;

  // Gtk stuff
  GtkWidget *frame;
  GtkWidget *layout;
  GtkWidget *canvas;

  // GDK stuff
  GdkPixmap *bg_pixmap, *fg_pixmap;
  GdkGC *gc;
  GdkColormap *colormap;
  GdkFont *font;

  // Default font name
  char *fontname;

  // The menu bar widget
  GtkWidget *menu_bar;

  // Xfig stuff
  FILE *file;

  // Physical size of window
  int sizex, sizey;
    
  // Coordinate transform
  // Logical coords of middle of canvas
  // and logical/device scale (eg m/pixel)
  double ox, oy;
  double sx, sy;

  // Flag set if canvas has been destroyed
  int destroyed;
  
  // Flags set if background or foreground need rendering
  int bg_dirty, fg_dirty;

  // Flag set if we should supress drawing of the background
  int bg_suppress;

  // Movement mask
  int movemask;

  // Flag set if we should export the canvas
  int export_flag;
  char *export_filename;
  sem_t export_sem;

  // List of figures
  struct _rtk_fig_t *fig;

  // Mutex for syncrhonization
  pthread_key_t key;
  pthread_mutex_t mutex;

  // Mouse stuff
  int mouse_mode;
  double mouse_start_x, mouse_start_y, mouse_start_a;
  struct _rtk_fig_t *mouse_over_fig;
  struct _rtk_fig_t *mouse_selected_fig;
  struct _rtk_fig_t *mouse_indicator_fig;
} rtk_canvas_t;


// Create a canvas on which to draw stuff.
rtk_canvas_t *rtk_canvas_create(rtk_app_t *app);

// Destroy the canvas
void rtk_canvas_destroy(rtk_canvas_t *canvas);

// Lock the canvas i.e. get exclusive access.
void rtk_canvas_lock(rtk_canvas_t *canvas);

// Unlock the canvas
void rtk_canvas_unlock(rtk_canvas_t *canvas);

// Set the size of a canvas
// (sizex, sizey) is the width and height of the canvas, in pixels.
void rtk_canvas_size(rtk_canvas_t *canvas, int sizex, int sizey);

// Get the canvas size
// (sizex, sizey) is the width and height of the canvas, in pixels.
void rtk_canvas_get_size(rtk_canvas_t *canvas, int *sizex, int *sizey);

// Set the origin of a canvas
// (ox, oy) specifies the logical point that maps to the center of the
// canvas.
void rtk_canvas_origin(rtk_canvas_t *canvas, double ox, double oy);

// Get the origin of a canvas
// (ox, oy) specifies the logical point that maps to the center of the
// canvas.
void rtk_canvas_get_origin(rtk_canvas_t *canvas, double *ox, double *oy);

// Scale a canvas
// Sets the pixel width and height in logical units
void rtk_canvas_scale(rtk_canvas_t *canvas, double sx, double sy);

// Get the scale of the canvas
// (sx, sy) are the pixel with and height in logical units
void rtk_canvas_get_scale(rtk_canvas_t *canvas, double *sx, double *sy);

// Set the movement mask
// Set the mask to a bitwise combination of RTK_MOVE_TRANS, RTK_MOVE_SCALE.
// to enable user manipulation of the canvas.
void rtk_canvas_movemask(rtk_canvas_t *canvas, int mask);

// Set the default font for text strokes
void rtk_canvas_font(rtk_canvas_t *canvas, const char *fontname);

// Export canvas to xfig file
void rtk_canvas_export(rtk_canvas_t *canvas, char *filename);

// Export the canvas to an image
void rtk_canvas_export_image(rtk_canvas_t *canvas, const char *filename);


/***************************************************************************
 * Figure functions
 ***************************************************************************/
  
// Structure describing a color
typedef GdkColor rtk_color_t;


// Structure describing a figure
typedef struct _rtk_fig_t
{
  // Linked list of figures
  struct _rtk_fig_t *next, *prev;

  // Pointer to parent canvas
  struct _rtk_canvas_t *canvas;

  // Pointer to our parent figure
  struct _rtk_fig_t *parent;

  // Layer this fig belongs to
  int layer;

  // Flag set to true if figure should be displayed
  int show;

  // Movement mask (a bit vector)
  int movemask;
  
  // Origin, scale of figure
  // relative to parent.
  double ox, oy, oa;
  double cos, sin;
  double sx, sy;

  // Origin, scale of figure
  // in global cs.
  double dox, doy, doa;
  double dcos, dsin;
  double dsx, dsy;    

  // List of strokes
  int stroke_size;
  int stroke_count;
  struct _rtk_stroke_t *stroke;
    
  // Drawing context information
  // Just a list of default args for drawing primitives
  rtk_color_t dc_color;
} rtk_fig_t;


// Struct describing a point
typedef struct
{
    double x, y;
} rtk_point_t;


// Struct describing a line stroke
typedef struct
{
    double ax, ay;
    double bx, by;
    GdkPoint points[2];
} rtk_fig_line_t;


// Struct describing rectangle stroke
typedef struct 
{
    double ox, oy, oa;
    double sx, sy;
    GdkPoint points[4];
    int filled;
} rtk_fig_rectangle_t;


// Struct describing ellipse stroke
typedef struct 
{
    double ox, oy, oa;
    double sx, sy;
    GdkPoint points[32];
    int filled;
} rtk_fig_ellipse_t;


// Struct describing arrow stroke
typedef struct 
{
    double ox, oy, oa;
    double len, head;
    rtk_point_t points[5];
} rtk_fig_arrow_t;


// Struct describing text stroke
typedef struct
{
    double ox, oy, oa;
    GdkPoint point;
    char *text;
} rtk_fig_text_t;


// Structure describing an image
typedef struct
{
    double ox, oy;
    void *image;
    int width, height, bpp;
    GdkPoint point;
} rtk_fig_image_t;


// Structure describing a stroke
typedef struct _rtk_stroke_t
{
  // Color
  rtk_color_t color;

  // Stroke specific data
  union
  {
    rtk_fig_line_t line;
    rtk_fig_rectangle_t rectangle;
    rtk_fig_ellipse_t ellipse;
    rtk_fig_arrow_t arrow;
    rtk_fig_text_t text;
    rtk_fig_image_t image;
  } data;

  // Function used to render stroke
  void (*drawfn) (struct _rtk_fig_t *fig, struct _rtk_stroke_t *stroke);

  // Function used to render stroke to xfig
  void (*xfigfn) (struct _rtk_fig_t *fig, struct _rtk_stroke_t *stroke);

  // Function used to compute onscreen appearance
  void (*calcfn) (struct _rtk_fig_t *fig, struct _rtk_stroke_t *stroke);
    
  // Function used to free data associated with stroke
  void (*freefn) (struct _rtk_stroke_t *stroke);
} rtk_stroke_t;


/* Figure creation/destruction */
rtk_fig_t *rtk_fig_create(rtk_canvas_t *canvas, rtk_fig_t *parent, int layer);
void rtk_fig_destroy(rtk_fig_t *fig);
void rtk_fig_clear(rtk_fig_t *fig);

// Show or hide the figure
void rtk_fig_show(rtk_fig_t *fig, int show);

/* Figure synchronization */
void rtk_fig_lock(rtk_fig_t *fig);
void rtk_fig_unlock(rtk_fig_t *fig);

// Set the movement mask
// Set the mask to a bitwise combination of RTK_MOVE_TRANS, RTK_MOVE_ROT, etc,
// to enable user manipulation of the figure.
void rtk_fig_movemask(rtk_fig_t *fig, int mask);

// See if the mouse is over this figure
int rtk_fig_mouse_over(rtk_fig_t *fig);

// See if the figure has been selected
int rtk_fig_mouse_selected(rtk_fig_t *fig);

/* Figure translation and scaling */
void rtk_fig_origin(rtk_fig_t *fig, double ox, double oy, double oa);
void rtk_fig_origin_global(rtk_fig_t *fig, double ox, double oy, double oa);
// Get the current figure origin
void rtk_fig_get_origin(rtk_fig_t *fig, double *ox, double *oy, double *oa);
void rtk_fig_scale(rtk_fig_t *fig, double scale);
void rtk_fig_layer(rtk_fig_t *fig, int layer);

// Set the color for strokes.
// Color is specified as an (r, g, b) tuple, with values in range [0, 1].
void rtk_fig_color(rtk_fig_t *fig, double r, double g, double b);

// Set the color for strokes.
// Color is specified as an RGB32 value (8 bits per color).
void rtk_fig_color_rgb32(rtk_fig_t *fig, int color);

/* Figure drawing primitives */
void rtk_fig_line(rtk_fig_t *fig, double ax, double ay, double bx, double by);
void rtk_fig_rectangle(rtk_fig_t *fig, double ox, double oy, double oa,
                       double sx, double sy, int filled);
void rtk_fig_ellipse(rtk_fig_t *fig, double ox, double oy, double oa,
                     double sx, double sy, int filled);
void rtk_fig_arrow(rtk_fig_t *fig, double ox, double oy, double oa,
                   double len, double head);
void rtk_fig_arrow_ex(rtk_fig_t *fig, double ax, double ay,
                      double bx, double by, double head);

// Draw single or multiple lines of text
// Lines are deliminted with '\n'
void rtk_fig_text(rtk_fig_t *fig, double ox, double oy, double oa,
                  const char *text);

// Draw an image
// bpp specifies the number of bits per pixel, and must be 24 or 32.
void rtk_fig_image(rtk_fig_t *fig, double ox, double oy,
                   int width, int height, int bpp, void *image);

// Draw a grid
// Grid is centered on (ox, oy) with size (dx, dy)
// with spacing (sp)
void rtk_fig_grid(rtk_fig_t *fig, double ox, double oy,
                  double dx, double dy, double sp);


/***************************************************************************
 * Menus : this needs cleaning up
 ***************************************************************************/

// Info about a menu
typedef struct
{
  rtk_canvas_t *canvas; // Which canvas we are attached to.
  GtkWidget *item;      // GTK widget holding the menu label widget.
  GtkWidget *menu;      // GTK menu item widget.
} rtk_menu_t;

// Info about a menu item
typedef struct
{
  rtk_menu_t *menu;     // Which menu we are attached to.
  GtkWidget *item;      // GTK menu item widget.
  int activated;        // Flag set if item has been activated.
  int checkitem;        // Flat set if this is a check-menu item
  int checked;          // Flag set if item is checked.
} rtk_menuitem_t;


// Create a new menu
rtk_menu_t *rtk_menu_create(rtk_canvas_t *canvas, const char *label);

// Create a new menu item
// Set check to TRUE if you want a checkbox with the menu item
rtk_menuitem_t *rtk_menuitem_create(rtk_menu_t *menu,
                                    const char *label, int check);

// Delete a menu item
void rtk_menuitem_destroy(rtk_menuitem_t *item);

// Test to see if the menu item has been activated
// Calling this function will reset the flag
int rtk_menuitem_isactivated(rtk_menuitem_t *item);

// Set the check state of a menu item
void rtk_menuitem_check(rtk_menuitem_t *item, int check);

// Test to see if the menu item is checked
int rtk_menuitem_ischecked(rtk_menuitem_t *item);


/***************************************************************************
 * Tables : provides a list of editable var = value pairs.
 ***************************************************************************/

// Info about a single item in a table
typedef struct _rtk_tableitem_t
{
  struct _rtk_tableitem_t *next, *prev;  // Linked list of items
  struct _rtk_table_t *table;
  GtkWidget *label;  // Label widget
  GtkObject *adj;    // Object for storing spin box results.
  GtkWidget *spin;   // Spin box widget
  double value;      // Last recorded value of the item
} rtk_tableitem_t;


// Info about a table
typedef struct _rtk_table_t
{
  struct _rtk_table_t *next, *prev;  // Linked list of tables
  rtk_app_t *app;         // Link to our parent app
  GtkWidget *frame;       // A top-level window to put the table in.
  GtkWidget *table;       // The table layout
  int destroyed;          // Flag set to true if the GTK frame has been destroyed.
  int item_count;         // Number of items currently in the table
  int row_count;          // Number of rows currently in the table
  rtk_tableitem_t *item;  // Linked list of items that belong in the table
} rtk_table_t;

// Create a new table
rtk_table_t *rtk_table_create(rtk_app_t *app, int width, int height);

// Delete the table
void rtk_table_destroy(rtk_table_t *table);

// Create a new item in the table
rtk_tableitem_t *rtk_tableitem_create_int(rtk_table_t *table,
                                          const char *label, int low, int high);

// Set the value of a table item (as an integer)
void rtk_tableitem_set_int(rtk_tableitem_t *item, int value);

// Get the value of a table item (as an integer)
int rtk_tableitem_get_int(rtk_tableitem_t *item);


// Create a new item in the table
rtk_tableitem_t *rtk_tableitem_create_float(rtk_table_t *table,
                                          const char *label, float low, float high, float incr);

// Set the value of a table item (as an floateger)
void rtk_tableitem_set_float(rtk_tableitem_t *item, float value);

// Get the value of a table item (as an floateger)
float rtk_tableitem_get_float(rtk_tableitem_t *item);


#ifdef __cplusplus
}
#endif

#endif


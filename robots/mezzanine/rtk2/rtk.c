
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <gdk/gdkkeysyms.h>

//#define DEBUG
#include "rtk.h"
#include "rtkprivate.h"


// Declare some local functions
static int rtk_app_on_timer(rtk_app_t *app);


// Initialise the library.
// Pass in the program arguments through argc, argv
int rtk_init(int *argc, char ***argv)
{
  // Initialise the gtk lib
  gtk_init(argc, argv);

  // Allow rgb image handling
  gdk_rgb_init();

  // Use shared mem if we can
  // This speeds things up *a lot*
  gdk_set_use_xshm(1);

  return 0;
}


// Create an app
rtk_app_t *rtk_app_create()
{
  rtk_app_t *app;

  app = malloc(sizeof(rtk_app_t));
  app->must_quit = FALSE;
  app->has_quit = FALSE;
  app->refresh_rate = 10;
  app->canvas = NULL;
  app->table = NULL;

  return app;
}


// Destroy the app
void rtk_app_destroy(rtk_app_t *app)
{
  int count;
    
  // Get rid of any canvases we still have
  count = 0;
  while (app->canvas)
  {
    rtk_canvas_destroy(app->canvas);
    count++;
  }
  if (count > 0)
    PRINT_WARN1("garbage collected %d canvases", count);

  // Get rid of any tables  we still have
  count = 0;
  while (app->table)
  {
    rtk_table_destroy(app->table);
    count++;
  }
  if (count > 0)
    PRINT_WARN1("garbage collected %d tables", count);

  // Free ourself
  free(app);
}


// Set the app refresh rate (in Hz).
// This function has no effect if called after the app as been started.
void rtk_app_refresh_rate(rtk_app_t *app, int rate)
{
  app->refresh_rate = rate;
}


// Kick off a thread to run the gui
void rtk_app_start(rtk_app_t *app)
{
  pthread_create(&app->thread, NULL,
                 (void *(*) (void *)) rtk_app_main, app); 
}


// Terminate the gui thread
void rtk_app_stop(rtk_app_t *app)
{
  app->must_quit = TRUE;
  pthread_join(app->thread, NULL);
}


// Check to see if its time to quit
int rtk_app_quit(rtk_app_t *app)
{
    return (app->has_quit);
}


// Main loop -- run in own thread
unsigned int rtk_app_main(rtk_app_t *app)
{
  rtk_canvas_t *canvas;
  rtk_table_t *table;
  
  // Display everything
  for (canvas = app->canvas; canvas != NULL; canvas = canvas->next)
    gtk_widget_show_all(canvas->frame);
  for (table = app->table; table != NULL; table = table->next)
    gtk_widget_show_all(table->frame);

  // Add a timer function
	app->timer = gtk_timeout_add(1000/app->refresh_rate, (GtkFunction) rtk_app_on_timer, app);
    
  // Let gtk take over
	gtk_main();

  // Note that the app has quit
  app->has_quit = TRUE;

  return 0;
}


// Handle timer events
int rtk_app_on_timer(rtk_app_t *app)
{
  rtk_canvas_t *canvas;
  rtk_table_t *table;

  // Update the display
  for (canvas = app->canvas; canvas != NULL; canvas = canvas->next)
    rtk_canvas_render(canvas, FALSE, NULL);

  // Quit the app if we have been told we should
  // We first destroy in windows that are still open.
  if (app->must_quit)
  {
    for (canvas = app->canvas; canvas != NULL; canvas = canvas->next)
      if (!canvas->destroyed)
        gtk_widget_destroy(canvas->frame);
    for (table = app->table; table != NULL; table = table->next)
      if (!table->destroyed)
        gtk_widget_destroy(table->frame);
    gtk_main_quit();
  }

  return TRUE;
}



#ifdef BUILD_APP

int main(int argc, char **argv)
{
  rtk_app_t *app;
  rtk_canvas_t *canvas;
  rtk_fig_t *fig1, *fig2, *fig3;

  int i;
    
  app = rtk_app_create();
  canvas = rtk_canvas_create(app);

  rtk_app_start(app);

  fig1 = rtk_fig_create(canvas, NULL);
  rtk_fig_rectangle(fig1, 0, 0, 0, 1.0, 0.5);
  rtk_fig_ellipse(fig1, 0, 0, 0, 1.0, 0.5);
  rtk_fig_arrow(fig1, 0, 0, M_PI / 4, 1.0, 0.2);
    
  fig2 = rtk_fig_create(canvas, fig1);
  rtk_fig_origin(fig2, 1, 0, 0);
  rtk_fig_scale(fig2, 0.5);
  rtk_fig_rectangle(fig2, 0, 0, 0, 1.0, 0.5);
  rtk_fig_ellipse(fig2, 0, 0, 0, 1.0, 0.5);
  rtk_fig_arrow(fig2, 0, 0, M_PI / 4, 1.0, 0.2);
    
  fig3 = rtk_fig_create(canvas, NULL);
    
  i = 0;
  while (!rtk_app_quit(app))
  {
    //rtk_fig_origin(fig1, 0, 0, i * 0.012);

    rtk_fig_clear(fig3);
    rtk_fig_origin(fig3, sin(i * 0.07), -1.0, 0);
    rtk_fig_rectangle(fig3, 0, 0, i * 0.1, 0.5, 1.0);
    rtk_fig_arrow(fig3, 0, 0, -i * 0.1, 0.8, 0.20);
    rtk_fig_text(fig3, 0.0, 0.2, 0, "some text");
        
    i++;
    usleep(20000);
  }

  rtk_app_stop(app);

  rtk_canvas_export(canvas, "test.fig");

  //rtk_canvas_destroy(canvas);
  rtk_app_destroy(app);

  return 0;
}

#endif

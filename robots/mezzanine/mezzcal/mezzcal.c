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
 * Desc: Mezzanine calibration tool.
 * Author: Andrew Howard
 * Date: 28 Mar 2002
 * CVS: $Id: mezzcal.c,v 1.3 2005-07-28 20:54:20 stack Exp $
 ***************************************************************************/

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "opt.h"
#include "mezzcal.h"

// Set flag to 1 to force program to quit
static int quit = 0;

// Handle quit signals
void sig_quit(int signum)
{
  quit = 1;
}

// convert slashes to '_' chars
void strip_slashes(char *s) {
  char *p;
  if (s != NULL) {
	for (p = s; *p != '\0'; ++p) {
	  if (*p == '\\' || *p == '/') {
		*p = '_';
	  }
	}
  }
}

// Main
int main(int argc, char **argv)
{
  mezz_mmap_t *mmap;
  int rate;
  rtk_app_t *app;  
  imagewnd_t *imagewnd;
  colorwnd_t *colorwnd;
  tablewnd_t *tablewnd;
  char *tmpdir;
  char ipcfilename[256];

  printf("MezzCal %s\n", VERSION);

  // Initialise rtk lib
  rtk_init(&argc, &argv);
    
  // Register signal handlers
  signal(SIGINT, sig_quit);
  signal(SIGQUIT, sig_quit);
  
  // Load program options
  if (opt_init(argc, argv, NULL) != 0)
    return -1;

  // Pick out some important program options
  rate = opt_get_int("gui", "rate", 5);

  // set up ipc file.
  tmpdir = getenv("TMP");
  if (tmpdir == NULL)
    tmpdir = "/tmp";

  snprintf(ipcfilename,sizeof(ipcfilename),"%s",
		   opt_get_string("fgrab","device","unknown_device"));
  
  // Initialise the IPC
  if (mezz_init(0,ipcfilename) < 0)
    return -1;
  mmap = mezz_mmap();

  // Make sure the IPC is running before we do anything else
  if (mmap->calibrate != -1) {
      mezz_wait_event();
  
  // Enable the vision stuff in the ipc
      mmap->calibrate++;
  }

  // Create gui
  app = rtk_app_create();

  // Create a window for the raw image
  imagewnd = imagewnd_init(app, mmap);
  if (!imagewnd)
    return -1;

  // Create a windiow for the YUV color space
  colorwnd = colorwnd_init(app, mmap);
  if (!colorwnd)
    return -1;

  // Create the property table
  tablewnd = tablewnd_init(app, mmap);
  if (!tablewnd)
    return -1;

  // Initialise interfaces to different parts of the
  // vision system.  
  if (image_init(imagewnd, mmap) != 0)
    return -1;
  if (mask_init(imagewnd, mmap) != 0)
    return -1;
  if (sample_init(imagewnd, colorwnd, mmap) != 0)
    return -1;
  if (classify_init(colorwnd, mmap) != 0)
    return -1;
  if (blobfind_init(imagewnd, tablewnd, mmap) != 0)
    return -1;
  if (dewarp_init(imagewnd, tablewnd, mmap) != 0)
    return -1;
  if (ident_init(imagewnd, mmap) != 0)
    return -1;

  // Check to see if there were any unrecognized options
  opt_warn_unused();

  // Start the gui
  rtk_app_refresh_rate(app, rate);
  rtk_app_start(app);
  
  while (!quit)
  {
    if (mmap->calibrate == -1) {
      struct timespec ts = { 0, 500 * 1000 * 1000 };

      nanosleep(&ts, NULL);
    }
    else {
      mezz_wait_event();
    }
    
    imagewnd_update(imagewnd); 
    
    image_update();
    mask_update();
    sample_update();
    classify_update();
    dewarp_update();
    blobfind_update();
    ident_update();
  }
  
  // Stop the gui
  rtk_app_stop(app);
  rtk_app_destroy(app);

  // Disable the vision stuff in the ipc
  mmap->calibrate--;

  // Finalise the IPC
  mezz_term(0);
  
  return 0;
}

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
 * Desc: Mezzanine entry point
 * Author: Andrew Howard
 * Date: 28 Mar 2002
 * CVS: $Id: mezzanine.c,v 1.2 2004-12-15 05:06:17 johnsond Exp $
 ***************************************************************************/

#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include "err.h"
#include "opt.h"
#include "mezzanine.h"
#include <string.h>

// Set flag to 1 to force program to quit
static int quit = 0;

// Handle quit signals
void sig_quit(int signum)
{
  quit = 1; 
}

// Get the wall clock time in seconds
double gettime()
{
  struct timeval time;
  gettimeofday(&time, NULL);
  return (double) time.tv_sec + (double) time.tv_usec / 1e6;
}

// convert slashes to '_' chars
char *strip_slashes(char *s) {
  char *p;
  if (s != NULL) {
	for (p = s; *p != '\0'; ++p) {
	  if (*p == '\\' || *p == '/') {
		*p = '_';
	  }
	}
  }
}

// Entry point
int main(int argc, char *argv[])
{
  double time[4];
  mezz_mmap_t *mmap;
  uint32_t *image;
  uint8_t *classimage;
  mezz_bloblist_t *bloblist;
  mezz_objectlist_t *objectlist;
  char optfile[256];
  char ipcfilename[256];
  char *tmpdir;

  printf("Mezzanine %s\n", VERSION);
  
  // Register signal handlers
  signal(SIGINT, sig_quit);
  signal(SIGQUIT, sig_quit);

  // Load program options
  snprintf(optfile, sizeof(optfile), "%s/mezzanine.opt", INSTALL_ETC);
  if (opt_init(argc, argv, optfile) != 0)
    return -1;

  // set up ipc file.
  tmpdir = getenv("TMP");
  if (tmpdir == NULL)
    tmpdir = "/tmp";

  //  char *sd = strdup(opt_get_string("fgrab","device","unknown_device"));

  snprintf(ipcfilename,sizeof(ipcfilename),"%s/mezz_instance%s.ipc",
		   tmpdir,opt_get_string("fgrab","device","unknown_device"));

  // Initialise the IPC
  if (mezz_init(1,ipcfilename) != 0)
    return -1;
  mmap = mezz_mmap();

  // Initialise the image processing stages
  if (fgrab_init(mmap) != 0)
    return -1;
  if (classify_init(mmap) != 0)
    return -1;
  if (blobfind_init(mmap) != 0)
    return -1;
  if (dewarp_init(mmap) != 0)
    return -1;
  if (ident_init(mmap) != 0)
    return -1;
  //if (track_init(mmap) != 0)
  //  return -1;
    
  // Check to see if there were any unrecognized options
  opt_warn_unused();

  // Run the simulation
  while (!quit)
  {
    // Get a new frame
    time[0] = -gettime();
    image = fgrab_get();
    time[0] += gettime();
    
    // Process the frame classify colors
    time[1] = -gettime();
    classimage = classify_update(image);
    time[1] += gettime();
    
    // Process the blobs to extract object id's
    time[2] = -gettime();
    bloblist = blobfind_update(classimage);
    time[2] += gettime();

    // Compute world coords of blobs
    time[3] = -gettime();
    bloblist = dewarp_update(bloblist);

    // Assign id's to blobs
    objectlist = ident_update(bloblist);

    // Track objects
/*     track_update(objectlist); */
    time[3] += gettime();

    // Display stats
    //    printf("%3.0f %3.0f %3.0f %3.0f    \r", 1000 * time[0], 1000 * time[1],
    //           1000 * time[2], 1000 * time[3]);
    //    fflush(stdout);

    mmap->time = gettime();
    mezz_raise_event();
  }

  // Close everything
  //track_term();
  ident_term();
  dewarp_term();
  blobfind_term();
  classify_term();
  fgrab_term();
  
  // Save the program options
  if (mmap->save)
    opt_save(NULL);

  // Finalise the IPC
  mezz_term(1);

  // Finialise options
  opt_term();
  
  return 0;
}



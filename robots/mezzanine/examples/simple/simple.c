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
 * Desc: Simple object tracking example
 * Author: Andrew Howard
 * Date: 28 Mar 2002
 * CVS: $Id: simple.c,v 1.2 2004-12-15 05:04:32 johnsond Exp $
 ***************************************************************************/

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "mezz.h"

// Set flag to 1 to force program to quit
static int quit = 0;

// Handle quit signals
void sig_quit(int signum)
{
  quit = 1;
}

// Main
int main(int argc, char **argv)
{
  int i;
  mezz_mmap_t *mmap;
  mezz_object_t *object;
  char *tmpdir;
  char ipcfilename[256];
    
  // Register signal handlers
  signal(SIGINT, sig_quit);
  signal(SIGQUIT, sig_quit);

  // set up ipc file.
  tmpdir = getenv("TMP");
  if (tmpdir == NULL)
    tmpdir = "/tmp";

  snprintf(ipcfilename,sizeof(ipcfilename),"%s",argv[1]);
  
  // Initialise the IPC
  if (mezz_init(0,ipcfilename) < 0)
    return -1;
  mmap = mezz_mmap();

  while (!quit)
  {
    // Make sure the IPC is running before we do anything else
    mezz_wait_event();

    printf("time [%13.3f] frame [%d]\n", mmap->time, mmap->count);
    for (i = 0; i < mmap->objectlist.count; i++)
    {
      object = mmap->objectlist.objects + i;
      printf("object [%d] pose [%+05.2f, %+05.2f, %+03.0f]\n",
             i, object->px, object->py, object->pa * 180 / M_PI);
    }
  }
  
  // Finalise the IPC
  mezz_term(0);
  
  return 0;
}

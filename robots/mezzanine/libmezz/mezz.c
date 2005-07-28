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
 * Desc: Mezzanine IPC wrapper
 * Author: Andrew Howard
 * Date: 28 Mar 2002
 * CVS: $Id: mezz.c,v 1.5 2005-07-28 20:54:18 stack Exp $ 
 **************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "mezz.h"

// Dummy signal handler
void mezz_sigusr1(int signum);

// Error macros
#define PRINT_ERR(m)         printf("\rmezzanine error : %s %s\n  "m"\n", \
                                    __FILE__, __FUNCTION__)
#define PRINT_ERRNO(m)       printf("\rmezzanine error : %s %s\n  "m" : %s\n", \
                                   __FILE__, __FUNCTION__, strerror(errno))
#define PRINT_ERRNO1(m, a)      printf("\rmezzanine error : %s %s\n  "m" : %s\n", \
                                   __FILE__, __FUNCTION__, a, strerror(errno))
#define PRINT_ERR1(m, a)     printf("\rmezzanine error : %s %s\n  "m"\n", \
                                    __FILE__, __FUNCTION__, a)
#define PRINT_ERR2(m, a, b)  printf("\rmezzanine error : %s %s\n  "m"\n", \
                                    __FILE__, __FUNCTION__, a, b)


// Structure to hold all the IPC stuff
typedef struct
{
  char *filename;
  int file;
  int mmap_size;
  mezz_mmap_t *mmap;
} ipc_t;
 

// List of IPC interfaces (one for each team)
static ipc_t *ipc = NULL;

// Initialise the interprocess comms.
int mezz_init(int create,char *ipcfilepath)
{
  int i;
  const char *tmpdir;
  char filename[128];

  // Create the wrapper for storing IPC data
  ipc = malloc(sizeof(ipc_t));
  ipc->mmap_size = sizeof(mezz_mmap_t);

  // Figure out the file name for the mmap
  tmpdir = getenv("TMP");
  if (tmpdir == NULL)
    tmpdir = "/tmp";
  if (ipcfilepath == NULL) {
	snprintf(filename, sizeof(filename), "%s/mezz_instance_default.ipc", 
			 tmpdir);
	ipc->filename = strdup(filename);
  }
  else {
	ipc->filename = strdup(ipcfilepath);
  }

  printf("ipcfile = '%s'\n",ipc->filename);
  
  strncpy(filename,ipc->filename,128);

  // Create a file to map
  if (create)
  {
    ipc->file = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (ipc->file < 0)
    {
      PRINT_ERRNO("failed to create mmap file");
      return -1;
    }
    if (ftruncate(ipc->file, ipc->mmap_size) < 0)
    {
      PRINT_ERRNO("failed to set mmap file size");
      return -1;
    }
  }
  else
  {
    ipc->file = open(filename, O_RDWR, S_IRUSR | S_IWUSR);
    if (ipc->file < 0)
    {
      PRINT_ERRNO1("failed to open mmap file [%s]", filename);
      PRINT_ERR("Did you forget to start mezzanine?");
      return -1;
    }
  }

  // Memory map the file
  ipc->mmap = mmap(NULL, ipc->mmap_size,
                   PROT_READ | PROT_WRITE, MAP_SHARED, ipc->file, 0);
  if (ipc->mmap == MAP_FAILED)
  {
    PRINT_ERRNO("failed to map mmap file");
    return -1;
  }

  // Initialise the file
  if (create)
    memset(ipc->mmap, 0, ipc->mmap_size);

  // Register ourself to get events
  signal(SIGUSR1, mezz_sigusr1);
  for (i = 0; i < sizeof(ipc->mmap->pids) / sizeof(ipc->mmap->pids[0]); i++)
  {
      /* weed out stale pids */
      if (ipc->mmap->pids[i] != 0) {
	  if (kill(ipc->mmap->pids[i],0) == -1 && errno == ESRCH) {
	      /* stale process */
	      printf("stale %d\n",ipc->mmap->pids[i]);
	      ipc->mmap->pids[i] = 0;
	      break;
	  }
	  else {
	      /* printf("not stale: %d\n",ipc->mmap->pids[i]); */
	  }
      }
      
    if (ipc->mmap->pids[i] == 0)
      break;
  }
  if (i >= sizeof(ipc->mmap->pids) / sizeof(ipc->mmap->pids[0]))
  {
    PRINT_ERR("cannot register event; pid table is full");
    return -1;
  }
  ipc->mmap->pids[i] = getpid();
  
  return 0;
}


// Finalize the ipc
// The server sets destroy to 1 to destroy the shared resources.
void mezz_term(int destroy)
{
  int i;

  // Unregister ourself from getting events
  for (i = 0; i < sizeof(ipc->mmap->pids) / sizeof(ipc->mmap->pids[0]); i++)
  {
    if (ipc->mmap->pids[i] == getpid())
      ipc->mmap->pids[i] = 0;
  }

  // Unmap the file
  munmap(ipc->mmap, ipc->mmap_size);
  close(ipc->file);

  // Destroy the file
  if (destroy)
    unlink(ipc->filename);
  free(ipc->filename);
  free(ipc);
  ipc = NULL;
}


// Get a pointer to the memory map
mezz_mmap_t *mezz_mmap()
{
  return ipc->mmap;
}


// Dummy signal handler
void mezz_sigusr1(int signum)
{
  signal(SIGUSR1, mezz_sigusr1);
  return;
}


// Raise an event
void mezz_raise_event()
{
  int i;
  
  // Send a signal to each registered process
  // Dont send it to ourself
  for (i = 0; i < sizeof(ipc->mmap->pids) / sizeof(ipc->mmap->pids[0]); i++)
  {
    if (ipc->mmap->pids[i] == getpid())
      continue;
    if (ipc->mmap->pids[i])
      kill(ipc->mmap->pids[i], SIGUSR1);
  }
}


// Wait for new data to arrive
int mezz_wait_event()
{
  pause();
  return 0;
}









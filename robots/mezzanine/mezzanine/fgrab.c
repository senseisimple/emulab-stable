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
 * Desc: Frame grabber interface
 * Author: Andrew Howard
 * Date: 28 Mar 2002
 * CVS: $Id: fgrab.c,v 1.2 2005-07-28 20:54:19 stack Exp $
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "capture.h"  // For Gavin's libfg
#include "err.h"      // Error handling
#include "opt.h"
#include "img.h"      // For loading stored images
#include "mezzanine.h"



// Information pertaining to the frame grabber interface
typedef struct
{
  mezz_mmap_t *mmap;       // Pointer to the vision mmap area
  int offline;              // Non-zero if we are operating offline (no framegrabber)
  char *device;
  FRAMEGRABBER* fg;         // Frame grabber interface
  FRAME* frame;             // The current frame
  int image_count;          // The number of stored test images (offline mode)
  uint32_t images[2][MEZZ_MAX_AREA];  // The stored test images (offline mode)
} fgrab_t;


// The one and only instance of the frame grabber
// the use of `static' here is redundant.
static fgrab_t *fgrab;


// Update the gui
void fgrab_gui(int *frame);

// Load an image from a file.
// This is a test function.
int fgrab_loadfile(int buffer, int raw, const char *filename);


// Initialise the frame grabber
int fgrab_init(mezz_mmap_t *mmap)
{
  const char *strnorm;
  int norm = 0;

  fgrab = malloc(sizeof(fgrab_t));

  fgrab->mmap = mmap;
  fgrab->mmap->count = 0;
  fgrab->offline = opt_get_int("fgrab", "offline", 0);

  if (!fgrab->offline)
  {
    strnorm = opt_get_string("fgrab", "norm", "pal");
    if (strcasecmp(strnorm, "pal") == 0)
      norm = VIDEO_MODE_PAL;
    else if (strcasecmp(strnorm, "ntsc") == 0)
      norm = VIDEO_MODE_NTSC;
    else
    {
      PRINT_ERR1("unknown video norm: [%s]", strnorm);
      return -1;
    }
    
    if (norm == VIDEO_MODE_PAL)
    {
      fgrab->mmap->width = opt_get_int("fgrab", "width", 768);
      fgrab->mmap->height = opt_get_int("fgrab", "height", 576);
      fgrab->mmap->area = fgrab->mmap->height * fgrab->mmap->width;
    }
    else if (norm == VIDEO_MODE_NTSC)
    {
      fgrab->mmap->width = opt_get_int("fgrab", "width", 640);
      fgrab->mmap->height = opt_get_int("fgrab", "height", 480);
      fgrab->mmap->area = fgrab->mmap->height * fgrab->mmap->width;
    }
  }
  else
  {
    fgrab->mmap->width = opt_get_int("fgrab", "width", 768);
    fgrab->mmap->height = opt_get_int("fgrab", "height", 576);
    fgrab->mmap->area = fgrab->mmap->height * fgrab->mmap->width;
  }

  // grab the device for this instance of mezzanine
  fgrab->device = opt_get_string("fgrab","device",FG_DEFAULT_DEVICE);

  // Get the color depth
  fgrab->mmap->depth = opt_get_int("fgrab", "depth", 16);
  if (fgrab->mmap->depth != 16 && fgrab->mmap->depth != 32 && fgrab->mmap->depth != 24)
  {
    PRINT_ERR1("depth must be 16 or 32; [%d] is not supported", fgrab->mmap->depth);
    return -1;
  }

  // If we are online, initialise the framegrabber.  Otherwise, if we
  // are offline, preload the test images we are going to use.
  if (!fgrab->offline)
  {
    fgrab->fg = fg_open( fgrab->device );
    if ( fgrab->fg == NULL )
      return -1;
	if (fg_set_source(fgrab->fg,FG_SOURCE_SVIDEO) == -1) {
	  fg_set_source( fgrab->fg, FG_SOURCE_COMPOSITE);
	}
    fg_set_source_norm( fgrab->fg, norm);
    fg_set_capture_window( fgrab->fg, 0, 0, fgrab->mmap->width, fgrab->mmap->height );

    if (fgrab->mmap->depth == 16)
    {
      fg_set_format( fgrab->fg, VIDEO_PALETTE_RGB565 );
      fgrab->frame = frame_new(fgrab->mmap->width, fgrab->mmap->height, VIDEO_PALETTE_RGB565 );
    }
    else if (fgrab->mmap->depth == 32)
    {
      fg_set_format( fgrab->fg, VIDEO_PALETTE_RGB32 );
      fgrab->frame = frame_new(fgrab->mmap->width, fgrab->mmap->height, VIDEO_PALETTE_RGB32 );
    }
  }
  else
  {
    fgrab->image_count = 0;

    char *image_file = opt_get_string("fgrab","image","testimage.ppm");
    printf("test image filename: %s\n",image_file);

    if (fgrab->mmap->depth == 32)
    {
      if (fgrab_loadfile(fgrab->image_count++, 0, image_file) != 0)
        return -1;
      else 
	  printf("loaded ppm32 successfully\n");
    }
    else if (fgrab->mmap->depth == 16)
    {
      if (fgrab_loadfile(fgrab->image_count++, 0, image_file) != 0)
        return -1;
      else 
	  printf("loaded ppm16 successfully\n");
    }
/*     else if (fgrab->mmap->depth == 24) { */
/* 	if (fgrab_loadfile(fgrab->image_count++, 1, image_file) != 0) */
/*         return -1; */
/*       else  */
/* 	  printf("loaded ppm24 successfully\n"); */
/*     } */
    else {
	printf("oops\n");
    }
  }
  
  return 0;
}


// Close the frame grabber
void fgrab_term()
{
  if (!fgrab->offline)
  {
    frame_release(fgrab->frame);
    fg_close(fgrab->fg);
  }
  free(fgrab);
}


// Get a pointer to the next frame (blocking)
int *fgrab_get()
{
  struct timespec sleep;

  if (!fgrab->offline)
  {
    // Grab the next frame (blocking)
    fg_grab_frame(fgrab->fg, fgrab->frame );

    // Update the gui
    if (fgrab->mmap->calibrate)
      fgrab_gui(fgrab->frame->data);

    fgrab->mmap->count++;
    return fgrab->frame->data;
  }
  else
  {
    // Go to sleep for a while, then "grab" the frame.
    sleep.tv_sec = 0;
    sleep.tv_nsec = (long) (0.040 * 1000000000L);
    nanosleep(&sleep, NULL);

    // Update the gui
    if (fgrab->mmap->calibrate)
      fgrab_gui(fgrab->images[fgrab->mmap->count % fgrab->image_count]);

    //printf("c: %d\n",fgrab->mmap->count % fgrab->image_count);

    fgrab->mmap->count++;
    return fgrab->images[fgrab->mmap->count % fgrab->image_count];
  }
}


// Load an image from a file.
// This is a test function.
int fgrab_loadfile(int buffer, int raw, const char *filename)
{
  img_t *img;

  if (!raw)
  {
    img = img_init(0, 0, 32);
    if (img_load_ppm(img, filename) != 0)
      return -1;
  }
  else
  {
    img = img_init(fgrab->mmap->width, fgrab->mmap->height, fgrab->mmap->depth);
    if (img_load_raw(img, filename) != 0)
      return -1;
  }
    
  assert(img->width == fgrab->mmap->width);
  assert(img->height == fgrab->mmap->height);
  memcpy(fgrab->images + buffer, img->pixels, img->size);

  img_term(img);
  
  return 0;
}


// Update the gui
void fgrab_gui(int *frame)
{
  // Just make a straight copy of the image to the mmap  
  memcpy(fgrab->mmap->image, frame,
         fgrab->mmap->width * fgrab->mmap->height * fgrab->mmap->depth / 8);
}

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
 * Desc: Display the both raw and processed frame data
 * Author: Andrew Howard
 * Date: 11 Apr 2002
 * CVS: $Id: image.c,v 1.2 2005-07-28 20:54:20 stack Exp $
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "color.h"
#include "mezzcal.h"

// Generate the image we are going to display
void image_make_image();


// Information about current images
typedef struct
{
  imagewnd_t *imagewnd; // Window containing raw image data.
  mezz_mmap_t *mmap;   // Pointer to vision calibration mmap
  rtk_fig_t *imagefig;  // Figure with raw image
  rtk_fig_t *blobfig;   // Figure for drawing the extracted blobs
  uint32_t image[MEZZ_MAX_AREA];  // The image we are going to display
} image_t;


// The one and only sample set
static image_t *image;


// Initialise the image interface
int image_init(imagewnd_t *imagewnd, mezz_mmap_t *mmap)
{
  image = malloc(sizeof(image_t));

  image->imagewnd = imagewnd;
  image->mmap = mmap;
  image->imagefig = imagewnd->imagefig;
  image->blobfig = rtk_fig_create(imagewnd->canvas, imagewnd->imagefig, 1);
  
  return 0;
}


// Update the image interface
void image_update()
{
  static int did_static_update = 0;
  
  // Draw in the color-classified image,
  // But dont do it for every image (hammers the machine).
  if (((image->mmap->calibrate != -1) && (image->mmap->count % 5 == 0)) ||
      ((image->mmap->calibrate == -1) &&
       (did_static_update != image->mmap->count)))
  {
    did_static_update = image->mmap->count;
    // Create the image to display
    image_make_image();
    
    // Show the image we just created
    rtk_fig_clear(image->imagefig);
    rtk_fig_image(image->imagefig, image->mmap->width/2, image->mmap->height/2,
                  image->mmap->width, image->mmap->height, 32, image->image);
  }
}


// Generate the image we are going to display
void image_make_image()
{
  int i;
  int pixel, r, g, b;
  int class;
  int showclass;

  showclass = rtk_menuitem_ischecked(image->imagewnd->classitem);
  
  // Classified pixels get their symbolic color (blue, yellow, etc).
  // Unclassified pixels get a dim version of the raw color.
  // Pixels outside the mask get a really dim version of the raw color.
  for (i = 0; i < image->mmap->area; i++)
  {
    if (image->mmap->depth == 16)
    {
      pixel = ((uint16_t*) image->mmap->image)[i];
      r = R_RGB16(pixel);
      g = G_RGB16(pixel);
      b = B_RGB16(pixel);
    }
    else if (image->mmap->depth == 32)
    {
      pixel = ((uint32_t*) image->mmap->image)[i];
      r = R_RGB32(pixel);
      g = G_RGB32(pixel);
      b = B_RGB32(pixel);
    }
    
    if (image->mmap->mask[i] == 0)
      image->image[i] = RGB32(r/4, g/4, b/4);
    else if (showclass)
    {
      class = image->mmap->class[i];
      if (class == 0xFF)
        image->image[i] = RGB32(r/2, g/2, b/2);
      else
        image->image[i] = image->mmap->classdeflist.classdefs[class].color;
    }
    else
      image->image[i] = RGB32(r, g, b);
  }
}


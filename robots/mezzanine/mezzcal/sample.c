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
 * Desc: Maintain samples
 * Author: Andrew Howard
 * Date: 11 Apr 2002
 * CVS: $Id: sample.c,v 1.1 2004-12-12 23:36:34 johnsond Exp $
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "color.h"
#include "mezzcal.h"


// Collect samples
void sample_collect();

// Update the YUV plots
void sample_update_yuv();


// Information about current samples
typedef struct
{
  mezz_mmap_t *mmap; // Pointer to vision calibration mmap
  double radius;        // Radius of the sample area (in pixels)
  rtk_fig_t *vufig;     // VU color space figure
  rtk_fig_t *yufig;     // YU color space figure
  rtk_fig_t *vyfig;     // YV color space figure
  int fig_count;        // Figures for specifying samples
  rtk_fig_t *figs[4];   //
  int pixel_count;      // A list of sampled pixels
  int pixels[0x10000];
} sample_t;


// The one and only sample set
static sample_t *sample;


// Initialise the sample interface
int sample_init(imagewnd_t *imagewnd, colorwnd_t *colorwnd, mezz_mmap_t *mmap)
{
  int i;
  double r;

  sample = malloc(sizeof(sample_t));

  sample->mmap = mmap;
  sample->radius = 5;

  // Create sample point figures on the image
  for (i = 0; i < ARRAYSIZE(sample->figs); i++)
  {
    sample->figs[i] = rtk_fig_create(imagewnd->canvas, imagewnd->imagefig, 1);
    rtk_fig_movemask(sample->figs[i], RTK_MOVE_TRANS);
    rtk_fig_origin(sample->figs[i], sample->mmap->width/2, sample->mmap->height/2, 0);
    rtk_fig_color(sample->figs[i], COLOR_SAMPLE);
    r = sample->radius + 2;
    rtk_fig_rectangle(sample->figs[i], 0, 0, 0, r, r, 0);
    rtk_fig_line(sample->figs[i], -2 * r, 0, -r, 0);
    rtk_fig_line(sample->figs[i], +2 * r, 0, +r, 0);
    rtk_fig_line(sample->figs[i], 0, -2 * r, 0, -r);
    rtk_fig_line(sample->figs[i], 0, +2 * r, 0, +r);
  }

  // Create figures to put on the color space
  sample->vufig = rtk_fig_create(colorwnd->canvas, colorwnd->vufig, 2);
  sample->yufig = rtk_fig_create(colorwnd->canvas, colorwnd->yufig, 2);
  sample->vyfig = rtk_fig_create(colorwnd->canvas, colorwnd->vyfig, 2);
  
  return 0;
}


// Update the sample interface
void sample_update()
{
  sample_collect();
  sample_update_yuv();
}


// Collect samples
void sample_collect()
{
  int i;
  int index, pixel;
  int x, y, dx, dy;
  double ox, oy, oa;
  
  sample->pixel_count = 0;
  
  for (i = 0; i < ARRAYSIZE(sample->figs); i++)
  {
    // Get the current position of the sample
    rtk_fig_get_origin(sample->figs[i], &ox, &oy, &oa);
    x = (int) ox; y = (int) oy;

    // Extract the pixels around the sample
    for (dy = -sample->radius; dy <= sample->radius; dy++)
    {
      for (dx = -sample->radius; dx <= sample->radius; dx++)
      {
        if (x + dx < 0 || x + dx >= sample->mmap->width)
          continue;
        if (y + dy < 0 || y + dy >= sample->mmap->height)
          continue;

        index = (x + dx) + (y + dy) * sample->mmap->width;

        if (sample->mmap->depth == 16)
          pixel = ((uint16_t*) sample->mmap->image)[index];
        else if (sample->mmap->depth == 32)
          pixel = ((uint32_t*) sample->mmap->image)[index];

        assert(sample->pixel_count < ARRAYSIZE(sample->pixels));
        sample->pixels[sample->pixel_count] = pixel;
        sample->pixel_count++;
      }
    }
  }
}


// Update the YUV plots
void sample_update_yuv()
{
  int i;
  int pixel, r, g, b;
  
  rtk_fig_clear(sample->vufig);
  rtk_fig_clear(sample->yufig);
  rtk_fig_clear(sample->vyfig);
  
  for (i = 0; i < sample->pixel_count; i++)
  {
    pixel = sample->pixels[i];

    if (sample->mmap->depth == 16)
    {
      r = R_RGB16(pixel);
      g = G_RGB16(pixel); 
      b = B_RGB16(pixel);
    }
    else if (sample->mmap->depth == 32)
    {
      r = R_RGB32(pixel);
      g = G_RGB32(pixel); 
      b = B_RGB32(pixel);
    }

    rtk_fig_color(sample->vufig, r / 256.0, g / 256.0, b / 256.0);
    rtk_fig_rectangle(sample->vufig, V_RGB(r, g, b), U_RGB(r, g, b), 0, 2, 2, 1);

    rtk_fig_color(sample->yufig, r / 256.0, g / 256.0, b / 256.0);
    rtk_fig_rectangle(sample->yufig, Y_RGB(r, g, b), U_RGB(r, g, b), 0, 2, 2, 1);

    rtk_fig_color(sample->vyfig, r / 256.0, g / 256.0, b / 256.0);
    rtk_fig_rectangle(sample->vyfig, V_RGB(r, g, b), Y_RGB(r, g, b), 0, 2, 2, 1);
  }
}

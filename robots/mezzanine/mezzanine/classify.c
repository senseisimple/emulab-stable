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
 * Desc: Segment frames into color classes.
 * Author: Andrew Howard
 * Date: 28 Mar 2002
 * CVS: $Id: classify.c,v 1.1 2004-12-12 23:36:33 johnsond Exp $
 ***************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "color.h"
#include "geom.h"
#include "opt.h"
#include "mezzanine.h"

// Generate the mask
void classify_create_mask();

// Generate the look-up table
void classify_create_lut(); 

// Do RGB16 color classification
void classify_update16(int *frame);

// Do RGB32 color classification
void classify_update32(int *frame);

// Determine the class of a pixel
uint8_t classify_classify_pixel(int pixel);

// Update the gui
void classify_gui();


// Information pertaining to the blob finder
typedef struct
{
  mezz_mmap_t *mmap;           // Pointer to the vision mmap area
  mezz_maskdef_t maskdef;      // Local copy of the mask definition.
  mezz_classdeflist_t classdeflist;  // Local copy of the color definitions.
  uint8_t lut[256*256*256];       // Class definitions (an RGB lookup table).
  uint8_t mask[MEZZ_MAX_AREA];   // Mask denoting the relevant part of the frame.
  uint8_t class[MEZZ_MAX_AREA];  // The color-classified image
} classify_t;


// The one and only instance of the blob finder
static classify_t *classify;


// Initialise the classifier
int classify_init(mezz_mmap_t *mmap)
{
  int i, j;
  mezz_maskdef_t *maskdef;
  mezz_classdef_t *classdef;
  char key[256];
  const char *name; int r, g, b;

  classify = malloc(sizeof(classify_t));
  memset(classify, 0, sizeof(classify_t));

  classify->mmap = mmap;

  // Load the mask definition
  for (i = 0; i < MEZZ_MAX_MASK; i++)
  {
    maskdef = &classify->mmap->maskdef;
    snprintf(key, sizeof(key), "mask.poly[%d]", i);
    opt_get_int2("classify", key, &maskdef->poly[i][0], &maskdef->poly[i][1]);
  }

  // Generate the mask.
  classify_create_mask();
  
  // Load the color definitions
  classify->mmap->classdeflist.count = 0;
  for (i = 0; i < MEZZ_MAX_CLASSES; i++)
  {
    classdef = classify->mmap->classdeflist.classdefs + i;

    snprintf(key, sizeof(key), "class[%d].name", i);
    name = opt_get_string("classify", key, NULL);
    if (name == NULL)
      break;
    strncpy(classdef->name, name, sizeof(classdef->name));

    snprintf(key, sizeof(key), "class[%d].color", i);
    opt_get_int3("classify", key, &r, &g, &b);
    classdef->color = RGB32(r, g, b);
    
    for (j = 0; j < 4; j++)
    {
      snprintf(key, sizeof(key), "class[%d].vupoly[%d]", i, j);
      opt_get_int2("classify", key,
                     &classdef->vupoly[j][0], &classdef->vupoly[j][1]);
      snprintf(key, sizeof(key), "class[%d].yupoly[%d]", i, j);
      opt_get_int2("classify", key,
                     &classdef->yupoly[j][0], &classdef->yupoly[j][1]);
      snprintf(key, sizeof(key), "class[%d].vypoly[%d]", i, j);
      opt_get_int2("classify", key,
                     &classdef->vypoly[j][0], &classdef->vypoly[j][1]);
    }
    classify->mmap->classdeflist.count++;
  }

  // Generate a complete LUT
  classify_create_lut();
  
  return 0;
}


// Close the classifier
void classify_term()
{
  int i, j;
  char key[256];
  mezz_maskdef_t *maskdef;
  mezz_classdef_t *classdef;
  
  // Save mask settings
  for (i = 0; i < MEZZ_MAX_MASK; i++)
  {
    maskdef = &classify->maskdef;
    snprintf(key, sizeof(key), "mask.poly[%d]", i);
    opt_set_int2("classify", key,
                 maskdef->poly[i][0], maskdef->poly[i][1]);
  }
  
  // Color definitions
  for (i = 0; i < classify->mmap->classdeflist.count; i++)
  {
    classdef = classify->mmap->classdeflist.classdefs + i;
    for (j = 0; j < 4; j++)
    {
      snprintf(key, sizeof(key), "class[%d].vupoly[%d]", i, j);
      opt_set_int2("classify", key,
                   classdef->vupoly[j][0], classdef->vupoly[j][1]);
      snprintf(key, sizeof(key), "class[%d].yupoly[%d]", i, j);
      opt_set_int2("classify", key,
                   classdef->yupoly[j][0], classdef->yupoly[j][1]);
      snprintf(key, sizeof(key), "class[%d].vypoly[%d]", i, j);
      opt_set_int2("classify", key,
                   classdef->vypoly[j][0], classdef->vypoly[j][1]);
    }
  }
  
  // Clean up
  free(classify);
}


// Do the color classification.  This routine is written for speed,
// but its really IO bound rather than CPU bound.
uint8_t *classify_update(int *frame)
{
  // Select the appropriate classification routine based on the image
  // depth.
  if (classify->mmap->depth == 16)
    classify_update16(frame);
  else if (classify->mmap->depth == 32)
    classify_update32(frame);

  // Update the gui
  // This may change the mask and lut.
  if (classify->mmap->calibrate)
    classify_gui();

  return classify->class;
}


// Do RGB16 color classification
void classify_update16(int *frame)
{
  register int i;
  register uint8_t *pmask;
  register uint16_t *ppixel; 
  register uint8_t *pclass;
  
  i = classify->mmap->area;
  ppixel = (uint16_t*) frame;
  pmask = classify->mask;
  pclass = classify->class;

  while (i)
  {
    if (*pmask)
      *pclass = classify->lut[*ppixel];
    else
      *pclass = 0xFF;

    i--;
    ppixel++;
    pmask++;
    pclass++;
  }
}


// Do RGB32 color classification
void classify_update32(int *frame)
{
  int i;
  int pixel, class;
  uint32_t *ppixel; 
  uint8_t *pmask, *pclass;

  i = 0;
  ppixel = frame;
  pmask = classify->mask;
  pclass = classify->class;
  while (i < classify->mmap->area)
  {
    *pclass = 0xFF;

    // Apply mask to determine if pixel is on the field
    if (*pmask != 0)
    {
      pixel = *ppixel;
      
      // Determine the classication
      // If the LUT doesnt contain this color,
      // compute the classification and insert it into
      // the LUT.
      class = classify->lut[pixel];
      if (class == 0xFE)
      {
        class = classify_classify_pixel(pixel);
        classify->lut[pixel] = class;
      }
      *pclass = class;
    }

    i++;
    ppixel++;
    pmask++;
    pclass++;
  }
}


// Generate the field mask.
// In addition to the user-specified field mask, we also
// mask out a one pixel border around the image.  This
// prevents segfaults in the blobfinder  (the blobfinder
// doesnt do bounds-checking; it takes too long).
void classify_create_mask()
{
  int x, y, i;
  geom_point_t p;
  geom_point_t poly[MEZZ_MAX_MASK];

  // Generate a polygon we can compare with
  for (i = 0; i < MEZZ_MAX_MASK; i++)
  {
    poly[i].x = classify->mmap->maskdef.poly[i][0];
    poly[i].y = classify->mmap->maskdef.poly[i][1];
  }

  // Now classify each pixel in the image as being
  // either inside or outside the field.
  for (y = 0, i = 0; y < classify->mmap->height; y++)
  {
    for (x = 0; x < classify->mmap->width; x++, i++)
    {
      if (y == 0 || y == classify->mmap->height - 1 ||
          x == 0 || x == classify->mmap->width - 1)
      {
        classify->mask[i] = 0;
      }
      else
      {
        p.x = x; p.y = y;
        classify->mask[i] = geom_poly_inside(poly, MEZZ_MAX_MASK, &p);
      }
    }
  }
}


// Update the LUT
// The LUT is really a cache, since it takes bloody ages
// to pre-computed.  Hence we just invalidate it here.
void classify_create_lut()
{
  int i;
  
  if (classify->mmap->depth == 16)
  {
    for (i = 0; i < 0x10000; i++)
      classify->lut[i] = classify_classify_pixel(i);
  }
  else if (classify->mmap->depth == 32)
  {
    memset(classify->lut, 0xFE, sizeof(classify->lut));
  }
}


// Determine the class of a pixel
uint8_t classify_classify_pixel(int pixel)
{
  int i, j;
  int y, u, v;
  uint8_t color;
  mezz_classdef_t *classdef;
  geom_point_t p;
  geom_point_t poly[4];

  color = 0xFF;
  
  for (i = 0; i < MEZZ_MAX_CLASSES; i++)
  {
    classdef = classify->mmap->classdeflist.classdefs + i;
    
    if (classify->mmap->depth == 16)
    {
      y = Y_RGB16(pixel);
      u = U_RGB16(pixel);
      v = V_RGB16(pixel);
    }
    else // if (classify->depth == 32)
    {
      y = Y_RGB32(pixel);
      u = U_RGB32(pixel);
      v = V_RGB32(pixel);
    }

    // Check VU polygon
    p.x = v; p.y = u;
    for (j = 0; j < 4; j++)
    {
      poly[j].x = classdef->vupoly[j][0];
      poly[j].y = classdef->vupoly[j][1];
    }
    if (!geom_poly_inside(poly, 4, &p))
      continue;

    // Check YU polygon
    p.x = y; p.y = u;
    for (j = 0; j < 4; j++)
    {
      poly[j].x = classdef->yupoly[j][0];
      poly[j].y = classdef->yupoly[j][1];
    }
    if (!geom_poly_inside(poly, 4, &p))
      continue;

    // Check VY polygon
    p.x = v; p.y = y;
    for (j = 0; j < 4; j++)
    {
      poly[j].x = classdef->vypoly[j][0];
      poly[j].y = classdef->vypoly[j][1];
    }
    if (!geom_poly_inside(poly, 4, &p))
      continue;

    // Color is a bitmap
    color = i;
    break;
  }
  
  return color;
}


// Update the gui
void classify_gui()
{
  // If the mask definition has changed,
  // we need to update the LUT
  if (memcmp(&classify->maskdef, &classify->mmap->maskdef,
             sizeof(classify->maskdef)) != 0)
  {
    memcpy(&classify->maskdef, &classify->mmap->maskdef,
           sizeof(classify->maskdef));
    classify_create_mask();    
  }

  // If the color definitions have changed,
  // we need to update the LUT.
  if (memcmp(&classify->classdeflist, &classify->mmap->classdeflist,
             sizeof(classify->classdeflist)) != 0)
  {
    memcpy(&classify->classdeflist, &classify->mmap->classdeflist,
           sizeof(classify->classdeflist));
    classify_create_lut();
  }

  // Copy the mask to the mmap
  memcpy(classify->mmap->mask, classify->mask,
         sizeof(classify->mmap->mask));

  // Copy the classified image to the mmaps
  memcpy(classify->mmap->class, classify->class,
         sizeof(classify->mmap->class));
}


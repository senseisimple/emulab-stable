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
 * Desc: Load, save and manipulate images
 * Author: Andrew Howard
 * Date: 11 Apr 2002
 * CVS: $Id: img.h,v 1.1 2004-12-12 23:36:33 johnsond Exp $
 **************************************************************************/

#ifndef IMG_H
#define IMG_H


// Structure describing a simple image.
typedef struct
{
  int width, height;  // The dimensions of the image, in pixels.
  int depth;          // Image depth (16, 24 or 32)
  int size;           // The size of the pixel array, in bytes.
  void *pixels;       // The pixel array.  The format depends on the depth.
} img_t;


// Create an empty image
img_t *img_init(int width, int height, int depth);

// Destroy an image
void img_term(img_t *img);

// Load an image from a raw file.  We assume that the width, height
// and depth of the image have already been specified.
int img_load_raw(img_t *img, const char *filename);

// Load an image from a file.
// This currently only supports PPM binary (P6) images.
int img_load_ppm(img_t *img, const char *filename);

#endif

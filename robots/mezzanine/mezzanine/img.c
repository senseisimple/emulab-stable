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
 * CVS: $Id: img.c,v 1.1 2004-12-12 23:36:33 johnsond Exp $
 **************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdint.h>     // For uint8_t, uint32_t, etc
#include <stdlib.h>
#include <string.h>
#include "err.h"
#include "img.h"


// Create an empty image
img_t *img_init(int width, int height, int depth)
{
  img_t *img;

  img = malloc(sizeof(img_t));

  img->width = width;
  img->height = height;
  img->depth = depth;
  img->size = img->depth/8 * img->width * img->height;
  img->pixels = malloc(img->size);

  return img;
}


// Destroy an image
void img_term(img_t *img)
{
  if (img->pixels)
    free(img->pixels);
  img->pixels = NULL;
  img->size = 0;
  return;
}


// Load an image from a raw file.  We assume that the width, height
// and depth of the image have already been specified.
int img_load_raw(img_t *img, const char *filename)
{
  FILE *file;

  file = fopen(filename, "r");
  if (!file)
  {
    PRINT_ERRNO1("unable to open image file [%s]", filename);
    return -1;
  }

  if (fread(img->pixels, img->size, 1, file) < 0)
  {
    PRINT_ERRNO1("error reading image file [%s], filename", filename);
    return -1;
  }

  fclose(file);
  return 0;
}


// Load an image from a ppm file
int img_load_ppm(img_t *img, const char *filename)
{
  FILE *file;
	char format[3];
  int width, height;
  int x, y, i;
  int r, g, b;

  // Open file
  file = fopen(filename, "r");
  if (!file)
  {
    PRINT_ERRNO1("unable to open image file [%s]", filename);
    return -1;
  }

	// Check format info
	fscanf(file, "%2s ", format);
	if (strcmp(format, "P6") != 0)
	{
    PRINT_ERR1("invalid file format [%s]", filename);
		fclose(file);
		return -1;
	}

	// Read comment lines
  // HACK
  while (fgetc(file) != '\n');

	// Read image dimensions
	fscanf(file, " %d %d \n", &width, &height);

	// Get colour count
	fscanf(file, " %*d \n");

  // Allocated space to store the pixels
  img->width = width;
  img->height = height;
  img->depth = 32;
  img->size = sizeof(int) * img->width * img->height;
  if (img->pixels)
    free(img->pixels);
  img->pixels = malloc(img->size);

  // Read in the pixels
  i = 0;
  for (y = 0; y < height; y++)
  {
    for (x = 0; x < width; x++)
    {
      b = fgetc(file);
      g = fgetc(file);
      r = fgetc(file);
      ((uint32_t*) img->pixels)[i++] = r | (g << 8) | (b << 16);
    }
  }

  fclose(file);
  return 0;
}



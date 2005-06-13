/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
 * Desc: Dewarp the blobs (i.e. transform form image -> world cs)
 * Author: Andrew Howard
 * Date: 17 Apr 2002
 * CVS: $Id: dewarp.c,v 1.3 2005-06-13 14:32:40 stack Exp $
 ***************************************************************************/

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_multifit.h>
#include "opt.h"
#include "mezzanine.h"

// Update the coordinate transforms
void dewarp_update_trans();

// Convert point from image to world coords
void dewarp_image2world(double i, double j, double *x, double *y);

// Convert point from world to image coords
void dewarp_world2image(double x, double y, double *i, double *j);

// Dewarp information
typedef struct
{
  mezz_mmap_t *mmap;       // Pointer th the mmap
  mezz_dewarpdef_t *def;   // Pointer to the mmaped dewarp stuff
} dewarp_t;

// The one and only instance of the dewarper
static dewarp_t *dewarp;


// Initialise the dewarper
int dewarp_init(mezz_mmap_t *mmap)
{
  int i;
  char key[64];

  dewarp = malloc(sizeof(dewarp_t));
  memset(dewarp, 0, sizeof(dewarp_t));

  dewarp->mmap = mmap;
  dewarp->def = &mmap->dewarpdef;

  // Load the calibration points
  dewarp->def->points = 0;
  for (i = 0; i < MEZZ_MAX_DEWARP; i++)
  {
    snprintf(key, sizeof(key), "wpos[%d]", i);
    if (opt_get_double2("dewarp", key, &dewarp->def->wpos[i][0], &dewarp->def->wpos[i][1]) < 0)
      break;
    snprintf(key, sizeof(key), "ipos[%d]", i);
    opt_get_int2("dewarp", key, &dewarp->def->ipos[i][0], &dewarp->def->ipos[i][1]);
    dewarp->def->points++;
  }

  // Generate the transfomr values
  dewarp_update_trans();
  
  return 0;
}


// Close the dewarper
void dewarp_term()
{
  int i;
  char key[64];
  
  // Save back the image coords of the calibration points
  for (i = 0; i < dewarp->def->points; i++)
  {
    snprintf(key, sizeof(key), "wpos[%d]", i);
    opt_set_double2("dewarp", key,
                    dewarp->def->wpos[i][0], dewarp->def->wpos[i][1], 3);
    snprintf(key, sizeof(key), "ipos[%d]", i);
    opt_set_int2("dewarp", key, dewarp->def->ipos[i][0], dewarp->def->ipos[i][1]);
  }

  // Clean up
  free(dewarp);
}


// Compute the world pose of all the blobs
mezz_bloblist_t *dewarp_update(mezz_bloblist_t *bloblist)
{
  int i;
  mezz_blob_t *blob;

  // If the gui is enabled, update the transform
  if (dewarp->mmap->calibrate)
    dewarp_update_trans();

  // Compute world coords for all the blobs
  for (i = 0; i < bloblist->count; i++)
  {
    blob = bloblist->blobs + i;
    dewarp_image2world(blob->ox, blob->oy,
                       &blob->wox, &blob->woy);
  }
  return bloblist;
}


// Update the coordinate transforms
void dewarp_update_trans()
{
  int i;
  double xx, yy;
  double ii, jj;
  gsl_matrix *a[2];
  gsl_vector *x[2];
  gsl_vector *b[2];
  gsl_matrix *cov; double chi;
  gsl_multifit_linear_workspace *work;
  
  a[0] = gsl_matrix_alloc(dewarp->def->points, 10);
  x[0] = gsl_vector_alloc(10);
  b[0] = gsl_vector_alloc(dewarp->def->points);

  a[1] = gsl_matrix_alloc(dewarp->def->points, 10);
  x[1] = gsl_vector_alloc(10);
  b[1] = gsl_vector_alloc(dewarp->def->points);

  cov = gsl_matrix_alloc(10, 10);
  work = gsl_multifit_linear_alloc(dewarp->def->points, 10);
  
  // Solve for the forward transform
  // image -> world
  for (i = 0; i < dewarp->def->points; i++)
  {
    ii = dewarp->def->ipos[i][0];
    jj = dewarp->def->ipos[i][1];

    gsl_matrix_set(a[0], i, 0, 1);
    gsl_matrix_set(a[0], i, 1, ii);
    gsl_matrix_set(a[0], i, 2, jj);
    gsl_matrix_set(a[0], i, 3, ii * ii);
    gsl_matrix_set(a[0], i, 4, jj * jj);
    gsl_matrix_set(a[0], i, 5, ii * jj);
    gsl_matrix_set(a[0], i, 6, ii * ii * ii);
    gsl_matrix_set(a[0], i, 7, jj * jj * jj);
    gsl_matrix_set(a[0], i, 8, ii * ii * jj);
    gsl_matrix_set(a[0], i, 9, jj * jj * ii);
    gsl_vector_set(b[0], i, dewarp->def->wpos[i][0]);

    gsl_matrix_set(a[1], i, 0, 1);
    gsl_matrix_set(a[1], i, 1, jj);
    gsl_matrix_set(a[1], i, 2, ii);
    gsl_matrix_set(a[1], i, 3, jj * jj);
    gsl_matrix_set(a[1], i, 4, ii * ii);
    gsl_matrix_set(a[1], i, 5, jj * ii);
    gsl_matrix_set(a[1], i, 6, jj * jj * jj);
    gsl_matrix_set(a[1], i, 7, ii * ii * ii);
    gsl_matrix_set(a[1], i, 8, jj * jj * ii);
    gsl_matrix_set(a[1], i, 9, ii * ii * jj);
    gsl_vector_set(b[1], i, dewarp->def->wpos[i][1]);
  }

  // Do the fit
  gsl_multifit_linear(a[0], b[0], x[0], cov, &chi, work);
  gsl_multifit_linear(a[1], b[1], x[1], cov, &chi, work);
  
  // Copy the new results back into the mmap
  for (i = 0; i < 10; i++)
  {
    dewarp->def->iwtrans[0][i] = gsl_vector_get(x[0], i);
    dewarp->def->iwtrans[1][i] = gsl_vector_get(x[1], i);
  }
  
  // Solve for the inverse transform
  // world -> image
  for (i = 0; i < dewarp->def->points; i++)
  {
    xx = dewarp->def->wpos[i][0];
    yy = dewarp->def->wpos[i][1];
    
    gsl_matrix_set(a[0], i, 0, 1);
    gsl_matrix_set(a[0], i, 1, xx);
    gsl_matrix_set(a[0], i, 2, yy);
    gsl_matrix_set(a[0], i, 3, xx * xx);
    gsl_matrix_set(a[0], i, 4, yy * yy);
    gsl_matrix_set(a[0], i, 5, xx * yy);
    gsl_matrix_set(a[0], i, 6, xx * xx * xx);
    gsl_matrix_set(a[0], i, 7, yy * yy * yy);
    gsl_matrix_set(a[0], i, 8, xx * xx * yy);
    gsl_matrix_set(a[0], i, 9, yy * yy * xx);
    gsl_vector_set(b[0], i, dewarp->def->ipos[i][0]);

    gsl_matrix_set(a[1], i, 0, 1);
    gsl_matrix_set(a[1], i, 1, yy);
    gsl_matrix_set(a[1], i, 2, xx);
    gsl_matrix_set(a[1], i, 3, yy * yy);
    gsl_matrix_set(a[1], i, 4, xx * xx);
    gsl_matrix_set(a[1], i, 5, yy * xx);
    gsl_matrix_set(a[1], i, 6, yy * yy * yy);
    gsl_matrix_set(a[1], i, 7, xx * xx * xx);
    gsl_matrix_set(a[1], i, 8, yy * yy * xx);
    gsl_matrix_set(a[1], i, 9, xx * xx * yy);
    gsl_vector_set(b[1], i, dewarp->def->ipos[i][1]);
    
  }
  
  // Do the fit
  gsl_multifit_linear(a[0], b[0], x[0], cov, &chi, work);
  gsl_multifit_linear(a[1], b[1], x[1], cov, &chi, work);

  // Copy the new results back into the mmap
  for (i = 0; i < 10; i++)
  {
    dewarp->def->witrans[0][i] = gsl_vector_get(x[0], i);
    dewarp->def->witrans[1][i] = gsl_vector_get(x[1], i);
  }

  /*
  // TESTING
  printf("dewarp %f %f %f %f : %f %f %f %f\n",
         dewarp->def->witrans[0][0], dewarp->def->witrans[0][1],
         dewarp->def->witrans[0][2], dewarp->def->witrans[0][3], dewarp->def->witrans[0][4],
         dewarp->def->witrans[0][5], dewarp->def->witrans[0][6], dewarp->def->witrans[0][7]);
  */

  gsl_multifit_linear_free(work);
  gsl_matrix_free(cov);

  gsl_matrix_free(a[0]);  
  gsl_vector_free(x[0]);
  gsl_vector_free(b[0]);
  
  gsl_matrix_free(a[1]);
  gsl_vector_free(x[1]);
  gsl_vector_free(b[1]);
}


// Convert point from image to world coords
void dewarp_image2world(double i, double j, double *x, double *y)
{
  *x = dewarp->def->iwtrans[0][0] + dewarp->def->iwtrans[0][1] * i +
    + dewarp->def->iwtrans[0][2] * j + dewarp->def->iwtrans[0][3] * i * i
    + dewarp->def->iwtrans[0][4] * j * j + dewarp->def->iwtrans[0][5] * i * j
    + dewarp->def->iwtrans[0][6] * i * i * i + dewarp->def->iwtrans[0][7] * j * j * j
    + dewarp->def->iwtrans[0][8] * i * i * j + dewarp->def->iwtrans[0][9] * j * j * i;

  *y = dewarp->def->iwtrans[1][0] + dewarp->def->iwtrans[1][1] * j
    + dewarp->def->iwtrans[1][2] * i + dewarp->def->iwtrans[1][3] * j * j
    + dewarp->def->iwtrans[1][4] * i * i + dewarp->def->iwtrans[1][5] * j * i
    + dewarp->def->iwtrans[1][6] * j * j * j + dewarp->def->iwtrans[1][7] * i * i * i
    + dewarp->def->iwtrans[1][8] * j * j * i + dewarp->def->iwtrans[1][9] * i * i * j;
}


// Convert point from world to image coords
void dewarp_world2image(double x, double y, double *i, double *j)
{
  *i = dewarp->def->witrans[0][0] + dewarp->def->witrans[0][1] * x +
    + dewarp->def->witrans[0][2] * y + dewarp->def->witrans[0][3] * x * x
    + dewarp->def->witrans[0][4] * y * y + dewarp->def->witrans[0][5] * x * y
    + dewarp->def->witrans[0][6] * x * fabs(x) + dewarp->def->witrans[0][7] * x * y * y;

  *j = dewarp->def->witrans[1][0] + dewarp->def->witrans[1][1] * y
    + dewarp->def->witrans[1][2] * x + dewarp->def->witrans[1][3] * y * y
    + dewarp->def->witrans[1][4] * x * x + dewarp->def->witrans[1][5] * y * x
    + dewarp->def->witrans[1][6] * y * fabs(y) + dewarp->def->witrans[1][7] * y * x * x;
}




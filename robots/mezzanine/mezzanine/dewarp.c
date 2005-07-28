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
 * CVS: $Id: dewarp.c,v 1.5 2005-07-28 20:54:19 stack Exp $
 ***************************************************************************/

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_multifit.h>
#include "opt.h"
#include "mezzanine.h"


#define WARP_SCALE		// Apply scaling.
#define WARP_COS		// Plus cosine lens distortion correction.
#define WARP_CANCEL		// Plus blended error vector cancellation.

#if defined(WARP_CANCEL)
# include "blend_tris.h"
#endif

// Update the coordinate transforms
void dewarp_update_trans();

// Convert point from image to world coords
void dewarp_image2world(double i, double j, double *x, double *y);

// Convert point from world to image coords
void dewarp_world2image(double x, double y, double *i, double *j);

// Dewarp information
typedef struct
{
  mezz_mmap_t *mmap;       // Pointer to the mmap
  mezz_dewarpdef_t *def;   // Pointer to the mmaped dewarp stuff
} dewarp_t;

// The one and only instance of the dewarper
static dewarp_t *dewarp;

#if defined(WARP_CANCEL)

/* Lay out the triangles for error cancellation.
 *
 * The required point order is left-to-right, bottom-to-top, so the lower-left
 * corner comes first on the bottom row, then the middle and top rows.
 * Triangles are generated clockwise from the bottom row.  Vertices are listed
 * clockwise from the center in each triangle, so edges will have the inside on
 * the right.
 *
 *  p6 ------ p7 ----- p8
 *   | \      |      / |
 *   |  \     |     /  |
 *   |   \ t4 | t5 /   |
 *   |    \   |   /    |
 *   | t3  \  |  /  t6 |
 *   |      \ | /      |
 *  p3 ------ p4 ----- p5
 *   |      / | \      |
 *   | t2  /  |  \  t7 |
 *   |    /   |   \    |
 *   |   / t1 | t0 \   |
 *   |  /     |     \  |
 *   | /      |      \ |
 *  p0 ------ p1 ----- p2
 */
#define N_BLEND_TRIS 8
#define N_CAL_PTS (N_BLEND_TRIS+1)	// One more point than triangle.
static int tri_pattern[N_BLEND_TRIS][3] = { 
  {4,2,1}, {4,1,0}, {4,0,3}, {4,3,6}, {4,6,7}, {4,7,8}, {4,8,5}, {4,5,2}
};
static int gotTris = 0;
static blendTriObj triangles[N_BLEND_TRIS];
#endif

// Initialise the dewarper
int dewarp_init(mezz_mmap_t *mmap)
{
  int i, j, k;
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
    if (opt_get_double2("dewarp", key, 
			&dewarp->def->wpos[i][0], &dewarp->def->wpos[i][1]) < 0)
      break;
    snprintf(key, sizeof(key), "ipos[%d]", i);
    opt_get_int2("dewarp", key, &dewarp->def->ipos[i][0], &dewarp->def->ipos[i][1]);
    dewarp->def->points++;
  }

  dewarp->def->warpFactor = opt_get_double("dewarp","warpFactor",1.414);
  dewarp->def->ocHeight = opt_get_double("dewarp","ocHeight",2.500);

  dewarp->def->scaleFactorX = (dewarp->mmap->width - 1)/2.0;
  dewarp->def->scaleFactorY = (dewarp->mmap->height - 1)/2.0;

  i = opt_get_double2("dewarp","scaleFactor",
		      &dewarp->def->scaleFactorX,
		      &dewarp->def->scaleFactorY);

  dewarp->def->ocX = (dewarp->mmap->width - 1)/2;
  dewarp->def->ocY = (dewarp->mmap->height - 1)/2;

  i = opt_get_double2("dewarp","cameraCenter",
		      &dewarp->def->ocX,
		      &dewarp->def->ocY);
  
  dewarp->def->gridX = 0.0;
  dewarp->def->gridY = 0.0;

  i = opt_get_double2("dewarp","gridoff",
		      &dewarp->def->gridX,
		      &dewarp->def->gridY);
  
# if defined(WARP_CANCEL)

  // Calculate error between nominal and actual world coords of calibration pts.
  double iwpos[N_CAL_PTS][2], epos[N_CAL_PTS][2];
  for ( i = 0; i < N_CAL_PTS; i++ )
  {
    // World coords calculated from image coords, with triangle blending off.
    dewarp_image2world(dewarp->def->ipos[i][0], dewarp->def->ipos[i][1], 
		       &iwpos[i][0], &iwpos[i][1]);
    // Difference between actual measured and ideal target world coordinates.
    epos[i][0] = iwpos[i][0] - (dewarp->def->wpos[i][0] + dewarp->def->gridX);
    epos[i][1] = iwpos[i][1] - (dewarp->def->wpos[i][1] + dewarp->def->gridY);
  }

  // Create triangles for piecewise linear blending of error corrections.
  for ( i = 0; i < N_BLEND_TRIS; i++ )
  {
    int validTri = 0;
    for ( j = 0; j < 3; j++ )
    {
      int validPt = 0;
      for ( k = 0; k < 2; k++ )
      {
	// Assumption: valid points have >= one non-zero image (pixel) coord.
	int pixCoord;
	triangles[i].verts[j][k] =  
	  pixCoord = dewarp->def->ipos[ tri_pattern[i][j] ][k];
	validPt |= pixCoord;

	triangles[i].target[j][k] = dewarp->def->wpos[ tri_pattern[i][j] ][k];
	triangles[i].error[j][k] =  epos[ tri_pattern[i][j] ][k];
      }
      if ( validPt ) validTri++;

      // XXX Horrid Hack Warning - We need right-handed coordinates, but the
      // image Y coordinate goes down from 0 at the top.  Negate it internally.
      triangles[i].verts[j][1] *= -1.0;
    }
    if ( validTri ) gotTris++;

    initBlendTri(&triangles[i]);    // Fill in the rest of the blend triangle.
  }

  if ( gotTris != N_BLEND_TRIS )
    printf("*** Only %d valid triangles, error cancellation is turned off.\n"
	   gotTris);
# endif

  // Generate the transform values
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
    //printf("blob %d %d %d\n",i,blob->ox,blob->oy);
    dewarp_image2world(blob->ox, blob->oy,
                       &blob->wox, &blob->woy);
  }

  dewarp->mmap->bloblist = *bloblist;

  return bloblist;
}


// Update the coordinate transforms
void dewarp_update_trans()
{
#if !defined(WARP_IDENTITY) && !defined(WARP_SCALE) && !defined(WARP_COS)
  int i;
  double xx, yy;
  double ii, jj;
  gsl_matrix *a[2];
  gsl_vector *x[2];
  gsl_vector *b[2];
  gsl_matrix *cov; double chi;
  gsl_multifit_linear_workspace *work;
  
  a[0] = gsl_matrix_alloc(dewarp->def->points, 8);
  x[0] = gsl_vector_alloc(8);
  b[0] = gsl_vector_alloc(dewarp->def->points);

  a[1] = gsl_matrix_alloc(dewarp->def->points, 8);
  x[1] = gsl_vector_alloc(8);
  b[1] = gsl_vector_alloc(dewarp->def->points);

  cov = gsl_matrix_alloc(8, 8);
  work = gsl_multifit_linear_alloc(dewarp->def->points, 8);
  
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
    gsl_matrix_set(a[0], i, 6, ii * fabs(ii));
    gsl_matrix_set(a[0], i, 7, ii * jj * jj);
    gsl_vector_set(b[0], i, dewarp->def->wpos[i][0]);

    gsl_matrix_set(a[1], i, 0, 1);
    gsl_matrix_set(a[1], i, 1, jj);
    gsl_matrix_set(a[1], i, 2, ii);
    gsl_matrix_set(a[1], i, 3, jj * jj);
    gsl_matrix_set(a[1], i, 4, ii * ii);
    gsl_matrix_set(a[1], i, 5, jj * ii);
    gsl_matrix_set(a[1], i, 6, jj * fabs(jj));
    gsl_matrix_set(a[1], i, 7, jj * ii * ii);
    gsl_vector_set(b[1], i, dewarp->def->wpos[i][1]);
  }

  // Do the fit
  gsl_multifit_linear(a[0], b[0], x[0], cov, &chi, work);
  gsl_multifit_linear(a[1], b[1], x[1], cov, &chi, work);
  
  // Copy the new results back into the mmap
  for (i = 0; i < 8; i++)
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
    gsl_matrix_set(a[0], i, 6, xx * fabs(xx));
    gsl_matrix_set(a[0], i, 7, xx * yy * yy);
    gsl_vector_set(b[0], i, dewarp->def->ipos[i][0]);

    gsl_matrix_set(a[1], i, 0, 1);
    gsl_matrix_set(a[1], i, 1, yy);
    gsl_matrix_set(a[1], i, 2, xx);
    gsl_matrix_set(a[1], i, 3, yy * yy);
    gsl_matrix_set(a[1], i, 4, xx * xx);
    gsl_matrix_set(a[1], i, 5, yy * xx);
    gsl_matrix_set(a[1], i, 6, yy * fabs(yy));
    gsl_matrix_set(a[1], i, 7, yy * xx * xx);
    gsl_vector_set(b[1], i, dewarp->def->ipos[i][1]);
  }
  
  // Do the fit
  gsl_multifit_linear(a[0], b[0], x[0], cov, &chi, work);
  gsl_multifit_linear(a[1], b[1], x[1], cov, &chi, work);

  // Copy the new results back into the mmap
  for (i = 0; i < 8; i++)
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
#endif
}

double dewarp_cos(double x, double y, mezz_dewarpdef_t *mmap)
{
    return cos(mmap->warpFactor * atan2(hypot(x,y), mmap->ocHeight));
}

// Convert point from image to world coords
void dewarp_image2world(double i, double j, double *x, double *y)
{

#ifdef WARP_IDENTITY
  // World coords are the same as pixel coords.
  *x = i;
  *y = j;

#elif defined(WARP_SCALE)
  // Pixels are  to [0 ... 639, 0 ... 479], with the origin in the upper-left.
  // World coordinates are [-1 ... 1, -1 ... 1], with the origin in the center.
  *x = (i - dewarp->def->ocX) / dewarp->def->scaleFactorX;
  *y = -(j - dewarp->def->ocY) / dewarp->def->scaleFactorY;

# if defined(WARP_COS)
  // Apply cosine dewarping.
  double f = 1.0;
  int lpc;
  
  for (lpc = 0; lpc < 8; lpc++) {
      f = dewarp_cos(*x / f, *y / f, dewarp->def);
  }

  *x /= f;
  *y /= f;

#   if defined(WARP_CANCEL)
  // Apply piecewise triangular linear blended error cancellation.
  int iTri;
  geomPt imPt;
  double bcs[3];
  geomVec cancelVec;

  // Find the triangle by looking at the center edges of the triangles.
  // We can actually blend linearly past the outer edges if necessary...
  if ( gotTris == N_BLEND_TRIS )   // Don't do it until triangles are initialized.
  {
    imPt[0] = i; imPt[1] = -j;	   // XXX Horrid Hack Warning - negate Y.
    for ( iTri = 0; iTri < N_BLEND_TRIS; iTri++ )
    {
      // Get the barycentric coords of the image point in the triangle.
      // Positive on the inside of an edge, reaching 1.0 at the opposing vertex.
      baryCoords(&triangles[iTri], imPt, bcs);
      if ( bcs[1] >= 0.0 && bcs[2] >= 0.0 )
      {
	// This is the triangle containing this image point.
	errorBlend(&triangles[iTri], bcs, cancelVec);
	*x += cancelVec[0];
	*y += cancelVec[1];

	break;
      }  
    }
  }
#   endif // WARP_CANCEL
# endif   // WARP_COS

#else
  *x = dewarp->def->iwtrans[0][0] + dewarp->def->iwtrans[0][1] * i +
    + dewarp->def->iwtrans[0][2] * j + dewarp->def->iwtrans[0][3] * i * i
    + dewarp->def->iwtrans[0][4] * j * j + dewarp->def->iwtrans[0][5] * i * j
    + dewarp->def->iwtrans[0][6] * i * fabs(i) 
    + dewarp->def->iwtrans[0][7] * i * j * j;

  *y = dewarp->def->iwtrans[1][0] + dewarp->def->iwtrans[1][1] * j
    + dewarp->def->iwtrans[1][2] * i + dewarp->def->iwtrans[1][3] * j * j
    + dewarp->def->iwtrans[1][4] * i * i + dewarp->def->iwtrans[1][5] * j * i
    + dewarp->def->iwtrans[1][6] * j * fabs(j) 
    + dewarp->def->iwtrans[1][7] * j * i * i;
#endif

}

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
 * Desc: Geometry functions
 * Author: Andrew Howard
 * Date: 28 Mar 2002
 * CVS: $Id: geom.c,v 1.1 2004-12-12 23:36:33 johnsond Exp $
 **************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "geom.h"


// Some useful macros
#define MIN(x, y) ((x) < (y) ? x : y)
#define MAX(x, y) ((x) > (y) ? x : y)


// Compute the normalized angle (-pi to pi)
double geom_normalize(double angle)
{
  return atan2(sin(angle), cos(angle));
}


// Compute the minimum distance between a line and a point
double geom_nearest(const geom_line_t *l,
                      const geom_point_t *p,
                      geom_point_t *n)
{
  double a, b, s;
  double nx, ny;
  double dx, dy;

	// Compute parametric intercept point
  a = (l->pb.x - l->pa.x);
  b = (l->pb.y - l->pa.y);
  s = (a * (p->x - l->pa.x) + b * (p->y - l->pa.y)) / (a * a + b * b);
  
	// Bound to lie between endpoints
	if (s < 0)
		s = 0;
	if (s > 1)
		s = 1;

	// Compute intercept point
  nx = l->pa.x + a * s;
  ny = l->pa.y + b * s;
  
  if (n)
  {
    n->x = nx;
    n->y = ny;
  }

	// Compute distance
  dx = nx - p->x;
  dy = ny - p->y;
  return sqrt(dx * dx + dy * dy);
}


// Compute intesection between two line segments
// Returns 0 if there is no intersection
int geom_intersect_lines(const geom_line_t *la,
                           const geom_line_t *lb,
                           geom_point_t *p)
{
  double a11, a12, a21, a22;
  double b11, b12, b21, b22;
  double s, t;
  
	a11 = (la->pb.x - la->pa.x);
	b11 = la->pa.x;
	a12 = (la->pb.y - la->pa.y);
	b12 = la->pa.y;

  a21 = (lb->pb.x - lb->pa.x);
	b21 = lb->pa.x;
	a22 = (lb->pb.y - lb->pa.y);
	b22 = lb->pa.y;

	// See if there is an intercept
	if (fabs(a12 * a21 - a11 * a22) < 1e-16)
		return 0;
	
	// Compute parmetric intercepts
	s = ((a22 * b11 - a21 * b12) - (a22 * b21 - a21 * b22))
    / (a12 * a21 - a11 * a22);
	t = ((a12 * b11 - a11 * b12) - (a12 * b21 - a11 * b22)) 
    / (a12 * a21 - a11 * a22);
  
	// See if there is an intercept within the segment
	if (s < 0 || s > 1)
		return 0;
	if (t < 0 || t > 1)
		return 0;

	// Compute the intercept
	if (p != NULL)
	{
		p->x = a11 * s + b11;
		p->y = a12 * s + b12;
	}
	return 1;
}


// Determine whether or not a point is inside a polygon
// The original version of this code was stolen from
// http://astronomy.swin.edu.au/~pbourke/geometry/insidepoly/
int geom_poly_inside(geom_point_t *polygon, int vertices,
                       const geom_point_t *p)
{
  int i;
  double xinters;
  geom_point_t p1, p2;
  int counter = 0;
  
  p1 = polygon[0];
  
  for (i = 1; i <= vertices; i++)
  {
    p2 = polygon[i % vertices];

    if (p->y > MIN(p1.y, p2.y))
    {
      if (p->y <= MAX(p1.y, p2.y))
      {
        if (p->x <= MAX(p1.x, p2.x))
        {
          if (p1.y != p2.y)
          {
            xinters = (p->y-p1.y)*(p2.x-p1.x)/(p2.y-p1.y)+p1.x;
            if (p1.x == p2.x || p->x <= xinters)
              counter++;
          }
        }
      }
    }
    p1 = p2;
  }

  return (counter % 2 == 1);
}





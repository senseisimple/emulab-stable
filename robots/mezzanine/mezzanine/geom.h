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
 * CVS: $Id: geom.h,v 1.1 2004-12-12 23:36:33 johnsond Exp $
 **************************************************************************/

#ifndef GEOM_H
#define GEOM_H


// Simple 2D point
typedef struct
{
  double x, y;
} geom_point_t;


// Simple 2D line
typedef struct
{
  geom_point_t pa, pb;
} geom_line_t;


// Compute the normalized angle (-pi to pi)
double geom_normalize(double angle);

// Compute the minimum distance between a line and a point
double geom_nearest(const geom_line_t *l,
                    const geom_point_t *p,
                    geom_point_t *n);

// Compute intesection between two line segments
// Returns 0 if there is no intersection
int geom_intersect_lines(const geom_line_t *la,
                         const geom_line_t *lb,
                         geom_point_t *p);

// Determine whether or not a point is inside a polygon
int geom_poly_inside(geom_point_t *polygon, int vertices,
                     const geom_point_t *p);


#endif

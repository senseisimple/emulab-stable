// Copyright 1998, Silicon Graphics, Inc. -- ALL RIGHTS RESERVED 
// 
// Permission is granted to copy, modify, use and distribute this
// software and accompanying documentation free of charge provided (i)
// you include the entirety of this reservation of rights notice in
// all such copies, (ii) you comply with any additional or different
// obligations and/or use restrictions specified by any third party
// owner or supplier of the software and accompanying documentation in
// other notices that may be included with the software, (iii) you do
// not charge any fee for the use or redistribution of the software or
// accompanying documentation, or modified versions thereof.
// 
// Contact sitemgr@sgi.com for information on licensing this software 
// for commercial use. Contact munzner@cs.stanford.edu for technical 
// questions. 
// 
// SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH RESPECT TO THIS
// SOFTWARE, EXPRESS, IMPLIED, OR OTHERWISE, INCLUDING WITHOUT
// LIMITATION, ALL WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE OR NONINFRINGEMENT. SILICON GRAPHICS SHALL NOT
// BE LIABLE FOR ANY SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES,
// INCLUDING, WITHOUT LIMITATION, LOST REVENUES, LOST PROFITS, OR LOSS
// OF PROSPECTIVE ECONOMIC ADVANTAGE, RESULTING FROM THE USE OR MISUSE
// OF THIS SOFTWARE.
// 
// U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND: 
// 
// Use, duplication or disclosure by the Government is subject to
// restrictions as set forth in FAR 52.227.19(c)(2) or subparagraph
// (c)(1)(ii) of the Rights in Technical Data and Computer Software
// clause at DFARS 252.227-7013 and/or in similar or successor clauses
// in the FAR, or the DOD or NASA FAR Supplement. Unpublished - rights
// reserved under the Copyright Laws of United States.
// Contractor/manufacturer is Silicon Graphics, Inc., 2011 N.
// Shoreline Blvd. Mountain View, CA 94039-7311.

// H3Viewer includes portions of geomview/OOGL. Copyright (c)
// 1992 The Geometry Center; University of Minnesota, 1300 South
// Second Street; Minneapolis, MN 55454, USA
// 
// geomview/OOGL is free software; you can redistribute it and/or
// modify it only under the terms given in the file COPYING, which you
// should have received along with this file. This and other related
// software may be obtained via anonymous ftp from geom.umn.edu;
// email: software@geom.umn.edu.
// 
// The incorporated portions of geomview/OOGL have been modified by
// Silicon Graphics, Inc. in 1998 for the purpose of the creation of
// this software.

#include <string>
NAMESPACEHACK

#include <math.h>
#ifdef WIN32
#include <string.h>
#else
#include <strings.h>
#endif
#include "HypTransform.h"

HypTransform HypTransform::concat(HypTransform Tb) {
  HypTransform Tprod;
  register int i;
  for( i=0; i<4; i++ ) {			
    Tprod.T[i][0] = T[i][0]*Tb.T[0][0] +	
      T[i][1]*Tb.T[1][0] +				
      T[i][2]*Tb.T[2][0] +		
      T[i][3]*Tb.T[3][0];		
    Tprod.T[i][1] = T[i][0]*Tb.T[0][1] +	
      T[i][1]*Tb.T[1][1] +		
      T[i][2]*Tb.T[2][1] +		
      T[i][3]*Tb.T[3][1];		
    Tprod.T[i][2] = T[i][0]*Tb.T[0][2] +	
      T[i][1]*Tb.T[1][2] +		
      T[i][2]*Tb.T[2][2] +		
      T[i][3]*Tb.T[3][2];		
    Tprod.T[i][3] = T[i][0]*Tb.T[0][3] +	
      T[i][1]*Tb.T[1][3] +		
      T[i][2]*Tb.T[2][3] +		
      T[i][3]*Tb.T[3][3];		
  }
  return Tprod;
}

void HypTransform::copy(HypTransform Tsrc) {
#ifdef WIN32
	memcpy( (char *)T, (char *)Tsrc.T, sizeof(double)*16);
#else
  bcopy( (char *)Tsrc.T, (char *)T, sizeof(double)*16); // bcopy src dst
#endif
}

void HypTransform::identity() {
  T[0][0] = 1.0; T[0][1] = 0.0; T[0][2] = 0.0; T[0][3] = 0.0; 
  T[1][0] = 0.0; T[1][1] = 1.0; T[1][2] = 0.0; T[1][3] = 0.0; 
  T[2][0] = 0.0; T[2][1] = 0.0; T[2][2] = 1.0; T[2][3] = 0.0; 
  T[3][0] = 0.0; T[3][1] = 0.0; T[3][2] = 0.0; T[3][3] = 1.0; 
}


void HypTransform::invert() {
  HypTransform t;
  register int i, j, k;
  double x, f;

  t.copy(*this);
  identity();

  for (i = 0; i < 4; i++) {
    int largest = i;
    double largesq = t.T[i][i]*t.T[i][i];
    for (j = i+1; j < 4; j++)
      if ((x = t.T[j][i]*t.T[j][i]) > largesq)
	largest = j,  largesq = x;

    /* swap t[i][] with t[largest][] */
    t.swap(i,0, largest);  t.swap(i,1, largest);  t.swap(i,2, largest);  t.swap(i,3, largest);
    swap(i,0, largest);  swap(i,1, largest);  swap(i,2, largest);  swap(i,3, largest);

    for (j = i+1; j < 4; j++) {
      f = t.T[j][i] / t.T[i][i];
      /* subtract f*t[i][] from t[j][] */
      
      t.sub(i,j,0,f); t.sub(i,j,1,f); t.sub(i,j,2,f); t.sub(i,j,3,f);
      sub(i,j,0,f); sub(i,j,1,f); sub(i,j,2,f); sub(i,j,3,f);
    }
  }
  for (i = 0; i < 4; i++) {
    f = t.T[i][i];
    for (k = 0; k < 4; k++) {
      t.T[i][k] /= f;
      T[i][k] /= f;
    }
  }
  for (i = 3; i >= 0; i--)
    for (j = i-1; j >= 0; j--) {
      f = t.T[j][i];
      t.sub(i,j,0,f); t.sub(i,j,1,f); t.sub(i,j,2,f); t.sub(i,j,3,f);
      sub(i,j,0,f); sub(i,j,1,f); sub(i,j,2,f); sub(i,j,3,f);
    }
}

void HypTransform::rotate(double angle, HypPoint axis) {
    HypPoint Vu;
    Vu.copy(axis);
    Vu.unit();

    double sinA = sin(angle); 
    double cosA = cos(angle); 
    double versA = 1 - cosA;

    identity();
    T[0][0] = Vu.x*Vu.x*versA + cosA;
    T[1][0] = Vu.x*Vu.y*versA - Vu.z*sinA;
    T[2][0] = Vu.x*Vu.z*versA + Vu.y*sinA;

    T[0][1] = Vu.y*Vu.x*versA + Vu.z*sinA;
    T[1][1] = Vu.y*Vu.y*versA + cosA;
    T[2][1] = Vu.y*Vu.z*versA - Vu.x*sinA;

    T[0][2] = Vu.x*Vu.z*versA - Vu.y*sinA;
    T[1][2] = Vu.y*Vu.z*versA + Vu.x*sinA;
    T[2][2] = Vu.z*Vu.z*versA + cosA;
}


void HypTransform::rotateBetween(HypPoint vfrom, HypPoint vto) {

  identity();
  double len = sqrt(vfrom.dotEuc(vfrom) * vto.dotEuc(vto));
  if(len == 0) return;

  double cosA = vfrom.dotEuc(vto) / len;
  double versA = 1 - cosA;

  HypPoint Vu;
  Vu = vfrom.cross(vto);
  double sinA = Vu.length() / len;
  if(sinA == 0) return;

  Vu.mult( 1/(len*sinA) );	/* Normalize Vu */

  T[0][0] = Vu.x*Vu.x*versA + cosA;
  T[1][0] = Vu.x*Vu.y*versA - Vu.z*sinA;
  T[2][0] = Vu.x*Vu.z*versA + Vu.y*sinA;

  T[0][1] = Vu.y*Vu.x*versA + Vu.z*sinA;
  T[1][1] = Vu.y*Vu.y*versA + cosA;
  T[2][1] = Vu.y*Vu.z*versA - Vu.x*sinA;

  T[0][2] = Vu.x*Vu.z*versA - Vu.y*sinA;
  T[1][2] = Vu.y*Vu.z*versA + Vu.x*sinA;
  T[2][2] = Vu.z*Vu.z*versA + cosA;
}

void HypTransform::rotateTowardZ(HypPoint pt) {
  HypTransform S, U;
  double r = pt.z;
  /* Construct T = rotation about x-axis moving pt into x-z plane */
  identity();
  r = sqrt(pt.y*pt.y + r*r);
  if (r > 0) {
    T[2][1] = -(T[1][2] = pt.y/r);
    T[2][2] = T[1][1] = pt.z/r;
  }
  /* Construct S = rotation about y axis moving T(pt) into y-z plane */
  S.identity();
  r = sqrt(pt.x*pt.x + r*r);
  if (r > 0) {
    S.T[2][0] = -(S.T[0][2] = pt.x/r);
    S.T[2][2] = S.T[0][0] = sqrt(pt.z*pt.z + pt.y*pt.y)/r;
  }
  /* Desired transform is then S * T */
  U = concat(S);
  copy(U);
}

/* carefulRotateTowardZ gives a matrix which rotates the world
 * about an axis perpendicular to both pos and the z axis, which moves
 * the -Z axis to pos (so [0,0,-1,0] * T = pos).
 * Unlike rotateTowardZ, the "twist" is well-defined
 * provided that it is not asked to rotate the negative z axis toward
 * the positive z axis.
 */
void HypTransform::rotateTowardZCareful(HypPoint pos) {
  HypTransform S, Sinv, U;
  static HypPoint minusZ = { 0,0,-1,0 };
  HypPoint perp, axis, posxy;
  double dist, c, s;
  /* first, find a rotation takes both pos and the z axis into the xy plane */
  perp.x = -pos.y; perp.y = pos.x; perp.z = 0; perp.w = 1;
  S.rotateTowardZ(perp);
   
  /* now, rotate pos and the -Z axis to this plane */
  axis = S.transformPoint(minusZ);
  posxy = S.transformPoint(pos);

  /* find the rotation matrix for the transformed points */
  c = axis.x * posxy.x + axis.y * posxy.y;
  s = axis.x * posxy.y - axis.y * posxy.x;
  dist = sqrt(c*c+s*s);
  identity();
  if (dist > 0) {
    c /= dist;
    s /= dist;
    T[0][0] =  c; T[0][1] = s;
    T[1][0] = -s; T[1][1] = c;
  } else if(pos.z > 0) {
    T[1][1] = T[2][2] = -1;	/* Singular rotation: arbitrarily flip YZ */
  } /* else no rotation */

  /* Finally, conjugate the result back to the original coordinate system */
  Sinv.copy(S);
  Sinv.invert();
  U = S.concat(*this);
  U = U.concat(Sinv);
  copy(U);
}

void HypTransform::scale(double sx, double sy, double sz) {
    register double *aptr;

    identity();

    aptr=T[0];

    /* row 1 */
    *aptr++ *= sx;
    *aptr++ *= sx;
    *aptr++ *= sx;
    *aptr++ *= sx;

    /* row 2 */
    *aptr++ *= sy;
    *aptr++ *= sy;
    *aptr++ *= sy;
    *aptr++ *= sy;

    /* row 3 */
    *aptr++ *= sz;
    *aptr++ *= sz;
    *aptr++ *= sz;
    *aptr++ *= sz;

    /* row 4 is unchanged */
}

HypPoint HypTransform::transformPoint(HypPoint pt1) {
  HypPoint pt2;
  register double x = pt1.x, y = pt1.y, z = pt1.z, w = pt1.w;

  pt2.x = x*T[0][0] + y*T[1][0] + z*T[2][0] + w*T[3][0];
  pt2.y = x*T[0][1] + y*T[1][1] + z*T[2][1] + w*T[3][1];
  pt2.z = x*T[0][2] + y*T[1][2] + z*T[2][2] + w*T[3][2];
  pt2.w = x*T[0][3] + y*T[1][3] + z*T[2][3] + w*T[3][3];
  return pt2;
}

void HypTransform::translateEuc(double tx, double ty, double tz) {
  HypPoint pt;
  pt.x = tx;
  pt.y = ty;
  pt.z = tz;
  pt.w = 1;
  translateOriginEuc(pt);
}

void HypTransform::translateHyp(double tx, double ty, double tz) {
  HypPoint pt;
  double t = sqrt( tx*tx + ty*ty + tz*tz );

  if(t > 0) {
    pt.x = sinh(t) * tx / t;
    pt.y = sinh(t) * ty / t;
    pt.z = sinh(t) * tz / t;
    pt.w = cosh(t);
    translateOriginHyp(pt);
  } else {
    identity();
  }
}

void HypTransform::translateOriginEuc(HypPoint pt) {
  identity();
  T[3][0] = pt.x / pt.w;
  T[3][1] = pt.y / pt.w;
  T[3][2] = pt.z / pt.w;
}

void HypTransform::translateOriginHyp(HypPoint pt) {
  HypTransform R, Rinv, U;
  HypPoint p;
  p.copy(pt); // nondestructive on passed in point
  p.normalizeHyp();
  identity();
  T[2][3] = T[3][2] = sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
  T[2][2] = T[3][3] = p.w;
  R.rotateTowardZ(p);
  Rinv.copy(R);
  Rinv.invert();
  U = R.concat(*this);
  U = U.concat(Rinv);
  copy(U);
}

void HypTransform::transpose() {
  double t;
  for( int i=0; i<4; i++ ) 
    for( int j=0; j<i; j++ ) {
      t = T[i][j];
      T[i][j] = T[j][i];
      T[j][i] = t;
    }
}



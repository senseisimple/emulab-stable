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

#ifdef WIN32
#include <math.h>
#include <float.h>

//Inverse Hyperbolic Cosine
double acosh(double dw) {  return log(dw+sqrt(dw*dw-1)); }
#else
extern "C" {
#include <math.h>
#include <float.h>
}
#endif

#include "HypPoint.h"

HypPoint HypPoint::add(HypPoint v2) {
  HypPoint v3;
  v3.x = x + v2.x;
  v3.y = y + v2.y;
  v3.z = z + v2.z;
  v3.w = 1.;
  return v3;
}

void HypPoint::copy(HypPoint psrc) {
  x = psrc.x;
  y = psrc.y;
  z = psrc.z;
  w = psrc.w;
}

HypPoint HypPoint::cross(HypPoint v2) {
  HypPoint v3;
  v3.x = y*v2.z-z*v2.y; 
  v3.y = z*v2.x-x*v2.z; 
  v3.z = x*v2.y-y*v2.x;
  v3.w = 1.;
  return v3;
}

double HypPoint::distanceEuc(HypPoint b) {
    double dx, dy, dz;
    double w1w2;

    w1w2 = w * b.w;
    if( w1w2 == 0. )
	return 0.;

    dx = b.w * x - b.x * w;
    dy = b.w * y - b.y * w;
    dz = b.w * z - b.z * w;

    return (sqrt( dx*dx + dy*dy + dz*dz )) / w1w2;
}

double HypPoint::distanceHyp(HypPoint b) {
  // cerr << "HypPoint::distanceHyp() called..." << endl;
  double aa, bb, ab;
  aa = dotHyp(*this);
  bb = b.dotHyp(b);
  ab = dotHyp(b);
  if (aa == bb && bb == ab) {
    return(0.0);      // points are equal. 
  }

  if (sqrt( fabs(aa * bb) ) == 0.0) {
    // cerr << "HypPoint::distanceHyp() returning " << acosh(9999) << endl;
    return(acosh(9999));
  }
  if (ab/sqrt(fabs(aa * bb)) < 1.0) {
    return(1.0);
  }
  // cerr << "HypPoint::distanceHyp() returning "
  //      << acosh(fabs(ab/sqrt(fabs(aa*bb)))) << endl;
  return acosh(fabs(ab/sqrt(fabs(aa*bb))));
}


double HypPoint::dotEuc(HypPoint v2) {
  return x*v2.x+y*v2.y+z*v2.z;
}

/* inner product of R(3,1) - hyperbolic */

double HypPoint::dotHyp(HypPoint b) {
  return x*b.x + y*b.y + z*b.z - w*b.w;
}

void HypPoint::identity() {
  x=y=z= 0.0; w = 1.0;
}

double HypPoint::length() {
  return sqrt( x*x + y*y + z*z );
}

void HypPoint::mult( double s) {
  x = s*x;
  y = s*y;
  z = s*z;
}

void HypPoint::normalizeEuc() {
  if (w != 1.0 && w != 0.0) {
    double inv = 1./w;
    x = x * inv;
    y = y * inv;
    z = z * inv;
    w = 1.0;
  }
}

void HypPoint::normalizeHyp() {
  double f = sqrt( w*w  - x*x - y*y - z*z );
  x /= f;
  y /= f;
  z /= f;
  w /= f;
}

HypPoint HypPoint::scale(double s) {
  HypPoint sa;
  sa.x = s * x;
  sa.y = s * y;
  sa.z = s * z;
  sa.w = w;
  return sa;
}

HypPoint HypPoint::sub(HypPoint v2) {
  HypPoint v3;
  v3.x = x - v2.x;
  v3.y = y - v2.y;
  v3.z = z - v2.z;
  v3.w = 1.;
  return v3;
}

void HypPoint::unit() {
  double len = length();
  if( len != 0. && len != 1. )
    mult( 1./len);
}

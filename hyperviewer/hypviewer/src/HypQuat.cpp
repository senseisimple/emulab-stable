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

#include <string>
NAMESPACEHACK

#include <math.h>
#include "HypQuat.h"

/* code grabbed directly from Watt & Watt, p. 363 */

HypQuat::HypQuat() {
  q[0]=0.; q[1]=0.; q[2]=0.; q[3]=1.;
  nxt[0] = 1; nxt[1] = 2; nxt[2] = 0;
  halfpi = 1.570796326794895;
}  

HypTransform HypQuat::toTransform() {
  HypTransform mat;
  double s, xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;
  s = 2.0/(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
  
  xs = q[0]*s; 
  ys = q[1]*s; 
  zs = q[2]*s; 
  wx = q[3]*xs; 
  wy = q[3]*ys; 
  wz = q[3]*zs; 
  xx = q[0]*xs;   
  xy = q[0]*ys;   
  xz = q[0]*zs;   
  yy = q[1]*ys;   
  yz = q[1]*zs;   
  zz = q[2]*zs;   

  mat.T[0][0] = 1.0 - (yy +zz);
  mat.T[0][1] = xy+wz;
  mat.T[0][2] = xz-wy;

  mat.T[1][0] = xy-wz;
  mat.T[1][1] = 1.0 - (xx +zz);
  mat.T[1][2] = yz+wx;

  mat.T[2][0] = xz+wy;
  mat.T[2][1] = yz-wx;
  mat.T[2][2] = 1.0 - (xx +yy);

  mat.T[0][3] = 0.0; mat.T[1][3] = 0.0; mat.T[2][3] = 0.0; mat.T[3][3] = 1.0;
  mat.T[3][0] = 0.0; mat.T[3][1] = 0.0; mat.T[3][2] = 0.0; 
  return mat;
}

void HypQuat::fromTransform(HypTransform mat) {

  double tr, s;
  int i,j,k;
  
  tr = mat.T[0][0] + mat.T[1][1] + mat.T[2][2];
  if (tr > 0.0) {
    s = sqrt(tr+1.0);
    q[3] = s*.5;
    s = .5/s;
    q[0] = (mat.T[1][2] - mat.T[2][1])*s;
    q[1] = (mat.T[2][0] - mat.T[0][2])*s;
    q[2] = (mat.T[0][1] - mat.T[1][0])*s;
  } else {
    i = 0;
    if (mat.T[1][1] > mat.T[0][0]) i = 1;
    if (mat.T[2][2] > mat.T[i][i]) i = 2;
    j = nxt[i]; k = nxt[j];

    s = sqrt( (mat.T[i][i] - ( mat.T[j][j]+mat.T[k][k])) + 1.0);
    q[i] = s*0.5;
    s = 0.5/s;
    q[3] = (mat.T[j][k] - mat.T[k][j])*s;
    q[j] = (mat.T[i][j] - mat.T[j][i])*s;
    q[k] = (mat.T[i][k] - mat.T[k][i])*s;
  }
}

HypQuat HypQuat::slerp(HypQuat p, float t) {
  double omega, cosom, sinom, sclp, sclq;
  int i;
  HypQuat qt;
  cosom = p.q[0]*q[0] + p.q[1]*q[1] + p.q[2]*q[2] + p.q[3]*q[3];
  if ((1.0+cosom) > .00001) {
    if ((1.0-cosom) > .00001) {
      omega = acos(cosom);
      sinom = sin(omega);
      sclp = sin( (1.0-t)*omega)/sinom;
      sclq = sin(t*omega)/sinom;
    } else {
      sclp = 1.0-t;
      sclq = t;
    } 
    for (i=0; i < 4; i++) qt.q[i] = sclp*p.q[i] + sclq*q[i];
  } else {
    qt.q[0] = -p.q[1];    qt.q[1] = p.q[2];
    qt.q[2] = -p.q[3];    qt.q[3] = p.q[2];
    sclp = sin((1.0-t)*halfpi);
    sclq = sin(t*halfpi);
    for (i = 0; i < 3; i++) qt.q[i] = sclp*p.q[i] + sclq*q[i];
  }
  return qt;
}

double HypQuat::fixSign() {
  // make sure we're taking the shortest path
  double ds, db;
  ds = 0; db = 0;
  HypQuat qs, qb, iq;
  int i;
  for (i = 0; i < 4; i++) {
    qs.q[i] = iq.q[i]-q[i]; // smaller quat
    qb.q[i] = iq.q[i]+q[i]; // bigger quat
    ds += qs.q[i]*qs.q[i]; 
    db += qb.q[i]*qb.q[i];
  }
  if (ds > db) {
    for (i = 0; i < 4; i++)
      q[i] = -q[i];
  }
  return ds;
}

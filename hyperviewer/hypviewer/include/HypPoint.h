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

#ifndef HYPPOINT_H
#define HYPPOINT_H

class HypPoint {

public:

  HypPoint add(HypPoint v2);
  void copy(HypPoint psrc);
  HypPoint cross(HypPoint v2);
  double distanceEuc(HypPoint b);
  double distanceHyp(HypPoint b);
  double dotEuc(HypPoint v2 );
  double dotHyp(HypPoint b);
  void identity();
  double length();
  void mult( double s);
  void normalizeEuc();
  void normalizeHyp();
  HypPoint scale(double s);
  HypPoint sub(HypPoint v2);
  void unit();
  double x;
  double y;
  double z;
  double w;

};

#endif

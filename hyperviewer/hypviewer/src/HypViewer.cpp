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

#include <stdio.h>
//#include <unistd.h>

#ifdef WIN32
#  include <sys/timeb.h>
#  if 0 && defined(_DEBUG)
#    include <AFX.H>  // For TRACE.
#  else
#    define TRACE printf
#  endif
#else
#  include <sys/time.h>
#endif

#include <math.h>
#include "HypQuat.h"
#include "HypViewer.h"

#ifdef HYPGLUT
#include <GL/glut.h>
#endif //HYPGLUT
#ifdef HYPGLX
  #ifdef HYPFREE
    #include <GL/GLwMDrawA.h>
    #include <GL/gl.h>
  #else
    #include <X11/GLw/GLwMDrawA.h>
  #endif //HYPFREE
#include <X11/cursorfont.h>
#ifdef XPMSNAP
 #include <X11/xpm.h>
#endif //XPMSNAP

#ifdef HYPIRIX
#include <iomanip.h>
#else
#include <iomanip>
#endif

#endif // HYPGLX

#ifdef WIN32
int gettimeofday(timeval *tv, int *tz)
{
  struct timeb timebuffer;

  ftime(&timebuffer);
  tv->tv_sec = (long) timebuffer.time;
  tv->tv_usec = (long) (1000 * (timebuffer.millitm));
  return 0;
}
#endif

struct mgcontext *_mgc = NULL;
struct mgfuncs *_mgf = NULL;

HypViewer *thehv = NULL;

#ifdef HYPGLX
  static void SetWatchCursor(Widget w)
  {
    static Cursor watch = (Cursor)0;
    if(!watch)
      watch = XCreateFontCursor(XtDisplay(w),XC_watch);
    XDefineCursor(XtDisplay(w),XtWindow(w),watch);
    XmUpdateDisplay(w);
    return;
  }

  static void SetDefaultCursor(Widget w)
  {
    XUndefineCursor(XtDisplay(w),XtWindow(w));
    XmUpdateDisplay(w);
    return;
  }
#endif  // HYPGLX

#ifdef WIN32
HypViewer::HypViewer(HypData *d, HDC w,HWND hwin) {
  wid = w;
  win = hwin;
#elif HYPGLX
HypViewer::HypViewer(HypData *d, Widget w) {
  wid = w;
  dpy = NULL;
  cxt = NULL;
#else
HypViewer::HypViewer(HypData *d) {
#endif
  hiliteCallback = (void (*)(const string &,int,int))0;
  pickCallback = (void (*)(const string &,int,int))0;
  hd = d;
  pick = 0;
  hiliteNode = -1;
  idleframe = 0;
  thehv = this;
  origin.identity();
  xaxis.x = 0.9;
  xaxis.y = -0.2;
  xaxis.z = 0.0;
  xaxis.w = 1.0;
  // fix "identity" quaternion so it matches the slightly tilted xaxis
  HypPoint rr = origin.sub(xaxis);
  HypTransform R;
  R.rotateBetween(rr, xaxis); 
  iq.fromTransform(R);

  leftClickCB=NULL;
  middleClickCB=NULL;
  rightClickCB=NULL;
  leftDragCB=NULL;
  middleDragCB=NULL;
  rightDragCB=NULL;
  passiveCB=NULL;
  motionCount = 0;
  passiveCount = 0;
  labelbase = 0;
  labelscaledsize = 0;
  labeltoright = 0;
  pickrange = 1.0 / (pow(2,32) -1.0);
}

HypViewer::~HypViewer()
{
#ifdef HYPGLX
  if (idleid) {
    XtRemoveWorkProc(idleid);
  }
#endif
  labelscaledsize = 0;
}


void HypViewer::setGraph(HypGraph *h){
  DrawnQ.erase(DrawnQ.begin(),DrawnQ.end());
  TraverseQ.erase(TraverseQ.begin(),TraverseQ.end());
  DrawQ.erase(DrawQ.begin(),DrawQ.end());
  DrawnLinks.erase(DrawnLinks.begin(),DrawnLinks.end());
  hg = h; 
  HypNode *n = hg->getNodeFromIndex(hd->centerindex);
  if (!n) return;
  setGraphCenter(n->getIndex());
  setCurrentCenter(n);
  largest = -1.0;
  idleFunc(1);
}

void HypViewer::newBackground() {
  framePrepare();
  glClearColor(hd->colorBack[0], hd->colorBack[1], hd->colorBack[2], 1.0);
}

void HypViewer::newLabelFont() {
  drawStringInit();
}

void HypViewer::newSphereColor() {
  sphereInit();
}

void HypViewer::newLayout() {
  HypNode *n = getCurrentCenter();
  HypNode *best = (HypNode *)0;
  while (n) {
    if (!n->getEnabled())
      best = n;
    n = n->getParent();
  }
  if (best != (HypNode *)0) {
    HypNode *lastcenter = best->getParent();
    if (!lastcenter)
      lastcenter = hg->getNodeFromIndex(hd->centerindex);
    //    if (lastcenter) setGraphCenter(lastcenter->getIndex());
    setCurrentCenter(lastcenter);
    // there was something not enabled above us
  }
  DrawQ.erase(DrawQ.begin(),DrawQ.end());
  TraverseQ.erase(TraverseQ.begin(),TraverseQ.end());
}

#ifdef HYPGLX

  //--------------------------------------------------------------------------
  //               void HypViewer::afterRealize(GLXContext cx)               
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::afterRealize(GLXContext cx)
  {
    cxt = cx;
    dpy = XtDisplay(wid);
    win = XtWindow(wid);
    glGetIntegerv ( GL_VIEWPORT, vp );      /* get viewport size */ 
    glInit();
    drawStringInit();
    idleFunc(1);
  }
#ifdef XPMSNAP
  //--------------------------------------------------------------------------
  //           bool HypViewer::XpmSnapshot(const string & fileName)          
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  bool HypViewer::XpmSnapshot(const string & fileName)
  {
    int         widgetWidth = 0, widgetHeight = 0;
    XImage     *image;
    struct timeval   start, end;
    struct timezone  tz;
    
    XtVaGetValues(wid,
                  XmNwidth,&widgetWidth,
                  XmNheight,&widgetHeight,
                  NULL);
    glXWaitGL();
    gettimeofday(&start,&tz);
    image = XGetImage(XtDisplay(wid),XtWindow(wid),
                      0,0,widgetWidth,widgetHeight,0xffffffff,XYPixmap);
    gettimeofday(&end,&tz);
    //    cerr << "XGetImage() took "
    //         << (((end.tv_sec * 1000.0) + (end.tv_usec/1000.0)) -
    //             ((start.tv_sec * 1000.0) + (start.tv_usec/1000.0)))
    //         << " ms" << endl;
    if (image == (XImage *)0) {
      cerr << "XGetImage() failed." << endl;
      return(false);
    }
    int rc = XpmWriteFileFromImage(XtDisplay(wid),(char *)fileName.c_str(),
                                   image,(XImage *)0,(XpmAttributes *)0);
    if (rc == XpmSuccess)
      return(true);
    return(false);
  }
#endif //XPMSNAP

#endif  // HYPGLX


#ifdef HYPGLUT
  
  //--------------------------------------------------------------------------
  //                      void HypViewer::afterRealize()                     
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::afterRealize()
  {
    glGetIntegerv ( GL_VIEWPORT, vp );      /* get viewport size */ 
    //printf("GLUT afterRealize vp=[%d %d %d %d], labelscaledsize=%g\n", vp[0], vp[1], vp[2], vp[3], labelscaledsize );
    resetLabelSize();
    glInit();
    drawStringInit();
    idleFunc(1);
  }

#endif  // HYPGLUT

#ifdef WIN32
void HypViewer::afterRealize(HGLRC cx, int w, int h) {
  cxt = cx;
  wglMakeCurrent(wid,cxt);
  vp[0] = 0;
  vp[1] = 0;
  vp[2] = w;
  vp[3] = h;
  glInit();
  drawStringInit();
  idleFunc(1);
  frameBegin();
  drawSphere();
  frameEnd();
}
#endif

//--------------------------------------------------------------------------
//                       void HypViewer::sphereInit()                      
//..........................................................................
//  
//--------------------------------------------------------------------------
void HypViewer::sphereInit()
{
  // sphere at inifinity stuff 
  // don't use GLU sphere object since can't get smooth wireframe
  float phi, theta;
  float phistep = 3.14/12.0;
  float thetastep = 6.28/8.0;
  glNewList(HV_INFSPHERE, GL_COMPILE);
  glDisable(GL_CULL_FACE);
  glLineWidth(1.0);
  glColor3f(hd->colorSphere[0], hd->colorSphere[1], hd->colorSphere[2]);
  for (phi = 0.0; phi < 3.14; phi += phistep) {
    glBegin(GL_LINE_LOOP);
    for (theta = 0.0; theta < 6.28; theta += .1) {
      glVertex3f(cos(theta)*sin(phi), cos(phi), sin(theta)*sin(phi));
    }
    glEnd();
  }
  for (theta = -.35; theta < 3.14; theta += thetastep) {
    glBegin(GL_LINE_LOOP);
    for (phi = 0.0; phi < 6.28; phi += .1) {
      glVertex3f(cos(theta)*sin(phi), cos(phi), sin(theta)*sin(phi));
    }
    glEnd();
  }
  glEndList();
}

//----------------------------------------------------------------------------
//                         void HypViewer::glInit()                        
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::glInit()
{
  //printf("glInit\n");
  int i, j;
  framePrepare();
  glClearColor(hd->colorBack[0], hd->colorBack[1], hd->colorBack[2], 1.0);
  sphereInit();
  static float box[6][4][3] = 
  {
    {{-HV_LEAFSIZE,     HV_LEAFSIZE,    -HV_LEAFSIZE},
     { HV_LEAFSIZE,     HV_LEAFSIZE,    -HV_LEAFSIZE},
     { HV_LEAFSIZE,    -HV_LEAFSIZE,    -HV_LEAFSIZE},
     {-HV_LEAFSIZE,    -HV_LEAFSIZE,    -HV_LEAFSIZE}
    }, 
    {{-HV_LEAFSIZE,    -HV_LEAFSIZE,     HV_LEAFSIZE},
     { HV_LEAFSIZE,    -HV_LEAFSIZE,     HV_LEAFSIZE},
     { HV_LEAFSIZE,     HV_LEAFSIZE,     HV_LEAFSIZE},
     {-HV_LEAFSIZE,     HV_LEAFSIZE,     HV_LEAFSIZE}
    },
    {{ HV_LEAFSIZE,     HV_LEAFSIZE,    -HV_LEAFSIZE},
     {-HV_LEAFSIZE,     HV_LEAFSIZE,    -HV_LEAFSIZE},
     {-HV_LEAFSIZE,     HV_LEAFSIZE,     HV_LEAFSIZE},
     { HV_LEAFSIZE,     HV_LEAFSIZE,     HV_LEAFSIZE}
    },
    {{-HV_LEAFSIZE,    -HV_LEAFSIZE,    -HV_LEAFSIZE},
     { HV_LEAFSIZE,    -HV_LEAFSIZE,    -HV_LEAFSIZE},
     { HV_LEAFSIZE,    -HV_LEAFSIZE,     HV_LEAFSIZE},
     {-HV_LEAFSIZE,    -HV_LEAFSIZE,     HV_LEAFSIZE}
    },
    {{-HV_LEAFSIZE,    -HV_LEAFSIZE,    -HV_LEAFSIZE},
     {-HV_LEAFSIZE,    -HV_LEAFSIZE,     HV_LEAFSIZE},
     {-HV_LEAFSIZE,     HV_LEAFSIZE,     HV_LEAFSIZE},
     {-HV_LEAFSIZE,     HV_LEAFSIZE,    -HV_LEAFSIZE}
    },
    {{ HV_LEAFSIZE,    -HV_LEAFSIZE,    -HV_LEAFSIZE},
     { HV_LEAFSIZE,     HV_LEAFSIZE,    -HV_LEAFSIZE},
     { HV_LEAFSIZE,     HV_LEAFSIZE,     HV_LEAFSIZE},
     { HV_LEAFSIZE,    -HV_LEAFSIZE,     HV_LEAFSIZE}
    }
  };

  glNewList(HV_CUBECOORDS, GL_COMPILE);
  glBegin(GL_QUADS);
  for ( i = 0; i < 6; i++) {
    //    glColor3f(c[i][0], c[i][1], c[i][2]);
    for ( j = 0; j < 4; j++)
      glVertex3f(box[i][j][0], box[i][j][1], box[i][j][2]);
  }
  glEnd();
  glEndList();

  glNewList(HV_CUBEFACES, GL_COMPILE);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glCallList(HV_CUBECOORDS);
  glEndList();
  glNewList(HV_CUBEEDGES, GL_COMPILE);
  glColor3f(0, 0, 0);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glCallList(HV_CUBECOORDS);
  glEndList();
  glNewList(HV_CUBEALL, GL_COMPILE);
  glCallList(HV_CUBEFACES);
  glCallList(HV_CUBEEDGES);
  glEndList();

  // OGL options
  glEnable(GL_DEPTH_TEST);
  glCullFace(GL_BACK);
  glShadeModel(GL_SMOOTH);

#ifdef GL_EXT_polygon_offset  
#ifndef HYPLINUX
  glEnable(GL_POLYGON_OFFSET_EXT);
  glPolygonOffsetEXT(.5, .001);
#endif
#endif

  /*
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  */
  camInit();
  glMatrixMode(GL_MODELVIEW);  
  gluLookAt(0.0, 0.0, 3.0, 
	    0.0, 0.0, 0.0, 
	    0.0, 1.0, 0.0);
  glGetDoublev(GL_MODELVIEW_MATRIX, (double*)viewxform.T);
  vx = (double*)(viewxform.T);


  worldxform.identity();
  // start out facing right
  wx = (double*)(worldxform.T);
  wx[0] = -1; wx[10] = -1;
  // make it tilt slightly so parent and child text don't always collide 
  // in the single-child case
  HypPoint rr = origin.sub(xaxis);
  HypTransform R;
  R.rotateBetween(rr, xaxis); 
  worldxform = worldxform.concat(R);
  identxform.copy(worldxform);
  // clear to a blank color during rest of setup
  int oldidle = idleframe;
  idleframe = 0;
  frameBegin();
  frameEnd();
  idleframe = oldidle;
  return;
}

//----------------------------------------------------------------------------
//                        void HypViewer::camInit()                        
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::camInit()
{
  // camera setup
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  float fov;
  float aspectratio = vp[2]/((float)vp[3]);
  if (!(hd->keepAspect)) {
    aspectratio = 1.0;
  }
  if (aspectratio > 1.0) {
    fov = 180.0*(2.0 * atan2 (2.25/2.0, 3.0))/3.1415;
  } else {
    fov = 180.0*(2.0 * atan2 (2.25/(2.0*aspectratio), 3.0))/3.1415;
    if (fov > 180) fov = 179.0;
  }
  gluPerspective( fov, aspectratio, 1.5, 4.5);
  glGetDoublev(GL_PROJECTION_MATRIX, (double*)camxform.T);
  cx = (double*)(camxform.T);
  resetLabelSize();
  return;
}

// todo: findCenter, HypNodeQueue

//----------------------------------------------------------------------------
//                       void HypViewer::drawFrame()                       
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::drawFrame()
{

  if (!labelscaledsize) return; // not yet initialized
  HypNode *centernode, *current, *node;
  HypNodeArray *nodes;
  int i, generation;
  struct timeval now, start, allotted, elapsed, elapsedidle;
  gettimeofday(&now, NULL);
  start = now;
  allotted = hd->dynamictime;
  elapsed.tv_sec = 0; elapsed.tv_usec = 0;
  framePrepare();
  //    fprintf(stderr, "TOP %d %d idle %d\n", now.tv_sec, now.tv_usec, idleframe);

  if (!idleframe) {

    centernode = getCurrentCenter();
    centernode->setDrawn(0); // just in case
    for (i = 0; i < DrawnQ.size(); i++) {
      DrawnQ[i]->setDrawn(0);
      DrawnQ[i]->setDistance(9999);
      DrawnQ[i]->setSize(-1);
      DrawnQ[i]->setGeneration(9999);
    }
    if (centernode->getDrawn() == 1) {
      //      fprintf(stderr, "no nodes drawn\n");
    }
    DrawnQ.erase(DrawnQ.begin(),DrawnQ.end());
    TraverseQ.erase(TraverseQ.begin(),TraverseQ.end());
    DrawQ.erase(DrawQ.begin(),DrawQ.end());
    DrawQ.push_back(centernode);
    centernode->setGeneration(0);
    for (i = 0; i < DrawnLinks.size(); i++)
      DrawnLinks[i]->setDrawn(0);
    DrawnLinks.erase(DrawnLinks.begin(),DrawnLinks.end());
    framestart = now;
    largest = -1.0;
    HypTransform T;
    T.copy(worldxform);
    T.invert();
    frameorigin = T.transformPoint(origin);
    frameorigin.normalizeEuc();
    frameBegin();
  } else {
    elapsedidle.tv_sec = now.tv_sec - framestart.tv_sec;
    if (now.tv_usec < framestart.tv_usec) {
      // manual carry...
      elapsedidle.tv_usec = now.tv_usec + (1000000-framestart.tv_usec);
      elapsedidle.tv_sec--; 
    } else {
      elapsedidle.tv_usec = now.tv_usec - framestart.tv_usec;
    }
    if (elapsedidle.tv_sec < hd->idletime.tv_sec ||
	 (elapsedidle.tv_sec == hd->idletime.tv_sec && 
	 elapsedidle.tv_usec < hd->idletime.tv_usec)) {
      frameContinue();
      //            fprintf(stderr, "idledraw %d %d\n", now.tv_sec, now.tv_usec);
    } else {
      DrawQ.erase(DrawQ.begin(),DrawQ.end());
      TraverseQ.erase(TraverseQ.begin(),TraverseQ.end());
      idleFunc(0);
      //            fprintf(stderr, "finished idle\n");
      return;
    }
  }
  nodesdrawn = 0;
  linksdrawn = 0;

  // draw until run out of time
  while ( (elapsed.tv_sec < allotted.tv_sec ||
	  (elapsed.tv_sec == allotted.tv_sec && 
	  elapsed.tv_usec < allotted.tv_usec))) {
    if (DrawQ.size() > 0) {
      current = DrawQ[0];
      if (!current) {
	idleFunc(0);
	TraverseQ.erase(TraverseQ.begin(),TraverseQ.end());
	break;
      }
      DrawQ.erase(DrawQ.begin());
      if (current->getDrawn())	continue;
    } else if (TraverseQ.size() > 0) {
      current = TraverseQ[0];
      TraverseQ.erase(TraverseQ.begin());
      node = current->getParent();
      generation = current->getGeneration();
      if (node && !node->getDrawn() && node->getEnabled()) {
	DrawQ.push_back(node);
	node->setGeneration(generation+1);
      }
      nodes = current->getChildren();
      for (i = 0; i < (*nodes).size(); i++) {
	HypNode *n = (*nodes)[i];
	if (n && !n->getDrawn() && n->getEnabled()) {
	  DrawQ.push_back(n);
	  n->setGeneration(generation+1);
	}
      }
      /*
      if (node) {
	nodes = node->getChildren();
	if (nodes) DrawQ.append(*nodes);
      }
      */
      continue;
    } else {
      idleFunc(0);
      TraverseQ.erase(TraverseQ.begin(),TraverseQ.end());
      DrawQ.erase(DrawQ.begin(),DrawQ.end());
      //            fprintf(stderr, "finished frame\n");
      break;
    }
    drawNode(current);
    current->setDrawn(1);
    drawLinks(current);
    insertSorted(&DrawnQ, current);
    insertSorted(&TraverseQ, current);
    gettimeofday(&now, NULL);
    elapsed.tv_sec = now.tv_sec - start.tv_sec;
    if (now.tv_usec < start.tv_usec) {
      // manual carry...
      elapsed.tv_usec = now.tv_usec + (1000000-start.tv_usec);
      elapsed.tv_sec--; 
    } else {
      elapsed.tv_usec = now.tv_usec - start.tv_usec;
    }
  }
  //    fprintf(stderr, "drew %d nodes, %d links %s\n", nodesdrawn, linksdrawn, 	    idleframe ? "idle" : "");

  frameEnd();
  return;
}

//----------------------------------------------------------------------------
//                     void HypViewer::drawPickFrame()                     
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::drawPickFrame()
{
  struct timeval now, start, allotted, elapsed;
  gettimeofday(&now, NULL);
  start = now;
  allotted = hd->picktime;
  elapsed.tv_sec = 0; elapsed.tv_usec = 0;
  picknodesdrawn = 0; picklinksdrawn = 0;
  int index = 0;
  HypNode *current;
  int length = DrawnQ.size();
  frameBegin();
  while ( index < length && 
	  (elapsed.tv_sec < allotted.tv_sec ||
	  (elapsed.tv_sec == allotted.tv_sec && 
	  elapsed.tv_usec < allotted.tv_usec))) {
    current = DrawnQ[index];
    drawNode(current);
    drawLinks(current);
    gettimeofday(&now, NULL);
    elapsed.tv_sec = now.tv_sec - start.tv_sec;
    elapsed.tv_usec = now.tv_usec - start.tv_usec;
    index++;
  }
  //  fprintf(stderr, "drew %d picknodes, %d picklinks\n", picknodesdrawn, picklinksdrawn);
  return;
}

//----------------------------------------------------------------------------
//   void HypViewer::insertSorted(HypNodeDistArray *Q, HypNode *current)   
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::insertSorted(HypNodeDistArray *Q, HypNode *current)
{
  int index;
  HypNode *n, *m;
  Q->push_back(current);
  index = (*Q).size();
  index--;
  while (index > 0) {
    n = (*Q)[index];
    m = (*Q)[index-1];
    if (m->compareDist(n) == -1) {
      //      Q->swap(index-1,index);
      swap((*Q)[index-1],(*Q)[index]);
      index--;
    } else
      break;
  }
  return;
}

//----------------------------------------------------------------------------
//                  void HypViewer::drawLinks(HypNode *n)                  
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::drawLinks(HypNode *n)
{
  int i;
  HypLink *l;
  HypNode *m;
  if (! n->getEnabled()) return;
  if (n->getGeneration() < hd->generationLinkLimit-1) {
    glLineWidth(2.0);
    l = n->getParentLink();
    if (l && !l->getDrawn()) drawLink(l);
    for(i = 0; i < n->getChildCount(); i++) {
      l = n->getChildLink(i);
      drawLink(l);
    }
  }

  glLineWidth(1.0);
  for(i = 0; i < n->getOutgoingCount(); i++) {
    l = n->getOutgoing(i);
    m = l->getChild();
    if (m && (m->getGeneration() < hd->generationLinkLimit-1
              || n->getGeneration() < hd->generationLinkLimit-1))
      drawLink(l);
  }
  
  glLineWidth(1.0);
  for(i = 0; i < n->getIncomingCount(); i++) {
    l = n->getIncoming(i);
    m = l->getParent();
    if (m && (m->getGeneration() < hd->generationLinkLimit-1
              || n->getGeneration() < hd->generationLinkLimit-1))
      drawLink(l);
  }
  return;
}

//----------------------------------------------------------------------------
//                   void HypViewer::drawNode(HypNode *n)                  
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::drawNode(HypNode *n)
{
  if (!n ||  ! n->getEnabled())
    return;
  double        xs;
  double        xdist;
  double        curdist;
  unsigned int  name = n->getIndex();
  glLoadName(name);
  //  fprintf(stderr, "drew %d\n", name);
  glPushMatrix();
  HypTransform  T = n->getC();
  double        glm[16];
  memcpy(&glm[0],T.T[0],sizeof(double)*4);
  memcpy(&glm[4],T.T[1],sizeof(double)*4);
  memcpy(&glm[8],T.T[2],sizeof(double)*4);
  memcpy(&glm[12],T.T[3],sizeof(double)*4);
  //printf("glm\n"); for(int i=0;i<16;i++){printf("  %f", glm[i]); if(i%4==3)printf("\n");}

  // glMultMatrixd((double*)T.T);
  glMultMatrixd(glm);
  
  if (!pick) {
    HypTransform  U;
    HypPoint      v, v1, v2, xp;
    U = T.concat(worldxform);
    xp = U.transformPoint(origin);
    // don't normalize xp!!!
    xp.x += HV_LEAFSIZE;
    xp.z += HV_LEAFSIZE+.02;
    v1 = W2S.transformPoint(xp);
    v1.normalizeEuc();
    xp.x -= 2*HV_LEAFSIZE;
    v2 = W2S.transformPoint(xp);
    v2.normalizeEuc();
    xs = v1.distanceEuc(v2);
    if (xs < 0.0) fprintf(stderr, "distance returned negative...\n");
    xs = fabs(xs);
    curdist = thecenter->getDistance();
    v = T.transformPoint(origin);
    xdist = v.distanceHyp(frameorigin);
    if (xs > largest && xs < 100.0 && xdist <= curdist) {
      if (hd->bCenterLargest)
	setCurrentCenter(n);
      //fprintf(stderr, "NEW CENTER: %d %g\n", name, xs);
      largest = xs;
    }
    n->setSize(xs);
    n->setDistance(xdist);
    if (n->getGeneration() < hd->generationNodeLimit) {
      // draw label
      if (hd->labels) {
	if (xs > labelscaledsize) {
	  if (labeltoright)
	    n->setLabelPos(v1.x, v1.y, -v1.z+.0);
	  else 
	    n->setLabelPos(v2.x, v2.y, -v2.z+.0);
	  drawLabel(n);
	}
      }
      nodesdrawn++;
    }
  } else {
    picknodesdrawn++;
  }

  if (hd->bCenterShow && n->getGeneration() == 0) {
    //    fprintf(stderr, "hilite %d\n", name);
    glColor3f(1,0,0);
  } else if (n->getHighlighted()) {
    //    fprintf(stderr, "hilite %d\n", name);
    glColor3f(hd->colorHilite[0],
	      hd->colorHilite[1],
	      hd->colorHilite[2]);
  } else if (n->getSelected()) {
    //    fprintf(stderr, "select %d\n", name);
    glColor3f(hd->colorSelect[0],
	      hd->colorSelect[1],
	      hd->colorSelect[2]);
  } else {
    float *col = hg->getColorNode(n);
    glColor3f(col[0], col[1], col[2]);
  }
  glLineWidth(1.0);
  if (n->getGeneration() < hd->generationNodeLimit) {
    glCallList(HV_CUBEFACES);
    if (!pick && xs > hd->edgesize)
      glCallList(HV_CUBEEDGES);
  }
  glPopMatrix();

  return;
}

//----------------------------------------------------------------------------
//                   void HypViewer::drawLink(HypLink *l)                  
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::drawLink(HypLink *l)
{
  if (!l->getEnabled()) return;
  unsigned int name;
  name = l->getIndex();
  glLoadName(-name-1);
  const float *cFrom, *cTo;
  if (hd->linkPolicy == HV_LINKINHERIT) {
    HypNode *p = l->getParent();
    HypNode *c = l->getChild();
    cFrom = hg->getColorNode(p);
    cTo = hg->getColorNode(c);
  } else if (hd->linkPolicy == HV_LINKCENTRAL) {
    cFrom = hd->colorLinkFrom;
    cTo = hd->colorLinkTo;
  } else { // HV_LINKLOCAL
    cFrom = l->getColorFrom();
    cTo = l->getColorTo();
  }
  glBegin(GL_LINES);
  glColor4f(cFrom[0], cFrom[1], cFrom[2], cFrom[3]);
  glVertex4d(l->start.x, l->start.y, l->start.z, l->start.w);
  glColor4f(cTo[0], cTo[1], cTo[2], cTo[3]);
  glVertex4d(l->end.x, l->end.y, l->end.z, l->end.w);
  glEnd();
  l->setDrawn(1);
  linksdrawn++;
  DrawnLinks.push_back(l);
  return;
}

//----------------------------------------------------------------------------
//                  void HypViewer::drawLabel(HypNode *n)                  
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::drawLabel(HypNode *n)
{
  int len;
  glPolygonMode(GL_FRONT, GL_FILL);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, vp[2] - vp[0], 0, vp[3] - vp[1], -1.0, 1.0);
  HypPoint lab = n->getLabelPos();
  const char *str;
  if (hd->labels == HV_LABELSHORT)
    str = n->getShortLabel().c_str();
  else 
    str = n->getLongLabel().c_str();
  if ( (len = strlen(str)) < 1) return;
  int labw = drawStringWidth(str);
  int charwidth = labw/len;

  // this is strange, but needed to work on weevil. 
  // remove next three lines after upgrade weevil to 6.2
  //  if (n->getHighlighted()) lab.z = 0.0;
  //  glColor3f(hd->colorLabel[0], hd->colorLabel[1], hd->colorLabel[2]);
  //  glBegin(GL_TRIANGLE_STRIP);
  //  glEnd();

  if (n->getHighlighted())
    glColor3f(hd->colorLabel[0], hd->colorLabel[1], hd->colorLabel[2]);
  else 
    glColor3f(hd->colorBack[0], hd->colorBack[1], hd->colorBack[2]);
  double labr, labl;

  if (labeltoright) {
    labl = 0.0;
    labr = labw;
    if ( lab.x > vp[3] )
      return;                   // Entirely off the right side of the viewport.
  } else {
    labl = -labw;
    labr = 0.0;
    if ( lab.x - charwidth < 0.0 )
      return;                   // Entirely off the left side of the viewport.

    if (lab.x+labl < 0.0) {     
      // GL won't display string if beginning is to left of window border:
      // truncate the requisite number of chars from the front
      int truncamt = (int) ((lab.x-labw)/charwidth);
      /* fprintf(stderr,"drawLabel str=%s, len=%d, truncamt=%d, labl=%f, lab.x=%f\n", 
         str, len, truncamt, labl, lab.x); /**/

      // Beware of hugely negative lab.x due to busted transform.
      if ( truncamt < 0 && abs(truncamt) < len && truncamt != 0x80000000 ) {
        //fprintf(stderr,"  str=%s\n", str);
        //const char * ostr = str;
        str += -truncamt;
        labl = -lab.x;
        //fprintf(stderr, "  str=%s\n", str);
      }
      else
        return;
    }
  }
  glBegin(GL_TRIANGLE_STRIP);
  glVertex3d(lab.x+labl-1.0,		lab.y-labdescent,	lab.z);
  glVertex3d(lab.x+labr+1.0,		lab.y-labdescent,	lab.z);
  glVertex3d(lab.x+labl-1.0,		lab.y+labascent,	lab.z);
  glVertex3d(lab.x+labr+1.0,		lab.y+labascent,	lab.z);
  glEnd();
  if (n->getHighlighted())
    glColor3f(hd->colorBack[0], hd->colorBack[1], hd->colorBack[2]);
  else 
    glColor3f(hd->colorLabel[0], hd->colorLabel[1], hd->colorLabel[2]);
  glRasterPos3d(lab.x+labl, lab.y, lab.z+.001);
  drawString(str);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  return;
}

//----------------------------------------------------------------------------
//                       void HypViewer::drawSphere()                      
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::drawSphere()
{
  glCallList(HV_INFSPHERE);
  return;
}

//----------------------------------------------------------------------------
//                        void HypViewer::camSetup()                       
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::camSetup()
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (pick) {
    double xd = pickx;
    double yd = picky;
    //    fprintf(stderr, "pickplace %g %g\n", xd, (GLdouble)(vp[3]-yd));
    //    glGetIntegerv ( GL_VIEWPORT, vp );      /* get viewport size */ 
    gluPickMatrix( xd, (GLdouble)(vp[3]-yd), 3.0, 3.0, vp);
  }
  glMultMatrixd((double*)cx);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMultMatrixd((double*)vx);  
  HypTransform V, S;
  V = viewxform.concat(camxform); 
  // V maps world to [-1..1],[-1..1],[-1..1] */
  S.translateEuc(1., 1., 0.);
  V = V.concat(S); // now maps to [0..2],[0..2],[-1..1]
  S.scale(.5*(vp[2]-vp[0]+1), .5*(vp[3]-vp[1]+1), 1.);
  // now maps to [0..xsize],[0..ysize],[-1..1]
  W2S = V.concat(S); // final world-to-screen matrix
  return;
}


//----------------------------------------------------------------------------
//                       void HypViewer::frameBegin()                      
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::frameBegin()
{
  glDrawBuffer(GL_BACK);
  glDepthFunc(GL_LESS);
  glClear((GLbitfield) (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
  camSetup();
  if (!pick && hd->bSphere)
    drawSphere();
  glMultMatrixd((double*)worldxform.T);
  return;
}

//----------------------------------------------------------------------------
//                     void HypViewer::frameContinue()                     
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::frameContinue()
{
  glDrawBuffer(GL_FRONT);
  glDepthFunc(GL_LESS);
  camSetup();
  glMultMatrixd((double*)worldxform.T);
  return;
}

//----------------------------------------------------------------------------
//                          void HypViewer::idle()                         
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::idle()
{
  idleframe = 1;
  drawFrame();
  idleframe = 0;
  idleCont();
  return;
}

#ifdef HYPGLX

  //--------------------------------------------------------------------------
  //                       int HypViewer::idleCB(void)
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  int HypViewer::idleCB(void)
  {
    thehv->idle();
    return 1;
  }
#endif //HYPGLX
#ifdef WIN32
   //--------------------------------------------------------------------------
  //                       int HypViewer::idleCB(void)
  //..........................................................................
  //
  //--------------------------------------------------------------------------
  int HypViewer::idleCB(void)
  {
    thehv->idle();
    return idleid;
  }


#endif //WIN32
#ifdef HYPGLUT

  //--------------------------------------------------------------------------
  //                       void HypViewer::idleCB(void)                      
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::idleCB(void)
  {
    thehv->idle();
  }

#endif //HYPGLUT

//----------------------------------------------------------------------------
//    void HypViewer::hiliteFuncCB(int x, int y, int shift, int control)   
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::hiliteFuncCB(int x, int y, int shift, int control)
{
  thehv->hiliteFunc(x,y,shift,control);
  return;
}

//----------------------------------------------------------------------------
//     void HypViewer::pickFuncCB(int x, int y, int shift, int control)    
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::pickFuncCB(int x, int y, int shift, int control)
{
  thehv->pickFunc(x,y,shift,control);
  return;
}

//----------------------------------------------------------------------------
//    void HypViewer::transFuncCB(int x, int y, int shift, int control)    
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::transFuncCB(int x, int y, int shift, int control)
{ 
  thehv->transFunc(x,y,shift,control);
  return;
}

//----------------------------------------------------------------------------
//     void HypViewer::rotFuncCB(int x, int y, int shift, int control)     
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::rotFuncCB(int x, int y, int shift, int control)
{
  thehv->rotFunc(x,y,shift,control);
  return;
}

//----------------------------------------------------------------------------
//                void HypViewer::bindCallback(int b, int c)               
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::bindCallback(int b, int c)
{
  void (*fp)(int,int,int,int);

  switch (c) {
    case HV_PICK:        fp = pickFuncCB;      break;
    case HV_HILITE:      fp = hiliteFuncCB;    break;
    case HV_TRANS:       fp = transFuncCB;     break;
    case HV_ROT:         fp = rotFuncCB;       break;
    default:                                   break;
  }

  switch (b) {
    case HV_LEFT_CLICK:      leftClickCB   = fp;    break;
    case HV_MIDDLE_CLICK:    middleClickCB = fp;    break;
    case HV_RIGHT_CLICK:     rightClickCB  = fp;    break;
    case HV_LEFT_DRAG:       leftDragCB    = fp;    break;
    case HV_MIDDLE_DRAG:     middleDragCB  = fp;    break;
    case HV_RIGHT_DRAG:      rightDragCB   = fp;    break;
    case HV_PASSIVE:         passiveCB     = fp;    break;
    default:                                        break;
  }
  
  return;
}

//----------------------------------------------------------------------------
//   void HypViewer::setPickCallback(void (*fp)(const string &,int,int))   
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::setPickCallback(void (*fp)(const string &,int,int))
{
  pickCallback = fp;
  return;
}

//----------------------------------------------------------------------------
//  void HypViewer::setHiliteCallback(void (*fp)(const string &,int,int))  
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::setHiliteCallback(void (*fp)(const string &,int,int))
{
  hiliteCallback = fp;
  return;
}

//----------------------------------------------------------------------------
//  void HypViewer::setFrameEndCallback(void (*fp)(int))  
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::setFrameEndCallback(void (*fp)(int))
{
  frameEndCallback = fp;
  return;
}

//----------------------------------------------------------------------------
//         void HypViewer::mouse(int btn, int state, int x, int y,         
//                               int shift, int control)                   
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::mouse(int btn, int state, int x, int y, 
		      int shift, int control)
{
  button = btn;
  buttonstate = state;

  switch (button) {
    case 0:
      if (leftClickCB)
        (*leftClickCB)(x,y,shift,control);
      break;
    case 1:
      if (middleClickCB)
        (*middleClickCB)(x,y,shift,control);
      break;
    case 2:
      if (rightClickCB)
        (*rightClickCB)(x,y,shift,control);
      break;
    default:
      break;
  }
  
  oldx = x; oldy = y;
  
  return;
}

//----------------------------------------------------------------------------
//       void HypViewer::motion(int x, int y, int shift, int control)      
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::motion(int x, int y, int shift, int control)
{
  if (motionCount < hd->motionCull) {
    motionCount++;
    return;
  }
  else {
    motionCount = 0;
  }

  switch (button) {
    case 0:
      if (leftDragCB)
        (*leftDragCB)(x,y,shift,control);
      break;
      case 1:
        if (middleDragCB)
          (*middleDragCB)(x,y,shift,control);
        break;
    case 2:
      if (rightDragCB)
        (*rightDragCB)(x,y,shift,control);
      break;
    default:
      break;
  }
  
  oldx = x; oldy = y;
  
  return;
}

//----------------------------------------------------------------------------
//      void HypViewer::passive(int x, int y, int shift, int control)      
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::passive(int x, int y, int shift, int control)
{
  if (passiveCount < hd->passiveCull) {
    passiveCount++;
    return;
  }
  else {
    passiveCount = 0;
  }
  if (passiveCB)
    (*passiveCB)(x,y,shift,control);
  
  oldx = x; oldy = y;
  
  return;
} 

//----------------------------------------------------------------------------
//     void HypViewer::hiliteFunc(int x, int y, int shift, int control)    
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::hiliteFunc(int x, int y, int shift, int control)
{
  HypNode *n;
  int picked = doPick(x,y);
#if 0 //#ifdef WIN32
  //TRACE("hiliteFunc %d\n", picked);
  // Kluge around problems with picking nodes on Windows.
  if (picked < 0 && picked != -INT_MAX ) picked = -picked;
#endif
  if (picked != hiliteNode) {
    n = hg->getNodeFromIndex(hiliteNode);
    glDrawBuffer(GL_FRONT);
    glDepthFunc(GL_LEQUAL);
    camSetup();
    glMultMatrixd(wx);
    if (n) {
      n->setHighlighted(0);    
      drawNode(n);
    }
    hiliteNode = picked;
    n = hg->getNodeFromIndex(hiliteNode);
    if (n) {
      n->setHighlighted(1);
      drawNode(n);
    }
    string foo;
    if (n) {
      foo = n->getId();
      if (foo.length() > 0) {
        if (hiliteCallback) {
          (*hiliteCallback)(foo.c_str(),shift,control);
        }
      }
    }
  }
  return;
}

//----------------------------------------------------------------------------
//      void HypViewer::pickFunc(int x, int y, int shift, int control)     
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::pickFunc(int x, int y, int shift, int control)
{
  if (buttonstate == 0) {
    mousemoved = 0;
    idleFunc(0);
    clickx = x;
    clicky = y;
  }
  else {
    double dx = (x-clickx);
    double dy = (y-clicky);
    // window dang well better be > (0,0)! 
    double sdx = (2.0*dx)/hd->winx;
    double sdy = (2.0*dy)/hd->winy;
    //    fprintf(stderr, "total trans %g %g\n", sdx, sdy);
    if (sdx <= .01 && sdx >= -.01 && sdy <= .01 && sdy >= -.01) {
      int picked = doPick(x,y);
#if 0 //#ifdef WIN32
      // Kluge around problems with picking nodes on Windows.
      if (picked < 0 && picked != -INT_MAX ) picked = -picked;
#endif
      if (picked >= 0) {
	// got a node
	HypNode *n = hg->getNodeFromIndex(picked);
        #ifdef HYPGLX
          SetWatchCursor(wid);
        #endif
	if (pickCallback)
          (*pickCallback)(n->getId().c_str(), shift, control);
        #ifdef HYPGLX
          SetDefaultCursor(wid);
        #endif
	idleFunc(1);
      } else if (picked == (-INT_MAX)) {
	// got the background
	if (pickCallback)
          (*pickCallback)("", shift, control);
      } else if (picked < 0) {
	// got an edge
	HypLink *l = hg->getLinkFromIndex(-picked-1);
	if (pickCallback)
          (*pickCallback)(l->getId().c_str(), shift, control);
      }
    }
  }

  #ifdef HYPXT
    XtSetKeyboardFocus(this->wid,this->wid);
  #endif

  return;
}

//----------------------------------------------------------------------------
//             void HypViewer::setSelected(HypNode *n, bool on)
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::setSelected(HypNode *n, bool on)
{
  n->setSelected(on);    
  if (!n->getEnabled())
    return;
  if (!n->getDrawn())
    return;
  glDrawBuffer(GL_FRONT);
  glDepthFunc(GL_LEQUAL);
  camSetup();
  glMultMatrixd(wx);
  drawNode(n);
  insertSorted(&DrawnQ, n);
  return;
}

//----------------------------------------------------------------------------
//  void HypViewer::rotFunc(int x, int y, int /*shift*/, int /*control*/)  
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::rotFunc(int x, int y, int /*shift*/, int /*control*/)
{
  if (buttonstate == 0) {
    idleFunc(0);
    static float Radius = 100.0;
    double dx = (x-oldx);
    double dy = (y-oldy);
    double dr = sqrt(dx*dx + dy*dy);
    double denom = sqrt(Radius*Radius + dr*dr);
    double theta = acos(Radius/denom);
    //    fprintf(stderr, "rot  %g ", theta);
    if (theta > .0001) {
      HypPoint p;
      p.x = dy/dr;   
      p.y = dx/dr;
      p.z = 0.0;
      p.w = 1.0;
      HypTransform T;
      T.rotate(theta, p);
      worldxform = worldxform.concat(T);
      mousemoved = 1;
      redraw();
    }
  } else {
  }
  //    redraw();
    idleFunc(1);
}

//----------------------------------------------------------------------------
// void HypViewer::transFunc(int x, int y, int /*shift*/, int /*control*/) 
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::transFunc(int x, int y, int /*shift*/, int /*control*/)
{
  if (buttonstate == 0) {
    idleFunc(0);
    double dx = (x-oldx);
    double dy = (y-oldy);
    // window dang well better be > (0,0)! 
    double sdx = (2.0*dx)/hd->winx;
    double sdy = (2.0*dy)/hd->winy;
    //    fprintf(stderr, "trans %g %g\n", sdx, sdy);
    if ( sdx > .0001 || sdx < -.0001 || sdy > .0001 || sdy < -.0001) {
      HypTransform T;
      T.translateHyp(sdx, -sdy, 0.0);
      worldxform = worldxform.concat(T);
      mousemoved = 1;
      redraw();
    }
  }
  //    redraw();
  idleFunc(1);
  return;
}


//----------------------------------------------------------------------------
//                   int HypViewer::doPick(int x, int y)                   
//............................................................................
//  
//----------------------------------------------------------------------------
int HypViewer::doPick(int x, int y)
{
  int picked = -INT_MAX;
  pickx = x; picky = y;
  pick = 1;
  GLuint hitbuf[5096];
  /* Set up picking: hit buffer, name stack. */ 
//  fprintf(stderr, "PICKSTART\n");
  glSelectBuffer ( 5096, hitbuf ); 
  glRenderMode ( GL_SELECT ); 
  glInitNames(); 
  glPushName(6555);
  drawPickFrame();
  glFlush();
  GLint hits = glRenderMode(GL_RENDER);
  if (hits > 0) {
    picked = hitbuf[3];
    GLuint zint = hitbuf[1];
    GLdouble xw, yw, zw;
    xw = (double) x;
    yw = (double) y;
    zw = (double) zint*pickrange;
    gluUnProject(xw, yw, zw, 
		 (double*)vx, // modelview
		 (double*)cx, // projection
		 vp, // viewport
		 &(pickpoint.x), &(pickpoint.y), &(pickpoint.z));
    pickpoint.y = -pickpoint.y;
    if (pickpoint.z > .9999 || pickpoint.z < -.9999) pickpoint.z = .001;
    pickpoint.w = 1.0;
  }
  pick = 0;
  return picked;
}

//----------------------------------------------------------------------------
//                int HypViewer::gotoPickPoint(int animate)                
//............................................................................
//  
//----------------------------------------------------------------------------
int HypViewer::gotoPickPoint(int animate)
{
  #ifdef HYPGLX
    SetWatchCursor(wid);
  #endif
    
  if (animate == HV_ANIMATE) {
    double hypdist = origin.distanceHyp(pickpoint);
    HypTransform U2W;
    U2W.copy(worldxform);
    doAnimation(pickpoint, iq, hypdist, 0.0, U2W);
  } else {
    HypTransform T;
    T.translateOriginHyp(pickpoint);
    T.invert();
    worldxform = worldxform.concat(T);
    redraw();
  }

  #ifdef HYPGLX
    SetDefaultCursor(wid);
  #endif
    
  return 1;
}

//----------------------------------------------------------------------------
//             int HypViewer::gotoNode(HypNode *n, int animate)            
//............................................................................
//  
//----------------------------------------------------------------------------
int HypViewer::gotoNode(HypNode *n, int animate)
{
  if (!n)
    return 0;
  HypPoint v,p,q,rr;
  HypTransform U2W, W2N, T, TI, N2WT, W2NT, U2NT, N2UT, H, R, ID;
  HypQuat rq, sq;
  double ds;

  #ifdef HYPGLX
    SetWatchCursor(wid);
  #endif
    
  W2N = n->getC();
  U2W.copy(worldxform);
  H = W2N.concat(U2W);
  v = H.transformPoint(origin);
  v.normalizeEuc();
  double len = v.length();
  if (len > 1) {
    fprintf(stderr, "Floating point error:  x %g y %g z %g w %g\n", 
	    v.x, v.y, v.z, v.w); 
    v.mult(1./len);
    v.x -= .01; v.y -= .01; v.z -= .01;
    fprintf(stderr, "Normalizing to:  x %g y %g z %g w %g\n", 
	    v.x, v.y, v.z, v.w); 
  }
  N2WT.translateOriginHyp(v);
  W2NT.copy(N2WT);
  W2NT.invert();
  U2NT = U2W.concat(W2NT);
  N2UT.copy(U2NT);
  N2UT.invert();
  ID.identity();
  if (W2N.T != ID.T) {
    // rotate
    // W2N contains both trans & rot, U2NT is just trans
    H = W2N.concat(U2NT);
    q = H.transformPoint(origin);
    T.translateOriginHyp(q);
    // T has just translational component from U to N
    TI.copy(T);
    TI.invert();
    H = H.concat(T);
    // H has just the rotational component of the cumulative xform
    p = H.transformPoint(xaxis);
    p.normalizeEuc();
    q = H.transformPoint(origin);
    q.normalizeEuc();
    rr = q.sub(p);
    R.rotateBetween(rr,xaxis);
    rq.fromTransform(R);
    ds = rq.fixSign();
  }
  if (animate == HV_JUMP) {
    worldxform = U2NT.concat(R);
    redraw();
    #ifdef HYPGLX
      SetDefaultCursor(wid);
    #endif
    return 1;
  }
  else {
    if (animate == HV_ANIMATE) {
      double hypdist = origin.distanceHyp(v);
      doAnimation(v, rq, hypdist, ds, U2W);
    }
    #ifdef HYPGLX
      SetDefaultCursor(wid);
    #endif
  }
  
  return 1;
}

//----------------------------------------------------------------------------
//           void HypViewer::doAnimation(HypPoint v, HypQuat rq,           
//                                       double transdist, double rotdist,  
//                                       HypTransform U2W)                 
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::doAnimation(HypPoint v, HypQuat rq, 
			    double transdist, double rotdist, 
			    HypTransform U2W)
{
  HypPoint p;
  float incr = 1.0;
  HypTransform TI, S;
  HypQuat sq;
  float k;

  if ((transdist > .0001) || (rotdist > .0001)) {
    hd->tossEvents = 1;
    for (k = hd->gotoStepSize; k <= 1.01; k+= hd->gotoStepSize) { 
      if (transdist > .0001) 
	incr = tanh(transdist*k)/tanh(transdist);
      p.x = incr*v.x; 
      p.y = incr*v.y; 
      p.z = incr*v.z; 
      p.w = 1;
      if (rotdist > .0001) {
	sq = rq.slerp(iq, k);
	S = sq.toTransform();
      }
      TI.translateOriginHyp(p);
      TI.invert();
      worldxform = U2W.concat(TI);
      if (rotdist > .0001) {
	worldxform = worldxform.concat(S);
      }
      drawFrame();
    }
  }
  return;
}

//----------------------------------------------------------------------------
//                void HypViewer::setGraphCenter(int cindex)               
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::setGraphCenter(int cindex)
{
  HypNode *c;
  worldxform.copy(identxform);
  if (cindex < 0) {
    c = hg->getNodeFromIndex(hd->centerindex);
  } else {
    c = hg->getNodeFromIndex(cindex);
    HypTransform t = c->getC();
    worldxform = worldxform.concat(t);
  }
  return;
}

//----------------------------------------------------------------------------
//            void HypViewer::setGraphPosition(HypTransform pos)           
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::setGraphPosition(HypTransform pos)
{
  worldxform.copy(identxform);
  worldxform = worldxform.concat(pos);
  return;
}

//----------------------------------------------------------------------------
//                 void HypViewer::setLabelToRight(bool on)                 
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::setLabelToRight(bool on)
{
  labeltoright = on;
}

//----------------------------------------------------------------------------
//                     void HypViewer::resetLabelSize()                    
//............................................................................
//  
//----------------------------------------------------------------------------
void HypViewer::resetLabelSize()
{
  // labelsize is measured in x direction
  float scaledvp;
  float aspectratio = vp[2]/((float)vp[3]);
  if (hd->keepAspect && aspectratio > 1.0) {
    scaledvp = vp[2]/aspectratio;
  } else {
    scaledvp = vp[2];
  }
  labelscaledsize = hd->labelsize*(scaledvp/1000.0);
  //printf("resetLabelSize vp=[%d %d %d %d], labelscaledsize=%g\n", vp[0], vp[1], vp[2], vp[3], labelscaledsize );

}


//----------------------------------------------------------------------------
//       int HypViewer::flashLink(HypNode *fromnode, HypNode *tonode)      
//............................................................................
//  
//----------------------------------------------------------------------------
int HypViewer::flashLink(HypNode *fromnode, HypNode *tonode)
{

  HypLink *link = NULL;
  link = new HypLink(fromnode->getId().c_str(), tonode->getId().c_str());
  link->setEnabled(1);
  link->setParent(fromnode);
  link->setChild(tonode);
  link->layout();
  float *flashcol = hd->colorSelect;
  link->setColorFrom(flashcol[0], flashcol[1], flashcol[2], flashcol[3]);
  link->setColorTo(flashcol[0], flashcol[1], flashcol[2], flashcol[3]);
  drawLink(link);
  return 1;
}


// window system specific code

//  glx specific code

#ifdef HYPGLX

  //--------------------------------------------------------------------------
  //                     void HypViewer::drawStringInit()                    
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::drawStringInit()
  {
# ifndef FONTHACK
    Font id;
    
    unsigned int first, last;
    if (!hd->labelfont) return;
    fontInfo = XLoadQueryFont(dpy, hd->labelfont);
    if (fontInfo) {
      id = fontInfo->fid;
      first = fontInfo->min_char_or_byte2;
      last = fontInfo->max_char_or_byte2;
      //    glXMakeCurrent(dpy, win, cxt);
      GLwDrawingAreaMakeCurrent(wid, cxt);
      /* reuse previous display lists */
      if (!labelbase)  labelbase = glGenLists(last+1);
      if (labelbase) {
        glXUseXFont(id, first, last-first+1, labelbase+first);
        labascent = fontInfo->ascent;
        labdescent = fontInfo->descent;
        return;
      }
    }
    hd->labels = 0;
    free(hd->labelfont);
    hd->labelfont = NULL;

# else // FONTHACK - glXUseXFont is busted on FreeBSD 4.9.  Use a fixed bitmap font instead.
#  include "rasters.cpp"
    GLuint i;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLwDrawingAreaMakeCurrent(wid, cxt);
    /* reuse previous display lists */
    if (!labelbase)  labelbase = glGenLists(128);
    if (labelbase) {
      for (i = 32; i < 127; i++) {
	  glNewList(i+labelbase, GL_COMPILE);
	  glBitmap(8, 13, 0.0, 2.0, 9.0, 0.0, rasters[i-32]);
	  glEndList();
      }
      labascent = 13;
      labdescent = 0;
    }
# endif // FONTHACK
    return;
  }

  //--------------------------------------------------------------------------
  //             int HypViewer::drawStringWidth(const string & s)            
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  int HypViewer::drawStringWidth(const string & s)
  {
# ifndef FONTHACK
    return XTextWidth(fontInfo, s.c_str(), s.length());
# else // FONTHACK - glXUseXFont is busted on FreeBSD 4.9.  Use a fixed bitmap font instead.
    return 8 * s.length();
# endif // FONTHACK
  }

  //--------------------------------------------------------------------------
  //               void HypViewer::drawString(const string & s)              
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::drawString(const string & s)
  {
    //  int len = strlen(s);
    glListBase(labelbase);
    glCallLists(s.length(), GL_UNSIGNED_BYTE, s.c_str());
    return;
  }


  void HypViewer::framePrepare() {
    //  glXMakeCurrent(dpy, win, cxt);
    GLwDrawingAreaMakeCurrent(wid, cxt);
  }

  //--------------------------------------------------------------------------
  //                        void HypViewer::frameEnd()                       
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::frameEnd()
  {
    //  if (!idleframe) glXSwapBuffers(dpy,win);
    //  if (!idleframe) GLwDrawingAreaSwapBuffers(wid);
    if (!idleframe)
      GLwDrawingAreaSwapBuffers(wid);
    return;
  }

  //--------------------------------------------------------------------------
  //                         void HypViewer::redraw()                        
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::redraw()
  {
    drawFrame();
    return;
  }

  //--------------------------------------------------------------------------
  //                  void HypViewer::reshape(int w, int h)                  
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::reshape(int w, int h)
  {
    GLwDrawingAreaMakeCurrent(wid, cxt);
    glXWaitX();
    glViewport(0,0,w,h);
    glScissor(0,0,w,h);
    vp[2] = w;
    vp[3] = h;
    hd->winx = w;
    hd->winy = h;
    // I'm not sure why we have to do this twice, but doesn't work just once
    // TMM 1/18/98
    camInit();
    drawFrame();  
    camInit();
    drawFrame();
    return;
  }

  //--------------------------------------------------------------------------
  //                    void HypViewer::idleFunc(bool on)                    
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::idleFunc(bool on)
  {
    //        fprintf(stderr, "idle %d\n", on);
    if (on && !idleid)
      idleid = XtAppAddWorkProc(XtWidgetToApplicationContext(wid), 
                                (XtWorkProc)idleCB, NULL);
    else if (!on && idleid) {
      //    XtRemoveWorkProc(idleid);
      idleid = 0;
    }
    return;
  }

  //--------------------------------------------------------------------------
  //                        void HypViewer::idleCont()                     
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::idleCont()
  {
    if (idleid)
      idleid = XtAppAddWorkProc(XtWidgetToApplicationContext(wid), 
                                (XtWorkProc)idleCB, NULL);
    return;
  }

#endif  // HYPGLX

//  glut specific code

#ifdef HYPGLUT

  //--------------------------------------------------------------------------
  //                     void HypViewer::drawStringInit()                    
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::drawStringInit()
  {
# ifdef PyHack // Use a fixed bitmap font instead of the GLUT fonts.
#  include "rasters.cpp"
    GLuint i;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /* reuse previous display lists */
    if (!labelbase)  labelbase = glGenLists(128);
    if (labelbase) {
      for (i = 32; i < 127; i++) {
	  glNewList(i+labelbase, GL_COMPILE);
	  glBitmap(8, 13, 0.0, 2.0, 9.0, 0.0, rasters[i-32]);
	  glEndList();
      }
      labascent = 13;
      labdescent = 0;
    }
# endif // PyHack

    labascent = 13;
    labdescent = 0;
    return;
  }

  //--------------------------------------------------------------------------
  //               void HypViewer::drawString(const string & s)              
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::drawString(const string & s)
  {
# ifndef PyHack
    if (!s.length())
      return;
    for (int i = 0; i < s.length(); i++) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, s[i]);
    }
# else // PyHack
    glListBase(labelbase);
    glCallLists(s.length(), GL_UNSIGNED_BYTE, s.c_str());
# endif // PyHack
    return;
  }

  //--------------------------------------------------------------------------
  //             int HypViewer::drawStringWidth(const string & s)            
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  int HypViewer::drawStringWidth(const string & s)
  {
    return 8 * s.length();
  }

  void HypViewer::framePrepare()
  {
  }

  //--------------------------------------------------------------------------
  //                        void HypViewer::frameEnd()                       
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::frameEnd()
  {
#ifndef PyHack
    if (!pick && !idleframe)
      glutSwapBuffers();
#else
    // No need to drag all of wxGLCanvas in here.  Call a simple function above.
    (*frameEndCallback)(0);
#endif
    return;
  }

  //--------------------------------------------------------------------------
  //                     void HypViewer::idleFunc(int on)                    
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::idleFunc(bool on)
  {
#ifndef PyHack
    if (on) 
      glutIdleFunc(idleCB);
    else 
      glutIdleFunc(NULL);
    return;
#endif
  }

  void HypViewer::idleCont() {;}

  //--------------------------------------------------------------------------
  //                         void HypViewer::redraw()                        
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::redraw()
  {
#ifndef PyHack
    glutPostRedisplay();
#else
    drawFrame();
#endif
    return;
  }

  //--------------------------------------------------------------------------
  //                  void HypViewer::reshape(int w, int h)                  
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::reshape(int w, int h)
  {
    glViewport(0,0,w,h);
    vp[2] = w;
    vp[3] = h;
    // I'm not sure why we have to do this twice, but doesn't work just once
    // TMM 1/18/98
    camInit();
#ifndef PyHack
    glutPostRedisplay();  
    camInit();
    glutPostRedisplay();
/* #else
    glXSwapBuffers();
    camInit();
    glXSwapBuffers();
*/
#endif
  }

#endif  // HYPGLUT

// window system specific code
//  WIN32 specific code
#ifdef WIN32

void HypViewer::drawStringInit() {
     LOGFONT     lf;
     HFONT       hFont,hOldFont;

     if (!hd->labelfont) return;

     // An hDC and an hRC have already been created.
     wglMakeCurrent( wid, cxt );

     // Let's create a TrueType font to display.
     memset(&lf,0,sizeof(LOGFONT));
     lf.lfHeight               =   hd->labelfontsize;
     lf.lfWeight               =   FW_NORMAL ;
     lf.lfCharSet              =   ANSI_CHARSET ;
     lf.lfOutPrecision         =   OUT_TT_ONLY_PRECIS;
     lf.lfClipPrecision        =   CLIP_DEFAULT_PRECIS ;
     lf.lfQuality              =   PROOF_QUALITY ;
     lf.lfPitchAndFamily       =   VARIABLE_PITCH | TMPF_TRUETYPE;
     lstrcpy (lf.lfFaceName, hd->labelfont) ;//   hd->labelfont
     hFont = CreateFontIndirect(&lf);
     hOldFont = (HFONT) SelectObject(wid,hFont);

     labascent = 13;
     labdescent = 0;

     TEXTMETRIC *tm;
     tm = new TEXTMETRIC;
     bool r = GetTextMetrics(wid,tm);

     labascent = tm->tmAscent;
     labdescent = tm->tmDescent;
     
     delete tm;

     memset(&iCharWidths,0,sizeof(iCharWidths));
     r = GetCharWidth(wid,0,255,iCharWidths);

     if (!labelbase)  labelbase = glGenLists(256);
     if (labelbase) {
         // Create a set of display lists based on the TT font we selected
          wglUseFontBitmaps(wid,0,256,labelbase);
     }

     DeleteObject(SelectObject(wid,hOldFont));

     return;

  }

  //--------------------------------------------------------------------------
  //               void HypViewer::drawString(const string & s)              
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::drawString(const string & s)
  {
    //  int len = strlen(s);
    glListBase(labelbase);
    glCallLists(s.length(), GL_UNSIGNED_BYTE, s.c_str());
    return;
  }

  //--------------------------------------------------------------------------
  //             int HypViewer::drawStringWidth(const string & s)
  //..........................................................................
  //
  //--------------------------------------------------------------------------
  int HypViewer::drawStringWidth(const string & s)
  {
//      SIZE sz;
//      GetTextExtentPoint32(wid, s.c_str(), s.length(), &sz);
//     return sz.cx;
     int iWidth = 0;
     for (int i=0; i<s.length(); i++)
        iWidth += iCharWidths[s[i]];
     return iWidth;
  }

void HypViewer::framePrepare() {
  wglMakeCurrent(wid, cxt);
}

void HypViewer::frameEnd() {
  if (!pick && !idleframe)  SwapBuffers(wid);
  idleCont();
}

  //--------------------------------------------------------------------------
  //                    void HypViewer::idleFunc(bool on)                    
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypViewer::idleFunc(bool on)
  {
  if (on) {
     idleid = 1;
   } else {
     idleid = 0;
   }
}

void HypViewer::idleCont() {
/*
   if (idleframe == 1) {
     OutputDebugString("idle cont\n");
     InvalidateRect(win,NULL,FALSE);
   } else {
      ValidateRect(win,NULL);
   }
   */
}

void HypViewer::redraw() {
  drawFrame();
}

void HypViewer::reshape(int w, int h) {
  wglMakeCurrent(wid,cxt);
  glViewport(0,0,w,h);
  glScissor(0,0,w,h);
  vp[2] = w;
  vp[3] = h;
  // I'm not sure why we have to do this twice, but doesn't work just once
  // TMM 1/18/98
 // camInit();
 // drawFrame();
 // camInit();
 // drawFrame();
}
#endif  //WIN32

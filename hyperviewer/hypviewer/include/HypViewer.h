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
#ifndef HYPVIEWER_H
#define HYPVIEWER_H

#ifdef WIN32
#include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>

#include "HypGraph.h"
#include "HypTransform.h"
#include "HypQuat.h"

#ifdef HYPGLUT
#include "GL/glut.h"
#endif

#ifdef HYPGLX
#include "GL/glx.h"
#include "X11/Xlib.h"
#include "Xm/Xm.h"
#endif

class HypViewer {

public:

#ifdef WIN32
  HypViewer(HypData *d, HDC w, HWND hwin);
  void afterRealize(HGLRC cx, int w, int h);  //(GLXContext cx);
  HDC getWidget() {return wid;}
  int idleCB(void);
#elif HYPGLX

  //--------------------------------------------------------------------------
  //                     HypViewer(HypData *d, Widget w)
  //..........................................................................
  //  constructor
  //  When using GLX, we want to render in a Widget.
  //--------------------------------------------------------------------------
  HypViewer(HypData *d, Widget w);

  //--------------------------------------------------------------------------
  //                    void afterRealize(GLXContext cx)
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void afterRealize(GLXContext cx);

  //--------------------------------------------------------------------------
  //                            Widget getWidget()                           
  //..........................................................................
  //  Returns the Widget being used for rendering.
  //--------------------------------------------------------------------------
  Widget getWidget()
  {
    return wid;
  }
#ifdef XPMSNAP
  //--------------------------------------------------------------------------
  //                bool XpmSnapshot(const string & fileName)
  //..........................................................................
  //  Takes a snapshot of the widget and stores it in the file named by
  //  fileName.  The output format is XPM.  Returns true on success, else
  //  returns false.
  //--------------------------------------------------------------------------
  bool XpmSnapshot(const string & fileName);
#endif

  //--------------------------------------------------------------------------
  //                        static int idleCB(void)
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  static int idleCB(void);
  
#else

  //--------------------------------------------------------------------------
  //                          HypViewer(HypData *d)                         
  //..........................................................................
  //  constructor
  //--------------------------------------------------------------------------
  HypViewer(HypData *d);

  //--------------------------------------------------------------------------
  //                           void afterRealize()                         
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void afterRealize();

  //--------------------------------------------------------------------------
  //                        static void idleCB(void)
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  static void idleCB(void);

#endif

  ~HypViewer();

  void drawFrame();
  void setGraph(HypGraph *h);
  void newLayout();
  int gotoPickPoint(int animate);
  int gotoNode(HypNode *n, int animate);
  void setGraphCenter(int cindex);
  void setGraphPosition(HypTransform pos);
  void resetLabelSize();
  void idleFunc(bool on);
  void idle();

  void reshape(int w, int h);
  void mouse(int btn, int state, int x, int y, int shift, int control);
  void motion(int x, int y, int shift, int control);
  void passive(int x, int y, int shift, int control);

  void bindCallback(int b, int c);
  void newBackground();
  void newLabelFont();
  void newSphereColor();
  void setPickCallback(void (*fp)(const string &,int,int));
  void setHiliteCallback(void (*fp)(const string &,int,int));
  void setSelected(HypNode *n, bool on);

  //--------------------------------------------------------------------------
  //                    void setCurrentCenter(HypNode *n)                    
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setCurrentCenter(HypNode *n)
  {
    thecenter = n;
    return;
  }
  
  //--------------------------------------------------------------------------
  //                       HypNode *getCurrentCenter()                       
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  HypNode *getCurrentCenter()
  {
    return thecenter;
  }
  
  int flashLink(HypNode *fromnode, HypNode *tonode);
  void setLabelToRight(bool on);
  void camInit();

private:

  void drawSphere();
  void drawNode(HypNode *n);
  void drawLinks(HypNode *n);
  void drawLink(HypLink *l);
  void drawLabel(HypNode *n);
  int doPick(int x, int y);
  void glInit();
  void camSetup();
  void insertSorted(HypNodeDistArray *Q, HypNode *current);
  void drawPickFrame();
  void doAnimation(HypPoint v, HypQuat rq, double transdist, double rotdist,
		   HypTransform U2W);

  int hiliteNode;
  int pick;
  int pickx;
  int picky;
  HypTransform worldxform;
  HypTransform camxform;
  HypTransform viewxform;
  HypTransform identxform;
  double *wx;
  double *cx;
  double *vx;
  int oldx;
  int oldy;
  int clickx;
  int clicky;
  int button;
  int buttonstate;
  int mousemoved;
  GLint vp[4];
  double largest;
  int  currlev;
  HypNode *thecenter;
  // for drawframe performance stats
  int idleframe;
  struct timeval framestart;
  int nodesdrawn;
  int linksdrawn;
  int picknodesdrawn;
  int picklinksdrawn;
  int passiveCount;
  int motionCount;
  int labelbase;
  int labascent;
  int labdescent;
  float labelscaledsize;
  bool labeltoright;


  HypNodeDistArray DrawQ;
  HypNodeDistArray TraverseQ;
  HypNodeDistArray DrawnQ;
  HypLinkArray DrawnLinks;
  HypPoint frameorigin;

  HypPoint origin;
  HypPoint xaxis;
  HypQuat iq;

  GLUquadricObj *qobj;
  HypData *hd;
  HypGraph *hg;
  HypTransform W2S;

  void (*leftClickCB)(int,int,int,int);     
  void (*middleClickCB)(int,int,int,int);     
  void (*rightClickCB)(int,int,int,int);     
  void (*leftDragCB)(int,int,int,int);     
  void (*middleDragCB)(int,int,int,int);     
  void (*rightDragCB)(int,int,int,int);     
  void (*passiveCB)(int,int,int,int);     

  void hiliteFunc(int x, int y, int shift, int control);
  void pickFunc(int x, int y, int shift, int control);
  void rotFunc(int x, int y, int shift, int control);
  void transFunc(int x, int y, int shift, int control);

  static void hiliteFuncCB(int x, int y, int shift, int control);
  static void pickFuncCB(int x, int y, int shift, int control);
  static void transFuncCB(int x, int y, int shift, int control);
  static void rotFuncCB(int x, int y, int shift, int control);

  float pickrange;
  HypPoint pickpoint;

  void (*pickCallback)(const string &,int,int);
  void (*hiliteCallback)(const string &,int,int);

  void frameBegin();
  void frameContinue();

  // window system specific code

// WIN32 specific storage
#ifdef WIN32
  HWND win;
  HDC wid;
  HGLRC cxt;
  int idleid;
  int iCharWidths[256];
#endif

// GLX specific storage
#ifdef HYPGLX
  XFontStruct *fontInfo;
  Display     *dpy;
  Window       win;
  GLXContext   cxt;
  Widget       wid;
  int          idleid;
#endif

// functions to instantiate
  void drawString(const string & s);
  void drawStringInit();
  int  drawStringWidth(const string & s);
  void sphereInit();
  void framePrepare();
  void frameEnd();
  void idleCont();
  void redraw();
  void drawChar(char c);
};

#endif

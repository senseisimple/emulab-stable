//
// EMULAB-COPYRIGHT
// Copyright (c) 2004 University of Utah and the Flux Group.
// All rights reserved.
//
// Permission to use, copy, modify and distribute this software is hereby
// granted provided that (1) source code retains these copyright, permission,
// and disclaimer notices, and (2) redistributions including binaries
// reproduce the notices in supporting documentation.
//
// THE UNIVERSITY OF UTAH ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
// CONDITION.  THE UNIVERSITY OF UTAH DISCLAIMS ANY LIABILITY OF ANY KIND
// FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
//

// hvmain.cpp - hypview for SWIG into _hv.so, based on hypviewer/examples/glut/main.cpp .

#include <stdio.h>
#include <memory>
#include <string>
#include <iostream>
NAMESPACEHACK

#ifndef WIN32
#include "wx/setup.h"
#include "wx/glcanvas.h"
#endif

extern "C" {
#include <stdlib.h>

#if 0
#include <GL/glut.h>
#endif

#ifndef WIN32
#include <ieeefp.h>
#endif

#ifdef HYPFREE
#include <floatingpoint.h>
#endif
#include <errno.h>
}

#ifdef HYPIRIX
#include <fstream.h>
#else
#include <fstream>
#endif

#include "HypView.h"
#include "HypData.h"
#include "HypNode.h"

//char prevsel[1024];
string prevsel;
#ifndef WIN32
auto_ptr<HypView> hv;  // We own this; make sure the pointed-to HypView will be deleted.
wxGLCanvas *glcanvas;  // We don't own this, so don't wrap it in an auto_ptr.
#else
HypView *hv = NULL;    // VC++6 doesn't support the reset operation to change an auto_ptr.
#endif

char const *getSelected(){
  return prevsel.c_str();
}

// I guess I don't know how to handle a "string" returned by the SWIG Python wrapper...
char * getGraphCenter(){
  return const_cast<char *>(hv->getGraphCenter().c_str());
}

void selectCB(const string & id, int shift, int control) {
  //printf("selectCB id=%s, prevsel=%s\n", id.c_str(), prevsel.c_str());
  //cerr << "selectCB id=" << id.c_str() << ", prevsel=" << prevsel << endl;
  hv->setSelected(id, 1);
  hv->setSelected(prevsel, 0);
  hv->gotoNode(id, HV_ANIMATE);
  hv->drawFrame();
  hv->idle(1);
  prevsel = id;
}

void frameEndCB(int) {
#ifndef WIN32
  //printf("frameEndCB glcanvas=0x%x\n", glcanvas);
  glcanvas->SwapBuffers();
#endif
}

void display() { hv->drawFrame(); }
void mouse(int b, int s, int x, int y) { hv->mouse(b,s,x,y,0,0); }
void motion(int x, int y) { hv->motion(x,y,0,0); }
void passive(int x, int y) { hv->passive(x,y,0,0); }
void idle() { hv->idle(); }
void reshape(int w, int h) { hv->reshape(w,h); }
#if 0
void visibility(int status) {
  if (status == GLUT_VISIBLE) {
    glutIdleFunc(NULL);
    display();
    glutIdleFunc(idle);
  } else {
    glutIdleFunc(NULL);
  }
}
#endif
#if 0
void keyboard(unsigned char key, int /*x*/, int /*y*/) { 
  static int labels = HV_LABELSHORT;
  static bool sphere = false;
  char buf[128];
  
  switch (key) {
    case 'f':
      cin.getline(buf,128);
      hv->gotoNode(buf, HV_ANIMATE);
      break;
    case 'l':
      if (labels == HV_LABELNONE) labels = HV_LABELSHORT;
      else if (labels == HV_LABELSHORT) labels = HV_LABELLONG;
      else if (labels == HV_LABELLONG) labels = HV_LABELNONE;
      hv->setLabels(labels);
      break;
    case 'r':
      hv->gotoCenterNode(HV_ANIMATE);
      break;
    case 's':
      sphere = !sphere;
      hv->setSphere(sphere);
      break;
    default:
      break;
  }
  display();
  glutIdleFunc(idle);
}
#endif

void PrintAllocations()
{
//  if (hv)
//    delete(hv);
#ifndef WIN32
  hv.reset(NULL);
#else
  hv = NULL;
#endif
  cerr << "HypNode objects not destroyed: " << HypNode::NumObjects() << endl;
  cerr << "HypLink objects not destroyed: " << HypLink::NumObjects() << endl;
  return;
}

#if 0
int main(int argc, char *argv[]) {
#else

#ifndef WIN32
HypView *hvMain(int argc, char *argv[], void * window, int width, int height) {
#else
HypView *hvMain(int argc, char *argv[], int window, int width, int height) {
#endif

#endif
  char *fname;
  if (argc > 1) 
    fname = strdup(argv[1]);
  else 
    fname = strdup("test.lvhist");

#ifndef WIN32
  fpsetmask(0);
#endif

#if 0
  atexit(PrintAllocations);
  
  glutInit(&argc,argv);
  //  glutInitWindowSize(hd->winx, hd->winy);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  ifstream inFile(fname);
  if (!inFile) {
    cerr << "Could not open '" << fname << "': " << strerror(errno) << endl;
    exit(1);
  }
  glutCreateWindow(fname);
#else
  ifstream inFile(fname);
  if (!inFile) {
    cerr << "Could not open '" << fname << "': " << strerror(errno) << endl;
    return 0;
  }
#endif

#ifndef WIN32
  hv.reset(new HypView());
#else
  HWND hWnd = (HWND)window;
  HDC hdc = ::GetDC(hWnd);
  hv = new HypView(hdc, hWnd);
  HGLRC ctx = wglCreateContext(hdc);
  if (!ctx) {
    printf("wglCreateContext Failed\n");
    exit(1);
  }
#endif

  // hv->clearSpanPolicy();
  // hv->addSpanPolicy(HV_SPANBFS);
  
#if 1
  hv->setGraph(inFile);
  inFile.close();
#endif
#ifndef WIN32
  // OK, this is nuts...  The canvas pointer comes through byte-swapped and shifted 4 bits!
  int canvas; char *w = (char *)&window, *c = (char *)&canvas; int *ic = (int *)&canvas;
  c[0] = w[3]; c[3] = w[0]; c[1] = w[2]; c[2] = w[1]; *ic = (*ic >> 4) & 0xfffffff;
  glcanvas = (wxGLCanvas *)canvas;
  //printf("hvmain window=0x%x, canvas=0x%x, width=%d, height=%d\n", window, canvas, width, height);

  hv->setFrameEndCallback(frameEndCB);
  hv->afterRealize();
#else
  //printf("hvmain ctx=0x%x, width=%d, height=%d\n", width, height);
  hv->afterRealize(ctx, width, height);
#endif
  hv->setSphere(1);
  hv->setColorSphere(.3f,.3f,.3f);
  hv->setColorBackground(1,1,1);
#ifndef WIN32
  hv->setLabelFont("-adobe-helvetica-medium-r-normal--12-120-75-75-p-67-iso8859-1");
#endif
  hv->setColorLabel(0,0,0);
  hv->setLabels(HV_LABELLONG);
  hv->setMotionCull(0);
  hv->setPassiveCull(2);
  hv->setGotoStepSize(.0833f);
  hv->setKeepAspect(1);
  hv->setCenterShow(0);

  // mime types

  hv->setGroupKey(0); // start out coloring by mime types

  hv->setColorGroup(0, "image", 1.0, 0.0, 1.0);
  hv->setColorGroup(0, "html", 0, 1, 1);
  hv->setColorGroup(0, "text", .90f, .35f, .05f);
  hv->setColorGroup(0, "image", .42f, 0, .48f);
  hv->setColorGroup(0, "application", .99f, .64f, .25f);
  hv->setColorGroup(0, "audio", .91f, .36f, .57f);
  hv->setColorGroup(0, "host", .375f, .75f, .375f);
  hv->setColorGroup(0, "lan", .375f, .375f, .75f);
  hv->setColorGroup(0, "source", .9f, .9f, .9f);
  hv->setColorGroup(0, "vrml", .09f, 0, 1);
  hv->setColorGroup(0, "invisible", 0, 0, 0);

  // tree position
  hv->setColorGroup(1, "host", 1.0, 1.0, 1.0);
  hv->setColorGroup(1, "mainbranch", 0, 1, 1);
  hv->setColorGroup(1, "orphan", 1.0, 0.0, 1.0);

  //  hv->gotoCenterNode(HV_JUMP);
  struct timeval idletime;
  idletime.tv_sec = 5;
  idletime.tv_usec = 0;
  hv->setIdleFrameTime(idletime);

#if 0
  glutDisplayFunc(display);
#endif
  hv->bindCallback(HV_LEFT_CLICK, HV_PICK);
  hv->bindCallback(HV_PASSIVE, HV_HILITE);
  hv->bindCallback(HV_LEFT_DRAG, HV_TRANS);
  hv->bindCallback(HV_MIDDLE_DRAG, HV_ROT);
  hv->setPickCallback(selectCB);
#if 0
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutPassiveMotionFunc(passive);
  glutVisibilityFunc(visibility);
  glutReshapeFunc(reshape);
  glutMainLoop();

//  delete(hv);
  PrintAllocations();

  return 0;
  
  // free(fname);
#else

#ifndef WIN32
  return hv.get();
#else
  return hv;
#endif

#endif
}

void hvKill(HypView * /*hv*/) {
#ifndef WIN32
  delete hv.get();
#else
  delete hv;
#endif
}

int hvReadFile(char *fname, int width, int height) {
  ifstream inFile(fname);
  if (!inFile) {
    cerr << "Could not open '" << fname << "': " << strerror(errno) << endl;
    return 1;
  }

  hv->setGraph(inFile);
  inFile.close();
#ifndef WIN32
  hv->afterRealize();
#else
  HDC hdc = hv->getWidget();
  HGLRC ctx = wglCreateContext(hdc);
  hv->afterRealize(ctx, width, height);
#endif
  return 0;
}

//
// EMULAB-COPYRIGHT
// Copyright (c) 2004 University of Utah and the Flux Group.
// All rights reserved.
//

// hvmain.cpp - hypview for SWIG into _hv.so, based on hypviewer/examples/glut/main.cpp .

#include <string>
NAMESPACEHACK

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

char prevsel[1024];
HypView *hv;

char *getSelected(){
  return prevsel;
}

void selectCB(const string & id, int shift, int control) {
  hv->setSelected(id, 1);
  hv->setSelected(prevsel, 0);
  hv->gotoNode(id, HV_ANIMATE);
  hv->drawFrame();
  hv->idle(1);
  strcpy(prevsel, id.c_str());
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
  if (hv)
    delete(hv);
  cerr << "HypNode objects not destroyed: " << HypNode::NumObjects() << endl;
  cerr << "HypLink objects not destroyed: " << HypLink::NumObjects() << endl;
  return;
}

#if 0
int main(int argc, char *argv[]) {
#else
HypView *hvmain(int argc, char *argv[]) {
#endif
  char *fname;
  if (argc > 1) 
    fname = strdup(argv[1]);
  else 
    fname = strdup("test.lvhist");

  fpsetmask(0);

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

  hv = new HypView();

  // hv->clearSpanPolicy();
  // hv->addSpanPolicy(HV_SPANBFS);
  
#if 1
  hv->setGraph(inFile);
  inFile.close();
#endif
  hv->afterRealize();
  hv->setSphere(1);
  hv->setColorSphere(.3,.3,.3);
  hv->setColorBackground(1,1,1);
  hv->setLabelFont("-adobe-helvetica-medium-r-normal--12-120-75-75-p-67-iso8859-1");
  hv->setColorLabel(0,0,0);
  hv->setLabels(HV_LABELLONG);
  hv->setMotionCull(0);
  hv->setPassiveCull(2);
  hv->setGotoStepSize(.0833);
  hv->setKeepAspect(1);
  hv->setCenterShow(0);

  // mime types

  hv->setGroupKey(0); // start out coloring by mime types

  hv->setColorGroup(0, "image", 1.0, 0.0, 1.0);
  hv->setColorGroup(0, "html", 0, 1, 1);
  hv->setColorGroup(0, "text", .90, .35, .05);
  hv->setColorGroup(0, "image", .42, 0, .48);
  hv->setColorGroup(0, "application", .99, .64, .25);
  hv->setColorGroup(0, "audio", .91, .36, .57);
  hv->setColorGroup(0, "host", .375, .75, .375);
  hv->setColorGroup(0, "lan", .375, .375, .75);
  hv->setColorGroup(0, "source", .9, .9, .9);
  hv->setColorGroup(0, "vrml", .09, 0, 1);
  hv->setColorGroup(0, "invisible", 0, 0, 0);

  // tree position
  hv->setColorGroup(1, "host", 1.0, 1.0, 1.0);
  hv->setColorGroup(1, "mainbranch", 0, 1, 1);
  hv->setColorGroup(1, "orphan", 1.0, 0.0, 1.0);

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

  delete(hv);
  PrintAllocations();
  
  // free(fname);
#else
  return hv;
#endif
}

int hvReadFile(char *fname) {
  ifstream inFile(fname);
  if (!inFile) {
    cerr << "Could not open '" << fname << "': " << strerror(errno) << endl;
    return 1;
  }

  hv->setGraph(inFile);
  inFile.close();
  hv->afterRealize();
  return 0;
}

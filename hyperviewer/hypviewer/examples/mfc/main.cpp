#include <string>
NAMESPACEHACK

#include <sys/timeb.h>

#include "HypView.h"
#include "HypData.h"
#include "HypNode.h"

#include <fstream>
#include "main.h"

char prevsel[1024];
HypView *hv;

int created = 0;
int gotfile = 0;
int width = 0;
int height = 0;
void draw() { if (created && gotfile) hv->drawFrame(); }
void motion(int x, int y, int shift, int control) {
  if (created && gotfile) hv->motion(x,y,shift,control);
}
void mouse(int b, int s, int x, int y, int shift, int control) {
  if (created && gotfile) hv->mouse(b,s,x,y,shift,control);
}
void passive(int x, int y, int shift, int control) {
  if (created && gotfile) hv->passive(x,y,shift,control);
}
void resize(int w, int h) {
  if (created && gotfile) hv->reshape(w,h);
  width = w;
  height = h;
}
int onidle() {
  int stayidle = 0;
  if (created && gotfile) stayidle = hv->idleCB();
  return stayidle;
}

void selectCB(const string & id, int shift, int control);

void keyboard(unsigned char key) {
  static int gen = 30;
  static int labels = 1;
  static int sphere = 1;
  static int external = 1;
  static int orphan = 1;
  static int media = 1;
  static int group = 0;
  static int incoming = 0;
  static int selincoming = 0;
  static int subincoming = 0;
  static int outgoing = 0;
  static int seloutgoing = 0;
  static int suboutgoing = 0;
  static int rendstyle = 0;
  static int drawlinks = 1;
  static int drawnodes = 1;
  static int showcenter = 0;
  static int negativehide = 0;
  static int selsubtree = 0;
  static int labelright = 0;
  static int backcolor = 0;
  static int keepaspect = 0;
  hv->idle(0);
  if (key >= '0' && key <= '9') {
    gen = key-'0';
    if (rendstyle) 
      hv->setGenerationNodeLimit(gen);
    else 
      hv->setGenerationLinkLimit(gen);
  } else if (key == '*') {
      hv->setGenerationNodeLimit(99);
      hv->setGenerationLinkLimit(99);
  } else if (key == 'l') {
    labels++;
    if (labels >= 3) labels = 0;
    hv->setLabels(labels);
  } else if (key == 'L') {
    labelright = !labelright;
    hv->setLabelToRight(labelright);
  } else if (key == 'b') {
    float labsize = hv->getLabelSize();
    fprintf(stderr, "labsize %f\n", labsize);
    --labsize;
    hv->setLabelSize(labsize);
  } else if (key == 'B') {
    float labsize = hv->getLabelSize();
    fprintf(stderr, "labsize %f\n", labsize);
    ++labsize;
    hv->setLabelSize(labsize);
  } else if (key == 's') {
    sphere = !(sphere);
    hv->setSphere(sphere);
  } else if (key == 'e') {
    external = !(external);
    hv->setDisableGroup(1, "external", external);    
  } else if (key == 'o') {
    orphan = !(orphan);
    hv->setDisableGroup(1, "orphan", orphan);    
  } else if (key == 'i') {
    incoming = !(incoming);
    hv->setDrawBackTo("HV_TOPNODE", incoming, 1);
  } else if (key == 'j') {
    selincoming = !(selincoming);
    hv->setDrawBackTo(prevsel, selincoming, 1);
  } else if (key == 'k') {
    subincoming = !(subincoming);
    hv->setDrawBackTo(prevsel, subincoming, 0);
  } else if (key == 'u') {
    outgoing = !(outgoing);
    hv->setDrawBackFrom("HV_TOPNODE", outgoing, 1);
  } else if (key == 'v') {
    seloutgoing = !(seloutgoing);
    hv->setDrawBackFrom(prevsel, seloutgoing, 1);
  } else if (key == 'w') {
    suboutgoing = !(suboutgoing);
    hv->setDrawBackFrom(prevsel, suboutgoing, 0);
  } else if (key == 'S') {
    selsubtree = !(selsubtree);
    hv->setSelectedSubtree(prevsel, selsubtree);
  } else if (key == 'c') {
    hv->gotoCenterNode(HV_ANIMATE);
  } else if (key == 'C') {
    showcenter = !(showcenter);
    hv->setCenterShow(showcenter);
  } else if (key == 'r') {
    rendstyle = !(rendstyle);
  } else if (key == 'R') {
    backcolor++;
    if (backcolor >= 3) backcolor = 0;
    if (backcolor == 0) {
      hv->setColorBackground(1.0, 1.0, 1.0);
      hv->setColorSphere(.89, .7, .6);
      hv->setColorLabel(0.0, 0.0, 0.0);
    } else if (backcolor == 1) {
      hv->setColorBackground(0.5, 0.5, 0.5);
      hv->setColorSphere(.5, .4, .3);
      hv->setColorLabel(0.0, 0.0, 0.0);
    } else {
      hv->setColorBackground(0.0, 0.0, 0.0);
      hv->setColorSphere(.25, .15, .1);
      hv->setColorLabel(1.0, 1.0, 1.0);
    }
  } else if (key == 'N') {
    drawlinks = !(drawlinks);
    hv->setDrawLinks(drawlinks);
  } else if (key == 'n') {
    drawnodes = !(drawnodes);
    hv->setDrawNodes(drawnodes);
  } else if (key == 'h') {
    negativehide = !(negativehide);
    hv->setNegativeHide(negativehide);
  } else if (key == 'f') {
    char buf[1024];
    if (gets(buf)) 
      hv->gotoNode(buf, HV_ANIMATE);
  } else if (key == 'K') {
    keepaspect = !(keepaspect);
    hv->setKeepAspect(keepaspect);
  } else if (key == 'm') {
    media = !(media);
    hv->setDisableGroup(0, "image", media);    
    hv->setDisableGroup(0, "text", media);    
    hv->setDisableGroup(0, "application", media);    
    hv->setDisableGroup(0, "audio", media);    
    hv->setDisableGroup(0, "audio", media);    
    hv->setDisableGroup(0, "video", media);    
    hv->setDisableGroup(0, "other", media);    
    //    hv->newLayout(NULL);
  } else if (key == 'g') {
    group = !(group);
    hv->setGroupKey(group);
  }
  hv->redraw();
}

void selectCB(const string & id, int /*shift*/, int /*control*/) {
  if (id.find('|') != id.npos) {
    // it's an edge, not a node
    hv->gotoPickPoint(HV_ANIMATE);
  }
  hv->setSelected(id, 1);
  hv->setSelected(prevsel, 0);
  hv->gotoNode(id, HV_ANIMATE);
  hv->redraw();
  strcpy(prevsel, id.c_str());
}

BOOL bSetupPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR *ppfd; 
    PIXELFORMATDESCRIPTOR pfd = { 
	sizeof(PIXELFORMATDESCRIPTOR),  //  size of this pfd 
	1,                              // version number 
	PFD_DRAW_TO_WINDOW |            // support window 
	PFD_SUPPORT_OPENGL |            // support OpenGL 
	PFD_DOUBLEBUFFER,               // double buffered 
	PFD_TYPE_RGBA,                  // RGBA type 
	24,                             // 24-bit color depth 
	0, 0, 0, 0, 0, 0,               // color bits ignored 
	0,                              // no alpha buffer 
	0,                              // shift bit ignored 
	0,                              // no accumulation buffer 
	0, 0, 0, 0,                     // accum bits ignored 
	32,                             // 32-bit z-buffer	 
	0,                              // no stencil buffer 
	0,                              // no auxiliary buffer 
	PFD_MAIN_PLANE,                 // main layer 
	0,                              // reserved 
	0, 0, 0                         // layer masks ignored 
    }; 
    int pixelformat;

    pfd.cColorBits = GetDeviceCaps(hdc,BITSPIXEL);
    ppfd = &pfd;
    pixelformat = ChoosePixelFormat(hdc, ppfd); 
    if ( (pixelformat = ChoosePixelFormat(hdc, ppfd)) == 0 ) {
        printf("ChoosePixelFormat failed");
        return FALSE; 
    }
    if (SetPixelFormat(hdc, pixelformat, ppfd) == FALSE) {
        printf("SetPixelFormat failed");
        return FALSE; 
    }
    return TRUE; 
}


void create(HWND hWnd, int w, int h) {
  HDC hdc = ::GetDC(hWnd);
  hv =  new HypView(hdc,hWnd);
  
  hv->clearSpanPolicy();
  hv->addSpanPolicy(HV_SPANHIER);
  hv->addSpanPolicy(HV_SPANLEX);
  hv->addSpanPolicy(HV_SPANBFS);

  hv->bindCallback(HV_LEFT_CLICK, HV_PICK);
  hv->bindCallback(HV_PASSIVE, HV_HILITE);
  hv->bindCallback(HV_LEFT_DRAG, HV_TRANS);
  hv->bindCallback(HV_RIGHT_DRAG, HV_ROT);
  hv->setPickCallback(selectCB);
  hv->setMotionCull(0);
  hv->setPassiveCull(2);
  //  hv->setLabelFont("-*-helvetica-bold-r-normal--12-*");
  hv->setSphere(1);
  hv->setLabels(HV_LABELLONG);
  hv->setGotoStepSize(.0833);
  hv->setCenterShow(0);

  created = 1;
}

int openfile(const char *fname) {
  HDC hdc = hv->getWidget();
  bSetupPixelFormat(hdc);
  HGLRC ctx = wglCreateContext(hdc);
  if (!ctx) {
    printf("wglCreateContext Failed\n");
    exit(1);
  }
  hv->afterRealize(ctx, width, height);

  ifstream inFile(fname);
  if (!inFile) {
    return(0);
  }
  if (! (hv->setGraph(inFile))) exit(0);

  // hv->newLayout(NULL);

  hv->setGroupKey(0); // start out coloring by mime types
  hv->setColorGroup(0, "html", 0.0, 1.0, 1.0);
  hv->setColorGroup(0, "image", 1.0, 0.0, 1.0);
  hv->setColorGroup(0, "router", 0, 1, 1);
  hv->setColorGroup(0, "text", .90, .35, .05);
  hv->setColorGroup(0, "source", .42, 0, .48);
  hv->setColorGroup(0, "application", .99, .64, .25);
  hv->setColorGroup(0, "audio", .91, .36, .57);
  hv->setColorGroup(0, "video", .91, .36, .57);
  hv->setColorGroup(0, "other", 0, .35, .27);
  hv->setColorGroup(0, "vrml", .09, 0, 1);
  hv->setColorGroup(0, "host", 1.0, 1.0, 1.0);
  hv->setColorGroup(0, "invisible", 0, 0, 0);

  // tree position
  //  hv->setColorGroup(1, "host", 1.0, 1.0, 1.0);
  hv->setColorGroup(1, "main", 0, 1, 1);
  hv->setColorGroup(1, "orphan", 1.0, 0.0, 1.0);
  hv->setColorGroup(1, "external", 1.0, 1.0, 1.0);

  hv->setKeepAspect(1);

  //  hv->gotoCenterNode(HV_JUMP);
  struct timeval idletime;
  idletime.tv_sec = 5;
  idletime.tv_usec = 0;
  hv->setIdleFrameTime(idletime);
  gotfile = 1;
  return 1;
}

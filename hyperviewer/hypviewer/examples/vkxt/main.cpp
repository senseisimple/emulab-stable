#include <string>
NAMESPACEHACK

#ifdef HYPVK
 #include <Vk/VkApp.h>
 #include <Vk/VkSimpleWindow.h>
#else
 #include <Xm/MainW.h>
 #include <Xm/Frame.h>
 #include <X11/Xutil.h>
#endif

#include <X11/keysym.h>
#include <Xm/Form.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#ifndef HYPIRIX
#include <fstream>
#else
#include <fstream.h>
#endif
#include <VkHypViewer.h>

HypView *hv = NULL;
char    prevsel[1024];

static int centerexternal = 0;
static int trail = 0;
static int trailnum = 0;

void selectCB(const string & id, int shift, int control);

void keyboard(Widget /*w*/, XtPointer /*clientData*/, XEvent *event, Boolean */*flag*/) {

  char buf[31];
  KeySym keysym;
  XLookupString(&event->xkey, buf, sizeof(buf),  &keysym, NULL);
  unsigned char key = buf[0];
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
    if (backcolor >= 5) backcolor = 0;
    if (backcolor == 0) {
      hv->setLinkPolicy(HV_LINKLOCAL);
      hv->setColorBackground(1.0, 1.0, 1.0);
      hv->setColorSphere(.89, .7, .6);
      hv->setColorLabel(0.0, 0.0, 0.0);
    } else if (backcolor == 1) {
      hv->setColorBackground(0.5, 0.5, 0.5);
      hv->setColorSphere(.5, .4, .3);
      hv->setColorLabel(0.0, 0.0, 0.0);
    } else if (backcolor == 2) {
      hv->setColorBackground(0.0, 0.0, 0.0);
      hv->setColorSphere(.25, .15, .1);
      hv->setColorLabel(1.0, 1.0, 1.0);
    } else if (backcolor == 3) {
      hv->setColorBackground(.6,.6,.6);
      hv->setColorSphere(.25, .15, .1);
      hv->setColorLabel(0.0, 0.0, 0.0);
      hv->setColorLinkFrom(.5,.5,.5);
      hv->setColorLinkTo(.4,.4,.4);
      hv->setLinkPolicy(HV_LINKCENTRAL);
      //      hv->setColorGroup(0, "html", 1, 0, .8);
      hv->setColorGroup(0, "html", 0, .1, 1.0);
      hv->setColorGroup(0, "vrml", 0, .1, 1.0);
    } else if (backcolor == 4) {
      hv->setColorBackground(.6,.6,.6);
      hv->setColorSphere(.25, .15, .1);
      hv->setColorLabel(0.0, 0.0, 0.0);
      hv->setLinkPolicy(HV_LINKINHERIT);
      //      hv->setColorGroup(0, "html", 1, 0, .8);
      hv->setColorGroup(0, "html", 0, .1, 1.0);
      hv->setColorGroup(0, "vrml", 0, .1, 1.0);
    }
  } else if (key == 'N') {
    drawlinks = !(drawlinks);
    hv->setDrawLinks(drawlinks);
  } else if (key == 'n') {
    drawnodes = !(drawnodes);
    hv->setDrawNodes(drawnodes);
  } else if (key == 'D') {
    centerexternal = !(centerexternal);
    hv->setCenterLargest(centerexternal);
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
  } else if (key =='t') {
    trail = !trail;
    trailnum++;
  } else if (key =='T') {
    trailnum = 0;
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

void doTrail(const char *id) {
  static int currnum = 0;
  static char *grp = NULL;
  static char *oldid = NULL;
  static float r,g,b;
  if (currnum != trailnum) {
    static char gchars[32];
    sprintf(gchars, "trail%02d", trailnum);
    if (grp) free(grp);
    grp = strdup(gchars);
    r =(float) rand()/RAND_MAX;
    g =(float) rand()/RAND_MAX;
    b =(float) rand()/RAND_MAX;
    hv->setColorGroup(0,grp,r,g,b);
    //    hv->setDisableGroup(0,grp,0);
    currnum = trailnum;
    free(oldid);
    oldid = NULL;
  }
  hv->setNodeGroup(id, 0, grp);
  if (oldid) {
    hv->setNodeGroup(oldid, 0, grp);
    int islink = hv->getDrawLink(oldid, id);
    if (islink == -1) 
      hv->addLink(oldid, id);
    hv->setDrawLink(oldid, id, 1);
    hv->setColorLink(oldid, id, r, g, b);
    free(oldid);
  }
  oldid = strdup(id);
}

void selectCB(const string & id, int /*shift*/, int /*control*/) {
  if (id.find('|') != id.npos) {
    // it's an edge, not a node
    hv->gotoPickPoint(HV_ANIMATE);
  }
  if (trail) {
    doTrail(id.c_str());
  } else {
    hv->setSelected(id, 1);
    hv->setSelected(prevsel, 0);
  }
  hv->gotoNode(id, HV_ANIMATE);
  hv->redraw();
  if (centerexternal) {
    hv->setCurrentCenter(id);
  }
  hv->redraw();
  strcpy(prevsel, id.c_str());
}


main(int argc, char *argv[]) {
  static int fallback[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 1, 
			      GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1, 
			      GLX_DEPTH_SIZE, 1, None};

  static String fallbackResources[] = { 
    "*glxarea*width: 300", "*glxarea*height: 300",
    "*frame*x: 20", "*frame*y: 20", 
    "*frame*topOffset: 20", "*frame*bottomOffset: 20", 
    "*frame*leftOffset: 20", "*frame*rightOffset: 20", 
    "*frame*shadowType: SHDOW_IN", NULL
  };
  //  static String fallbackResources[] = { "" };

  Display *dpy;
#ifdef HYPVK
  VkApp *app = new VkApp("VkHyp", &argc, argv);
  Widget toplevel = app->baseWidget();
  dpy = XtDisplay(toplevel);
#else
  XtAppContext app;
  Widget toplevel = XtAppInitialize(&app, "hypviewer", NULL, 0, &argc, argv,
				    fallbackResources, NULL, 0);
  dpy = XtDisplay(toplevel);
  //  dpy = XOpenDisplay(NULL);
#endif

  Arg args[5];
  int arg_count = 0;
  XtSetArg( args[arg_count], XmNwidth,    700 );  arg_count++;
  XtSetArg( args[arg_count], XmNheight,    700 );  arg_count++;

#ifdef HYPVK
  VkSimpleWindow *win = new VkSimpleWindow("win", args, arg_count);
  Widget mw = win->mainWindowWidget();
#else
  //  Widget win = XmCreateMainWindow(toplevel, "mainw", NULL, 0);
  //  XtManageChild(win);
  Widget mw = toplevel;
#endif

  /* if you don't pass in your own visual it will use a default. 
     we could skip the following four lines if we just used the initialization
     VkHypViewer *vhv = new VkHypViewer("HypViewer", win->mainWindowWidget());
     */
  XVisualInfo    *visinfo;
  if (!(visinfo = glXChooseVisual(dpy, DefaultScreen(dpy), fallback))) {
    fprintf(stderr, "no suitable RGB visual");
    exit(0);
  }
  Widget form = XmCreateForm(mw, "form", args, arg_count); 
  XtManageChild(form); 
  VkHypViewer *vhv = new VkHypViewer("HypViewer", mw, form, visinfo);

#ifdef HYPVK
  win->addView(vhv);
  win->open();
  win->show();
#else
  XtRealizeWidget(toplevel);
#endif
  //  vhv->show();
  vhv->init();


  hv = vhv->getHypView();
  if (argc < 2) {
    fprintf(stderr, "need filename\n");
    exit(0);
  }
  //  hv->setSpanPolicy(HV_SPANHIER);
  if (argc > 2 && argv[2][0] == '-') {
    hv->clearSpanPolicy();
    int len = strlen(argv[2]);
    int i = 1;
    while (i < len) {
      char c = argv[2][i];
      if (c == 'k')
	hv->addSpanPolicy(HV_SPANKEEP);
      else if (c == 'f')
	hv->addSpanPolicy(HV_SPANFOREST);
      else if (c == 'h')
	hv->addSpanPolicy(HV_SPANHIER);
      else if (c == 'n')
	hv->addSpanPolicy(HV_SPANNUM);
      else if (c == 'l')
	hv->addSpanPolicy(HV_SPANLEX);
      else if (c == 'b')
	hv->addSpanPolicy(HV_SPANBFS);
      i++;
    }
  }

  hv->bindCallback(HV_LEFT_CLICK, HV_PICK);
  hv->bindCallback(HV_PASSIVE, HV_HILITE);
  hv->bindCallback(HV_LEFT_DRAG, HV_TRANS);
  hv->bindCallback(HV_MIDDLE_DRAG, HV_ROT);
  hv->setPickCallback(selectCB);
  hv->setMotionCull(0);
  hv->setPassiveCull(2);
  hv->setLabelFont("-*-helvetica-bold-r-normal--12-*");
  hv->setSphere(1);
  hv->setLabels(HV_LABELLONG);
  hv->setGotoStepSize(.0833);
  hv->setCenterShow(0);
  // mime types

  // must set graph before can do group stuff
  ifstream inFile(argv[1]);
  if (!inFile)
    exit(1);

  if (! (hv->setGraph(inFile))) exit(0);
  //  if (! (hv->setGraph(argv[1]))) exit(0);

  // disable orphans at first

  hv->setGroupKey(0); // start out coloring by mime types
  hv->setDisableGroup(1, "orphan", 1);    
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


  //  hv->gotoCenterNode(HV_JUMP);
  struct timeval idletime;
  idletime.tv_sec = 5;
  idletime.tv_usec = 0;
  hv->setIdleFrameTime(idletime);
  XtAddEventHandler(hv->getWidget(), KeyReleaseMask, FALSE, keyboard, NULL);
  struct timeval now;
  gettimeofday(&now, NULL);
  srand(now.tv_usec);
#ifdef HYPVK
  app->run();
#else 
  XtAppMainLoop(app);
#endif
}

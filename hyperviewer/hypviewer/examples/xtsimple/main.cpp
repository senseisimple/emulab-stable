#include <string>
NAMESPACEHACK

#include <Xm/MainW.h>
#include <Xm/Frame.h>
#include <X11/Xutil.h>

#include <X11/keysym.h>
#include <Xm/Form.h>

#include <stdio.h>
#include <stdlib.h>

#ifndef HYPIRIX
#include <fstream>
#else
#include <fstream.h>
#endif
#include <VkHypViewer.h>

HypView *hv = NULL;
string  prevsel;

void selectCB(const string & id, int /*shift*/, int /*control*/) {
  hv->setSelected(id, 1);
  hv->setSelected(prevsel, 0);
  hv->gotoNode(id, HV_ANIMATE);
  hv->drawFrame();
  hv->idle(1);
  prevsel = id;
}


main(int argc, char *argv[]) {

  XtAppContext app;
  Widget toplevel = XtAppInitialize(&app, "hypviewer", NULL, 0, &argc, argv,
				    NULL, NULL, 0);
  Widget mw = toplevel;

  Arg args[5];
  int arg_count = 0;
  XtSetArg( args[arg_count], XmNwidth,    700 );  arg_count++;
  XtSetArg( args[arg_count], XmNheight,    700 );  arg_count++;

  Widget form = XmCreateForm(mw, "form", args, arg_count); 
  XtManageChild(form); 
  VkHypViewer *vhv = new VkHypViewer("HypViewer", mw, form, NULL);

  XtRealizeWidget(toplevel);
  vhv->init();


  hv = vhv->getHypView();
  if (argc < 2) {
    fprintf(stderr, "need filename\n");
    exit(0);
  }
  hv->clearSpanPolicy();
  hv->addSpanPolicy(HV_SPANHIER);
  hv->addSpanPolicy(HV_SPANLEX);
  hv->addSpanPolicy(HV_SPANBFS);

  hv->bindCallback(HV_LEFT_CLICK, HV_PICK);
  hv->bindCallback(HV_PASSIVE, HV_HILITE);
  hv->bindCallback(HV_LEFT_DRAG, HV_TRANS);
  hv->bindCallback(HV_MIDDLE_DRAG, HV_ROT);
  hv->setPickCallback(selectCB);
  hv->setMotionCull(0);
  hv->setPassiveCull(2);

  // must set graph before can do group stuff
  ifstream inFile(argv[1]);
  if (!inFile)
    exit(1);

  if (! (hv->setGraph(inFile))) exit(0);

  XtAppMainLoop(app);
}

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

  hv->setDrawBackFrom("HV_TOPNODE", 1, 1);

  // make the graph from scratch without loading a file
  string root, group, node1, node2;
  int gkey = 0;
  root = "base";
  group = "mygroup";
  node1 = "a_child";
  node2 = "another_child";
  hv->initGraph(root, gkey, group);
  hv->addNode(root,node1);
  hv->addLink(root,node1);
  hv->setNodeGroup(node1, gkey, group);
  hv->addNode(root,node2);
  hv->addLink(root,node2);
  hv->setNodeGroup(node2, gkey, group);
  hv->addLink(node1,node2);
  hv->newLayout(root);

  hv->setGroupKey(gkey);
  hv->setColorGroup(gkey, group, 1.0, 0.0, 1.0);

  XtAppMainLoop(app);
}

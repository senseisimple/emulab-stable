#include <Vk/VkApp.h>
#include <Vk/VkSimpleWindow.h>

#include <stdio.h>
#include <stdlib.h>

#include <hv/VkHypViewer.h>

char    prevsel[1024];
HypView *hv=NULL;

void selectCB(char *id, int /*shift*/, int /*control*/) {
  hv->setSelected(id, 1);
  hv->setSelected(prevsel, 0);
  hv->gotoNode(id, HV_ANIMATE);
  hv->drawFrame();
  hv->idle(1);
  strcpy(prevsel, id);
}

main(int argc, char *argv[]) {

  VkApp *app = new VkApp("VkHyp", &argc, argv);

  Arg args[2];
  int arg_count = 0;
  XtSetArg( args[arg_count], XmNwidth,    300 );  arg_count++;
  XtSetArg( args[arg_count], XmNheight,    300 );  arg_count++;

  VkSimpleWindow *win = new VkSimpleWindow("win", args, arg_count);

  VkHypViewer *vhv = new VkHypViewer("HypViewer", 
                                     win->mainWindowWidget(),
                                     win->mainWindowWidget());

  win->addView(vhv);
  win->show();

  hv = vhv->getHypView();
  if (argc < 2) {
    fprintf(stderr, "need filename\n");
    exit(0);
  }
  hv->setGraph(argv[1]);
  hv->bindCallback(HV_LEFT_CLICK, HV_PICK);
  hv->bindCallback(HV_PASSIVE, HV_HILITE);
  hv->bindCallback(HV_LEFT_DRAG, HV_TRANS);
  hv->bindCallback(HV_MIDDLE_DRAG, HV_ROT);
  hv->setPickCallback(selectCB);
  hv->setMotionCull(0);
  hv->setPassiveCull(2);
  
  app->run();
}

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

#ifdef HYPGLX

#include <string>
NAMESPACEHACK

#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>

#ifdef HYPFREE
  #include <GL/GLwMDrawA.h>
#else
  #include <X11/GLw/GLwMDrawA.h>
#endif

#include "VkHypViewer.h"

#ifdef HYPVK
VkHypViewer::VkHypViewer(const char* name, Widget top, Widget parent, 
			 XVisualInfo *vi) : VkComponent(name) {
#else
VkHypViewer::VkHypViewer(const char* name, Widget top, Widget parent, 
			 XVisualInfo *vi) {
#endif

  hv = NULL;

  if (!vi) {
    static int fallback[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 1, 
			      GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1, 
			      GLX_DEPTH_SIZE, 1, None};
    XVisualInfo *visinfo;    
    Display *d = XtDisplay(top);
    if (!(visinfo = glXChooseVisual(d, DefaultScreen(d), fallback))) {
      XtAppError(XtDisplayToApplicationContext(XtDisplay(top)), 
		 "HypViewer: no suitable RGB visual");
      return;
    }
    vi = visinfo;
  }

  Pixel bg = WhitePixelOfScreen(XtScreen(top));

  int pfwidth = 0;
  int pfheight = 0;
  XtVaGetValues(top,       /* get width, height of parent_form */
		XmNwidth, &pfwidth,
		XmNheight, &pfheight,
		NULL);
  glxarea = XtVaCreateManagedWidget("glxwidget", 
				    glwMDrawingAreaWidgetClass, parent,
				    GLwNrgba, TRUE,
				    GLwNdoublebuffer, TRUE, 
				    GLwNredSize, 1,
				    GLwNgreenSize, 1,
				    GLwNblueSize, 1, 
				    GLwNdepthSize, 15, 
				    XmNleftAttachment,   XmATTACH_FORM,
				    XmNrightAttachment,  XmATTACH_FORM,
				    XmNtopAttachment,    XmATTACH_FORM,
				    XmNbottomAttachment, XmATTACH_FORM,
				    XmNbackground,       bg,
				    NULL);

  par = parent;

#ifdef HYPVK
  _baseWidget = glxarea;
  installDestroyHandler();
#endif

  //  XtAddCallback(glxarea, GLwNginitCallback, initCallback, (XtPointer)this);
  XtAddEventHandler(glxarea, 
		    PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
		    StructureNotifyMask | ExposureMask, 
		    FALSE, eventHandlerCB, (XtPointer) this);

  hv = new HypView(glxarea);
  cx = NULL;
}


VkHypViewer::~VkHypViewer() {
}

#ifdef HYPVK
const char* VkHypViewer::className() 
{
    return "VkHypViewer";
}
#endif

void VkHypViewer::initCallback(Widget /*w*/, XtPointer clientData, XtPointer /*callData*/) {
  VkHypViewer *vk = (VkHypViewer*) clientData;
  vk->init();
}

void VkHypViewer::eventHandlerCB(Widget /*w*/, XtPointer clientData, 
					XEvent *event, Boolean */*flag*/) {
  VkHypViewer *vk = (VkHypViewer*) clientData;
  vk->eventHandler(event);
}

void VkHypViewer::init() {
  XVisualInfo *vi;
  if (!cx) {
    XtVaGetValues(glxarea, GLwNvisualInfo, &vi, NULL); 
    cx = glXCreateContext(XtDisplay(glxarea), vi, 0, GL_TRUE);
    glXMakeCurrent(XtDisplay(glxarea), XtWindow(glxarea), cx);
    hv->afterRealize(cx);
  }
}


void VkHypViewer::eventHandler(XEvent *event) {
  // may need to drain away events which occurred while HypView was busy
  int tossEvents = hv->getTossEvents();
  if (tossEvents) {
#ifdef HYPVK
    XtAppContext app = theApplication->appContext();
#else
    XtAppContext app = XtWidgetToApplicationContext (glxarea);
#endif
    XEvent e;
    while (XtAppPending(app)) {
      XtAppNextEvent(app, &e);     // just throw it away
      XtDispatchEvent(&e);
    }
    hv->setTossEvents(0);
    // if one of these ignored events was the rightmouse then 
    // Motif has already handed us input focus. must give it back! 
    XUngrabPointer(XtDisplay(glxarea), CurrentTime);
  }

  switch (event->type) {
  case ButtonPress:
  case ButtonRelease:
    hv->mouse(event->xbutton.button-1, 
	      event->xmotion.state&(Button1Mask|Button2Mask|Button3Mask),
	      event->xmotion.x, event->xmotion.y, 
	      event->xmotion.state&(ShiftMask), 
	      event->xmotion.state&(ControlMask)
	      );
    break;
  case MotionNotify:
    if (event->xmotion.state&(Button1Mask|Button2Mask|Button3Mask)) {
      hv->motion(event->xmotion.x, event->xmotion.y,
		 event->xmotion.state&(ShiftMask), 
		 event->xmotion.state&(ControlMask)
		 );
    } else {
      hv->passive(event->xmotion.x, event->xmotion.y,
		  event->xmotion.state&(ShiftMask), 
		  event->xmotion.state&(ControlMask)
		  );
    }
    break;
  case ConfigureNotify:
    hv->reshape(event->xconfigure.width, event->xconfigure.height);
    break;
  case Expose:
    hv->idle(1);
    hv->drawFrame();
    break;
  }
  // make the idle work right
  if (XtPending()) {
    hv->idle(0);
  }
}

void VkHypViewer::afterRealizeHook() {
  init();
}
#endif

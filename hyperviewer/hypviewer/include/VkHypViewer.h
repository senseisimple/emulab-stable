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
#ifndef VKHYPVIEWER_H
#define VKHYPVIEWER_H

#ifdef HYPVK
 #include <Vk/VkComponent.h>
 #include <Vk/VkApp.h>
#else
 #include <Xm/Xm.h>
#endif

#include "HypView.h"
#include "HypData.h"


class HypView;

/*

CLASS
 VkHypViewer

 VkHypViewer is the ViewKit/Xt layer on top of the base HypView class. 

OVERVIEW

 After initialization, getHypView() should be used to get the HypView
 object, which is the main interface to the functionality of the
 library. Although this class is called VkHypViewer for historical
 reasons, most people will want to use the Xt flavor as opposed to the 
 ViewKit flavor. 

AUTHOR

<pre>
 Tamara Munzner
 http://graphics.stanford.edu
 munzner@cs.stanford.edu
</pre>

*/

#ifdef HYPXT
class VkHypViewer {
#else
class VkHypViewer: public VkComponent {
#endif

public:

  /// Initialization. Arguments: name string, top-level
  // widget of window, parent widget for drawing area, XVisualInfo for
  // the window. If no XVisualInfo given, fallback is RGBA
  // doublebuffered with maximum possible red, green, blue, and depth
  // buffers. 
  VkHypViewer(const char *name, Widget top, Widget parent, 
	      XVisualInfo *vi = NULL);

  /// Destructor.
  ~VkHypViewer();

#ifdef HYPVK
  /// Classname method as required by ViewKit.
  virtual const char *className();
#endif

  /// Initialization callback used by afterRealizeHook and/or
  // initCallback() can also be called directly if necessary. 
  void init();

  /// Return the HypView object which is the main interface to the
  // functionality of the library. 
  HypView* getHypView() const	{ return hv; };


  /// Handle the Xt events. Calls the HypView mouse(), motion(),
  // passive(), reshape(), idle(), drawFrame() callbacks. 
  static void eventHandlerCB(Widget w, XtPointer clientData, 
			     XEvent *event, Boolean *flag);

  /// Calls the init() function. 
  static void initCallback(Widget w, XtPointer clientData, 
			     XtPointer callData);

private:
  GLXContext cx;
  Widget glxarea;
  Widget par;
  HypView *hv;
  void eventHandler(XEvent *event);
  void afterRealizeHook();
};


#endif

%module hv
%{
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

#include <string>
NAMESPACEHACK

#include "HypView.h"
%}

/* Magic from /usr/local/share/doc/swig/Doc/Manual/Python.html
 * "19.8.2 Expanding a Python object into multiple arguments".
 */
%typemap(in) (int argc, char *argv[]) {
  /* Check if is a list */
  if (PyList_Check($input)) {
    int i;
    $1 = PyList_Size($input);
    $2 = (char **) malloc(($1/*size*/+1)*sizeof(char *));
    for (i = 0; i < $1; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyString_Check(o))
	$2[i] = PyString_AsString(PyList_GetItem($input,i));
      else {
	PyErr_SetString(PyExc_TypeError,"list must contain strings");
	free($2);
	return NULL;
      }
    }
    $2[i] = 0;
  } else {
    PyErr_SetString(PyExc_TypeError,"not a list");
    return NULL;
  }
}
%typemap(freearg) (int argc, char **argv) {
  free((char *) $2);
}

// It's easier to return the pointer to the HypView object rather than access the global.
//extern HypView  *hvmain(int argc, char *argv[], int window, int width, int height);

#ifndef WIN32
extern HypView *hvMain(int argc, char *argv[], void *window, int width, int height);
#else
extern HypView *hvMain(int argc, char *argv[], int window,  int width, int height);
#endif
//extern int hvmain(int argc, char *argv[]);
//%include "cpointer.i"
//%pointer_class(HypView,hvp)
//extern HypView *hv;

extern void hvKill(HypView *hv);

// Separate out file reading from the main program.
extern int hvReadFile(char *fname, int width, int height);

// Get the node id string last selected by the selectCB function.
extern char const *getSelected();

// Get the node id string at the graph center.
extern char *getGraphCenter();

// std::string is used for INPUT args to HypView methods.
%include "std_string.i"
// How come %apply doesn't work for std::string?  Workaround with sed instead.
///namespace std {
//%apply const std::string & INPUT { const string & id };
///}

// This callback is used in picking.  Call it to simulate picking.
extern void selectCB(const std::string & INPUT, int shift, int control);

//================================================================

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
#ifndef HYPVIEW_H
#define HYPVIEW_H

#ifndef WIN32
extern "C" {
#include <X11/Intrinsic.h>
#include <GL/glx.h>
}
#endif

#include <string>



#include "HypGraph.h"
#include "HypViewer.h"

/*

CLASS
 HypView

 HypView is the main interface class for the HypViewer library, which
 provides layout and interactive navigation of node-link graphs in 3D
 hyperbolic space. The library can handle quite large graphs of up to
 100,000 edges quickly and with minimal visual clutter. The hyperbolic
 view allows the user to see a great deal of the context around the
 current focus node.

OVERVIEW
 
 This class is the main interface between the entire library and the
 application programmer. No get methods are documented - see the
 corresponding set method.




LAYOUT

 The HypView layout algorithm finds a spanning tree from an input
 graph and then computes a layout of that tree. A spanning tree
 touches every node in a graph, but only a subset of the links. In a
 graph a node can have many parents, but in a tree a canonical parent
 is chosen for each child. Links which appear in the graph but not in
 the spanning tree are called "non-tree links". These links do not
 affect the layout computation and are only drawn on demand for a
 selected node or nodes.
 
 The backbone spanning tree used by the layout and drawing algorithms
 strongly influences the visual impact on the user. The approach
 hinges on the idea that there are many graphs for which the right
 spanning tree can provide a useful mental model of the entire
 structure. As a fallback, a default spanning tree can always be found
 using breadth-first search from a root node. However, exploiting a
 small amount of domain-specific knowledge can allow the library to
 construct a better spanning tree which provides a more useful mental
 model. For instance, for a Web site one can use the directory
 structure encoded in the URL to choose which of the incoming
 hyperlinks to a site should be chosen as the main parent in the
 spanning tree. The specific spanning tree policies used by the
 library are controllable with the clearSpanPolicy() and
 addSpanPolicy() methods. 


DRAWING

 The HypView class has a multi layer scheme which preserves window
 system independence while providing flexibility for the application
 programmer. First, the mouse(), motion(), and passive() methods are a way
 for a window system layer to pass event loop information about the
 mouse into the HypView class. Second, the bindCallback() allows the
 application programmer to bind the main interaction functions to the
 desired mouse behavior. The setPickCallback() and
 setHiliteCallback() functions can be used to return control to
 application program callbacks when the user moves the mouse over nodes
 and links. Finally, methods like setSelected() and gotoNode() can be
 used inside the pick or highlight callbacks in the application
 program. 

 The HypView drawing algorithm depends on the number of visible, not total,
 number of nodes and edges. The projection from hyperbolic to
 euclidean space guarantees that nodes sufficiently far from the
 center will project to less than a single pixel. Thus the visual
 complexity of the scene has a guaranteed bound - only a local
 neighborhood of nodes in the graph will be visible at any given time.

 A guaranteed frame rate is extremely important for a fluid
 interactive user experience. The HypView adaptive drawing algorithm
 is designed to always maintain a target frame rate even on low-end
 graphics systems. A high constant frame rate is maintained by drawing
 only as much of the neighborhood around a center point as is possible
 in the allotted time. When the user is idle, the system fills in more
 of the surrounding scene. A slow graphics system will simply show
 less of the context surrounding the node of interest during
 interactive manipulation.
  
 The amount of time devoted to a frame should depend on the activity
 of the user. The HypView library allows separate control over the
 drawing frame time, idle frame time, and picking frame time using the
 setDynamicFrameTime(), setIdleFrameTime(), and setPickFrameTime()
 methods. The dynamic frame time is simply the time budget in which to
 draw a single frame during user mouse movement or an animated
 transition. It is clear that the drawing time should be explicitly
 bounded instead of increasing as the node/edge count rises. The time
 spent casting pick rays into the scene must be similarly bounded.
 When the user and application are idle, the system can fill in more
 of the surrounding scene. However, the time spent on this, the idle
 frame time, should still be bounded to eventually free the CPU for
 other tasks.


INITIALIZATION

 Most functions have no effect or return NULL if called after a
 HypView object is instantiated but before the setGraph member is
 called. However, the following functions can be successfully called
 even before the first setGraph() call:

addSpanPolicy(), 
bindCallback(), 
clearSpanPolicy(), 
getDynamicFrameTime(),
getIdleFrameTime(),
getPickFrameTime(),
getSphere(),
getEdgeSize(),
getLabels(),
getLabelSize(),
getLabelFont(),
getPassiveCull(),
getMotionCull(),
getAreaFudge(),
getLengthFudge(),
getLeafRad(),
getMaxLength(),
getGenerationNodeLimit(),
getGenerationLinkLimit(),
getCenterShow(),
getCenterLargest(),
getNegativeHide(),
getSpanPolicy(),
getGotoStepSize(),
getTossEvents(),
getWidget(),
setCenterLargest(), 
setCenterShow(), 
setColorHilite(), 
setColorSelect(), 
setColorLabel(),
setColorLinkFrom(), 
setColorLinkTo().
setEdgeSize(),
setGenerationNodeLimit(), 
setGenerationLinkLimit(), 
setGotoStepSize(),
setGroupKey(),
setHiliteCallback(),
setKeepAspect(), 
setLabels(), 
setLabelSize(), 
setLabelToRight(), 
setLeafRad(),
setLinkPolicy(),
setMaxLength(), 
setNegativeHide(), 
setMotionCull(), 
setPassiveCull(), 
setPickCallback(), 
setSphere(),
setTossEvents(), 
setDynamicFrameTime(), 
setIdleFrameTime(),
setPassiveFrameTime(), 

 The following functions cannot be called until after the window is shown,
 since they depend on having a graphics context:
drawFrame(), 
redraw(), 
reshape(),



INPUT FILE FORMAT

 Each line of the input file is of the form

<pre>
  depth identifier 1 group1 [group2 ... groupN]
</pre>

with the following types

<pre>
  int string int string [string ... string]
</pre>

 Each line corresponds to a node. The order of the lines is
 meaningful. The integer at the beginning of the line is the depth in
 the tree. A line with depth one greater than the line above it
 indicates a link between two nodes from the line above to the line
 below. The identifier strings are assumed to uniquely specify a node.
 If an identifier occurs twice that means a node has more than one
 incoming link, so the file describes a general graph, as opposed to a
 tree or a directed acyclic graph (DAG). The identifier string is used
 as an interface between the library and the application program in
 many methods which have the argument "string & id".

 A file with structure like

<pre>

0       A   [...]
1       B   [...]
2       C   [...]
3       D   [...]
4       A   [...]
3       E   [...]
2       F   [...]
1       G   [...]
1       H   [...]
2       I   [...]
2       J   [...]
1       K   [...]

</pre>

corresponds to a graph that looks like:

<pre>

  .-------- A
 /      ____|_______
 |     /   |   \    |
 |    B    G    H   K
 |   / \       / \
 |  C   F     I   J
 \ / \
  D   E

</pre>

 The very first node is the root, the numbers correspond to the number
 of layers deep in the hierarchy.

 The software computes a spanning tree to use as the base for layout
 and display. The addSpanPolicy() method controls the policies used to
 create this spanning tree. If the HV_SPANFOREST policy is used, then
 you can have a forest of distinct subtrees that are not mingled. The
 other policies, like HV_SPANBFS for breadth-first search, are then
 applied to each subtree independently. A forest is specified by
 having a toplevel starting root which has depth < 0. For instance,

<pre>
-1 top 1 html
 0 subtree1/root 1 html
  1 subtree1/thing1 1 html
  1 subtree1/thing2 1 html
[...]
 0 subtree2/root 1 html
  1 subtree2/thing1 1 html
  1 subtree2/thing2 1 html
</pre>

 Groups and collections are an organizational mechanism for filtering
 and coloring nodes. A node can belong to exactly one group per
 collection. In the input file, the collections correspond to columns
 and groups correspond to the values which are in the columns. A node
 can also be associated with a group in a collection with the
 setNodeGroup() method. Collections are designated by a number,
 starting with 0. There must be at least one collection, and there can
 be up to 16 different ones. Each collection can have any number of
 groups. Group names are designated by text strings and can be used in
 different collections. Any group in a collection can be disabled.
 A node will be drawn if none of its associated groups have been disabled. 
 Groups are enabled by default and may be explicitly  disabled/reenabled 
 by the application program using the setDisableGroup() method.
 Each group in each collection can have a color assigned to it with
 the setColorGroup() method. The setGroupKey() method allows the
 application program to change which of the collections is used for
 coloring. 

 For example, the file

<pre>

 0 http://hyper/ 1 html main
 1 http://hyper/index.html html main
 1 http://hyper/logo.gif image main
 1 http://hyper/old.html html orphan

</pre>

 with the following code in the application 

<pre>

  hv->setColorGroup(0, "image", .42, 0, .48);
  hv->setColorGroup(0, "html", 0, 1, 1);
  hv->setColorGroup(1, "main", 1, 1, 1);
  hv->setColorGroup(1, "orphan", .2, .2, .2);

  hv->setDisbleGroup(1, "orphan", 0);
  hv->setGroupKey(0);

</pre>

 would result in three visible nodes, colored by collection 0: the
 logo.gif node would be colored purple and the index.html and
 top-level hyper nodes would be colored cyan. The old.html node would
 not be drawn since the orphan group is disabled.


 Finally, the third item in the input file line must be an integer,
 which should be set 1 (for historical reasons).


DEPENDENCIES

 The HypView libraries are written in C++ and use OpenGL/Mesa and STL.
 There is a small amount of OS/window system dependent code which is
 segregated by "ifdef"s. The three main flavors are Unix XWindows and
 Microsoft Windows, and GLUT. The XWindows version is tagged as "GLX"
 in the Makefiles and has two flavors: Xt and ViewKit. Xt is probably
 the most useful version. ViewKit is a library built on top of Xt (so
 the code is quite similar) which is most popular under Irix, but has
 been ported to other platforms. The libraries were originally written
 under ViewKit, so there are a lot of historical "vk" tags still lying
 around despite the fact that Xt is now the main version.

 GLUT is a window-system independent library from Mark Kilgard
 (formerly of SGI), downloadable from
 http://reality.sgi.com/opengl/glut3/glut3.html. It should
 theoretically be able to run under Windows but has only been tested
 under Irix. The GLUT version is simpler than the Xt one but not as
 powerful, which unsurprisingly mirrors the relationship of GLUT and
 Xt/ViewKit.

 The Windows version has been tested on Windows NT under Visual C++.
 The Borland version has not been tested for a while. 

MAKEFILES

 Select the appropriate flavor of OS and window system by uncommenting
 the relevant line in Makefile.main, then edit the relevant Makefile
 for your system to make sure all paths are set correctly for your system. 

AUTHOR

<pre>
 Tamara Munzner
 http://graphics.stanford.edu
 munzner@cs.stanford.edu
</pre>

*/

class HypView {

public:

  // GROUP: Initialization

#ifdef WIN32
  HypView(HDC w,HWND hwin);
  void afterRealize(HGLRC cx, int w, int h); //(GLXContext cx);

  /// Return the drawing area widget passed in at initialization time.
  HDC getWidget();

#elif HYPGLX

  /// GLX version. Creation function for HypView class.
  HypView(Widget w);

  /// GLX version. This function should be called immediately after
  // opening the window and must precede all drawing. It can be called
  // before or after setGraph. We pass in the GL context,
  // which cannot be created until after a window is opened.
  // See the INITIALIZATION section for details on which functions
  // cannot be called before afterRealize(). 
  void afterRealize(GLXContext cx);

  /// GLX version only. Return the drawing area widget passed in at
  // initialization time.  
  Widget getWidget();

#else

  /// GLUT version. Creation function for HypView class. 
  HypView();

  /// GLUT version. This function should be called immediately after
  // opening the window and must precede all drawing. It can be called
  // before or after setGraph.
  void afterRealize();

#endif

  /// Destructor for HypView class.
  ~HypView();

  // GROUP: HypView
  // 

  /// Output: array of node identifier strings, last one set to NULL.
  // Returns every node in the subtree beneath the input node. This
  // could be a large amount of data: in the limit, if the root node is
  // given, it will return every node in the graph.
  char** enumerateSubtree(const std::string & INPUT);

  /// Briefly flash the link between two nodes.
  int    flashLink(const std::string & INPUT, const std::string & INPUT);

  /// Return identifier of node which is currently nearest the center.
  // Do not cache this identifier for later use, since it can change
  // on every redraw.
  string getCenter();

  /// Move indicated node to the center. The motion includes both a
  // translational and a rotational component, so that the node is in a
  // canonical position with all its ancestors to the left and all its
  // descendants to the right.
  // If animate is HV_ANIMATE, use an animated transition. If animate
  // is HV_JUMP, just jump.  
  int    gotoNode(const std::string & INPUT, int animate);

  /// Reset by moving root node to center. Use root node given by the
  // last setGraphCenter() call, or the top node of the current tree if
  // no setGraphCenter() call was made.
  // If animate is HV_ANIMATE, use an animated transition. If animate
  // is HV_JUMP, just jump.  
  void   gotoCenterNode(int animate);

  /// Move structure so that the exact point where the user last
  // clicked is at the center. The motion will include only
  // translational components, as opposed to the other goto* commands
  // which include a rotational component.
  // If animate is HV_ANIMATE, use an animated transition. If animate
  // is HV_JUMP, just jump.  
  int    gotoPickPoint(int animate);


  /// Explicitly trigger new graph layout in viewer. Mainly intended
  // for debugging, since this always happens as a side effect from
  // other calls: always for setEnableGraph(), sometime from gotoNode() and
  // gotoCenterNode().
  void   newLayout(const std::string & INPUT);

  /// Save graph structure into file "fname", in format that can later
  // be read by setGraph() call.
  // File format details given elsewhere. 
  int    saveGraph(const std::string & INPUT);

  /// Set the current center node for the drawing algorithm to be the
  // given node identifier. Not recommended for casual use: requires
  // understanding of library internals.
  void   setCurrentCenter(const std::string & INPUT);

  /// Set the root node for the tree, for use by subsequent calls to
  // the gotoCenterNode() reset function.
  void   setGraphCenter(const std::string & INPUT);

  /// Get the name of the root node of the tree, which may have been
  // set by the gotoCenterNode() reset function.
  string getGraphCenter();

  /// Load a graph into the viewer. The string can either be a
  // filename or the actual data: if the string length is < 256
  // characters and a file by that name exists, assume it is a filename.
  // Input file format details given elsewhere.
  //
  // NOTE: we now take an istream reference.  Hence we can accept an
  // ifstream (for data from disk) or an istrstream (for data from
  // memory).     - dwm@caida.org
  int    setGraph(istream & str);

  /// Initializes a graph for the viewer.  This does _not_ complete
  //  initialization; you still have to call addNode() to add nodes,
  //  then walk the HypLinkArray in the graph and set parents for
  //  each link, then call doLayout() on the HypGraph.
  //  This was added by dwm@caida.org to permit loading directly
  //  from skitter data files.
  int initGraph(string & rootId, int rootPriority,
                string & rootGroup);
  
  /// Disable or enable a group in a collection. The group name is
  // "group", the collection number is "groupkey". The node will not be
  // drawn if its associated group in any collection is disabled. 
  int    setDisableGroup(int groupkey, const std::string & INPUT, bool on);

  /// Set the collection to be used for coloring nodes. 
  void   setGroupKey(int i);

  /// Use the selected color for this node if on=1, or revert to
  // normal color if on=0.
  void   setSelected(const std::string & INPUT, bool on);

  /// Use the selected color for this node and all nodes in the
  // subtree beneath it in if on=1, or revert them all to normal color
  // if on=0. 
  void   setSelectedSubtree(const std::string & INPUT, bool on);

#ifdef XPMSNAP
  /// Takes a snapshot in XPM format and stores it in the file named
  //  by fileName.  Returns true on success, else returns false.
  bool XpmSnapshot(const std::string & INPUTName);
#endif //XPMSNAP
  
  // GROUP: HypGraph 
  //

  /// Add a new link from one node to another. Returns 0 for failure
  // unless both nodes already exist. Returns 1 for success.
  int  addLink(const std::string & INPUT, const std::string & INPUT);

  /// Add a new child node. Returns 0 for failure if parent does not
  // exist. Returns 1 for success. A link from the parent to the child
  // is created automatically.
  int  addNode(const std::string & INPUT, const std::string & INPUT);


  /// Return number of direct children of the node. 
  int  getChildCount(const std::string & INPUT);

  /// Return 1 if link is enabled for drawing, 0 if not. 
  int  getDrawLink(const std::string & INPUT, const std::string & INPUT);

  /// Return 1 if node is enabled for drawing, 0 if not. 
  int  getDrawNode(const std::string & INPUT);

  /// Return the number of enabled non-tree incoming links for the
  // node. The count does not include the direct parent.
  int  getIncomingCount(const std::string & INPUT);

  /// Return the number of enabled non-tree incoming links for the
  // node. The count does not include the direct children.
  int  getOutgoingCount(const std::string & INPUT);

  /// Reset the color of the link to the global defaults, which can be
  // set using setColorLinkTo() and setColorLinkFrom(). Useful after using
  // setColorLink.
  int  resetColorLink(const std::string & INPUT, const std::string & INPUT);

  /// Change policy used to select link colors. The HV_LINKLOCAL
  // setting uses the colors explicitly set in the link datastructure. 
  // The HV_LINKCENTRAL setting uses the most recently set global link
  // color (as set by setColorLinkTo and setColorLinkFrom). The
  // HV_LINKINHERIT setting makes the link be the color of its
  // parent's node.
  // 
  // Default: HV_LINKLOCAL
  void  setLinkPolicy(int s);

  /// Set the color of the group associated with collection i to an
  // RGB value. 
  void setColorGroup(int i, const std::string & INPUT, float r, float g, float b);

  /// Set the color of the link between two nodes to an RGB value. 
  int  setColorLink(const std::string & INPUT, const std::string & INPUT, float r, float g, float b);

  /// Turn on or off the outgoing non-tree links for a node. Does not
  // apply to direct children. The descend flag controls whether the
  // command applies to only the node itself or the entire subtree
  // beneath it. If special value "HV_TOPNODE" can be passed in
  // instead of an identifier the command is applied to the root node
  // of the entire tree. Returns 0 if node does not exist, 1 for
  // success on existing enabled or disabled node.
  int  setDrawBackFrom(const std::string & INPUT, bool on, int descend);


  /// Turn on or off the incoming non-tree links for a node. Does not
  // apply to the direct parent. The descend flag controls whether the
  // command applies to the node itself or the entire subtree beneath
  // it. If special value "HV_TOPNODE" is passed in instead of an
  // identifier the command is applied to the root node of the entire
  // tree. Returns 0 if node does not exist, 1 for success on existing
  // enabled or disabled node.
  int  setDrawBackTo(const std::string & INPUT, bool on, int descend);

  /// Enable or disable drawing the link between two nodes.
  int  setDrawLink(const std::string & INPUT, const std::string & INPUT, bool on);

  /// Enable or disable drawing all links in the entire graph. 
  // 
  // Default: 1
  void setDrawLinks(bool on);

  /// Enable or disable drawing a particular node.
  int  setDrawNode(const std::string & INPUT, bool on);

  /// Enable or disable drawing all nodes in the entire graph. 
  // 
  // Default: 1
  void setDrawNodes(bool on);

  /// Disable drawing parent-child links between nodes which have a
  // negative level number in the input graph when on = 1. This
  // command will have no visible effect if the root node of the graph
  // is at or above level 0. For example, if the root node is at level
  // -1 and its descendants are at level 0, the visual effect of
  // setNegativeHide() will be a forest of trees which are unconnected
  // at the highest level.
  // 
  // Default: 0
  void setNegativeHide(bool on);

  /// Set the group of a node in a collection. The group name is
  // "group", the collection number is "groupkey".
  void setNodeGroup(const std::string & INPUT, int  groupkey, const std::string & INPUT);

  // 
  // GROUP: HypViewer 
  //


  /// Initialization routine for binding specific combinations of
  // mouse events to functions. 
  // 
  // The choices for b (mouse events) are HV_LEFT_CLICK,
  // HV_MIDDLE_CLICK, HV_RIGHT_CLICK, HV_LEFT_DRAG, HV_MIDDLE_DRAG,
  // HV_RIGHT_DRAG, and HV_PASSIVE. Passive is the mode when no mouse
  // button is held down. The other modes are obvious from their
  // names. The choices for c (functions) are HV_PICK, HV_HILITE,
  // HV_TRANS, and HV_ROT.
  // 
  // The translate and motion functions will move the structure in
  // accordance with the incremental mouse motion as reported by the
  // drag or passive events. 
  // The translate motion function will move the structure in 3D
  // hyperbolic space. The visual effect is to change the center of
  // focus. The rotate motion function will spin the structure around
  // the center of the ball in a familiar 3D euclidean transformation.
  // 
  // The pick and highlight functions both act on the node in the 3D
  // scene directly underneath the mouse cursor in the 2D window. The
  // highlight function will change the color of the node when the
  // mouse is directly over it. If the setHiliteCallback() method has
  // been used to assign a highlight callback function, control will
  // be then passed to the application program along with the name of
  // the node and the status of the shift and control keys. The pick
  // function does not trigger any direct action itself, but if the
  // setPickCallback() function has been used to assign a pick callback
  // control will also be passed to the application along with the
  // node name and shift/control key status.
  // 
  // Although the application programmer is free to bind the pick and
  // highlight functions to any mouse combination, a common use is
  // binding an explicit click to the pick to trigger selection and
  // using passive mouse motion for lightweight actions with the
  // highlight callback. 
  void bindCallback(int b, int c);

  /// Request a single refresh. Control is immediately returned to the
  // application program after a single frame is drawn. Not usually
  // recommended for application programmer use: requires
  // understanding of library internals. The redraw command is
  // usually more appropriate. 
  void drawFrame();

  /// Set the idle flag which allows the library to continue drawing
  // as much as possible of the scene during the idle times when the
  // user is not moving the mouse and no animated transitions are
  // occurring. Not usually recommended for application programmer
  // use: requires understanding of library internals. The redraw
  // command is usually more appropriate. 
  void idle(bool on);

  /// Trigger the actual idle callback in the drawing routines. 
  // 
  // Mainly intended for debugging, definitely not recommended for
  // application programmer use.
  void idle();

#ifndef HYPGLUT
  /// Trigger the actual idle callback.
  //
  // Returns 1 if the idle mode should stay on, 0 if the idle processing
  // is done. Not usually recommended for application programmer
  // use: requires understanding of library internals. The redraw
  // command is usually more appropriate.
  int idleCB();
#endif

  /// Inform the library of the current (x,y) position of the mouse
  // continuously during a drag. 
  // 
  // The (x,y) position should be given using the X window coordinate
  // system, where the origin is in the upper left corner of the
  // window. The booleans "shift" and "control" are set to 1 if the
  // respective keyboard keys are depressed, or 0 if they are not.
  // 
  // Intended for use by a window system layer above this class as
  // opposed to the application programmer.
  void motion(int x, int y, int shift, int control);


  /// Inform the library of the current (x,y) position of the mouse
  // during mousedown or mouseup: that is, a click. If the user moves
  // the mouse while the button is down the motion function will be
  // called in between the mousedown invocation of this function and
  // the mouseup invocation. 
  // 
  // The button boolean "b" is set to 0, 1, or 2 to indicate which
  // button is depressed. The state boolean "s" is set to 0 when the
  // button is pressed down and 1 when the button is released.
  // The (x,y) position should be given using the X window coordinate
  // system, where the origin is in the upper left corner of the
  // window. The booleans "shift" and "control" are set to 1 if the
  // respective keyboard keys are depressed, or 0 if they are not.
  // 
  // Intended for use by a window system layer above this class as
  // opposed to the application programmer.
  void mouse(int b, int s, int x, int y, int shift, int control);

  /// Inform the library of the current (x,y) position of the mouse
  // while no mouse buttons are being pressed. 
  // 
  // The (x,y) position should be given using the X window coordinate
  // system, where the origin is in the upper left corner of the
  // window. The booleans "shift" and "control" are set to 1 if the
  // respective keyboard keys are depressed, or 0 if they are not.
  // 
  // Intended for use by a window system layer above this class as
  // opposed to the application programmer.
  void passive(int x, int y, int shift, int control);

  /// Request a single refresh and set the idle flag. Control is
  // returned to the application program after a single frame is
  // drawn, but allow the library to continue drawing as much as
  // possible as soon as the user becomes idle. This command is the
  // preferred way to request a redraw and is in fact equivalent to
  // "drawFrame(); idle(1);". 
  void redraw();

  /// Inform the library that the drawing area has been resized to a
  // new width and height, specified in pixels. Intended for use by a
  // window system layer above this class as opposed to the
  // application programmer.
  void reshape(int w, int h);

  /// Set the highlight callback to a function pointer. Function arguments
  // are the node identifier string and two boolean values indicating
  // whether the shift or control key was held down. See the
  // bindCallback documentation for more details. 
  void setHiliteCallback(void (*fp)(const string &,int,int));

  /// If on = 1, draw text labels so that they begin at the center of
  // a node and stretch to its right. If on = 0, draw text labels so
  // that they begin to the left of a node and end at its center.
  // 
  // Default: 0
  void setLabelToRight(bool on);

  /// Set the pick callback to a function pointer. Function arguments
  // are the node identifier string and two boolean values indicating
  // whether the shift or control key was held down. See the
  // bindCallback() documentation for more details. 
  void setPickCallback(void (*fp)(const string &,int,int));

  /// Set the highlight callback to a function pointer.  Windowing system
  // interface code may call this in HypViewer::FrameEnd().
  void setFrameEndCallback(void (*fp)(int));


  // GROUP: HypData 
  //

  /// Add a new spanning tree policy. Possible policies, in order of
  // priority: HV_SPANKEEP, change nothing (which means result will be
  // depth-first search tree). HV_SPANFOREST, treat the input graph as
  // a forest of subtrees separated by negative level numbers, and do
  // not move nodes between subtrees (see setNegativeHide()).
  // HV_SPANHIER, use hierarchical identifiers to guide choice of main
  // parent. HV_SPANBFS, use breadth-first search to guide choice.
  // HV_SPANLEX, use lexicographic (alphabetical) ordering. Policies
  // must be added one by one. The order they are added is irrelevant,
  // since the above ordering is used to establish priority. 
  // There is no deleteSpanPolicy to change individual policies,
  // instead use clearSpanPolicy to reset and then add the desired
  // policies. 
  void addSpanPolicy(int s);

  /// Reset the spanning tree policy to change nothing. 
  void clearSpanPolicy();

  // GROUP: HypData get

  ///
  struct timeval getDynamicFrameTime();

  ///
  struct timeval getIdleFrameTime();

  ///
  struct timeval getPickFrameTime();

  ///
  /* float getAreaFudge(); */

  ///
  int   getCenterShow();

  ///
  int   getCenterLargest();

  ///
  float getEdgeSize();

  ///
  int   getGenerationNodeLimit();

  ///
  int   getGenerationLinkLimit();

  ///
  float getGotoStepSize();

  ///
  int   getLabels();

  ///
  float getLabelSize();

  ///
  char* getLabelFont();

  ///
  float getLeafRad();

  ///
  /* float getLengthFudge(); */

  ///
  float getMaxLength();

  ///
  int   getMotionCull();

  ///
  int   getNegativeHide();

  ///
  int   getPassiveCull();

  ///
  int   getSphere();

  ///
  int   getSpanPolicy();

  ///
  int   getTossEvents();

  // GROUP: HypData set


  /// Set whether to use the largest node on the previous frame as the
  // center node for the next frame. Intended for debugging. Not
  // recommended for casual use: requires understanding of library
  // internals. 
  // Default: 1
  void setCenterLargest(bool on);

  /// Set whether to draw the current center node used by the drawing
  // algorithm in red instead of its usual color. Intended for
  // debugging.
  // 
  // Default: 0
  void setCenterShow(bool on);

  /// Control how many nodes are drawn with edges. Set the minimum
  // size in pixels of the projected screen are of a node which should
  // be drawn with edges. If the black edges are drawn even when nodes are
  // very small, i.e. only a few pixels, then the user will only see
  // black instead of the color coding of the cube. When the cubes are
  // big, then drawing the edges helps them be perceived as cubes
  // instead of amorphous blobs of color. 
  // 
  // Default: 5
  void setEdgeSize(float s);

  /// Control the number of nodes drawn. Nodes which are within the
  // given number of "hops" or "generations" from the current center
  // will be drawn. When this number is set to a small integer value only
  // a small neighborhood of the graph around the current center will
  // be drawn. Expects integer value >= 1. 
  // 
  // Default: 30 (i.e. draw as much as possible)
  void setGenerationNodeLimit(int s);

  /// Control the number of links drawn. Links which are within the
  // given number of "hops" or "generations" from the current center
  // will be drawn. When this number is set to a small integer value only
  // a small neighborhood of the graph around the current center will
  // be drawn. Expects integer value >= 1.
  // 
  // Default: 30 (i.e. draw as much as possible)
  void setGenerationLinkLimit(int s);

  /// Set the number of steps to take during an animated transition. 
  // Expects float between 0 and 1: 1.0 / s is the number of steps. 
  // 
  // Default: .05 (20 steps)
  void setGotoStepSize(float s);

  /// Set whether to keep the aspect ratio of the scene square or
  // allow it to change depending on the shape of the window. 
  // 
  // Default: 0
  void setKeepAspect(bool on);

  /// Set the text label mode to: HV_LABELNONE, draw no labels.
  // HV_LABELSHORT, draw only the last part of a label (everything
  // past the last '\'). HV_LABELLONG, draw the entire label.
  // 
  // Default: HV_LABELLONG
  void setLabels(int on);

  /// Control how many nodes are drawn with text labels. Set the
  // minimum size in pixels of the projected screen area of a node
  // which should be drawn with a text label. The size corresponds to
  // a window 1000 pixels wide. The number is internally scaled
  // properly according to the actual window size so the choice of
  // which labels to draw stays relatively constant as the window is
  // resized. Floating point value should be >= 1.0. At this minimum
  // value all visible nodes will have labels drawn. As the value
  // increases, fewer and fewer labels will be drawn. 
  // 
  // Default: 20
  void setLabelSize(float s);

  /// Set the font used for text labels. 
  // X version: expects X font string. 
  // X default: -*-courier-medium-r-normal--12-*
  // GLUT version: not implemented, you get fixed-width 8x13 bitmap fonts.
  // Windows version: expects font name and integer size
  // Windows default: name "Arial" size 12
#ifdef WIN32
  void setLabelFont(const std::string & INPUT, int sz);
#else
  void setLabelFont(const std::string & INPUT);
#endif


  /// Set the size allocated to a leaf node during the layout process:
  // specifically, the radius its disk. Size is a floating point value
  // in hyperbolic coordinates. Not recommended for casual use,
  // requires some understanding of library internals and hyperbolic
  // geometry.
  // 
  // Default: .3
  void setLeafRad(float s);

  /// Set maximum hyperbolic length of a link. Not recommended for
  // casual use: requires understanding of library internals and
  // hyperbolic geometry.
  // 
  // Default: 2.7
  void setMaxLength(float s);

  /// Set how many events to throw away for every one that is
  //processed when the mouse is moving with a mouse button down:
  //provides a way for the drawing to "keep up with" the mouse.
  // 
  // Default: 5
  void setMotionCull(int s);

  /// Set how many events to throw away for every one that is
  // processed when the mouse is still or moving with no mouse button
  // down: provides a way for the drawing and picking to "keep up
  // with" the mouse.
  // 
  // Default: 5
  void setPassiveCull(int s);

  /// Set whether to draw the "sphere at infinity" which provides
  // a visual boundary of the area in which the structure can be
  // drawn. 
  // 
  // Default: 0
  void setSphere(bool on);

  /// Set whether to throw away user input events which might happen
  // when the library is busy, for instance during animated
  // transitions or when recomputing layouts. If events are not thrown
  // away the user might be confused by a sudden jump when all the
  // mouse events are processed at once.
  // 
  // Default: 0
  void setTossEvents(bool on);


  /// Set amount of time for a single dynamic frame. Dynamic frames
  // happen during user input or animated transitions. The adaptive
  // drawing algorithm will draw as much as possible and then
  // relinquish control after the allotted time, no matter how large
  // the input graph. If the frame time is set to less than a few
  // frames per second the user experience will not be interactive. 30
  // FPS is more than enough for most applications. Use timeval struct
  // to set seconds and microseconds.
  // 
  // Default: 0 sec, 50000 usec (20 FPS)
  void setDynamicFrameTime(struct timeval time);

  /// Set total amount of idle time available for use. When the system
  // is in idle mode, control is passed from the application to the
  // library so that it can draw a single frame. The duration of that
  // frame is set with setDynamicFrameTime(). Single frames continue
  // to be drawn until their aggregate time is greater than the idle
  // time value. Control then returns to the application program until
  // new user or program input occurs. The system immediately switches
  // from idle to dynamic mode when user or program input occurs, even
  // if the idle time is not yet exhausted. The limit on the total
  // idle time exists so that the amount of CPU time devoted to this
  // library can be strictly bounded, even when handling very large
  // graphs. The idle time is diced into single-frame increments in
  // order to guarantee fast interactive response as soon as user
  // input begins. Use timeval struct to set seconds and
  // microseconds. Setting the seconds to 5 and the microseconds to
  // 0 allows 5 seconds of drawing after user input stops. The idle
  // time should usually be an order of magnitude  greater than the
  // dynamic time.  
  // 
  // Default: 1 sec, 0 usec 
  void setIdleFrameTime(struct timeval time);

  /// Set amount of time for a single pick frame. Picking occurs when
  // the user moves the mouse around the window, to discover which
  // node is directly underneath the mouse. Picking must be almost as
  // fast as drawing in order to guarantee interactive performance.
  // The pick time should be the same order of magnitude as the
  // dynamic time, but it is safe to set it a small multiple greater
  // than the dynamic time. Use timeval struct to set seconds and
  // microseconds.
  // 
  // Default: 0 sec, 100000 usec (10 FPS)
  void setPickFrameTime(struct timeval time);


  /// Set window background color to red, green, blue values between 0
  // and 1.
  // 
  // Default: 1,1,1
  void setColorBackground(float r, float g, float b);

  /// Set highlighted node color to red, green, blue values between 0
  // and 1.
  // 
  // Default: 0,1,0
  void setColorHilite(float r, float g, float b);

  /// Set node label text color to red, green, blue values between 0 and 1.
  // This color should contrast with the background color. 
  // 
  // Default: 0,0,0
  void setColorLabel(float r, float g, float b);

  /// Set link color on parent end to red, green, blue values between 0 and 1. 
  // If child end color is different, it will be smoothly
  // interpolated along the link. 
  // 
  // Default: .6, .2, .2
  void setColorLinkFrom(float r, float g, float b);

  /// Set link color on child end to red, green, blue values between 0 and 1. 
  // If parent end color is different, it will be smoothly
  // interpolated along the link.
  // 
  // Default: .2, .2, .6
  void setColorLinkTo(float r, float g, float b);

  /// Set selected node color to red, green, blue values between 0 and 1.
  // 
  // Default: 1,1,0
  void setColorSelect(float r, float g, float b);

  /// Set color of the wireframe sphere at infinity to red, green, blue
  // values between 0 and 1. Usually this color should be slightly, but not
  // overly, different from the background color. The sphere is
  // intended to be much less obtrusive than the nodes and links of
  // the structure. 
  // 
  // Default: .9, .7, .6
  void setColorSphere(float r, float g, float b);

  /// Returns the internal pointer to the HypGraph.  This is currently
  // not very friendly since it breaks encapsulation, but a lot more work
  // needs to be done to make the return const.  You shouldn't use this
  // unless you really know what you're doing.
  //
  HypGraph *getHypGraph()
  {
    return((HypGraph *)this->hypGraph);
  }
  
private:

  // hide this data
  HypGraph   *hypGraph;
  HypViewer  *hypViewer;
  HypData    *hypData;
};


#endif

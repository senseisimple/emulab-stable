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

#include <string>
NAMESPACEHACK

extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
}

#include "HypView.h"

#include "HypData.h"
#include "HypGraph.h"
#include "HypViewer.h"

#ifdef WIN32
  //--------------------------------------------------------------------------
  //                        HypView::HypView(Widget w)                       
  //..........................................................................
  //  
  //--------------------------------------------------------------------------

  HypView::HypView(HDC w, HWND hwin) 
  {
    hypData = new HypData;
    hypViewer = new HypViewer(((HypData*)hypData), w, hwin);
    hypGraph = NULL;
  }
#elif HYPGLX

  //--------------------------------------------------------------------------
  //                        HypView::HypView(Widget w)                       
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  HypView::HypView(Widget w)
  {
    hypData = new HypData;
    hypViewer = new HypViewer(hypData,w);
    hypGraph = (HypGraph *)0;
  }

#else
  //--------------------------------------------------------------------------
  //                            HypView::HypView()
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  HypView::HypView()
  {
    hypData = new HypData;
    hypViewer = new HypViewer(hypData);
    hypGraph = (HypGraph *)0;
  }

#endif  


#ifdef WIN32

  //--------------------------------------------------------------------------
  //                void HypView::afterRealize(HGLRC cx, int w, int h)
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypView::afterRealize(HGLRC cx, int w, int h) {
    ((HypViewer*)hypViewer)->afterRealize(cx,w,h);
  }

  //--------------------------------------------------------------------------
  //                       Widget HDC::getWidget()                     
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  HDC HypView::getWidget()
  {
    return hypViewer->getWidget();
  }

#elif HYPGLX

  //--------------------------------------------------------------------------
  //                void HypView::afterRealize(GLXContext cx)              
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypView::afterRealize(GLXContext cx)
  {
    hypViewer->afterRealize(cx);
  }

  //--------------------------------------------------------------------------
  //                       Widget HypView::getWidget()                     
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  Widget HypView::getWidget()
  {
    return hypViewer->getWidget();
  }
#ifdef XPMSNAP
  //--------------------------------------------------------------------------
  //            bool HypView::XpmSnapshot(const string & fileName)           
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  bool HypView::XpmSnapshot(const string & fileName)
  {
    return(hypViewer->XpmSnapshot(fileName));
  }
#endif //XPMSNAP
#else

  //--------------------------------------------------------------------------
  //                       void HypView::afterRealize()                      
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void HypView::afterRealize()
  {
    hypViewer->afterRealize();
  }
#ifdef XPMSNAP
  //--------------------------------------------------------------------------
  //            bool HypView::XpmSnapshot(const string & fileName)           
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  bool HypView::XpmSnapshot(const string & fileName)
  {
    cerr << "HypView::XpmSnapshot(const string & fileName) not implemented! "
         << "{" << __FILE__ << ":" << __LINE__ << "}" << endl;
    return(false);
  }
#endif //XPMSNAP
#endif  // HYPGLX


//----------------------------------------------------------------------------
//                           HypView::~HypView()                           
//............................................................................
//  
//----------------------------------------------------------------------------
HypView::~HypView()
{
  if (hypViewer)
    delete(hypViewer);
  if (hypGraph)
    delete(hypGraph);
  if (hypData)
    delete(hypData);
}

//----------------------------------------------------------------------------
//                   int HypView::setGraph(string str)                  
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::setGraph(istream & str)
{
  if (!str)
    return 0;

  if (hypGraph) {
    hypGraph->newGraph(str);
  }
  else {
    hypGraph = new HypGraph(hypData,str);
  }

  hypViewer->setGraph(hypGraph);
  HypTransform pos = hypGraph->getInitialPosition();
  hypViewer->setGraphPosition(pos);
  return 1;
}

//----------------------------------------------------------------------------
//        int HypView::initGraph(string & rootId, int rootPriority,        
//                               string & rootGroup)                       
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::initGraph(string & rootId, int rootPriority,
                       string & rootGroup)
{
  if (hypGraph) {
    delete(hypGraph);
    hypGraph = (HypGraph *)0;
  }
  hypGraph = new HypGraph(hypData,rootId,rootPriority,rootGroup);
  hypViewer->setGraph(hypGraph);
  HypTransform pos = hypGraph->getInitialPosition();
  hypViewer->setGraphPosition(pos);
  return 1;
}

//----------------------------------------------------------------------------
//               int HypView::saveGraph(const string & fname)              
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::saveGraph(const string & fname)
{
  if (!hypGraph)
    return 0;
  string str = hypGraph->saveTree();
  int len = str.length();
  if (len > 0) {
    FILE *f = fopen(fname.c_str(), "w");
    fprintf(f, "%s", str.c_str());
    fclose(f);
  }
  return len;
}

//----------------------------------------------------------------------------
// int HypView::setDisableGroup(int groupkey, const string & group, bool on) 
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::setDisableGroup(int groupkey, const string & group, bool on)
{
  if (!hypGraph) return 0;
  string g = group;
  int result = hypGraph->setDisableGroup(groupkey, g, on);
  if (result) {
    hypViewer->newLayout();
  }
  return result;
}

//----------------------------------------------------------------------------
//          void HypView::setSelected(const string & id, bool on)          
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setSelected(const string & id, bool on)
{
  if (!hypGraph)
    return;
  HypNode *n = hypGraph->getNodeFromId(id);
  if (n)
    hypViewer->setSelected(n, on);
  return;
}

//----------------------------------------------------------------------------
//       void HypView::setSelectedSubtree(const string & id, bool on)      
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setSelectedSubtree(const string & id, bool on)
{
  if (!hypGraph)
    return;
  StringArray strs = hypGraph->enumerateSubtree(id, -1);
  for (int i = 0; i < strs.size(); i++) {
    HypNode *n = hypGraph->getNodeFromId(strs[i]);
    hypViewer->setSelected(n, on);
  }
  return;
}

//----------------------------------------------------------------------------
//           char ** HypView::enumerateSubtree(const string & id)          
//............................................................................
//  
//----------------------------------------------------------------------------
char ** HypView::enumerateSubtree(const string & id)
{
  char **urlArray = NULL;
  if (!hypGraph)
    return urlArray;
  StringArray strs = hypGraph->enumerateSubtree(id, -1);
  int num = strs.size();
  if (0 < num) {
    urlArray = new char*[num + 1];
    for (int i = 0; i < num; i++) 
      if (strs[(u_int)i].length() > 0) 
        urlArray[i] = strdup((char *)strs[(u_int)i].c_str());
    urlArray[num] = NULL;
  }
  return urlArray;
}

//----------------------------------------------------------------------------
//                 int HypView::gotoPickPoint(int animate)                 
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::gotoPickPoint(int animate)
{
  if (!hypGraph) return 0;
  return hypViewer->gotoPickPoint(animate);
}

//----------------------------------------------------------------------------
//          int HypView::gotoNode(const string & id, int animate)          
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::gotoNode(const string & id, int animate)
{
  int success = 0;
  if (!hypGraph)
    return 0;
  HypNode *n = hypGraph->getNodeFromId(id);
  if (n && n->getEnabled()) {
    double furthest = n->getFurthest();
    if (furthest > 100) {
      //      fprintf(stderr, "furthest %.04g\n", furthest);
      hypData->tossEvents = 1;
      hypGraph->newLayout(n);
      success = hypViewer->gotoNode(n, HV_JUMP);
    } else {
      success = hypViewer->gotoNode(n, animate);
    }
  }
  return success;
}

//----------------------------------------------------------------------------
//                void HypView::gotoCenterNode(int animate)                
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::gotoCenterNode(int animate)
{
  if (!hypGraph)
    return;
  HypNode *n = hypGraph->getNodeFromIndex(hypData->centerindex);
  if (n && n->getEnabled()) {
    double furthest = n->getFurthest();
    if (furthest > 100) {
      hypGraph->newLayout(n);
      hypViewer->setGraphCenter(n->getIndex());
      hypViewer->gotoNode(n, HV_JUMP);
    } else {
      hypViewer->gotoNode(n, animate);
    }
  }
  return;
}

//----------------------------------------------------------------------------
//            void HypView::setCurrentCenter(const string & id)            
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setCurrentCenter(const string & id)
{
  if (!hypGraph)
    return;
  HypNode *n = hypGraph->getNodeFromId(id);
  if (n && n->getEnabled()) {
    hypViewer->setCurrentCenter(n);
  }
  return;
}

//----------------------------------------------------------------------------
//             void HypView::setGraphCenter(const string & id)             
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setGraphCenter(const string & id)
{
  if (!hypGraph)
    return;
  
  HypNode *n = hypGraph->getNodeFromId(id);
  if (n && n->getEnabled()) {
    hypViewer->setGraphCenter(n->getIndex());
  }
  return;
}

//----------------------------------------------------------------------------
//                       string HypView::getCenter()                       
//............................................................................
//  
//----------------------------------------------------------------------------
string HypView::getCenter()
{
  HypNode *n = hypViewer->getCurrentCenter();
  if (n)
    return n->getId();
  else 
    return string("");
}

//----------------------------------------------------------------------------
//                void HypView::newLayout(const string & id)               
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::newLayout(const string & id)
{
  if (!hypGraph)
    return;
  
  HypNode *n = NULL;
  if (id.length() > 0)
    n = hypGraph->getNodeFromId(id);
  if (! n || ! n->getEnabled()) 
    n = hypGraph->getNodeFromIndex(hypData->centerindex);
  hypGraph->newLayout(n);
  
  return;
}

//----------------------------------------------------------------------------
//    int HypView::flashLink(const string & fromid, const string & toid)   
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::flashLink(const string & fromid, const string & toid)
{
  if (!hypGraph)
    return 0;
  
  HypNode *fromnode = hypGraph->getNodeFromId(fromid);
  HypNode *tonode = hypGraph->getNodeFromId(toid);
  if (fromnode && fromnode->getEnabled() && tonode && tonode->getEnabled()) 
    return hypViewer->flashLink(fromnode, tonode);
  else 
    return 0;
}

//----------------------------------------------------------------------------
//                   void HypView::setKeepAspect(bool on)                  
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setKeepAspect(bool on)
{
  hypData->keepAspect = on;
  hypViewer->camInit();
  return;
}

// hypgraph
//

//----------------------------------------------------------------------------
//     int HypView::addLink(const string & fromid, const string & toid)    
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::addLink(const string & fromid, const string & toid)
{
  if (!hypGraph)
    return 0;
  HypNode *fromnode = hypGraph->getNodeFromId(fromid);
  HypNode *tonode = hypGraph->getNodeFromId(toid);
  if (fromnode && tonode) 
    return hypGraph->addLink(fromnode, tonode);
  else
    return 0;
}

//----------------------------------------------------------------------------
//    int HypView::addNode(const string & parent, const string & child)    
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::addNode(const string & parent, const string & child)
{
  if (!hypGraph) return 0;
  return hypGraph->addNode(parent, child);
}

//----------------------------------------------------------------------------
//           int HypView::setDrawNode(const string & id, bool on)          
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::setDrawNode(const string & id, bool on)
{ 
  if (!hypGraph) return 0;
  HypNode *n = hypGraph->getNodeFromId(id);
  if (n)
    return hypGraph->setDrawNode(n, on);
  else
    return 0;
}

//----------------------------------------------------------------------------
//               int HypView::getDrawNode(const string & id)               
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::getDrawNode(const string & id)
{ 
  if (!hypGraph) return 0;
  HypNode *n = hypGraph->getNodeFromId(id);
  if (n)
    return n->getEnabled();
  else
    return -1;
}

//----------------------------------------------------------------------------
// int HypView::resetColorLink(const string & fromid, const string & toid) 
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::resetColorLink(const string & fromid, const string & toid)
{ 
  if (!hypGraph) return 0;
  HypNode *fromnode = hypGraph->getNodeFromId(fromid);
  HypNode *tonode = hypGraph->getNodeFromId(toid);
  if (fromnode && tonode) 
    return hypGraph->resetColorLink(fromnode, tonode);
  else
    return 0;
}


//----------------------------------------------------------------------------
//  int HypView::setLinkPolicy(int s)
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setLinkPolicy(int s)
{
  hypData->linkPolicy = s;
}

//----------------------------------------------------------------------------
//  int HypView::setColorLink(const string & fromid, const string & toid,  
//                            float r, float g, float b)                   
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::setColorLink(const string & fromid, const string & toid, 
			  float r, float g, float b)
{
  if (!hypGraph)
    return 0;
  HypNode *fromnode = hypGraph->getNodeFromId(fromid);
  HypNode *tonode = hypGraph->getNodeFromId(toid);
  if (fromnode && fromnode->getEnabled() && tonode && tonode->getEnabled()) 
    return hypGraph->setColorLink(fromnode, tonode, r, g, b);
  else
    return 0;
}

//----------------------------------------------------------------------------
//             int HypView::setDrawLink(const string & fromid,             
//                                      const string & toid, bool on)      
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::setDrawLink(const string & fromid,
                         const string & toid, bool on)
{
  if (!hypGraph)
    return 0;
  HypNode *fromnode = hypGraph->getNodeFromId(fromid);
  HypNode *tonode = hypGraph->getNodeFromId(toid);
  if (fromnode && tonode)
    return hypGraph->setDrawLink(fromnode, tonode, on);
  else
    return 0;
}

//----------------------------------------------------------------------------
//   int HypView::getDrawLink(const string & fromid, const string & toid)  
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::getDrawLink(const string & fromid, const string & toid)
{
  if (!hypGraph)
    return 0;
  HypNode *fromnode = hypGraph->getNodeFromId(fromid);
  HypNode *tonode = hypGraph->getNodeFromId(toid);
  if (fromnode && tonode) 
    return hypGraph->getDrawLink(fromnode, tonode);
  else
    return -1;
}

//----------------------------------------------------------------------------
//         void HypView::setColorGroup(int i, const string & group,        
//                                     float r, float g, float b)          
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setColorGroup(int i, const string & group,
                            float r, float g, float b)
{
  if (!hypGraph) return;
  hypGraph->setColorGroup(i, group, r, g, b);
  return;
}

//----------------------------------------------------------------------------
//              int HypView::getChildCount(const string & id)              
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::getChildCount(const string & id)
{
  if (!hypGraph) return 0;
  return hypGraph->getChildCount(id);
}

//----------------------------------------------------------------------------
//             int HypView::getIncomingCount(const string & id)            
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::getIncomingCount(const string & id)
{
  if (!hypGraph) return 0;
  return hypGraph->getIncomingCount(id);
}

//----------------------------------------------------------------------------
//             int HypView::getOutgoingCount(const string & id)            
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::getOutgoingCount(const string & id)
{
  if (!hypGraph) return 0;
  return hypGraph->getOutgoingCount(id);
}

//----------------------------------------------------------------------------
//  int HypView::setDrawBackFrom(const string & id, bool on, int descend)  
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::setDrawBackFrom(const string & id, bool on, int descend)
{
  if (!hypGraph) return 0;
  return hypGraph->setDrawBackFrom(id, on, descend);
}

//----------------------------------------------------------------------------
//   int HypView::setDrawBackTo(const string & id, bool on, int descend)   
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::setDrawBackTo(const string & id, bool on, int descend)
{
  if (!hypGraph) return 0;
  return hypGraph->setDrawBackTo(id, on, descend);
}

//----------------------------------------------------------------------------
//                   void HypView::setDrawNodes(bool on)                   
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setDrawNodes(bool on)
{
  if (!hypGraph) return;
  hypGraph->setDrawNodes(on);
  return;
}

//----------------------------------------------------------------------------
//                   void HypView::setDrawLinks(bool on)                   
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setDrawLinks(bool on)
{
  if (!hypGraph) return;
  hypGraph->setDrawLinks(on);
  return;
}

//----------------------------------------------------------------------------
//                  void HypView::setNegativeHide(bool on)                 
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setNegativeHide(bool on)
{
  if (!hypGraph) return;
  hypGraph->setNegativeHide(on);
  return;
}

//----------------------------------------------------------------------------
//       void HypView::setNodeGroup(const string & id, int  groupkey,      
//                                  const string & group)                  
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setNodeGroup(const string & id, int  groupkey,
                           const string & group)
{
  if (!hypGraph) return;
  hypGraph->setNodeGroup(id, groupkey, group);
  return;
}

// hypviewer
//

//----------------------------------------------------------------------------
//                       void HypView::idle(bool on)                       
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::idle(bool on)
{
  hypViewer->idleFunc(on);
  return;
}

#ifndef HYPGLUT
//----------------------------------------------------------------------------
//                           void HypView::idle()                          
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::idleCB()
{
  return hypViewer->idleCB();
}
#endif

//----------------------------------------------------------------------------
//    void HypView::setPickCallback(void (*fp)(const string &,int,int))    
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setPickCallback(void (*fp)(const string &,int,int))
{
  hypViewer->setPickCallback(fp);
  return;
}

//----------------------------------------------------------------------------
//   void HypView::setHiliteCallback(void (*fp)(const string &,int,int))   
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setHiliteCallback(void (*fp)(const string &,int,int))
{
  hypViewer->setHiliteCallback(fp);
  return;
}

//----------------------------------------------------------------------------
//                        void HypView::drawFrame()                        
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::drawFrame()
{
  if (!hypGraph) return;
  hypViewer->drawFrame();
  return;
}

//----------------------------------------------------------------------------
//                          void HypView::redraw()                         
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::redraw()
{
  if (!hypGraph) return;
  hypViewer->drawFrame(); idle(1);
  return;
}

//----------------------------------------------------------------------------
//                   void HypView::reshape(int w, int h)                   
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::reshape(int w, int h)
{
  hypViewer->reshape(w,h);
  return;
}

//----------------------------------------------------------------------------
//                           void HypView::idle()                          
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::idle()
{
  hypViewer->idle();
  return;
}

//----------------------------------------------------------------------------
// void HypView::mouse(int b, int s, int x, int y, int shift, int control) 
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::mouse(int b, int s, int x, int y, int shift, int control) 
{
  hypViewer->mouse(b,s,x,y,shift,control);
  return;
}

//----------------------------------------------------------------------------
//        void HypView::motion(int x, int y, int shift, int control)       
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::motion(int x, int y, int shift, int control) 
{
  hypViewer->motion(x,y,shift,control);
  return;
}

//----------------------------------------------------------------------------
//       void HypView::passive(int x, int y, int shift, int control)       
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::passive(int x, int y, int shift, int control) 
{
  hypViewer->passive(x,y,shift,control);
  return;
}

//----------------------------------------------------------------------------
//                 void HypView::bindCallback(int b, int c)                
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::bindCallback(int b, int c)
{
  hypViewer->bindCallback(b,c);
  return;
}

//----------------------------------------------------------------------------
//                  void HypView::setLabelToRight(bool on)                 
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setLabelToRight(bool on)
{
  hypViewer->setLabelToRight(on);
  return;
}


// hypdata
//

//----------------------------------------------------------------------------
//              struct timeval HypView::getDynamicFrameTime()              
//............................................................................
//  
//----------------------------------------------------------------------------
struct timeval HypView::getDynamicFrameTime()
{
  return hypData->dynamictime;
}

//----------------------------------------------------------------------------
//                struct timeval HypView::getIdleFrameTime()               
//............................................................................
//  
//----------------------------------------------------------------------------
struct timeval HypView::getIdleFrameTime()
{
  return hypData->idletime;
}

//----------------------------------------------------------------------------
//                struct timeval HypView::getPickFrameTime()               
//............................................................................
//  
//----------------------------------------------------------------------------
struct timeval HypView::getPickFrameTime()
{
  return hypData->picktime;
}

//----------------------------------------------------------------------------
//                         int HypView::getSphere()                        
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::getSphere()
{
  return hypData->bSphere;
}

//----------------------------------------------------------------------------
//                       float HypView::getEdgeSize()                      
//............................................................................
//  
//----------------------------------------------------------------------------
float HypView::getEdgeSize()
{
  return hypData->edgesize;
}

//----------------------------------------------------------------------------
//                         int HypView::getLabels()                        
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::getLabels()
{
  return hypData->labels;
}

//----------------------------------------------------------------------------
//                      float HypView::getLabelSize()                      
//............................................................................
//  
//----------------------------------------------------------------------------
float HypView::getLabelSize()
{
  return hypData->labelsize;
}

//----------------------------------------------------------------------------
//                      char* HypView::getLabelFont()                      
//............................................................................
//  
//----------------------------------------------------------------------------
char* HypView::getLabelFont()
{
  return hypData->labelfont;
}

//----------------------------------------------------------------------------
//                      int HypView::getPassiveCull()                      
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::getPassiveCull()
{
  return hypData->passiveCull;
}

//----------------------------------------------------------------------------
//                       int HypView::getMotionCull()                      
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::getMotionCull()
{
  return hypData->motionCull;
}


//----------------------------------------------------------------------------
//                       float HypView::getLeafRad()                       
//............................................................................
//  
//----------------------------------------------------------------------------
float HypView::getLeafRad()
{
  return hypData->leafrad;
}

//----------------------------------------------------------------------------
//                      float HypView::getMaxLength()                      
//............................................................................
//  
//----------------------------------------------------------------------------
float HypView::getMaxLength()
{
  return hypData->maxlength;
}

//----------------------------------------------------------------------------
//                  int HypView::getGenerationNodeLimit()                  
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::getGenerationNodeLimit()
{
  return hypData->generationNodeLimit;
}

//----------------------------------------------------------------------------
//                  int HypView::getGenerationLinkLimit()                  
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::getGenerationLinkLimit()
{
  return hypData->generationLinkLimit;
}

//----------------------------------------------------------------------------
//                       int HypView::getCenterShow()                      
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::getCenterShow()
{
  return hypData->bCenterShow;
}

//----------------------------------------------------------------------------
//                     int HypView::getCenterLargest()                     
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::getCenterLargest()
{
  return hypData->bCenterLargest;
}

//----------------------------------------------------------------------------
//                      int HypView::getNegativeHide()                     
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::getNegativeHide()
{
  return hypData->bNegativeHide;
}

//----------------------------------------------------------------------------
//                       int HypView::getSpanPolicy()                      
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::getSpanPolicy()
{
  return hypData->spanPolicy;
}

//----------------------------------------------------------------------------
//                     float HypView::getGotoStepSize()                    
//............................................................................
//  
//----------------------------------------------------------------------------
float HypView::getGotoStepSize()
{
  return hypData->gotoStepSize;
}

//----------------------------------------------------------------------------
//                       int HypView::getTossEvents()                      
//............................................................................
//  
//----------------------------------------------------------------------------
int HypView::getTossEvents()
{
  return hypData->tossEvents;
}

//----------------------------------------------------------------------------
//          void HypView::setDynamicFrameTime(struct timeval time)         
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setDynamicFrameTime(struct timeval time)
{
  hypData->dynamictime = time;
  return;
}

//----------------------------------------------------------------------------
//           void HypView::setIdleFrameTime(struct timeval time)           
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setIdleFrameTime(struct timeval time)
{
  hypData->picktime = time;
  return;
}

//----------------------------------------------------------------------------
//           void HypView::setPickFrameTime(struct timeval time)           
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setPickFrameTime(struct timeval time)
{
  hypData->idletime = time;
  return;
}

//----------------------------------------------------------------------------
//                     void HypView::setSphere(bool on)                    
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setSphere(bool on)
{
  hypData->bSphere = on;
  return;
}

//----------------------------------------------------------------------------
//                    void HypView::setEdgeSize(float s)                   
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setEdgeSize(float s)
{
  hypData->edgesize = s;
  return;
}

//----------------------------------------------------------------------------
//                     void HypView::setGroupKey(int i)                    
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setGroupKey(int i)
{
  if (hypData->groupKey != i) {
    hypData->groupKey = i;
  }
  return;
}

//----------------------------------------------------------------------------
//                     void HypView::setLabels(int on)                    
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setLabels(int on)
{
  hypData->labels = on;
  return;
}

//----------------------------------------------------------------------------
//                   void HypView::setLabelSize(float s)                   
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setLabelSize(float s)
{
  hypData->labelsize = s;
  hypViewer->resetLabelSize();
  return;
}

#ifdef WIN32
//----------------------------------------------------------------------------
//               void HypView::setLabelFont(const string & s, int sz)          
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setLabelFont(const string & s, int sz)
{
  hypData->labelfont = strdup(s.c_str());
  hypData->labelfontsize = sz;
  hypViewer->newLabelFont();
  return;
}
#else
//----------------------------------------------------------------------------
//               void HypView::setLabelFont(const string & s)              
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setLabelFont(const string & s)
{
  hypData->labelfont = strdup(s.c_str());
  hypViewer->newLabelFont();
  return;
}
#endif

//----------------------------------------------------------------------------
//                   void HypView::setPassiveCull(int s)                   
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setPassiveCull(int s)
{
  hypData->passiveCull = s;
  return;
}

//----------------------------------------------------------------------------
//                    void HypView::setMotionCull(int s)                   
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setMotionCull(int s)
{
  hypData->motionCull = s;
  return;
}

//----------------------------------------------------------------------------
//                    void HypView::setLeafRad(float s)                    
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setLeafRad(float s)
{
  hypData->leafrad = s;
  return;
}

//----------------------------------------------------------------------------
//                   void HypView::setMaxLength(float s)                   
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setMaxLength(float s)
{
  hypData->maxlength = s;
  return;
}

//----------------------------------------------------------------------------
//               void HypView::setGenerationNodeLimit(int s)               
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setGenerationNodeLimit(int s)
{
  hypData->generationNodeLimit = s;
  return;
}

//----------------------------------------------------------------------------
//               void HypView::setGenerationLinkLimit(int s)               
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setGenerationLinkLimit(int s)
{
  hypData->generationLinkLimit = s;
  return;
}

//----------------------------------------------------------------------------
//                 void HypView::setCenterLargest(bool on)                 
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setCenterLargest(bool on)
{
  hypData->bCenterLargest = on;
  return;
}

//----------------------------------------------------------------------------
//                   void HypView::setCenterShow(bool on)                  
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setCenterShow(bool on)
{
  hypData->bCenterShow = on;
  return;
}

//----------------------------------------------------------------------------
//                  void HypView::setGotoStepSize(float s)                 
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setGotoStepSize(float s)
{
  hypData->gotoStepSize = s;
  return;
}

//----------------------------------------------------------------------------
//                    void HypView::addSpanPolicy(int s)                   
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::addSpanPolicy(int s)
{
  hypData->spanPolicy |= s;
  return;
}

//----------------------------------------------------------------------------
//                     void HypView::clearSpanPolicy()                     
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::clearSpanPolicy()
{
  hypData->spanPolicy = 0;
  return;
}

//----------------------------------------------------------------------------
//                   void HypView::setTossEvents(bool on)                  
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setTossEvents(bool on)
{
  hypData->tossEvents = on;
  return;
}

//----------------------------------------------------------------------------
//       void HypView::setColorBackground(float r, float g, float b)       
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setColorBackground(float r, float g, float b)
{
  hypData->colorBack[0] = r; 
  hypData->colorBack[1] = g; 
  hypData->colorBack[2] = b;
  hypViewer->newBackground();
  return;
}

//----------------------------------------------------------------------------
//         void HypView::setColorSphere(float r, float g, float b)         
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setColorSphere(float r, float g, float b)
{
  hypData->colorSphere[0] = r; 
  hypData->colorSphere[1] = g; 
  hypData->colorSphere[2] = b;
  hypViewer->newSphereColor();
  return;
}

//----------------------------------------------------------------------------
//         void HypView::setColorHilite(float r, float g, float b)         
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setColorHilite(float r, float g, float b)
{
  hypData->colorHilite[0] = r; 
  hypData->colorHilite[1] = g; 
  hypData->colorHilite[2] = b;
  return;
}

//----------------------------------------------------------------------------
//         void HypView::setColorSelect(float r, float g, float b)         
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setColorSelect(float r, float g, float b)
{
  hypData->colorSelect[0] = r; 
  hypData->colorSelect[1] = g; 
  hypData->colorSelect[2] = b;
  return;
}

//----------------------------------------------------------------------------
//          void HypView::setColorLabel(float r, float g, float b)         
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setColorLabel(float r, float g, float b)
{
  hypData->colorLabel[0] = r; 
  hypData->colorLabel[1] = g; 
  hypData->colorLabel[2] = b;
  return;
}

//----------------------------------------------------------------------------
//        void HypView::setColorLinkFrom(float r, float g, float b)        
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setColorLinkFrom(float r, float g, float b)
{
  hypData->colorLinkFrom[0] = r;
  hypData->colorLinkFrom[1] = g;
  hypData->colorLinkFrom[2] = b;
  return;
}

//----------------------------------------------------------------------------
//         void HypView::setColorLinkTo(float r, float g, float b)         
//............................................................................
//  
//----------------------------------------------------------------------------
void HypView::setColorLinkTo(float r, float g, float b)
{
  hypData->colorLinkTo[0] = r; 
  hypData->colorLinkTo[1] = g; 
  hypData->colorLinkTo[2] = b;
  return;
}



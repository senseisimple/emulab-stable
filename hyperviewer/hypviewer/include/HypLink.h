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
#ifndef HYPLINK_H
#define HYPLINK_H

#include <string>


#include "HypPoint.h"
#include "HypNode.h"

class HypNode;

//----------------------------------------------------------------------------
//                              class HypLink                              
//----------------------------------------------------------------------------
//  
//----------------------------------------------------------------------------
class HypLink
{
public:
  //--------------------------------------------------------------------------
  //                                HypLink()                                
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  HypLink()
  {
    ++this->_numObjects;
  }
  
  //--------------------------------------------------------------------------
  //                HypLink(string ParentId, string ChildId);                
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  HypLink(string ParentId, string ChildId);

  //--------------------------------------------------------------------------
  //                                ~HypLink()                               
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  ~HypLink()
  {
    --this->_numObjects;
  }

  void destroy() { this->~HypLink(); } // <-- needed!!

  //--------------------------------------------------------------------------
  //                         bool getSelected() const                        
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  bool getSelected() const
  {
    if ((drawFlags & 0x01) != 0)
      return(true);
    return(false);
  }
  
  //--------------------------------------------------------------------------
  //                        void setSelected(bool on)                        
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setSelected(bool on)
  {
    if (on)
      drawFlags |= 0x01;
    else
      drawFlags &= (~0x01);
    return;
  }

  //--------------------------------------------------------------------------
  //                         bool getDesired() const                         
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  bool getDesired() const
  {
    if ((drawFlags & 0x02) != 0)
      return(true);
    return(false);
  }
  
  //--------------------------------------------------------------------------
  //                         void setDesired(bool on)                        
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setDesired(bool on)
  {
    if (on)
      drawFlags |= 0x02;
    else
      drawFlags &= (~0x02);
    return;
  }
  
  //--------------------------------------------------------------------------
  //                         bool getEnabled() const                         
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  bool getEnabled() const
  {
    if ((drawFlags & 0x04) != 0)
      return(true);
    return(false);
  }
  
  //--------------------------------------------------------------------------
  //                         void setEnabled(bool on)                        
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setEnabled(bool on)
  {
    if (on)
      drawFlags |= 0x04;
    else
      drawFlags &= (~0x04);
  }
  
  //--------------------------------------------------------------------------
  //                          bool getDrawn() const                          
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  bool getDrawn() const
  {
    if ((drawFlags & 0x08) != 0)
      return(true);
    return(false);
  }
  
  //--------------------------------------------------------------------------
  //                          void setDrawn(bool on)                         
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setDrawn(bool on)
  {
    if (on)
      drawFlags |= 0x08;
    else
      drawFlags &= (~0x08);
    return;
  }

  //--------------------------------------------------------------------------
  //                           int getIndex() const                          
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  int getIndex() const
  {
    return index;
  }
  
  //--------------------------------------------------------------------------
  //                           HypNode* getChild()                           
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  HypNode* getChild()
  {
    return child;
  }
  
  //--------------------------------------------------------------------------
  //                           HypNode* getParent()                          
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  HypNode* getParent()
  {
    return parent;
  }

  //--------------------------------------------------------------------------
  //                    const string & getChildId() const                    
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  const string & getChildId() const
  {
    return childId;
  }
  
  //--------------------------------------------------------------------------
  //                    const string & getParentId() const                   
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  const string & getParentId() const
  {
    return parentId;
  }
  
  //--------------------------------------------------------------------------
  //                       const string & getId() const                      
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  const string & getId() const
  {
    return linkId;
  }
  
  //--------------------------------------------------------------------------
  //                         int getPriority() const                         
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  int getPriority() const
  {
    return priority;
  }
  
  //--------------------------------------------------------------------------
  //                       float *getColorFrom() const                       
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  const float *getColorFrom() const
  {
    return colorFrom;
  }
  
  //--------------------------------------------------------------------------
  //                        float *getColorTo() const                        
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  const float *getColorTo() const
  {
    return colorTo;
  }

  //--------------------------------------------------------------------------
  //                      void setId(const string & id)                      
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setId(const string & id)
  {
    linkId = id;
    return;
  }
  
  //--------------------------------------------------------------------------
  //                           void setIndex(int i)                          
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setIndex(int i)
  {
    index = i;
    return;
  }
  
  //--------------------------------------------------------------------------
  //                        void setChild(HypNode *n)                        
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setChild(HypNode *n)
  {
    child = n;
    return;
  }
  
  //--------------------------------------------------------------------------
  //                        void setParent(HypNode *n)                       
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setParent(HypNode *n)
  {
    parent = n;
    return;
  }
  
  //--------------------------------------------------------------------------
  //                         void setPriority(int p)                         
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setPriority(int p)
  {
    priority = p;
    return;
  }

  //--------------------------------------------------------------------------
  //          void setColorFrom(float r, float g, float b, float a)          
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setColorFrom(float r, float g, float b, float a)
  { 
    colorFrom[0] = r;
    colorFrom[1] = g;
    colorFrom[2] = b;
    colorFrom[3] = a;
    return;
  }
  
  //--------------------------------------------------------------------------
  //           void setColorTo(float r, float g, float b, float a)           
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setColorTo(float r, float g, float b, float a)
  { 
    colorTo[0] = r;
    colorTo[1] = g;
    colorTo[2] = b;
    colorTo[3] = a;
    return;
  }
  
  void layout();

  //--------------------------------------------------------------------------
  //                    static unsigned long NumObjects()                    
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  static unsigned long NumObjects()
  {
    return(_numObjects);
  }
  
  HypPoint end;
  HypPoint start;
  HypPoint middle;

private:
  long index;  // link
  unsigned char drawFlags;
  int priority;
  short linewidth;
  float colorFrom[4];
  float colorTo[4];
  HypNode *parent;	// same as mainparent unless backlink
  HypNode *child;  // node
  string childId;
  string parentId;
  string linkId;
  static unsigned long _numObjects;
};

#endif

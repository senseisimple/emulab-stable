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
#ifndef HYPNODE_H
#define HYPNODE_H

#include <vector>


#include "StringArray.h"
#include "HypData.h"
#include "HypLink.h"
#include "HypLinkArray.h"
#include "HypNodeArray.h"
#include "HypTransform.h"

#ifdef WIN32
#include <assert.h>
#endif

class HypNode;
class HypLink;
class HypNodeArray;
class HypNodeDistArray;


///////////////////////////////////////////////////////////////////////////
//
// OkPtrArray classes
//
///////////////////////////////////////////////////////////////////////////


//----------------------------------------------------------------------------
//                              class SortKid                              
//----------------------------------------------------------------------------
//  
//----------------------------------------------------------------------------
class SortKid
{
public: 
  float comparator;
  int index;
  int compare(SortKid* o);

  //--------------------------------------------------------------------------
  //                                SortKid()                                
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  SortKid()
  {
    index = 0; comparator = 0;
  }
  
  //--------------------------------------------------------------------------
  //                                ~SortKid()                               
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  ~SortKid()
  {
  }

  //--------------------------------------------------------------------------
  //                              void destroy()                             
  //..........................................................................
  //  This is not a good way to do things, so currently it's commented out.
  //  - dwm@caida.org
  //--------------------------------------------------------------------------
  //  void destroy()
  //  {
  //    this->~SortKid();
  //  }
               
};

//----------------------------------------------------------------------------
//               class SortKidArray : public vector<SortKid*>              
//----------------------------------------------------------------------------
//  
//----------------------------------------------------------------------------
class SortKidArray : public vector<SortKid*>
{
public:
  ~SortKidArray()
  {
    this->erase(this->begin(),this->end());
  }
protected:
  int compareElements(void* e1, void* e2);
};

//----------------------------------------------------------------------------
//                              class HypNode                              
//----------------------------------------------------------------------------
//  
//----------------------------------------------------------------------------
class HypNode
{
  friend class HypTree;

public: 
  HypNode();
  HypNode(HypData *h, HypNode *par, int lev);
  ~HypNode();
  //  void destroy() { this->~HypNode(); } // <-- needed!!

  // enumerate subtree down to levels
  StringArray enumerateSubtree(int levels);
  // lay out nodes in hyperbolic space
  void layout(int up);	
  // finish layout: save cumulative xforms
  void layoutC(int up);	
  // start layout: initialize top node
  void layoutT();	
  // recursively save yourself and your children in "flat file" format
  string save(int lev);
  // put children, incoming, outgoing in lexicographic order
  void sort();
  void markEnable(int on, int descend);
  void deleteChild(HypNode *n);
  void deleteChildLink(HypLink *l);
  void deleteOutgoing(HypLink *l);
  void setEnabledOutgoing(int on, int descend);
  void setEnabledIncoming(int on, int descend);
  double getFurthest();

  // access functions

  void addChild(HypNode *n) { children.push_back(n); }
  void addParent(HypNode *n) { parent = n; }
  void addParentLink(HypLink *l) { parentlink = l; }
  void addChildLink(HypLink *l) { childlinks.push_back(l); }
  void addOutgoing(HypLink *l) { outgoing.push_back(l); }
  void addIncoming(HypLink *l) { incoming.push_back(l); }

  int compare(HypNode *o);
  int compareDist(HypNode *o);

  //--------------------------------------------------------------------------
  //                    const HypTransform & getC() const                    
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  const HypTransform & getC() const
  {
    return(C);
  }

  //--------------------------------------------------------------------------
  //                        int getChildCount() const                        
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  int getChildCount() const
  {
    return(children.size());
  }
  
  //--------------------------------------------------------------------------
  //                       int getIncomingCount() const                      
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  int getIncomingCount() const
  {
    return(incoming.size());
  }
  
  //--------------------------------------------------------------------------
  //                       int getOutgoingCount() const                      
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  int getOutgoingCount() const
  {
    return(outgoing.size());
  }
  
  //--------------------------------------------------------------------------
  //                      int getEnabledIncoming() const                     
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  int getEnabledIncoming() const
  {
    return(bEnabledIncoming);
  }
  
  //--------------------------------------------------------------------------
  //                      int getEnabledOutgoing() const                     
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  int getEnabledOutgoing() const
  {
    return(bEnabledOutgoing);
  }
  
  //--------------------------------------------------------------------------
  //                           int getLevel() const                          
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  int getLevel() const
  {
    return(Level);
  }
  
  //--------------------------------------------------------------------------
  //                          HypNode * getParent()                          
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  HypNode * getParent()
  {
    return(parent);
  }
  
  //--------------------------------------------------------------------------
  //                       HypNodeArray *getChildren()                       
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  HypNodeArray *getChildren()
  {
    return(&children);
  }

  //--------------------------------------------------------------------------
  //                       HypLink *getChildLink(int i)                      
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  HypLink *getChildLink(int i)
  {
    assert(i < childlinks.size());
    return(childlinks[i]);
  }
  
  //--------------------------------------------------------------------------
  //                         HypLink *getParentLink()                        
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  HypLink *getParentLink()
  {
    return(parentlink);
  }
  
  //--------------------------------------------------------------------------
  //                       HypLink *getOutgoing(int i)                       
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  HypLink *getOutgoing(int i)
  {
    assert(i < outgoing.size());
    return(outgoing[i]);
  }
  
  //--------------------------------------------------------------------------
  //                       HypLink *getIncoming(int i)                       
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  HypLink *getIncoming(int i)
  {
    assert(i < incoming.size());
    return(incoming[i]);
  }

  //--------------------------------------------------------------------------
  //                   const string & getGroup(int i) const                  
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  const string & getGroup(int i) const
  {
    assert(i < thegroups.size());
    return(thegroups[i]);
  }
  
  //--------------------------------------------------------------------------
  //                           int getIndex() const                          
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  int getIndex() const
  {
    return(theindex);
  }
  
  //--------------------------------------------------------------------------
  //                       const string & getId() const                      
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  const string & getId() const
  {
    return(theurl);
  }
  
  //--------------------------------------------------------------------------
  //                   const string & getShortLabel() const                  
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  const string & getShortLabel() const
  {
    return(shortlab);
  }

  //--------------------------------------------------------------------------
  //                   const string & getLongLabel() const                   
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  const string & getLongLabel() const
  {
    return(longlab);
  }

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
  //                       bool getHighlighted() const                       
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  bool getHighlighted() const
  {
    if ((drawFlags & 0x02) != 0)
      return(true);
    return(false);
  }
  
  //--------------------------------------------------------------------------
  //                       void setHighlighted(bool on)                      
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setHighlighted(bool on)
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
  //                        double getDistance() const                       
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  double getDistance() const
  {
    return(dist);
  }
  
  //--------------------------------------------------------------------------
  //                          double getSize() const                         
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  double getSize() const
  {
    return(size);
  }
  
  //--------------------------------------------------------------------------
  //                        int getGeneration() const                        
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  int getGeneration() const
  {
    return(generation);
  }
  
  //--------------------------------------------------------------------------
  //                          HypPoint getLabelPos()                         
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  HypPoint getLabelPos()
  {
    return(label);
  }
  
  //--------------------------------------------------------------------------
  //                         int getPriority() const                         
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  int getPriority() const
  {
    return(priority);
  }

  //--------------------------------------------------------------------------
  //                void setGroup(int i, const string & group)               
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setGroup(int i, const string & group)
  {
    while (thegroups.size() < (i+1)) {
      thegroups.push_back(string("_"));
    }
    thegroups[i] = group;
    return;
  }

  //--------------------------------------------------------------------------
  //                         int getNumGroups() const                        
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  int getNumGroups() const
  {
    return(this->thegroups.size());
  }
  
  //--------------------------------------------------------------------------
  //                      void setId(const string & id)                      
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setId(const string & id)
  {
    theurl = id;
    return;
  }
  
  //--------------------------------------------------------------------------
  //                  void setShortLabel(const string & id)                  
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setShortLabel(const string & id)
  {
    shortlab = id;
    return;
  }
  
  //--------------------------------------------------------------------------
  //                   void setLongLabel(const string & id)                  
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setLongLabel(const string & id)
  {
    longlab = id;
    return;
  }
  
  //--------------------------------------------------------------------------
  //                           void setIndex(int i)                          
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setIndex(int i)
  {
    theindex = i;
    return;
  }
  
  //--------------------------------------------------------------------------
  //                        void setDistance(double d)                       
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setDistance(double d)
  {
    dist = d;
    return;
  }

  //--------------------------------------------------------------------------
  //                          void setSize(double d)                         
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setSize(double d)
  {
    size = d;
    return;
  }
  
  //--------------------------------------------------------------------------
  //                        void setGeneration(int s)                        
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setGeneration(int s)
  {
    generation = s;
    return;
  }

  //--------------------------------------------------------------------------
  //              void setLabelPos(double x, double y, double z)             
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setLabelPos(double x, double y, double z)
  {
    label.x = x;
    label.y = y;
    label.z = z;
    label.w = 1.0;
    return;
  }
  
  //--------------------------------------------------------------------------
  //                           void setLevel(int i)                          
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setLevel(int i)
  {
    Level = i;
    return;
  }

  //--------------------------------------------------------------------------
  //                         void setPriority(int s)                         
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  void setPriority(int s)
  {
    priority = s;
    return;
  }

  //--------------------------------------------------------------------------
  //                          int getProgeny() const                         
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  int getProgeny() const
  {
    return(this->progeny);
  }
      
  //--------------------------------------------------------------------------
  //                    static unsigned long NumObjects()                    
  //..........................................................................
  //  
  //--------------------------------------------------------------------------
  static unsigned long NumObjects()
  {
    return(_numObjects);
  }
  
  unsigned char bEnabledIncoming:1, bEnabledOutgoing:1;

  HypTransform I;		// incremental xform

private:
  HypTransform C;		// cumulative xform
  HypPoint theedge;	// edge to draw from "main" parent
  HypPoint label;	
  double r;		// radius of our sphere (cumulative)
  double phi;		// our latitude on sphere
  double theta;		// our longitude on sphere
  int progeny;		// how many descendants we have (cumulative)
  string theurl;	// URL of this node
  string shortlab;	// short label
  string longlab;	// long label
  int  theindex;	// index of this node (i.e. what findUrl returns)
  vector<string> thegroups;	// group to which this node belongs
  int Level;		// depth in flat file from root 
  int priority;		// spanning tree priority
  unsigned char drawFlags;
  double dist;		// distance from center, changes per frame
  double size;		// projected screen area, changes per frame
  int generation;			     
  HypData *hd;
  HypNode *parent;	// ptr to main parent of node
  HypLink *parentlink;			     
  HypNodeArray children;	// table of longs
  HypLinkArray childlinks;	// table of longs
  HypLinkArray incoming;	// table of longs
  HypLinkArray outgoing;	// table of longs
  static unsigned long _numObjects;
  void insertSorted(SortKidArray *Q, SortKid *current);
  void placeKids(int up, HypNode *base, SortKidArray *sk);
};


#endif

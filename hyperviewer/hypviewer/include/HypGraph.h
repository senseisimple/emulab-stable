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
#ifndef HYPGRAPH_H
#define HYPGRAPH_H

#include <string>
#include <map>


#include "HypNode.h"
#include "HypLink.h"
#include "HypData.h"
#include "HypGroupArray.h"
#include "HypTransform.h"

class HypGraph
{
  friend class HypNode;

public:
  
  // instantiate a HypTree from a string containing the flat file 
  // to use another file, delete current HypTree and create fresh one.
  // (if string is a filename, use contents of the file.) 
  HypGraph(HypData *h, istream & g);

  HypGraph(HypData * h, string & rootId,
           int rootPriority, string & rootGroup);
  
  ~HypGraph();
  
  // go ahead and lay out the tree, after user has had chance to set flags
  void init();
  
  void clearMarks();

  // enumerate URLs in a subtree to depth "levels", returning array of strings
  StringArray enumerateSubtree(string URL, int levels);

  // return whether we deal with non-html media types
  int getMedia() { return doMedia; }

  int getNodeEnabled(string URL);

  // return group of this node
  string getNodeGroup(string URL, int groupkey);

  // return absolute position
  HypTransform getNodePosition(string URL);

  // return position possibly saved in input file
  HypTransform getInitialPosition();

  // return "flat file" string
  string saveTree();
  
  // control nodes and links
  int  addLink(HypNode *fromnode, HypNode *tonode);

  int  addNode(string parentid, string childid);

  int  getDrawLink(HypNode *fromnode, HypNode *tonode);

  int  setDrawBackFrom(string URL, int on, int descend);

  int  setDrawBackTo(string URL, int on, int descend);

  void setDrawNodes(int on); // turn on/off all nodes

  void setDrawLinks(int on); // turn on/off all links

  int  setDrawNode(HypNode *n, int on);

  int  setDrawLink(HypNode *fromnode, HypNode *tonode, int on);

  void setNegativeHide(int on);

  int  resetColorLink(HypNode *fromnode, HypNode *tonode);

  int  setColorLink(HypNode *fromnode, HypNode *tonode,
                    float r, float g, float b);
  
  // ht needs to be told about groups so expanded node is
  // assigned to proper group. return 1 is URL is valid.
  int setNodeGroup(string URL, int groupkey, string group);

  int setKey(string URL, string group);

  void setColorGroup(int groupkey, string group, float r, float g, float b);

  int setDisableGroup(int groupkey, string group, int on);

  float *getColorGroup(int groupkey, string group);
  float *getColorNode(HypNode *n);
  
  bool getNodeExists(string URL)
  {
    return((findUrl(URL) < 0) ? false : true);
  }
  
  HypNode *getNodeFromIndex(int i)
  { 
    return ((nodes.size() <= i) ? (HypNode*)NULL : nodes[i]);
  }
  
  HypLink *getLinkFromIndex(int i)
  { 
    return((links.size() <= i) ? (HypLink*)NULL : links[i]);
  }
  
  HypNode *getNodeFromId(string id)
  {
    int i=findUrl(id); return((i>=0)? nodes[i] : (HypNode*)NULL);
  }
  
  HypLink *getLinkFromId(string id)
  {
    int i=findLink(id); return((i>=0)? links[i] : (HypLink*)NULL);
  }

  const map<string,int,less<string> > & NodeTable() const
  {
    return(this->nodetable);
  }

  map<string,int,less<string> > & NodeTable()
  {
    return(this->nodetable);
  }

  const HypNodeArray & NodeArray() const
  {
    return(this->nodes);
  }
  
  HypNodeArray & NodeArray()
  {
    return(this->nodes);
  }

  HypLinkArray & LinkArray()
  {
    return(this->links);
  }
  
  int getChildCount(const string & id);
  int getIncomingCount(const string & id);
  int getOutgoingCount(const string & id);
  
  void doLayout(HypNode *n);
  void newLayout(HypNode *n);
  void newGraph(istream & flat);

  void newGraph(string & rootId, int rootPriority,
                string & rootGroup);
  
protected:
  
  // main access function, index into nodes. return -1 if not found
  int findUrl(string URL) const; 
  int findLink(string id) const; 
  
  // add this node to our flat array
  void newNode(HypNode *newnode, string id); 
  // add this node to our flat array
  void newLink(HypLink *newlink, string id); 
  
  
private:

  HypData *hd;		// misc shared data
  HypNode *treeroot;	// current root node of tree
  HypNode *treecenter;	// current center node of tree 
			//  (first Level 0 node in input data)
  HypNodeArray nodes;	// flat array of nodes (values stored here)
  HypLinkArray links;	// flat array of links (values stored here)

  float backstopcol[3]; // backstop color if none specified for group

  int debug;
  int doMedia;
  int doExternal;
  int doOrphan;
			// global values for all nodes in tree
			// both of these measured in *hyperbolic* units
  map<string,int,less<string> >  nodetable;
  map<string,int,less<string> > linktable;
  HypTransform initPos;	// optional initial position of tree
  string filename;       // name of tmp transfer file, delete if from dirview

  StringArray disabledGroup[HV_MAXGROUPS];
  HypGroupArray colorGroup[HV_MAXGROUPS];

void sort();
void loadSpanGraph(istream & flat);
void markEnable(HypNode *n);
int maybeSwap(HypNode *current, HypNode *oldpar, string id, int pri);
int maybeAddChild(HypNode *current, string id);
int maybeAddOutgoing(HypNode *current, string id);
HypNode *doAddNode(HypNode *current, string id, int level, int pri);

};


#endif

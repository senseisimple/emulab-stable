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

#ifdef WIN32
#include <stdio.h>
#else
extern "C" {
#include <stdlib.h>
#include <stdio.h>
}
#endif

#include <string>
NAMESPACEHACK


#include "HypGraph.h"
#include "HypData.h"

#ifdef WIN32
#include <algorithm>
#else
#include <algo.h>
#endif //WIN32

///////////////////////////////////////////////////////////////////////////
//
// HypGraph class
//
///////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
//                    int HypGraph::findUrl(string URL) const
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::findUrl(string URL) const
{
  map<string,int,less<string> >::const_iterator  nodeMapIter;
  
  nodeMapIter = nodetable.find(URL);
  if (nodeMapIter != nodetable.end()) {
    return(nodes[(*nodeMapIter).second]->getIndex());
  }
  return(-1);
}

//----------------------------------------------------------------------------
//                    int HypGraph::findLink(string id) const
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::findLink(string id) const
{
  map<string,int,less<string> >::const_iterator  linkMapIter;
  
  linkMapIter = linktable.find(id);
  if (linkMapIter != linktable.end())
    return(links[(*linkMapIter).second]->getIndex());
  return(-1);
}

//----------------------------------------------------------------------------
//           void HypGraph::newNode(HypNode *newnode, string id)           
//............................................................................
//  
//----------------------------------------------------------------------------
void HypGraph::newNode(HypNode *newnode, string id)
{
  int curlen = nodes.size();
  nodetable[id] = curlen;
  nodes.push_back(newnode);
  newnode->setIndex(curlen);
  newnode->setId(id);
  string longlab, shortlab;
  int pos = 0;
  longlab = id;
  int count = 0;
  while (count < 3 && pos < id.length()) {
    pos = id.find('/',pos+1);
    count++;
  }
  if (pos >= id.length()-1)
    pos = 0;
  longlab = id.substr(pos, id.length()-pos);
  pos = longlab.length();
  int newpos = longlab.rfind('/', pos);
  if (newpos = pos-1) // ends in slash, go back one more
    newpos = longlab.rfind('/', newpos);
  if (newpos <= 0)
    shortlab = longlab;
  else
    shortlab = longlab.substr(newpos, pos-newpos);
  newnode->setShortLabel(shortlab);
  newnode->setLongLabel(longlab);
}

//----------------------------------------------------------------------------
//           void HypGraph::newLink(HypLink *newlink, string id)           
//............................................................................
//  
//----------------------------------------------------------------------------
void HypGraph::newLink(HypLink *newlink, string id)
{
  int curlen = links.size();
  linktable[id] = curlen;
  //  linktable->insert(curlen, (char*)id.c_str());
  links.push_back(newlink);
  newlink->setIndex(curlen);
  newlink->setId(id);
}

  
////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
//                 HypGraph::HypGraph(HypData *h, istream & g)
//............................................................................
//  
//----------------------------------------------------------------------------
HypGraph::HypGraph(HypData *h, istream & g)
{
  if (getenv("LINKVIEW_DEBUG") != (char *)0) {
    debug = atoi(getenv("LINKVIEW_DEBUG"));
  }
  else {
    debug = 0;
  }
  
  hd = h;
  doMedia = 1;
  doExternal = 1;
  doOrphan = 1;
  newGraph(g);
  backstopcol[0] = .7;
  backstopcol[1] = .7;
  backstopcol[2] = .7;
}

HypGraph::HypGraph(HypData * h, string & rootId,
                   int rootPriority, string & rootGroup)
{
  if (getenv("LINKVIEW_DEBUG") != (char *)0) {
    debug = atoi(getenv("LINKVIEW_DEBUG"));
  }
  else {
    debug = 0;
  }

  hd = h;
  doMedia = 1;
  doExternal = 1;
  doOrphan = 1;
  newGraph(rootId,rootPriority,rootGroup);
  backstopcol[0] = .7;
  backstopcol[1] = .7;
  backstopcol[2] = .7;
}

//----------------------------------------------------------------------------
//                          HypGraph::~HypGraph()                          
//............................................................................
//  
//----------------------------------------------------------------------------
HypGraph::~HypGraph()
{
  int i;
  for ( i = 0; i < nodes.size(); i++)
    delete nodes[i];
  nodes.erase(nodes.begin(),nodes.end());
  
  for ( i = 0; i < links.size(); i++)
    delete links[i];
  links.erase(links.begin(),links.end());
}

//----------------------------------------------------------------------------
//                       void HypGraph::clearMarks()                       
//............................................................................
//  
//----------------------------------------------------------------------------
void HypGraph::clearMarks()
{
  double md = hd->maxdist;
  for (int i = 0; i < nodes.size(); i++) {
    nodes[i]->setDrawn(0);
    nodes[i]->setDistance(md);
  }
}

//----------------------------------------------------------------------------
//      StringArray HypGraph::enumerateSubtree(string URL, int levels)     
//............................................................................
//  
//----------------------------------------------------------------------------
StringArray HypGraph::enumerateSubtree(string URL, int levels)
{
  StringArray strs;
  int i;
  if ((i = findUrl(URL)) >= 0) {
    if (nodes[i]->getEnabled()) {
      HypNode *n = nodes[i];
      int foo = levels;
      StringArray strs2 = n->enumerateSubtree(foo);
      strs.insert(strs.end(),strs2.begin(),strs2.end());
    }
  }
  return strs;
}

//----------------------------------------------------------------------------
//                 int HypGraph::getNodeEnabled(string URL)                
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::getNodeEnabled(string URL)
{
  int i;
  if ((i = findUrl(URL)) >= 0) return nodes[i]->getEnabled();
  return 0; 
}

//----------------------------------------------------------------------------
//               HypTransform HypGraph::getInitialPosition()               
//............................................................................
//  
//----------------------------------------------------------------------------
HypTransform HypGraph::getInitialPosition()
{
  return initPos;
}

//----------------------------------------------------------------------------
//         string HypGraph::getNodeGroup(string URL, int groupkey)         
//............................................................................
//  
//----------------------------------------------------------------------------
string HypGraph::getNodeGroup(string URL, int groupkey)
{
  string str;
  int i;
  if ((i = findUrl(URL)) >= 0) str = nodes[i]->getGroup(groupkey);
  return str;
}

//----------------------------------------------------------------------------
//            HypTransform HypGraph::getNodePosition(string URL)           
//............................................................................
//  
//----------------------------------------------------------------------------
HypTransform HypGraph::getNodePosition(string URL)
{
  int i;
  HypTransform T;
  if ((i = findUrl(URL)) >= 0) {
    HypNode *n = nodes[i];
    T = nodes[i]->getC();
  } else {
    T.identity();
  }
  return T;
}

//----------------------------------------------------------------------------
//                    void HypGraph::newGraph(istream & g)
//............................................................................
//  
//----------------------------------------------------------------------------
void HypGraph::newGraph(istream & g)
{
  // toss old data
  int i;
  if (nodetable.size() > 0)
    nodetable.erase(nodetable.begin(),nodetable.end());
  
  if (linktable.size() > 0)
    linktable.erase(linktable.begin(),linktable.end());
  
  for ( i = 0; i < nodes.size(); i++) {
    delete nodes[i];
  }
  for ( i = 0; i < links.size(); i++) {
    delete links[i];
  }
  nodes.erase(nodes.begin(),nodes.end());
  links.erase(links.begin(),links.end());
  initPos.identity();
  hd->centerindex = -1;
  loadSpanGraph(g);
  treeroot = nodes[0];
  HypNode *treestart =  nodes[hd->centerindex];
  if (!treestart) treestart = treeroot;
  doLayout(treestart);
  if (debug) {
    string savestr = saveTree();
    FILE *f = fopen("/tmp/hypview", "w");
    if (f) {
      fprintf(f, "%s", (char*)savestr.c_str());
      fclose(f);
    }
  }
  return;
}

void HypGraph::newGraph(string & rootId, int rootPriority,
                        string & rootGroup)
{
  // toss old data
  int i;
  if (nodetable.size() > 0)
    nodetable.erase(nodetable.begin(),nodetable.end());
  
  if (linktable.size() > 0)
    linktable.erase(linktable.begin(),linktable.end());
  
  for ( i = 0; i < nodes.size(); i++) {
    delete nodes[i];
  }
  for ( i = 0; i < links.size(); i++) {
    delete links[i];
  }
  nodes.erase(nodes.begin(),nodes.end());
  links.erase(links.begin(),links.end());
  initPos.identity();
  hd->centerindex = -1;

  HypNode *newnode = doAddNode((HypNode *)0,rootId,0,rootPriority);
  
  if (newnode) {
    newnode->setGroup(0,rootGroup);
    hd->centerindex = 0;
  }

  int linkparentindex, linkchildindex;
  for (i = 0; i < links.size(); i++) {
    //    fprintf(stderr, "link %s\n", (char*)links[i]->getId().c_str());
    HypLink *link = links[i];
    string par = link->getParentId();
    linkparentindex = findUrl(par);
    if (linkparentindex >= 0) 
      link->setParent(nodes[linkparentindex]);
    string child = link->getChildId();
    linkchildindex = findUrl(child);
    link->setChild(nodes[linkchildindex]);
  }     
  for (i = 0; i < nodes.size(); i++)
    markEnable(nodes[i]);

  treeroot = nodes[0];
  doLayout(treeroot);
  if (debug) {
    string savestr = saveTree();
    FILE *f = fopen("/tmp/hypview", "w");
    if (f) {
      fprintf(f, "%s", (char*)savestr.c_str());
      fclose(f);
    }
  }
  
  return;
}

//----------------------------------------------------------------------------
//                   void HypGraph::newLayout(HypNode *n)                  
//............................................................................
//  
//----------------------------------------------------------------------------
void HypGraph::newLayout(HypNode *n)
{
  doLayout(n);
}

//----------------------------------------------------------------------------
//                   void HypGraph::doLayout(HypNode *n)                   
//............................................................................
//  
//----------------------------------------------------------------------------
void HypGraph::doLayout(HypNode *n)
{
  int i;
  HypNode *p = n->getParent();
  n->layoutT();		// initialize top node
  n->layout(0);		// lay out nodes in hyperbolic space below n
  n->layoutC(0);	// save cumulative xforms below
  if (p) {
    n->layout(1);		// lay out nodes in hyperbolic space above n
    n->layoutC(1);	// save cumulative xforms above
  }
  for (i = 0; i < links.size(); i++)
    links[i]->layout();
}

//----------------------------------------------------------------------------
//                void HypGraph::loadSpanGraph(istream & flat)                
//............................................................................
//  
//----------------------------------------------------------------------------
void HypGraph::loadSpanGraph(istream & flat)
{
  unsigned       linepos = 0;
  HypNode       *current = NULL;
  HypNode       *descend = NULL;
  string         id, type, mainid, line;
  char           buf[1024];
  string         token;
  StringArray    groups;
  int            level; 
  static string  whitespace = " \t\n";
  int            currlev = -999;
  int            linecount = 0;
  int            groupkey = 0;
  int            i,j;
  int            pri = 0;
  HypNodeArray   parents;

  while (! flat.eof()) {

    // parse line
    linecount++;
    buf[0] = '\0';
    flat.getline(buf,1024,'\n');
    line = string(buf);
    if (line.length() <= 1)
      continue;

    // skip leading whitespace
    linepos = 0;
    linepos = line.find_first_not_of(" \t\n",linepos);
    
    // get the level
    token = line.substr(linepos,line.find(' ',linepos) - linepos);
    linepos += token.length() + 1;
    linepos = line.find_first_not_of(" \t\n",linepos);
    level = atoi(token.c_str());

    // get the id
    if (line.find(' ',linepos) != line.npos)
      id = line.substr(linepos,line.find(' ',linepos) - linepos);
    else
      id = line.substr(linepos,line.length() - linepos);
    
    linepos += id.length() + 1;
    linepos = line.find_first_not_of(" \t\n",linepos);
    
    if (level < -98) {
      if (id == "position") {
	for ( i = 0; i < 4; i++) {
	  for ( j = 0; j < 4; j++) {
            token = line.substr(linepos,line.find(' ',linepos) - linepos);
            initPos.T[i][j] = atof(token.c_str());
            linepos += token.length() + 1;
	    linepos = line.find_first_not_of(" \t\n",linepos);
          }
        }
	continue; 
      } else {
	continue;  // hack: -99 for level means comment. 
      }
    }

    // get the priority
    if (hd->spanPolicy & HV_SPANNUM) {
      token = line.substr(linepos,line.find(' ',linepos) - linepos);
      pri = atoi(token.c_str());
      linepos += token.length() + 1;
      linepos = line.find_first_not_of(" \t\n",linepos);
    }
    
    // enable file value is thrown away since it's obsolete 
    token = line.substr(linepos,line.find(' ',linepos) - linepos);
    linepos += token.length() + 1;
    linepos = line.find_first_not_of(" \t\n",linepos);

    // get the group(s)
    groups.erase(groups.begin(),groups.end());
    groupkey = 0;
    while (linepos < line.length() -1) {
      string g = line.substr(linepos,line.find(' ',linepos) - linepos);
      linepos += g.length() + 1;
      linepos = line.find_first_not_of(" \t\n",linepos);
      groups.push_back(g);
      groupkey++;
    }
    
    if (level > currlev+1) {
      if (currlev < -98) {
        currlev = level;
      }
      else {
        if (current) {
          parents.insert(parents.begin(),current);
          current = descend; // down
          currlev++;
        }
      }
    }
    else {
      if (level <= currlev) {
        while (currlev >= level) {
          if (parents.size() > 0) {
            current = parents[0];
            parents.erase(parents.begin());
          }
          currlev--;
        }
      }  // else stay at the same level
    }

    HypNode *newnode = doAddNode(current, id, level, pri);

    if (newnode) {
      for ( i = 0; i < groupkey && i < groups.size(); i++) {
	newnode->setGroup(i, groups[i]);
      }
      descend = newnode;
      if (!current) {
	current = newnode;
	hd->centerindex = 0;
      }
    }
    else {
      int exists = findUrl(id);
      HypNode *oldnode = nodes[exists];
      descend = oldnode;
    }
  }

  int linkparentindex, linkchildindex;
  for (i = 0; i < links.size(); i++) {
    //    fprintf(stderr, "link %s\n", (char*)links[i]->getId().c_str());
    HypLink *link = links[i];
    string par = link->getParentId();
    linkparentindex = findUrl(par);
    if (linkparentindex >= 0) 
      link->setParent(nodes[linkparentindex]);
    string child = link->getChildId();
    linkchildindex = findUrl(child);
    link->setChild(nodes[linkchildindex]);
  }     

  int disabled = 0;
  for (i = 0; !disabled && i < HV_MAXGROUPS; i++) 
    if (disabledGroup[i].size() > 0) disabled = 1;
  for (i = 0; disabled && i < nodes.size(); i++)
    markEnable(nodes[i]);

  return;
}

//----------------------------------------------------------------------------
//                       string HypGraph::saveTree()                       
//............................................................................
//  
//----------------------------------------------------------------------------
string HypGraph::saveTree()
{
  string str;
  int nl = nodes.size();
  int ll = links.size();
  if (debug) fprintf(stderr, "loaded %d nodes, %d links\n", nl, ll);
  //  sort();
  str = treeroot->save(treeroot->getLevel()); 
  return str;
}

//----------------------------------------------------------------------------
//                   void HypGraph::setDrawNodes(int on)                   
//............................................................................
//  
//----------------------------------------------------------------------------
void HypGraph::setDrawNodes(int on)
{
  for (int i = 0; i < nodes.size(); i++)
    nodes[i]->setEnabled(on);
}

//----------------------------------------------------------------------------
//                   void HypGraph::setDrawLinks(int on)                   
//............................................................................
//  
//----------------------------------------------------------------------------
void HypGraph::setDrawLinks(int on)
{
  for (int i = 0; i < links.size(); i++) {
    links[i]->setEnabled(on);
    links[i]->setDesired(on);
  }
}

//----------------------------------------------------------------------------
//                  void HypGraph::setNegativeHide(int on)                 
//............................................................................
//  
//----------------------------------------------------------------------------
void HypGraph::setNegativeHide(int on)
{
  hd->bNegativeHide = on;
  for (int i = 0; i < links.size(); i++) {
    HypNode *par = links[i]->getParent();
    if (par->getLevel() < 0) links[i]->setEnabled(!on);
  }
}
  

//----------------------------------------------------------------------------
//      int HypGraph::setDrawBackFrom(string URL, int on, int descend)     
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::setDrawBackFrom(string URL, int on, int descend)
{
  int i;
  if ((i = findUrl(URL)) >= 0) {
    nodes[i]->setEnabledOutgoing(on, descend);
  } else if (URL == "HV_TOPNODE") {
    nodes[0]->setEnabledOutgoing(on, descend);
  } else {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
//       int HypGraph::setDrawBackTo(string URL, int on, int descend)      
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::setDrawBackTo(string URL, int on, int descend)
{
  int i;
  if ((i = findUrl(URL)) >= 0) {
    nodes[i]->setEnabledIncoming(on, descend);
  } else if (URL == "HV_TOPNODE") {
    nodes[0]->setEnabledIncoming(on, descend);
  } else {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
//    int HypGraph::setNodeGroup(string URL, int groupkey, string group)   
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::setNodeGroup(string URL, int groupkey, string group)
{
  int i;
  if ((i = findUrl(URL)) >= 0) {
    nodes[i]->setGroup(groupkey, group.c_str());
    if (groupkey == hd->groupKey) {
      markEnable(nodes[i]);
    }
    return 1;
  } else {
    return 0;
  }
}

//----------------------------------------------------------------------------
//     int HypGraph::setEnableGroup(int groupkey, string group, int on)    
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::setDisableGroup(int groupkey, string group, int on)
{
  int i;
  int gotit = -1;

  for (i = 0; i < (disabledGroup[groupkey]).size(); i++) {
    if ( (disabledGroup[groupkey])[i] == group) {
      gotit = 1;
      break;
    }
  }
  if ( (gotit < 0 && !on) || (gotit >= 0 && on))
    return 0;
  if (on) {
    disabledGroup[groupkey].push_back(group);
  } else {
    swap(disabledGroup[groupkey][i],disabledGroup[groupkey][0]);
    disabledGroup[groupkey].erase(disabledGroup[groupkey].begin());
  }
  for (i = 0; i < nodes.size(); i++)
    markEnable(nodes[i]);
  return 1;
}

//----------------------------------------------------------------------------
//         void HypGraph::setColorGroup(int groupkey, string group,        
//                                      float r, float g, float b)         
//............................................................................
//  
//----------------------------------------------------------------------------
void HypGraph::setColorGroup(int groupkey, string group,
                             float r, float g, float b)
{
  int gotit = -1;
  for (int i = 0; i < (colorGroup[groupkey]).size(); i++) {
    HypGroup *hg = (colorGroup[groupkey])[i];
    string id = hg->getGroup();
    if (id == group) {
      gotit = 1;
      hg->setColor(r,g,b);
      break;
    }
  }
  if (gotit == -1) {
    HypGroup *hg = new HypGroup();
    hg->setColor(r,g,b);
    hg->setGroup(group);
    colorGroup[groupkey].push_back(hg);
  }
}

//----------------------------------------------------------------------------
//        float *HypGraph::getColorGroup(int groupkey, string group)       
//............................................................................
//  
//----------------------------------------------------------------------------
float *HypGraph::getColorGroup(int groupkey, string group)
{
  HypGroup *hg;
  if (groupkey > (HV_MAXGROUPS - 1))
    return((float *)0);
  
  for (int i = 0; i < (colorGroup[groupkey]).size(); i++) {
    hg = (colorGroup[groupkey])[i];
    string id = hg->getGroup();
    if (id == group) {
      return(hg->getColor());
    }
  }
  return backstopcol;
}

//----------------------------------------------------------------------------
//        float *HypGraph::getColorNode(HypNode *n)
//............................................................................
//  
//----------------------------------------------------------------------------
float *HypGraph::getColorNode(HypNode *n) 
{
  string id = n->getGroup(hd->groupKey);
  return getColorGroup(hd->groupKey, id);
}


//----------------------------------------------------------------------------
//     int HypGraph::resetColorLink(HypNode *fromnode, HypNode *tonode)    
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::resetColorLink(HypNode *fromnode, HypNode *tonode)
{
  string linkid = fromnode->getId() + "|" + tonode->getId(); 
  HypLink *link = getLinkFromId(linkid);
  if (link) {
    link->setColorFrom(hd->colorLinkFrom[0], hd->colorLinkFrom[1], 
		 hd->colorLinkFrom[2], hd->colorLinkFrom[3]);
    link->setColorTo(hd->colorLinkTo[0], hd->colorLinkTo[1], 
		 hd->colorLinkTo[2], hd->colorLinkTo[3]);
    return 1;
  } else return 0;

}

//----------------------------------------------------------------------------
//      int HypGraph::setColorLink(HypNode *fromnode, HypNode *tonode,     
//                                 float r, float g, float b)              
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::setColorLink(HypNode *fromnode, HypNode *tonode, 
                           float r, float g, float b) 
{
  string linkid = fromnode->getId() + "|" + tonode->getId(); 
  HypLink *link = getLinkFromId(linkid);
  if (link) {
    link->setColorFrom(r,g,b,1.0);
    link->setColorTo(r,g,b,1.0);
    return 1;
  } else {
    return 0;
  }
}

//----------------------------------------------------------------------------
//        int HypGraph::addLink(HypNode *fromnode, HypNode *tonode)        
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::addLink(HypNode *fromnode, HypNode *tonode)
{
  HypLink *link;
  string linkid = fromnode->getId() + "|" + tonode->getId(); 
  if ((link = getLinkFromId(linkid)) == NULL) {
    link = new HypLink(fromnode->getId(), tonode->getId());
    newLink(link, linkid);
    link->setColorFrom(hd->colorLinkFrom[0], hd->colorLinkFrom[1], 
		       hd->colorLinkFrom[2], hd->colorLinkFrom[3]);
    link->setColorTo(hd->colorLinkTo[0], hd->colorLinkTo[1], 
		     hd->colorLinkTo[2], hd->colorLinkTo[3]);
    link->setParent(fromnode);
    link->setChild(tonode);
    link->setDesired(1);
    link->setEnabled(1);
    fromnode->addOutgoing(link);
    tonode->addIncoming(link);
    link->layout();
  }
  link->setEnabled(1);
  link->setDesired(1);
  return 1;
}

//----------------------------------------------------------------------------
//  int HypGraph::setDrawLink(HypNode *fromnode, HypNode *tonode, int on)  
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::setDrawLink(HypNode *fromnode, HypNode *tonode, int on)
{
  string linkid = fromnode->getId() + "|" + tonode->getId();
  HypLink *link = getLinkFromId(linkid);
  if (link) {
    link->setEnabled(on);
    link->setDesired(on);
    return 1;
  } else {
    return 0;
  }
}

//----------------------------------------------------------------------------
//      int HypGraph::getDrawLink(HypNode *fromnode, HypNode *tonode)      
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::getDrawLink(HypNode *fromnode, HypNode *tonode)
{
  string linkid = fromnode->getId() + "|" + tonode->getId();
  HypLink *link = getLinkFromId(linkid);
  if (link) {
    return link->getEnabled();
  } else {
    return -1;
  }
}

//----------------------------------------------------------------------------
//              int HypGraph::setDrawNode(HypNode *n, int on)              
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::setDrawNode(HypNode *n, int on)
{
  if (n) {
    n->setEnabled(on);
    return 1;
  } else {
    return 0;
  }
}

//----------------------------------------------------------------------------
//          int HypGraph::addNode(string parentid, string childid)         
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::addNode(string parentid, string childid)
{
  int i;
  if ((i = findUrl(parentid)) >= 0) {
    HypNode *parent = nodes[i];
    int level = parent->getLevel();
    doAddNode(parent, childid, level+1, 0);
    return 1;
  } else {
    return 0;
  }
}


////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////

/* ensure canonical lexicographic ordering */
//----------------------------------------------------------------------------
//                          void HypGraph::sort()                          
//............................................................................
//  
//----------------------------------------------------------------------------
void HypGraph::sort()
{
  for (int i = 0; i < nodes.size(); i++)
    nodes[i]->sort();
}

//----------------------------------------------------------------------------
//         int HypGraph::maybeAddChild(HypNode *current, string id)        
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::maybeAddChild(HypNode *current, string id)
{
  if (id == current->getId()) {
    // fprintf(stderr, "identical parent/child %s\n", (char*)id);
    return 0;
  }
  HypNodeArray *kids = current->getChildren();
  // don't add same child twice 
  // can't use find, it uses compareElements
  //      if ( kids->find(oldnode) != -1 || id == current->getId()) {
  int gotit = -1;
  int i;
  for (i = 0; i < kids->size(); i++) {
    HypNode *thekid = (*kids)[i];
    if (current->getId() == thekid->getId()) {
      gotit = i;
      break;
    }
  }
  if (gotit >= 0) {
    // fprintf(stderr, "already have child %s in %s\n",
    //         (char*)id, (char*)current->getId());
    return 0;
  }
  // else safe to add as child
  return 1;
}

//----------------------------------------------------------------------------
//       int HypGraph::maybeAddOutgoing(HypNode *current, string id)       
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::maybeAddOutgoing(HypNode *current, string id)
{
  // only add if it's not already there
  int nogo = 0;
  int i;
  HypLink *l;
  for (i = 0; i < current->getOutgoingCount(); i++) {
    l = current->getOutgoing(i);
    if (l->getChildId() == id) {
      nogo = 1;
      break;
    }
  }
  if (!nogo) {
    for (i = 0; i < current->getChildCount(); i++) {
      l = current->getChildLink(i);
      if (l->getChildId() == id) {
	nogo = 1;
	break;
      }
    }
  }
  if (nogo) {
    // fprintf(stderr,
    //         "%d nontree: already has outgoing or child %s in %s\n",
    //         linecount,(char*)id, (char*)current->getId());
    return 0;
  } else {
    return 1;
  }
}

//----------------------------------------------------------------------------
//        HypNode *HypGraph::doAddNode(HypNode *current, string id,        
//                                     int level, int pri)                 
//............................................................................
//  
//----------------------------------------------------------------------------
HypNode *HypGraph::doAddNode(HypNode *current, string id,
                             int level, int pri)
{
  string linkid;
  int swap = 0;
  int exists = findUrl(id);
  if (current && exists >= 0) {
    // possibly choose new main parent
    HypNode *oldnode = nodes[exists];
    if (!maybeAddChild(current, id))
      return (HypNode *)0;
    HypNode *oldpar = oldnode->getParent();
    swap = maybeSwap(current, oldpar, id, pri);
    if (1 == swap) {
      /* switch: found better main parent */

      linkid = current->getId() + "|" + id; // parent|child
      HypLink *oldlink = oldnode->getParentLink();
      //      fprintf(stderr, "swap in %s, swap out %s\n", (char*)linkid.c_str(), (char*)oldlink->getId().c_str());
      oldpar->addOutgoing(oldlink);
      oldpar->deleteChildLink(oldlink);
      oldpar->deleteChild(oldnode);
      HypLink *newlink = new HypLink(current->getId(), id);
      newLink(newlink, linkid);
      newlink->setParent(current);
      newlink->setChild(oldnode);
      newlink->setEnabled(1);
      newlink->setDesired(1);
      oldlink->setEnabled(0);
      oldlink->setDesired(0);
      newlink->setColorFrom(hd->colorLinkFrom[0], hd->colorLinkFrom[1], 
			    hd->colorLinkFrom[2], hd->colorLinkFrom[3]);
      newlink->setColorTo(hd->colorLinkTo[0], hd->colorLinkTo[1], 
			  hd->colorLinkTo[2], hd->colorLinkTo[3]);
      current->addChildLink(newlink);
      current->addChild(oldnode);
      oldnode->addParentLink(newlink);
      oldnode->addParent(current);
      oldnode->setLevel(current->getLevel()+1);
      oldnode->addIncoming(oldlink);
    } else {
      // outgoing nontree link
      if (!maybeAddOutgoing(current, id))
        return (HypNode *)0;
      /* mainparent and child nodes might not exist yet, add later */
      linkid = current->getId() + "|" + id; // parent|child
      HypLink *newlink = new HypLink(current->getId(), id);
      newLink(newlink, linkid);
      newlink->setParent(current);
      newlink->setChild(oldnode);
      newlink->setEnabled(0);
      newlink->setColorFrom(hd->colorLinkFrom[0], hd->colorLinkFrom[1], 
			    hd->colorLinkFrom[2], hd->colorLinkFrom[3]);
      newlink->setColorTo(hd->colorLinkTo[0], hd->colorLinkTo[1], 
			  hd->colorLinkTo[2], hd->colorLinkTo[3]);
      //	newlink->setEnabled(1);
      current->addOutgoing(newlink);
      oldnode->addIncoming(newlink);
    }

    return (HypNode *)0;

  } else {
    // child node
    //      int thelev = current ? current->getLevel()+1 : level;
    //      HypNode *newnode = new HypNode(hd, current, thelev);
    HypNode *newnode = new HypNode(hd, current, level);
    newNode(newnode, id);
    newnode->setEnabled(1);
    if (current) {
      linkid = current->getId() + "|" + id; // parent|child
      HypLink *newlink = new HypLink(current->getId(), id);
      newLink(newlink, linkid);
      newlink->setParent(current);
      newlink->setChild(newnode);
      newlink->setEnabled(1);
      newlink->setDesired(1);
      newlink->setColorFrom(hd->colorLinkFrom[0], hd->colorLinkFrom[1], 
			    hd->colorLinkFrom[2], hd->colorLinkFrom[3]);
      newlink->setColorTo(hd->colorLinkTo[0], hd->colorLinkTo[1], 
			  hd->colorLinkTo[2], hd->colorLinkTo[3]);
      current->addChildLink(newlink);
      current->addChild(newnode);
      newnode->addParentLink(newlink);
      newnode->addParent(current);
    }

    return newnode;

  }
}

//----------------------------------------------------------------------------
//                  void HypGraph::markEnable(HypNode *n)                  
//............................................................................
//  
//----------------------------------------------------------------------------
void HypGraph::markEnable(HypNode *n)
{
  StringArray::iterator gotit;
  string thegroup;
  int i;

  for (i = 0; i < n->getNumGroups(); i++) {
    thegroup = n->getGroup(i);
    gotit = find(disabledGroup[i].begin(),disabledGroup[i].end(),thegroup);
    if (gotit != disabledGroup[i].end()) {
      n->markEnable(0,0);
      return;
    }
  }
  n->markEnable(1, 0);
}

//----------------------------------------------------------------------------
//        int HypGraph::maybeSwap(HypNode *current, HypNode *oldpar,       
//                                string id, int pri)                      
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::maybeSwap(HypNode *current, HypNode *oldpar,
                        string id, int pri)
{
  /* possible policies, in order of precedence
     keep - change nothing
     forest - keep swaps within subtrees starting at level 0
     hierarchy - best match on identifier
     bfs - breadth-first search
     lexicographic - alphanumeric ordering
     */
  if (hd->spanPolicy & HV_SPANKEEP) return 0;
  if (!oldpar || !current || id.length() < 1) return 0;
  //  if ((hd->spanPolicy & HV_SPANFOREST) && (oldpar->getLevel() <= 0))
  //    return 0;

  if (hd->spanPolicy & HV_SPANFOREST) {
    HypNode *newcur, *oldcur;
    int newlev, oldlev;
    newcur = current;
    newlev = newcur->getLevel();
    while (newcur && newlev > 0) {
      newcur = newcur->getParent();      
      newlev = newcur->getLevel();
    }
    oldcur = oldpar;
    oldlev = oldcur->getLevel();
    while (oldcur && oldlev > 0) {
      oldcur = oldcur->getParent();      
      oldlev = oldcur->getLevel();
    }
    if (oldcur != newcur) return 0;
  }

  string oldpiece, newpiece, kidpiece;
  int swap = -1;
  if (hd->spanPolicy & HV_SPANNUM) {
    int oldpri;
    HypLink *curparlink = current->getParentLink();
    if (curparlink) {
      oldpri = curparlink->getPriority();
      if (pri > oldpri) {
	swap = 1;
      } else if (pri == oldpri) {
	swap = -1;
      } else swap = -2;
    }
  }
  if (hd->spanPolicy & HV_SPANHIER) {
    unsigned oldpos, newpos, kidpos;
    oldpos = newpos = kidpos = 0;
    unsigned oldplace, newplace, kidplace;
    oldplace = newplace = kidplace = 0;
    string oldid = oldpar->getId();
    string newid = current->getId();
    swap = 0;
    while (!swap) {
      oldplace = oldid.find('/', oldpos);
      newplace = newid.find('/', newpos);
      kidplace = id.find('/', kidpos);
      oldpiece = oldid.substr(oldpos, oldplace-oldpos);
      newpiece = newid.substr(newpos, newplace-newpos);
      kidpiece = id.substr(kidpos, kidplace-kidpos);
      oldpos = oldplace+1;
      newpos = newplace+1;
      kidpos = kidplace+1;
      if (oldpiece == kidpiece) {
	swap = (newpiece == kidpiece) ? 0 : -2;
	if (oldpos == string::npos && kidpos == string::npos) swap = -1;
      } else 
	swap = (newpiece == kidpiece) ? 1 : -1;
    }
  } else {
    oldpiece = oldpar->getId();
    newpiece = current->getId();
  }
  if (hd->spanPolicy & HV_SPANBFS && swap == -1) {
    if (current->getLevel() < oldpar->getLevel()) {
      swap = 1; // new wins
    } else if (current->getLevel() == oldpar->getLevel()) {
      swap = -1; // no change
    } else {
      swap = -2; // old wins
    }
  }
  if (hd->spanPolicy & HV_SPANLEX && swap == -1) {
    if (newpiece < oldpiece) swap = 1;
  }
  HypNode *par;
  if (1 == swap) {
    // make sure we're not introducing a cycle!
    par = current->getParent();
    while (par && swap == 1) {
      if (par->getId() == id) swap = -1;
      par = par->getParent();
    }
  }
  return swap;
}

//----------------------------------------------------------------------------
//              int HypGraph::getChildCount(const string & id)             
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::getChildCount(const string & id)
{
  int count = 0;
  HypNode *n = getNodeFromId(id);
  if (n && n->getEnabled()) {
    for (int i = 0; i < n->getChildCount(); i++) {
      HypLink *l = n->getChildLink(i);
      if (l->getEnabled()) count++;
    }
    return count;
  }
  return 0;
}

//----------------------------------------------------------------------------
//            int HypGraph::getIncomingCount(const string & id)            
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::getIncomingCount(const string & id)
{
  int count = 0;
  HypNode *n = getNodeFromId(id);
  if (n && n->getEnabled()) {
    for (int i = 0; i < n->getIncomingCount(); i++) {
      HypLink *l = n->getIncoming(i);
      HypNode *p = l->getParent();
      HypNode *c = l->getChild();
      if (p->getEnabled() && c->getEnabled())
        count++;
    }
    //    if (id != treeroot->getId())
    //      count++;
    return count;
  }
  return 0;
}

//----------------------------------------------------------------------------
//            int HypGraph::getOutgoingCount(const string & id)            
//............................................................................
//  
//----------------------------------------------------------------------------
int HypGraph::getOutgoingCount(const string & id)
{
  int count = 0;
  HypNode *n = getNodeFromId(id);
  if (n && n->getEnabled()) {
    for (int i = 0; i < n->getOutgoingCount(); i++) {
      HypLink *l = n->getOutgoing(i);
      HypNode *p = l->getParent();
      HypNode *c = l->getChild();
      if (p->getEnabled() && c->getEnabled())
        count++;
    }
    return count;
  }
  return 0;
}

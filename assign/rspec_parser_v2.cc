/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * XML parser for RSPEC version 2.0
 */

#ifdef WITH_XML

#include "common.h"
#include "rspec_parser_v2.h"
#include "rspec_parser.h"

#include <cstdlib>
#include <string>
#include <vector>
#include <xercesc/dom/DOM.hpp>

#include "xstr.h"

XERCES_CPP_NAMESPACE_USE

using namespace std;
using namespace rspec_emulab_extension;

vector<struct link_interface> 
rspec_parser_v2::readLinkInterface (const DOMElement* link, int& ifaceCount)
{
  vector<struct link_interface> rv;
  // We can't use getElementsByTagName here because
  // that returns all interface_refs in all descendants of link
  // which would include any component hops
  // as would be present in a partially-mapped rspec.
  vector<DOMElement*> interfaceRefs 
    = this->getChildrenByName(link, "interface_ref");
  ifaceCount = interfaceRefs.size();
  
  for (int i = 0; i < ifaceCount; i += 2) {
    DOMElement* ref = interfaceRefs[i];
    string sourceId = this->getAttribute(ref, "component_id");
    if (this->rspecType == RSPEC_TYPE_REQ) {
      sourceId = this->getAttribute(ref, "client_id");
    }
    if ((this->ifacesSeen).find(sourceId) == (this->ifacesSeen).end()) {
      cerr << "*** Could not find source interface " << sourceId << endl;
      exit(EXIT_FATAL);
    }

    string destId = "";
    if ((i+1) < ifaceCount) {
      DOMElement* nextRef = interfaceRefs[i+1];
      destId = this->getAttribute(nextRef, "component_id");
      if (this->rspecType == RSPEC_TYPE_REQ) {
	destId = this->getAttribute(nextRef, "client_id");
      }
      if ((this->ifacesSeen).find(destId) == (this->ifacesSeen).end()) {
	cerr << "*** Could not find destination interface " << destId << endl;
	exit(EXIT_FATAL);
      }
    }

    struct link_interface srcIface, dstIface;
    if (this->rspecType == RSPEC_TYPE_ADVT) {
      srcIface.physicalNodeId = this->ifacesSeen[sourceId];
      srcIface.physicalIfaceId = sourceId;
      dstIface.physicalNodeId = this->ifacesSeen[destId];
      dstIface.physicalIfaceId = destId;
    }
    else { // if (this->rspecType == RSPEC_TYPE_REQ)
      srcIface.virtualNodeId = this->ifacesSeen[sourceId];
      srcIface.virtualIfaceId = sourceId;
      dstIface.virtualNodeId = this->ifacesSeen[destId];
      dstIface.virtualIfaceId = destId;
    }
    
    rv.push_back(srcIface);
    rv.push_back(dstIface);
  }
  return rv;
}

// Since this parser is meant to be used by assign,
// and assign doesn't allow for asymmetric properties on a single link,
// this only returns the specified values when there is only one link property
// 
// Returns a link_characteristics element
struct link_characteristics 
rspec_parser_v2 :: readLinkCharacteristics (const DOMElement* link,
					    int& count,
					    int defaultBandwidth,
					    int unlimitedBandwidth)
{
  DOMNodeList* properties = link->getElementsByTagName(XStr("property").x());
  
  string strBw = "", strLat = "", strLoss = "";
  bool hasBandwidth, hasLatency, hasPacketLoss;
  count = properties->getLength();

  //  if (count == 1) {
    DOMElement* property = dynamic_cast<DOMElement*>(properties->item(0));
    strBw = this->getAttribute(property, "capacity", hasBandwidth);
    strLat = this->getAttribute(property, "latency", hasLatency);
    strLoss = this->getAttribute(property, "packet_loss", hasPacketLoss);
    //  }

  int bandwidth = 0, latency = 0;
  float packetLoss = 0.0;
  if (!hasBandwidth)
    bandwidth = defaultBandwidth;
  else if(strBw == "unlimited")
    bandwidth = unlimitedBandwidth;
  else
    bandwidth = atoi(strBw.c_str());
  
  latency = hasLatency ? atoi(strLat.c_str()) : 0 ;
  packetLoss = hasPacketLoss ? atof(strLoss.c_str()) : 0.0;
  
  struct link_characteristics rv = {bandwidth, latency, packetLoss};
  return rv;
}

vector<struct node_type> 
rspec_parser_v2::readNodeTypes (const DOMElement* node,
				int& typeCount,
				int unlimitedSlots)
{
  string defHw = "pc";
  string defSl = "raw-pc";
  bool isSwitch = false;

  vector<struct node_type> types;
  DOMNodeList* sliverTypes 
    = node->getElementsByTagName(XStr("sliver_type").x());
  DOMNodeList* hardwareTypes 
    = node->getElementsByTagName(XStr("hardware_type").x());

  if (hardwareTypes->getLength() == 0) {
    // If there are no sliver types, return an empty vector
    if (sliverTypes->getLength() == 0) {
      typeCount = 0;
      return vector<struct node_type>();
    }

    for (int j = 0; j < sliverTypes->getLength(); j++) {
      DOMElement* slNode = dynamic_cast<DOMElement*>(sliverTypes->item(j));
      string slTypeName = this->getAttribute(slNode, "name");
      string typeName = this->convertType(defHw, slTypeName);
      struct node_type type = {typeName, 1, false};
      types.push_back(type);
    }
  }
  
  for (int i = 0; i < hardwareTypes->getLength(); i++) {
    DOMElement* hardwareNode 
      = dynamic_cast<DOMElement*>(hardwareTypes->item(i));
    
    string hwTypeName = this->getAttribute (hardwareNode, "name");

    string slot = (this->emulabExtensions)->readTypeSlots(hardwareNode);
    // Default number of slots
    if (slot == "") {
      slot = "unlimited";
    }
    int typeSlots 
      = (slot == "unlimited") ? unlimitedSlots : (int)stringToNum(slot);
    bool isStatic = (this->emulabExtensions)->readStaticType(hardwareNode);

    // XXX: If the node is a switch, add it to the list of switches
    // Since switches are treated specially in assign 
    // and it's such a hardcoded &#$*% mess, if a hardware type is "switch"
    // only switch is returned and the sliver types are ignored
    // If someone wants a switch with sliver type openvz or something like that
    // they are out of luck
    if (hwTypeName == "switch") {
      isSwitch = true;
      struct node_type type = {"switch", typeSlots, isStatic};
      types.push_back(type);
      continue;
    }
    
    if (sliverTypes->getLength() == 0) {
      string typeName = this->convertType(hwTypeName, defSl);
      struct node_type type = {typeName, typeSlots, isStatic};
      types.push_back(type);
    }

    for (int j = 0; j < sliverTypes->getLength(); j++) {
      DOMElement* slNode = dynamic_cast<DOMElement*>(sliverTypes->item(j));
      string slTypeName = this->getAttribute(slNode, "name");
      string typeName = this->convertType(hwTypeName, slTypeName);
      
      struct node_type type = {typeName, typeSlots, isStatic};
      types.push_back(type);
    }
  }

  if (isSwitch) {
    this->addSwitch(node);
  }

  typeCount = types.size();
  return types;
}

map< pair<string, string>, pair<string, string> >
rspec_parser_v2::readInterfacesOnNode  (const DOMElement* node, 
					bool& allUnique)
{
  DOMNodeList* ifaces = node->getElementsByTagName(XStr("interface").x());
  map< pair<string, string>, pair<string, string> > fixedInterfaces;
  allUnique = true;
  for (int i = 0; i < ifaces->getLength(); i++) {
    DOMElement* iface = dynamic_cast<DOMElement*>(ifaces->item(i));
    bool hasAttr;
    string nodeId = "";
    string ifaceId = "";
    if (this->rspecType == RSPEC_TYPE_ADVT) {
      nodeId = this->readPhysicalId (node, hasAttr);
      ifaceId = XStr(iface->getAttribute(XStr("component_id").x())).c();
      bool hasShortName = false;
      string shortName 
	= (this->emulabExtensions)->readShortInterfaceName(iface,hasShortName);
      if (hasShortName) {
	(this->shortNames).insert(pair<string,string>(ifaceId, shortName));
      }
    }
    else { //(this->rspecType == RSPEC_TYPE_REQ)
      nodeId = this->readVirtualId (node, hasAttr);
      ifaceId = XStr(iface->getAttribute(XStr("client_id").x())).c();
      bool isFixed = false;
      string fixedTo
	= (this->emulabExtensions)->readFixedInterface(iface, isFixed);
      if (isFixed) {
	fixedInterfaces.insert (make_pair 
				(make_pair(nodeId,ifaceId),
				 make_pair("", fixedTo)));
      }
    }
    allUnique &= ((this->ifacesSeen).insert	
		  (pair<string, string> (ifaceId, nodeId))).second;
  }
  return (fixedInterfaces);
}

// Reads the available tag. In v2, it is mandatory
string rspec_parser_v2::readAvailable (const DOMElement* node, bool& hasTag) 
{
  DOMNodeList* tags = node->getElementsByTagName(XStr("available").x());
  hasTag = (tags->getLength() > 0);
  if (hasTag) {
    DOMElement* availableNode = dynamic_cast<DOMElement*>(tags->item(0));
    return (this->getAttribute(availableNode, "now"));
  }
  return "";
}

vector<struct link_type> 
rspec_parser_v2::readLinkTypes (const DOMElement* link, int& typeCount)
{
  DOMNodeList* linkTypes = link->getElementsByTagName(XStr("link_type").x());
  vector<struct link_type> types;
  for (int i = 0; i < linkTypes->getLength(); i++) {
    DOMElement* tag = dynamic_cast<DOMElement*>(linkTypes->item(i));
    string name = this->getAttribute(tag, "name");

    struct link_type type = {name, name};
    types.push_back(type);
  }
  typeCount = types.size();
  return types;
}

string rspec_parser_v2::readSubnodeOf (const DOMElement* node,
				       bool& isSubnode,
				       int& count)
{
  int cnt = 0;
  string parent = "";
  DOMNodeList* relations = node->getElementsByTagName(XStr("relations").x());
  for (int i = 0, cnt = 0; i < relations->getLength(); i++) {
    DOMElement* relation = dynamic_cast<DOMElement*>(relations->item(0));
    string type = this->getAttribute(node, "type");
    if (type == "parent") {
      bool hasSubnode = false;
      parent = (this->emulabExtensions)->readSubnodeOf(relation, hasSubnode);
      if (hasSubnode) {
	isSubnode = true;
	++cnt;
      }
    }
  }
  count = cnt;
  if (count == 1) {
    return parent;
  }
  return "";
}

vector<struct vclass>
rspec_parser_v2::readVClasses(const DOMElement* tag) 
{
  return ((this->emulabExtensions)->readVClasses(tag));
}

vector<struct type_limit>
rspec_parser_v2::readTypeLimits (const DOMElement* tag, int& count)
{
  return ((this->emulabExtensions)->readTypeLimits(tag, count));
}

vector<struct fd>
rspec_parser_v2::readFeaturesDesires (const DOMElement* tag, int& count)
{
  return ((this->emulabExtensions)->readFeaturesDesires(tag, count));
}

bool rspec_parser_v2::readDisallowTrivialMix (const DOMElement* tag)
{
  return ((this->emulabExtensions)->readDisallowTrivialMix(tag));
}

bool rspec_parser_v2::readUnique (const DOMElement* tag)
{
  return ((this->emulabExtensions)->readUnique(tag));
}

int rspec_parser_v2::readTrivialBandwidth (const DOMElement* tag,
					   bool& hasTrivialBw)
{
  return ((this->emulabExtensions)->readTrivialBandwidth(tag, hasTrivialBw));
}

string rspec_parser_v2::readHintTo (const DOMElement* tag, bool& hasHintTo)
{
  return ((this->emulabExtensions)->readHintTo(tag, hasHintTo));
}

bool rspec_parser_v2::readNoDelay (const DOMElement* tag)
{
  return ((this->emulabExtensions)->readNoDelay(tag));
}

bool rspec_parser_v2::readTrivialOk (const DOMElement* tag)
{
  return ((this->emulabExtensions)->readTrivialOk(tag));
}

bool rspec_parser_v2::readMultiplexOk (const DOMElement* tag)
{
  return ((this->emulabExtensions)->readMultiplexOk(tag));
}

vector<struct policy> 
rspec_parser_v2::readPolicies (const DOMElement* tag, int& count) 
{
  return ((this->emulabExtensions)->readPolicies(tag, count));
}

string rspec_parser_v2::convertType (const string hwType, const string slType) 
{
  return rspec_parser_helper::convertType(hwType, slType);
}

string rspec_parser_v2::convertType (const string type)
{
  return rspec_parser_helper::convertType(type);
}


#endif // #ifdef WITH_XML

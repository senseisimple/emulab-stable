/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * XML parser for RSPEC version 2.0
 */

#ifdef WITH_XML

#include "rspec_parser_v2.h"
#include "rspec_parser.h"

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
  DOMNodeList* properties = link->getElementsByTagName(XStr("property").x());
  
  ifaceCount = properties->getLength();
  if (ifaceCount == 0) {
    ifaceCount = RSPEC_ERROR_BAD_IFACE_COUNT;
    return vector<struct link_interface>();
  }
  
  for (int i = 0; i < ifaceCount; i++) {
    DOMElement* property = dynamic_cast<DOMElement*>(properties->item(i));
    
    string sourceId = this->getAttribute(property, "source_id");
    string destId = this->getAttribute(property, "dest_id");

    // Check if the interfaces have been seen before on nodes
    if ((this->ifacesSeen).find(sourceId) == (this->ifacesSeen).end()) {
      ifaceCount = RSPEC_ERROR_UNSEEN_NODEIFACE_SRC;
      return vector<struct link_interface>();
    }
    if ((this->ifacesSeen).find(destId) == (this->ifacesSeen).end()) {
      ifaceCount = RSPEC_ERROR_UNSEEN_NODEIFACE_DST;
      return vector<struct link_interface>();
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

/* ************* WARNING. Need clarification ***************/
// XXX: 0 or more link properties are allowed. 
// I am returning only the first one if this is the case.
// Perhaps we should return an error instead. This needs to be sorted out

// Returns a link_characteristics element
struct link_characteristics 
rspec_parser_v2 :: readLinkCharacteristics (const DOMElement* link,
					    int& count,
					    int defaultBandwidth,
					    int unlimitedBandwidth)
{
  DOMElement* property 
    = dynamic_cast<DOMElement*> 
    ((link->getElementsByTagName(XStr("property").x()))->item(0));
  
  bool hasBandwidth, hasLatency, hasPacketLoss;
  string strBw = this->getAttribute(property, "capacity", hasBandwidth);
  string strLat = this->getAttribute(property, "latency", hasLatency);
  string strLoss = this->getAttribute(property, "packet_loss", hasPacketLoss);
  
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
  DOMNodeList* sliverTypes 
    = node->getElementsByTagName(XStr("sliver_type").x());
  // There is exactly only sliver type node


  DOMNodeList* hardwareTypes 
    = node->getElementsByTagName(XStr("hardware_type").x());
  vector<struct node_type> types;

  if (hardwareTypes->getLength() == 0) {
    // If there are no sliver types, return an empty vector
    if (sliverTypes->getLength() == 0) {
      typeCount = 0;
      return vector<struct node_type>();
    }
    for (int j = 0; j < sliverTypes->getLength(); j++) {
      DOMElement* slNode = dynamic_cast<DOMElement*>(sliverTypes->item(j));
      string typeName = this->getAttribute(slNode, "name");
      struct node_type type = {typeName, 1, false};
      types.push_back(type);
    }
  }
  
  for (int i = 0; i < hardwareTypes->getLength(); i++) {
    DOMElement* hardwareNode = dynamic_cast<DOMElement*>(hardwareTypes->item(i));
    
    string hwTypeName = this->getAttribute (hardwareNode, "name");
    string slot = (this->emulabExtensions)->readTypeSlots(hardwareNode);
    // Default number of slots
    if (slot == "") {
      slot = "unlimited";
    }
    bool isStatic = (this->emulabExtensions)->readStaticType(hardwareNode);
    
    if (sliverTypes->getLength() == 0) {
      string typeName = hwTypeName;
      int typeSlots 
	= (slot == "unlimited") ? unlimitedSlots : (int)stringToNum(slot);
      struct node_type type = {typeName, typeSlots, isStatic};
      types.push_back(type);
    }

    for (int j = 0; j < sliverTypes->getLength(); j++) {
      DOMElement* slNode = dynamic_cast<DOMElement*>(sliverTypes->item(j));
      string slTypeName = this->getAttribute(slNode, "name");
      
      string typeName;
      typeName.append(hwTypeName);
      typeName.append("@");
      typeName.append(slTypeName);
      
      int typeSlots 
	= (slot == "unlimited") ? unlimitedSlots : (int)stringToNum(slot);
      
      struct node_type type = {typeName, typeSlots, isStatic};
      types.push_back(type);
    }
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
    }
    else { //(this->rspecType == RSPEC_TYPE_REQ)
      nodeId = this->readVirtualId (node, hasAttr);
      ifaceId = XStr(iface->getAttribute(XStr("client_id").x())).c();
      if (iface->hasAttribute(XStr("component_id").x())) {
	bool hasComponentId;
	string componentNodeId = this->readPhysicalId (node, hasComponentId);
	string componentIfaceId = this->getAttribute(iface, "component_id");
	fixedInterfaces.insert (make_pair 
				(make_pair(nodeId,ifaceId),
				 make_pair(componentNodeId,componentIfaceId)));
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

vector<struct type_limit>
rspec_parser_v2::readTypeLimits (const DOMElement* tag, int& count)
{
  return ((this->emulabExtensions)->readTypeLimits(tag, count));
}

#endif // #ifdef WITH_XML

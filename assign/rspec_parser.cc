/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Header for RSPEC parser files
 */

# ifdef WITH_XML

#include "rspec_parser.h"
#include "xmlhelpers.h"

#include <string>
#include <vector>
#include "xstr.h"
#include <xercesc/dom/DOM.hpp>

using namespace rspec_emulab_extension;

rspec_parser :: rspec_parser (int type)
{
  this->emulabExtensions = new emulab_extensions_parser(type);
  this->rspecType = type; 
}

rspec_parser :: ~rspec_parser ()
{
  free(this->emulabExtensions);
}

struct link_interface rspec_parser :: getIface (const DOMElement* tag)
{
  bool exists;
  struct link_interface rv = 
    {
      string(XStr(tag->getAttribute(XStr("virtual_node_id").x())).c()),
      string(XStr(tag->getAttribute(XStr("virtual_interface_id").x())).c()),
      string(XStr(tag->getAttribute(XStr("component_node_uuid").x())).c()),
      string(XStr(tag->getAttribute(XStr("component_interface_id").x())).c())
    };
  return rv;
}

// Returns the component_id. Sets an out parameter to true if an ID is present
string rspec_parser :: readPhysicalId (const DOMElement* tag, 
				       bool& hasComponentId)
{
  return (this->getAttribute(tag, "component_id", hasComponentId));
}

// Returns the client_id Sets an out parameter to true if an ID is present
string rspec_parser :: readVirtualId (const DOMElement* tag, bool& hasClientId)
{
  return (this->getAttribute(tag, "client_id", hasClientId));
}

// Returns the CMID and sets an out parameter to true if an ID is present
string rspec_parser :: readComponentManagerId (const DOMElement* tag, 
					       bool& hasCmId)
{
  return (this->getAttribute(tag, "component_manager_id", hasCmId));
}

string rspec_parser::readVirtualizationType (const DOMElement* tag, 
					     bool& hasVirtualizationType)
{
  return(this->getAttribute(tag, "virtualization_type",
			    hasVirtualizationType));
}

// 

// Returns true if the latitude and longitude tags are present
// Absence of the country tag will be caught by the schema validator
vector<string> rspec_parser :: readLocation (const DOMElement* tag,
					     int& rvLength)
{
  bool hasCountry, hasLatitude, hasLongitude;
  
  string country = this->getAttribute(tag, "country", hasCountry);
  string latitude = this->getAttribute(tag, "latitude", hasLatitude);
  string longitude = this->getAttribute(tag, "longitude", hasLongitude);
  
  rvLength = (hasLatitude && hasLongitude) ? 3 : 1;
  
  vector<string> rv;
  rv.push_back(country);
  rv.push_back(latitude);
  rv.push_back(longitude);
  return rv;
}

// Returns a list of node_type elements
// The out parameter contains the number of elements found
vector<struct node_type> rspec_parser::readNodeTypes (const DOMElement* node,
						      int& typeCount,
						      int unlimitedSlots)
{
  DOMNodeList* nodeTypes = node->getElementsByTagName(XStr("node_type").x());
  vector<struct node_type> types;
  for (int i = 0; i < nodeTypes->getLength(); i++) 
    {
      DOMElement *tag = dynamic_cast<DOMElement*>(nodeTypes->item(i));
      
      string typeName = XStr(tag->getAttribute(XStr("type_name").x())).c();
      
      int typeSlots;
      string slot = XStr(tag->getAttribute(XStr("type_slots").x())).c();
      if (slot == "unlimited")
	typeSlots = unlimitedSlots;
      else 
	typeSlots = (int)stringToNum(slot);
      
      bool isStatic = tag->hasAttribute(XStr("static").x());
      struct node_type type = {typeName, typeSlots, isStatic};
      types.push_back(type);
    }
  typeCount = nodeTypes->getLength();
  return types;
}

// Returns any fixed interfaces which are found
map< pair<string, string>, pair<string, string> >
rspec_parser::readInterfacesOnNode  (const DOMElement* node, 
				     bool& allUnique)
{
  DOMNodeList* ifaces = node->getElementsByTagName(XStr("interface").x());
  map< pair<string, string>, pair<string, string> > fixedInterfaces;
  allUnique = true;
  for (int i = 0; i < ifaces->getLength(); i++)
    {
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
	  string componentNodeId = 
	    this->readPhysicalId (node, hasComponentId);
	  string componentIfaceId = 
	    this->getAttribute(iface, "component_id");
	  fixedInterfaces.insert (make_pair 
				  (make_pair(nodeId,ifaceId),
				   make_pair(componentNodeId,componentIfaceId)));
	}
      }
      allUnique &= ((this->ifacesSeen).insert
		    (pair<string, string>(nodeId, ifaceId))).second;
    }
  return (fixedInterfaces);
}

// Returns a link_characteristics element
struct link_characteristics 
rspec_parser :: readLinkCharacteristics (const DOMElement* link,
					 int& count,
					 int defaultBandwidth,
					 int unlimitedBandwidth)
{
  bool hasBandwidth, hasLatency, hasPacketLoss;
  string strBw = this->readChild(link, "bandwidth", hasBandwidth);
  string strLat = this->readChild(link, "latency", hasLatency);
  string strLoss = this->readChild(link, "packet_loss", hasPacketLoss);
  
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

vector<struct link_interface> 
rspec_parser :: readLinkInterface (const DOMElement* link, int& ifaceCount)
{
  DOMNodeList* ifaceRefs =
    link->getElementsByTagName(XStr("interface_ref").x());
  ifaceCount = ifaceRefs->getLength();
  
  if (ifaceCount != 2) {
    ifaceCount = RSPEC_ERROR_BAD_IFACE_COUNT;
    return vector<struct link_interface>();
  }
  
  struct link_interface srcIface 
    = this->getIface(dynamic_cast<DOMElement*>(ifaceRefs->item(0)));
  struct link_interface dstIface
    = this->getIface(dynamic_cast<DOMElement*>(ifaceRefs->item(1)));
  
  pair<string, string> srcNodeIface;
  pair<string, string> dstNodeIface;
  if (this->rspecType == RSPEC_TYPE_ADVT) {
    srcNodeIface = make_pair(srcIface.physicalNodeId, srcIface.physicalIfaceId);
    dstNodeIface = make_pair(dstIface.physicalNodeId, dstIface.physicalIfaceId);
  }
  else {//(this->rspecType == RSPEC_TYPE_REQ)
    srcNodeIface = make_pair(srcIface.virtualNodeId, srcIface.virtualIfaceId);
    dstNodeIface = make_pair(dstIface.virtualNodeId, dstIface.virtualIfaceId);
  }
	
  vector<struct link_interface> rv;
  // Check if the node-interface pair has been seen before.
  // If it hasn't, it is an error
  if ((this->ifacesSeen).find(srcNodeIface) == (this->ifacesSeen).end()) {
    ifaceCount = RSPEC_ERROR_UNSEEN_NODEIFACE_SRC;
    return rv;
  }
  if ((this->ifacesSeen).find(dstNodeIface) == (this->ifacesSeen).end()) {
    ifaceCount = RSPEC_ERROR_UNSEEN_NODEIFACE_DST;
    return rv;
  }
  
  rv.push_back(srcIface);
  rv.push_back(dstIface);
  return rv;
}


vector<struct link_type> rspec_parser::readLinkTypes (const DOMElement* link,
						      int& typeCount)
{
  DOMNodeList* linkTypes = link->getElementsByTagName(XStr("link_type").x());
  vector<struct link_type> types;
  for (int i = 0; i < linkTypes->getLength(); i++)  {
    DOMElement *tag = dynamic_cast<DOMElement*>(linkTypes->item(i));
    
    string name = XStr(tag->getAttribute(XStr("name").x())).c();
    string typeName = XStr(tag->getAttribute(XStr("type_name").x())).c();
    
    struct link_type type = {name, typeName};
    types.push_back(type);
  }
  typeCount = linkTypes->getLength();
  return types;
}

string rspec_parser :: readSubnodeOf (const DOMElement* tag, bool& isSubnode)
{
  return (this->readChild(tag, "subnode_of", isSubnode));
}

string rspec_parser :: readExclusive (const DOMElement* tag, bool& isExclusive)
{
  return (this->readChild(tag, "exclusive", isExclusive));
}

string rspec_parser :: readAvailable (const DOMElement* tag, bool& isAvailable)
{
  return (this->readChild(tag, "available", isAvailable));
}

vector<struct type_limit> 
rspec_parser::readTypeLimits (const DOMElement* tag, int& count) 
{
  count = 0;
  return vector<struct type_limit>();
}

vector<struct fd> 
rspec_parser::readFeaturesDesires (const DOMElement* tag, int& count) 
{
  count = 0;
  return vector<struct fd>();
}

#endif

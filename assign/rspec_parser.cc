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

// Returns the attribute value and an out paramter if the attribute exists
string rspec_parser :: getAttribute(const DOMElement* tag, 
																		const string attrName,
																		bool& hasAttr)
{
	hasAttr = tag->hasAttribute(XStr(attrName.c_str()).x());
	if (hasAttr)
		return XStr(tag->getAttribute(XStr(attrName.c_str()).x())).c();
	return "";
}

bool rspec_parser :: hasAttribute(const DOMElement* tag, const string attrName)
{
	return (tag->hasAttribute(XStr(attrName).x()));
}

string rspec_parser :: readSubnodeOf (const DOMElement* tag, bool& isSubnode)
{
	isSubnode = hasChildTag(tag, "subnode_of");
	string rv = "";
	if (isSubnode)
		rv = XStr(getChildValue(tag, "subnode_of")).c();
	return rv;
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
		                                          bool& cmId)
{
	return (this->getAttribute(tag, "component_manager_id", cmId));
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
list<struct node_type> rspec_parser::readNodeTypes (const DOMElement* node,
																											int& typeCount)
{
	DOMNodeList* nodeTypes = node->getElementsByTagName(XStr("node_type").x());
	list<struct node_type> types;
	for (int i = 0; i < nodeTypes->getLength(); i++) 
	{
		DOMElement *tag = dynamic_cast<DOMElement*>(nodeTypes->item(i));
		
		string typeName = XStr(tag->getAttribute(XStr("type_name").x())).c();
		
		int typeSlots;
		string slot = XStr(tag->getAttribute(XStr("type_slots").x())).c();
		if (slot.compare("unlimited") == 0)
			typeSlots = 1000;
		else 
			typeSlots = atoi(slot.c_str());
			
		bool isStatic = tag->hasAttribute(XStr("static").x());
		struct node_type type = {typeName, typeSlots, isStatic};
		types.push_back(type);
	}
	typeCount = nodeTypes->getLength();
	return types;
}

int rspec_parser::readInterfacesOnNode (const DOMElement* node, bool& allUnique)
{
	DOMNodeList* ifaces = node->getElementsByTagName(XStr("interface").x());
	allUnique = true;
	for (int i = 0; i < ifaces->getLength(); i++)
	{
		DOMElement* iface = dynamic_cast<DOMElement*>(ifaces->item(i));
		bool hasAttr;
		string nodeId = "";
		string ifaceId = "";
		if (this->rspecType == RSPEC_TYPE_ADVT)
		{
			nodeId = this->readPhysicalId (node, hasAttr);
			ifaceId = XStr(iface->getAttribute(XStr("component_id").x())).c();
		}
		else //(this->rspecType == RSPEC_TYPE_REQ)
		{
			nodeId = this->readVirtualId (node, hasAttr);
			ifaceId = XStr(iface->getAttribute(XStr("client_id").x())).c();
		}
		allUnique &= ((this->ifacesSeen).insert
														(pair<string, string>(nodeId, ifaceId))).second;
	}
	return (ifaces->getLength());
}

// Returns a link_characteristics element
struct link_characteristics rspec_parser :: readLinkCharacteristics 
		                                      (const DOMElement* link,
																					 int defaultBandwidth,
																					 int unlimitedBandwidth,
		                                       int& count)
{
	bool hasBandwidth, hasLatency, hasPacketLoss;
	string strBw = this->getAttribute(link, "bandwidth", hasBandwidth);
	string strLat = this->getAttribute(link, "latency", hasLatency);
	string strLoss = this->getAttribute(link, "packet_loss", hasPacketLoss);
	
	int bandwidth = 0, latency = 0;
	float packetLoss = 0.0;
	if (!hasBandwidth)
		bandwidth = defaultBandwidth;
	else if(strBw == "unlimited")
		// This is the bandwidth used to simulate an unlimited value
		// We don't expect it to change, so it's constant here
		bandwidth = unlimitedBandwidth;
	else
		bandwidth = atoi(strBw.c_str());
	
	latency = hasLatency ? atoi(strLat.c_str()) : 0 ;
	packetLoss = hasPacketLoss ? atof(strLoss.c_str()) : 0.0;
	
	struct link_characteristics rv = {bandwidth, latency, packetLoss};
	return rv;
}

vector<struct link_interface> rspec_parser :: readLinkInterface
							(const DOMElement* link, int& ifaceCount)
{
	DOMNodeList* ifaceRefs =
			         link->getElementsByTagName(XStr("interface_ref").x());
	ifaceCount = ifaceRefs->getLength();
	
	if (ifaceCount != 2) {
		ifaceCount = -1;
		return vector<link_interface>();
	}
	
	struct link_interface srcIface 
							= this->getIface(dynamic_cast<DOMElement*>(ifaceRefs->item(0)));
	struct link_interface dstIface
							= this->getIface(dynamic_cast<DOMElement*>(ifaceRefs->item(1)));
	
	pair<string, string> srcNodeIface;
	pair<string, string> dstNodeIface;
	if (this->rspecType == RSPEC_TYPE_ADVT)
	{
		srcNodeIface = make_pair(srcIface.physicalNodeId, srcIface.physicalIfaceId);
		dstNodeIface = make_pair(dstIface.physicalNodeId, dstIface.physicalIfaceId);
	}
	else // (this->rspecType == RSPEC_TYPE_REQ)
	{
		srcNodeIface = make_pair(srcIface.virtualNodeId, srcIface.virtualIfaceId);
		dstNodeIface = make_pair(dstIface.virtualNodeId, dstIface.virtualIfaceId);
	}
	
	vector<struct link_interface> rv;
	// Check if the node-interface pair has been seen before.
	// If it hasn't, it is an error
	if ((this->ifacesSeen).find(srcNodeIface) == (this->ifacesSeen).end()
					|| (this->ifacesSeen).find(dstNodeIface) == (this->ifacesSeen).end())
	{
		ifaceCount = -1;
		return rv;
	}
	
	rv.push_back(srcIface);
	rv.push_back(dstIface);
	return rv;
}

#endif

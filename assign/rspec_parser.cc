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

#include <string>
#include "xstr.h"
#include <xercesc/dom/DOM.hpp>

// Returns true if the attribute exists and it's value is a non-empty string
string rspec_parser :: getAttribute(const DOMElement* tag, 
									const string attrName,
									bool& hasAttr)
{
	XMLCh* attr = XStr(attrName.c_str()).x();
	attrValue = XStr(tag->getAttribute(attr)).c();
	hasAttr = tag->hasAttribute(attrName) && attrValue != "";
	return attrValue;
}

struct link_interface rspec_parser :: getIface (DOMElement* tag)
{
	struct link_interface rv = 
	{
		string(XStr(tag->getAttribute(XStr("virtual_node_id").x())).c()),
		string(XStr(tag->getAttribute(XStr("virtual_interface_id").x())).c()),
		string(XStr(find_urn(tag, "component_node"))),
		string(XStr(tag->getAttribute(XStr("component_interface_id").x())).c())
	};
	return rv;
}

// Returns true if the attribute exists and its value is a non-empty string
string rspec_parser :: readComponentId (const DOMElement* tag, 
			     							 bool& hasComponentId)
{
	return (this->getAttribute(tag, "component_id", hasComponentId));
}

// Returns true if the attribute exists and its value is a non-empty string
string rspec_parser :: readVirtualId (const DOMElement* tag, bool& hasVirtualId)
{
	return (this->getAttribute(tag, "virtual_id", hasVirtualId));
}

// Returns true if the attribute exists and its value is a non-empty string
string rspec_parser :: readComponentManagerId (const DOMElement* tag, 
		                                          bool& cmId)
{
	return (this->getAttribute(tag, "component_manager_id", cmId));
}

// Returns true if the latitude and longitude tags are present
// Absence of the country tag will be caught by the schema validator
list<string> rspec_parser :: readLocation (DOMElement* tag, int& rvLength)
{
	XMLCh* country_attr = XStr("country").x();
	XMLCh* latitude_attr = XStr("latitude").x();
	XMLCh* longitude_attr = XStr("longitude").x();
	
	country = string(XStr(tag->getAttribute(country_attr).c());
	latitude = string(Xstr(tag->getAttribute(latitude_attr)).c());
	longitude = string(XStr(tag->getAttribute(longitude_attr)).c());
	
	rvLength = ((country == "" ? 0 : 1)
			     + (latitude == "" ? 0 : 1)
			     + (longitude == "" ? 0 : 1));	
	
	return (list<string> (country, latitude, longitude));
}

// Returns a list of node_type elements
// The out parameter contains the number of elements found
list< struct node_type > rspec_parser :: readNodeTypes (DOMElement* node,
						                           int& typeCount)
{
	DOMNodeList* nodeTypes = node->getElementsByTagName(XStr("node_type").x());
	list< node_type > types;
	for (int i = 0; i < types->getLength(); i++) 
	{
		DOMElement *tag = dynamic_cast<DOMElement*>(types->item(i));
		
		string typeName = XStr(tag->getAttribute(XStr("type_name").x())).c();
		
		int typeSlots;
		string slot = XStr(tag->getAttribute(XStr("type_slots").x())).c();
		if (slotString.compare("unlimited") == 0)
			typeSlots = 1000;
		else 
			typeSlots = slotString.i();
			
		bool isStatic = tag->hasAttribute("static");
		node_type type (typeName, typeSlots, isStatic);
		types.push_back(type);
	}
	typeCount = types->getLength();
	return types;
}

// Returns a link_characteristics element
struct link_characteristics rspec_parser :: readLinkCharacteristics 
		                                      (const DOMElement* link,
		                                       int& count)
{
	bool hasBandwidth, hasLatency, hasPacketLoss;
    string strBw = this->getAttribute(link, "bandwidth", hasBandwidth&);
	string strLat = this->getAttribute(link, "latency", hasLatency&);
	string strLoss = this->getAttribute(link, "packet_loss", hasPacketLoss&);
	
	int bandwidth, latency, packetLoss;
	if (!hasLatency)
		bandwidth = DEFAULT_BANDWIDTH;
	else if(strBw == "unlimited")
		// This is the bandwidth used to simulate an unlimited value
		// We don't expect it to change, so it's constant here
		bandwidth = UNLIMITED_BANDWIDTH;
	else
		bandwidth = strBw.i();
	
	latency = (hasLatency ? strLat.i() : 0);
	packetLoss = (hasPacketLoss ? strLoss.d() : 0);
	
	return (link_characteristics(bandwidth, latency, packetLoss));
}

struct list<link_interface> rspec_parser :: readLinkInterface
							(const DOMElement* link, int& ifaceCount)
{
	DOMNodeList* ifaceRefs =
			         link->getElementsByTagName(XStr("interface_ref").x());
	ifaceCount = ifaceRefs->getLength()
	
	if (ifaceCount > 2)
		return NULL;

	srcIface = this->getIface (ifaceRefs->item(0));
	dstIface = this->getIface (ifaceRefs->item(1));
	
	return list<link_interface>(srcIface, dstIface)
}

#endif
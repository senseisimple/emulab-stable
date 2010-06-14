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

vector<struct link_interface> rspec_parser_v2::readLinkInterface
													(const DOMElement* link, 
													 int& ifaceCount)
{
	DOMNodeList* properties = link->getElementsByTagName(XStr("property").x());
	DOMNodeList* interfaces 
			= link->getElementsByTagName(XStr("interface_ref").x());
	ifaceCount = interfaces->getLength();
	
	if (ifaceCount == 0) {
		ifaceCount = RSPEC_ERROR_BAD_IFACE_COUNT;
		return vector<struct link_interface>();
	}
	
	for (int i = 0; i < ifaceCount; i++)
	{
		DOMElement* iface = dynamic_cast<DOMElement*>(interfaces->item(i));
		string ifaceId = this->getAttribute(iface, "client_id");
		
		// Check if the node has been seen before. If not, it is an error
		if ((this->ifacesSeen).find(ifaceId) == (this->ifacesSeen).end()) {
			ifaceCount = RSPEC_ERROR_UNSEEN_NODEIFACE_SRC;
			return vector<struct link_interface>();
		}
	}
	
	vector<struct link_interface> rv;
	for (int i = 0; i < properties->getLength(); i++)
	{
		DOMElement* property = dynamic_cast<DOMElement*>(properties->item(i));
		
		string sourceId = this->getAttribute(property, "source_id");
		string destId = this->getAttribute(property, "dest_id");
		
		struct link_interface srcIface, dstIface;
		
		if (this->rspecType == RSPEC_TYPE_ADVT)
		{
			srcIface.physicalNodeId = this->ifacesSeen[sourceId];
			srcIface.physicalIfaceId = sourceId;
			dstIface.physicalNodeId = this->ifacesSeen[destId];
			dstIface.physicalIfaceId = destId;
		}
		else // if (this->rspecType == RSPEC_TYPE_REQ)
		{
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
struct link_characteristics rspec_parser_v2 :: readLinkCharacteristics 
												(const DOMElement* link,
												 int& count,
												 int defaultBandwidth,
												 int unlimitedBandwidth)
{
	DOMElement* property 
			= dynamic_cast<DOMElement*> 
				((link->getElementsByTagName(XStr("property").x()))->item(0));
	
	bool hasBandwidth, hasLatency, hasPacketLoss;
	string strBw = this->readChild(property, "capacity", hasBandwidth);
	string strLat = this->readChild(property, "latency", hasLatency);
	string strLoss = this->readChild(property, "packet_loss", hasPacketLoss);
	
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

vector<struct node_type> rspec_parser_v2::readNodeTypes 
											(const DOMElement* node,
												int& typeCount,
												int unlimitedSlots)
{
	DOMNodeList* sliverTypes 
					= node->getElementsByTagName(XStr("sliver_type").x());
	DOMNodeList* hardwareTypes 
					= node->getElementsByTagName(XStr("hardware_type").x());
	vector<struct node_type> types;
	
	for (int i = 0; i < sliverTypes->getLength(); i++)
	{
		DOMElement* sliverNode= dynamic_cast<DOMElement*>(sliverTypes->item(i));
		
		string sliverName = this->getAttribute (sliverNode, "name");
		
		// Read type slots and static from an Emulab-specific extension here
		string slot = (this->emulabExtensions)->readTypeSlots(sliverNode);
		bool isStatic = (this->emulabExtensions)->readStaticType(sliverNode);
		/////////////////////////////////////////////
		if (hardwareTypes->getLength() == 0)
		{
			int typeSlots = (slot == "unlimited") ? 
									unlimitedSlots : (int)stringToNum(slot);
			struct node_type type = {sliverName, typeSlots, isStatic};
			types.push_back(type);
		}
		for (int j = 0; j < hardwareTypes->getLength(); j++)
		{
			string hardwareName 
					= this->getAttribute
						(dynamic_cast<DOMElement*>(hardwareTypes->item(j)),
						 "name");
			
			string typeName;
			typeName.assign(sliverName);
			typeName.append("@");
			typeName.append(hardwareName);
			
			int typeSlots = (slot == "unlimited") ? 
					unlimitedSlots : (int)stringToNum(slot);
			
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
			if (iface->hasAttribute(XStr("component_id").x()))
			{
				bool hasComponentId;
				string componentNodeId = 
						this->readPhysicalId (node, hasComponentId);
				string componentIfaceId = 
						this->getAttribute(iface, "component_id");
				fixedInterfaces.insert
						(make_pair 
						(make_pair(nodeId,ifaceId),
						 make_pair(componentNodeId,componentIfaceId)));
			}
		}
		allUnique &= ((this->ifacesSeen).insert	
							(pair<string, string> (ifaceId, nodeId))).second;
	}
	return (fixedInterfaces);
}


#endif // #ifdef WITH_XML

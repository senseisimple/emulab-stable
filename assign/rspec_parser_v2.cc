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
	ifaceCount = properties->getLength();
	
	if (ifaceCount == 0) {
		ifaceCount = RSPEC_ERROR_BAD_IFACE_COUNT;
		return vector<struct link_interface>();
	}
	
	vector<struct link_interface> rv;
	for (int i = 0; i < ifaceCount; i++)
	{
		DOMElement* property = dynamic_cast<DOMElement*>(properties->item(i));
		
		string sourceId = this->getAttribute(property, "source_id");
		string destId = this->getAttribute(property, "dest_id");
		
		struct link_interface srcIface, dstIface;
		
		if (this->rspecType == RSPEC_TYPE_ADVT)
		{
			srcIface.physicalNodeId = sourceId;
			dstIface.physicalNodeId = destId;
		}
		else // if (this->rspecType == RSPEC_TYPE_REQ)
		{
			srcIface.virtualNodeId = sourceId;
			dstIface.virtualNodeId = destId;
		}
		
		// Check if the node has been seen before. If not, it is an error
		if ((this->nodesSeen).find(sourceId) == (this->nodesSeen).end()) {
			ifaceCount = RSPEC_ERROR_UNSEEN_NODEIFACE_SRC;
			return vector<struct link_interface>();
		}
		if ((this->nodesSeen).find(sourceId) == (this->nodesSeen).end()) {
			ifaceCount = RSPEC_ERROR_UNSEEN_NODEIFACE_DST;
			return vector<struct link_interface>();
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
	string strBw = this->readChild(property, "bandwidth", hasBandwidth);
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

#endif // #ifdef WITH_XML

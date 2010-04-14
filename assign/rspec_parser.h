/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Header for RSPEC parser files
 */

# ifdef WITH_XML

#ifndef __RSPEC_PARSER__
#define __RSPEC_PARSER__

#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <xercesc/dom/DOM.hpp>

#define RSPEC_TYPE_ADVT 0
#define RSPEC_TYPE_REQ 1

struct node_type 
{
	std::string typeName;
	int typeSlots;
	bool isStatic;
};

struct link_characteristics
{
	int bandwidth;
	int latency;
	double packetLoss;
};

struct link_interface
{
	std::string virtualNodeId;
	std::string virtualIfaceId;
	std::string physicalNodeId;
	std::string physicalIfaceId;
};

struct node_interface
{
	std::string componentId;
	std::string clientId;
};

class rspec_parser
{
	private:
			
	protected:
		int rspecType; 
		std::set< std::pair<std::string, std::string> >ifacesSeen;
		std::string getAttribute (const xercesc::DOMElement*, const std::string,
															bool&);
		bool hasAttribute (const xercesc::DOMElement*, const std::string);
		struct link_interface getIface (const xercesc::DOMElement*);
		
	public:
		
		rspec_parser () { }
		rspec_parser(int type) { this->rspecType = type; }
		
		// Common functions
		virtual std::string readPhysicalId (const xercesc::DOMElement*, bool&);
		virtual std::string readVirtualId (const xercesc::DOMElement*, bool&);
		virtual std::string readComponentManagerId (const xercesc::DOMElement*,
				                                    bool&);
		
		// Reads subnode tag
		virtual std::string readSubnodeOf (const xercesc::DOMElement*, bool&);
		
		// Functions for nodes
		virtual std::vector<std::string> readLocation(const xercesc::DOMElement*,
																									int&);
		virtual std::list<struct node_type> 
				readNodeTypes (const xercesc::DOMElement*, int&);
		
		// Functions for links
		virtual struct link_characteristics 
				readLinkCharacteristics (const xercesc::DOMElement*, 
																 int defaultBandwidth, 
								 								 int unlimitedBandwidth,
																 int&);
				
		virtual std::vector< struct link_interface >
				readLinkInterface (const xercesc::DOMElement*, int&);
		
		// Reads all the interfaces on a node
		// Returns the number of interfaces found. 
		// This only populates the internal data structures of the object.
		// Ideally, this ought to be done automatically, but in the current setup,
		// there doesn't seem to be a clean way of making it happen.
		virtual int readInterfacesOnNode (const xercesc::DOMElement*, bool&);
		virtual void dummyFun();
};


#endif

#endif

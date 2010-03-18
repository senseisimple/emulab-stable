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
#include <string>
#include <utility>
#include <xercesc/dom/DOM.hpp>


struct node_type 
{
	std::string typeName;
	int typeCount;
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
	std::string srcNode;
	std::string srcIface;
	std::string dstNode;
	std::string dstIface;
};

class rspec_parser
{
	private:
		std::map<std::string, std::string> interfacesSeen;
	
	protected:
		bool getAttribute (const xercesc::DOMElement*, const std::string,
						          std::string&);
		struct link_interface getIface (const xercesc::DOMElement*);
		
	public:
		
		const int LINK_BANDWIDTH = 0;
		const int LINK_LATENCY = 1;
		const int LINK_PACKET_LOSS = 2;
		
		// Common functions
		virtual std::string readComponentId (const xercesc::DOMElement*, bool&);
		virtual std::string readVirtualid (const xercesc::DOMElement*, bool&);
		virtual std::string readComponentManagerId (const xercesc::DOMElement*,
				                                    bool&);
		
		// Functions for nodes
		virtual std::list<std::string> 
				readLocation(const xercesc::DOMElement*, int&);
		virtual std::list<struct node_type> 
				readNodeTypes (const xercesc::DOMElement*, int&);
		virtual std::string readIfaceDecl (const xercesc::DOMElement*);
		
		
		// Functions for links
		virtual struct link_characteristics 
				readLinkCharacteristics (const xercesc::DOMElement*, int&);
		
		virtual struct link_interface 
				readInterface (const xercesc::DOMElement*);
};


#endif

#endif

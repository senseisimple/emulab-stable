/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008, 2009 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __EMULAB_EXTENSIONS_PARSER_H__
#define __EMULAB_EXTENSIONS_PARSER_H__

#ifdef WITH_XML

#include <string>
#include <xercesc/dom/DOM.hpp>
#include "rspec_parser_helper.h"

#define LOCAL_OPERATOR 0
#define GLOBAL_OPERATOR 1

#define HARD 0
#define SOFT 1

struct emulab_operator {
	std::string op;
	int type;
};

struct fd 
{
	std::string fd_name;
	float fd_weight;
	bool violatable;
	struct emulab_operator op;
};

struct node_flags
{
	int trivialBandwidth;
	std::string subnodeOf;
	bool unique;
	bool disallowTrivialMix;
};

struct link_flags
{
	bool noDelay;
	bool multiplexOk;
	bool trivialOk;
	std::string fixSrcIface;
	std::string fixDstIface;
};

struct hardness {
	int type;
	float weight;	
};

struct vclass {
	std::string name;
	struct hardness;
	std::string physicalType;
};

struct property {
	std::string name;
	float value;
	float penalty;
	bool violatable;
	struct emulab_operator op;
};

class emulab_extensions_parser : public rspec_parser_helper {
	private:
		int type;
		
	public:
		// Constructor
		emulab_extensions_parser(int type) { this->type = type; }
		
		// Functions
		struct fd readFeatureDesire (xercesc::DOMElement* tag);
		struct node_flags readNodeFlag (xercesc::DOMElement* tag);
		struct link_flags readLinkFlag (xercesc::DOMElement* tag);
		struct property readProperty (xercesc::DOMElement* tag);
		struct vclass readVClass (xercesc::DOMElement* tag);
		std::string readAssignedTo (xercesc::DOMElement* tag);
		std::string readHintTo (xercesc::DOMElement* tag);
};

#endif // WITH_XML

#endif // __EMULAB_EXTENSIONS_PARSER_H__

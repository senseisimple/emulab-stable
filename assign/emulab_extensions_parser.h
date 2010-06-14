/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008, 2009 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __EMULAB_EXTENSIONS_PARSER_H__
#define __EMULAB_EXTENSIONS_PARSER_H__

#ifdef WITH_XML

#include <string>
#include <vector>
#include <xercesc/dom/DOM.hpp>
#include "rspec_parser_helper.h"

namespace rspec_emulab_extension {

	#define LOCAL_OPERATOR 0
	#define GLOBAL_OPERATOR 1
	#define NORMAL_OPERATOR 2
	
	#define HARD_VCLASS 0
	#define SOFT_VCLASS 1
	
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
		struct hardness type;
		std::string physicalType;
	};
	
	struct property {
		std::string name;
		std::string value;
		float penalty;
		bool violatable;
		struct emulab_operator op;
	};
	
	class emulab_extensions_parser : public rspec_parser_helper {
		private:
			int type;
			struct emulab_operator readOperator(xercesc::DOMElement* tag);
			struct hardness readHardness (xercesc::DOMElement* tag);
			
		public:
			// Constructor
			emulab_extensions_parser(int type) { this->type = type; }
			
			// Functions
			std::vector<struct fd> readAllFeaturesDesires 
										(xercesc::DOMElement* ele);
			struct fd readFeatureDesire (xercesc::DOMElement* tag);
			struct node_flags readNodeFlag (xercesc::DOMElement* tag);
			struct link_flags readLinkFlag (xercesc::DOMElement* tag);
			std::vector<struct property> readAllProperties
											(xercesc::DOMElement* elem);
			struct property readProperty (xercesc::DOMElement* tag);
			std::vector<struct vclass> readAllVClasses (xercesc::DOMElement*);
			struct vclass readVClass (xercesc::DOMElement* tag);
			std::string readAssignedTo (xercesc::DOMElement* tag);
			std::string readHintTo (xercesc::DOMElement* tag);
			std::string readTypeSlots (xercesc::DOMElement* tag);
			bool readStaticType (xercesc::DOMElement* tag);
	};

} // namespace rspec_emulab_extension

#endif // WITH_XML

#endif // __EMULAB_EXTENSIONS_PARSER_H__

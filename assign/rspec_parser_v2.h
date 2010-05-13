/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Parser class for rspec version 2.0
 */
 
#ifdef WITH_XML 
 
#ifndef __RSPEC_PARSER_V2_H__
#define __RSPEC_PARSER_V2_H__

#include "rspec_parser.h"

class rspec_parser_v2 : public rspec_parser
{
	private:
		std::set<std::string> nodesSeen;
	
	// Reads the interfaces on a link		
		std::vector< struct link_interface >
			readLinkInterface (const xercesc::DOMElement*, int&);
	
		struct link_characteristics readLinkCharacteristics 
									(const xercesc::DOMElement*, 
									  int&,
									  int defaultBandwidth = -1, 
									  int unlimitedBandwidth = -1);
	public:
		rspec_parser_v2 (int type) : rspec_parser (type) { ; }
};



#endif // #ifndef __RSPEC_PARSER_V2_H__

#endif // #ifdef WITH_XML



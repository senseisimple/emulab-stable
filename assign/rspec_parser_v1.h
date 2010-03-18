/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Parser class for rspec version 1
 */
 
#ifdef WITH_XML 
 
#ifndef __RSPEC_PARSER_V1_H__
#define __RSPEC_PARSER_V1_H__

#include "rspec_parser.h"
#include "xstr.h"

#include <string>
#include <xercesc/dom/DOM.hpp>

class rspec_parser_v1 : public rspec_parser 
{
	// Functions specific to rspec version 1 should be declared here. 
	// Most of the functions needed to parse rspecs in general should 
	// already have been inherited from rspec parser
	private:
		

	public:
		std::string find_urn(const xercesc::DOMElement* element, 
							 std::string const& prefix);

};

#endif

#endif

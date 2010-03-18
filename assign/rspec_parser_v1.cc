/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * XML parser for RSPEC version 1.0
 */

#ifdef WITH_XML

#include "rspec_parser_v1.h"

#include <list>
#include <map>
#include <string>
#include <utility>
#include <xercesc/dom/DOM.hpp>

#include "xstr.h"

XERCES_CPP_NAMESPACE_USE

using namespace std;

string rspec_parser_v1::find_urn(const DOMElement* element, 
								 string const& prefix)
{
	string uuid = prefix + "_uuid";
	string urn = prefix + "_urn";
	if (element->hasAttribute(XStr(uuid.c_str()).x()))
	{
		return string(XStr(element->getAttribute(XStr(uuid.c_str()).x())).c());
	}
	else //if (element->hasAttribute(XStr(urn.c_str()).x()))
	{
		return string(XStr(element->getAttribute(XStr(urn.c_str()).x())).c());
	}
}

// Returns true if the attribute exists and its value is a non-empty string
bool rspec_parser_v1 :: readComponentId (const DOMElement* elem, 
										 string& componentId)
{
    componentId = this->find_urn(elem, "component");
	return (componentId != "");
}

// Returns true if the attribute exists and its value is a non-empty string
bool rspec_parser_v1 :: readVirtualId (const DOMElement* elem, 
									   string& virtualId)
{
	virtualId = this->find_urn(elem, "virtual");
	return (virtualId != "");
}

// Returns true if the attribute exists and its value is a non-empty string
bool rspec_parser_v1 :: readComponentManagerId (const DOMElement* elem, 
		                                          string& cmId)
{
	cmId = this->find_urn(elem, "component_manager");
	return (cmId != "");
}

#endif

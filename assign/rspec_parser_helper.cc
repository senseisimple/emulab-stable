/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Header for RSPEC parser files
 */

# ifdef WITH_XML

#include "rspec_parser_helper.h"
#include "rspec_parser.h"
#include "xmlhelpers.h"

#include <string>
#include "xstr.h"
#include <xercesc/dom/DOM.hpp>

// Returns the attribute value and an out paramter if the attribute exists
string rspec_parser_helper :: getAttribute(const DOMElement* tag, 
											const string attrName,
		 bool& hasAttr)
{
	hasAttr = tag->hasAttribute(XStr(attrName.c_str()).x());
	if (hasAttr)
		return XStr(tag->getAttribute(XStr(attrName.c_str()).x())).c();
	return "";
}

string rspec_parser_helper :: getAttribute(const DOMElement* tag, 
										   const string attr)
{
	bool dummy = false;
	return (this->getAttribute(tag, attr, dummy));
}

bool rspec_parser_helper :: hasAttribute(const DOMElement* tag, 
									const string attrName)
{
	return (tag->hasAttribute(XStr(attrName).x()));
}

string rspec_parser_helper :: readChild (const DOMElement* elt, 
										  const char* tagName,
		  									bool& hasTag)
{
	hasTag = hasChildTag(elt, tagName);
	if (hasTag)
		return XStr(getChildValue(elt, tagName)).c();
	return "";
}

string rspec_parser_helper :: readChild (const DOMElement* elt, 
										 const char* tagName)
{
	bool dummy = false;
	return (this->readChild(elt, tagName, dummy));
}

bool rspec_parser_helper::hasChild (const DOMElement* elt, 
									const char* childName)
{
	return ((elt->getElementsByTagName(XStr(childName).x()))->getLength() 
			!= 0);
}

string rspec_parser_helper :: numToString(int num)
{
	std::ostringstream oss;
	oss << num;
	return oss.str();
}

string rspec_parser_helper :: numToString(double num)
{
	std::ostringstream oss;
	oss << num;
	return oss.str();
}

float rspec_parser_helper :: stringToNum (string s)
{
	float num;
	std::istringstream iss(s);
	iss >> num;
	return num;
}

int rspec_parser_helper :: getRspecVersion (DOMElement* root)
{
	string schemaLocAttr 
			=  (XStr(root->getAttribute(XStr("xsi:schemaLocation").x())).c());
	string schemaLocation = schemaLocAttr.substr(0,schemaLocAttr.find(' '));
	float tmpRspecVersion 
			=  rspec_parser_helper::stringToNum
						(schemaLocation.substr(schemaLocation.rfind('/')+1));
	// XXX: There's an obvious bug here
	// Since floating point equality is dangerous at best, this is the closest
	// you can get without doing a string comparison on schemaLocation
	if (tmpRspecVersion > 0 && tmpRspecVersion < 1)
		return 1;
	return (int)tmpRspecVersion;
}

#endif //WITH_XML

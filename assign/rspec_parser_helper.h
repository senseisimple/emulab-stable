/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Helper class for the rspec parser
 */
 
#ifdef WITH_XML 
 
#ifndef __RSPEC_PARSER_HELPER_H__
#define __RSPEC_PARSER_HELPER_H__

#include "xstr.h"

#include <sstream>
#include <string>
#include <xercesc/dom/DOM.hpp>

class rspec_parser_helper
{
 public:
  std::string getAttribute (const xercesc::DOMElement*, 
			    const std::string,
			    bool&);
  std::string getAttribute(const xercesc::DOMElement*, const std::string);
  bool hasAttribute (const xercesc::DOMElement*, const std::string);
  std::string readChild (const xercesc::DOMElement*, const char*, bool&);
  std::string readChild (const xercesc::DOMElement*, const char*);
  bool hasChild (const xercesc::DOMElement*, const char*);
  
  // Methods to convert between strings and other data types
  static std::string numToString (int num);
  static std::string numToString (double num);
  static float stringToNum (std::string str);
  
  // To determine the rspec version from the root element
  static int getRspecVersion (xercesc::DOMElement* root);
};

#endif // __RSPEC_PARSER_HELPER_H__

#endif // WITH_XML

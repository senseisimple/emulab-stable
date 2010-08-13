/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2010 University of Utah and the Flux Group.
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

struct node_interface_v1 : node_interface
{
  std::string componentName;
};

class rspec_parser_v1 : public rspec_parser 
{
  // Functions specific to rspec version 1 should be declared here. 
  // Most of the functions needed to parse rspecs in general should 
  // already have been inherited from rspec parser
 private:
  std::string find_urn(const xercesc::DOMElement* element, 
		       std::string const& prefix, bool&);
  
 public:
  
  rspec_parser_v1 (int type) : rspec_parser(type) { ; }
  
  std::string readPhysicalId (const xercesc::DOMElement*, bool&);
  std::string readVirtualId (const xercesc::DOMElement*, bool&);
  std::string readComponentManagerId (const xercesc::DOMElement*, bool&);
  
  std::map< std::pair<std::string, std::string>, 
    std::pair<std::string, std::string> > 
    readInterfacesOnNode (const xercesc::DOMElement*, bool&);
  std::vector<struct link_interface> readLinkInterface
    (const xercesc::DOMElement* link, int& ifaceCount);
  struct link_interface getIface (const xercesc::DOMElement*);
  std::string readSubnodeOf (const xercesc::DOMElement* tag,
			     bool& isSubnode,
			     int& count);
};

#endif

#endif

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
  
 protected:
  std::map<std::string, std::string> ifacesSeen;
  struct link_interface getIface (const xercesc::DOMElement*);
  
 public:
  rspec_parser_v2 (int type) : rspec_parser (type) { ; }
  // Reads the interfaces on a link		
  std::vector< struct link_interface >
    readLinkInterface (const xercesc::DOMElement*, int&);
  
  struct link_characteristics readLinkCharacteristics 
    (const xercesc::DOMElement*, 
     int&,
     int defaultBandwidth = -1, 
     int unlimitedBandwidth = -1);
  
  std::vector<struct node_type> readNodeTypes
    (const xercesc::DOMElement*,
     int& typeCount,
     int unlimitedSlots);
  
  map< pair<string, string>, pair<string, string> >
    readInterfacesOnNode (const xercesc::DOMElement* node, 
			  bool& allUnique);

  std::string readAvailable(const xercesc::DOMElement* node, bool& hasTag);

  std::vector<struct link_type> 
    readLinkTypes (const xercesc::DOMElement* link, int& typeCount);

  std::vector<struct rspec_emulab_extension::type_limit>
    readTypeLimits(const xercesc::DOMElement* tag, int& count);

  std::vector<struct rspec_emulab_extension::fd>
    readFeaturesDesires(const xercesc::DOMElement* tag, int& count);
};



#endif // #ifndef __RSPEC_PARSER_V2_H__

#endif // #ifdef WITH_XML



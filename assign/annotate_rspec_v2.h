/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Base class for the annotater. 
 */

#ifdef WITH_XML

#ifndef __ANNOTATE_RSPEC_V2_H
#define __ANNOTATE_RSPEC_V2_H

#include "annotate.h"
#include "annotate_rspec.h"

#include <list>
#include <map>
#include <set>
#include <utility>
#include <string>

#include <xercesc/dom/DOM.hpp>

class annotate_rspec_v2 : public annotate_rspec
{
 private:
  // Enumeration of which interface in a hop 
  // is an interface to a link end point
  enum endpoint_interface_enum { NEITHER, SOURCE, DESTINATION, BOTH };
  std::map< std::string, std::set<std::string> > lan_links_map;

  std::map< std::string, std::string >* vInterfaceMap;
  std::map< std::string, std::string >* pInterfaceMap;
  
 public:
  annotate_rspec_v2 ();
  ~annotate_rspec_v2 () { ; }
  
  // Annotates nodes and direct links in the rspec
  void annotate_element(const char* v_name, const char* p_name);
  
  // Annotates intraswitch and interswitch links in the rspec
  void annotate_element(const char* v_name, 
			std::list<const char*>* links);
  
  // Annotate a trivial link
  void annotate_element(const char* v_name);
  
  // Annotates an interface element on a link
  void annotate_interface (const xercesc::DOMElement* plink, 
			   const xercesc::DOMElement* vlink, 
			   int interface_number,
			   bool is_trivial_link);
  
  // Annotates an interface element on a non-trivial link
  void annotate_interface (const xercesc::DOMElement* plink, 
			   const xercesc::DOMElement* vlink, 
			   int interface_number);
  
  // Annotates an interface element on a trivial link
  void annotate_interface (const xercesc::DOMElement* vlink,
			   int interface_number);
  
  // Creates a hop from a switch till the next end point. 
  // Adds the hop to the vlink
  // Returns the hop element that was created
  xercesc::DOMElement* 
    create_component_hop (const xercesc::DOMElement* plink, 
			  xercesc::DOMElement* vlink, 
			  int endpoint_interface, 
			  const xercesc::DOMElement* prev_hop);
  
  // Creates a component_hop for a trivial link
  // Adds the hop to the vlink
  // Returns the hop element that was created
  xercesc::DOMElement* create_component_hop (xercesc::DOMElement* vlink);
  
  // Finds the next link in the path returned by assign
  xercesc::DOMElement* find_next_link_in_path 
    (xercesc::DOMElement *prev, 
     std::list<const char*>* links);
  
  // Copies the component spec from the source to the destination
  void copy_component_spec(const xercesc::DOMElement* src, 
			   xercesc::DOMElement* dst);
  
  // Copies the component hop from the auto-generated link 
  // to the requested link
  void copy_component_hop(xercesc::DOMElement* requested_link, 
			  xercesc::DOMElement* component_hop);
  
  // Checks if the link contains an interface with 
  // virtual_interface_id = id
  bool has_interface_with_id (std::string link_id, std::string id);
  
  // Removes all extra tags and generated elements from the XML document
  void cleanup ();
  
  // Checks whether an element of type tag 
  // with attr_name = attr_value is a generated element
  bool is_generated_element (const char* tag, 
			     const char* attr_name, 
			     const char* attr_value);

  // Given an interface Id, returns the node on which the interface is present
  std::string lookupIface (std::map<std::string, std::string>* map,
			   std::string ifaceId, bool&);

  // Returns the interface on the physical link
  // which is declared on the physical node with component_id, physNodeId
  const xercesc::DOMElement* 
    getIfaceOnNode(const xercesc::DOMElement* plink, std::string physNodeId);

  // Retuns the id of the physical source node
  // for the nth interface of a link
  std::string getNodeForNthInterface (const xercesc::DOMElement* link, int n);

  // Annotates the end point of a link
  bool annotate_endpoint(xercesc::DOMElement* iface, std::string virtId);

  // Extracts the short interface name from the interface URN
  std::string getShortInterfaceName (std::string interfaceURN);

  // Adds a fixed interface element to an interface
  void addFixedInterface(xercesc::DOMElement* interface,std::string shortname);
};

#endif //for __ANNOTATE_RSPEC_H

#endif // for WITH_XML

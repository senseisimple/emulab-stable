/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Base class for the annotater. 
 */

#ifdef WITH_XML

#ifndef __ANNOTATE_RSPEC_H
#define __ANNOTATE_RSPEC_H

#include "annotate.h"

#include <list>
#include <map>
#include <set>
#include <utility>
#include <string>

#include <xercesc/dom/DOM.hpp>

class annotate_rspec : public annotate
{
	private:
		// Enumeration of which interface in a hop 
		// is an interface to a link end point
		enum endpoint_interface_enum { NEITHER, SOURCE, DESTINATION, BOTH };
		std::map< std::string, std::set<std::string> > lan_links_map;
	
	public:
		annotate_rspec ();
		~annotate_rspec () { ; }
		
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
		xercesc::DOMElement* create_component_hop 
				 						(const xercesc::DOMElement* plink, 
										 xercesc::DOMElement* vlink, 
		   								 int endpoint_interface, 
			  							 const xercesc::DOMElement* prev_hop);
		
		// Creates a component_hop for a trivial link
		// Adds the hop to the vlink
		// Returns the hop element that was created
		xercesc::DOMElement* create_component_hop (xercesc::DOMElement* vlink);
			
		// If the interface is the end point of a link/path, 
		// add two additional attributes to it
		void set_interface_as_link_endpoint (xercesc::DOMElement* interface, 
											  const char* virtual_node_id, 
			 								  const char* virtual_interface_id);
	
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
};

#endif //for __ANNOTATE_RSPEC_H
 
#endif // for WITH_XML

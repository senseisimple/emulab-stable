/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Base class for the annotater. 
 */

/* This is ugly, but we only really need this file if we are building with XML support */

#ifdef WITH_XML

#ifndef __ANNOTATE_RSPEC_H
#define __ANNOTATE_RSPEC_H

#include "annotate.h"

#include <list>
#include <map>
#include <utility>
#include <string>

#include <xercesc/dom/DOM.hpp>

class annotate_rspec : public annotate
{
	private:
		// Enumeration of which interface in a hop is an interface to a link end point
		enum endpoint_interface_enum { NEITHER, SOURCE, DESTINATION, BOTH };
	
	public:
		annotate_rspec ();
		~annotate_rspec () { ; }
		
		// Annotates nodes and direct links in the rspec
		 void annotate_element(const char* v_name, const char* p_name);
	
		// Annotates intraswitch and interswitch links in the rspec
		 void annotate_element(const char* v_name, std::list<const char*>* links);
	
		// Annotates an interface element on a link
		 void annotate_interface (const xercesc::DOMElement* plink, xercesc::DOMElement* vlink, int interface_number);
			
		// Creates a hop from a switch till the next end point. Adds the hop to the vlink and returns the hop element that was created
		 xercesc::DOMElement* create_component_hop (const xercesc::DOMElement* plink, xercesc::DOMElement* vlink, int endpoint_interface, const xercesc::DOMElement* prev_component_hop);
			
		// If the interface is the end point of a link/path, add two additional attributes to it
		 void set_interface_as_link_endpoint (xercesc::DOMElement* interface, const char* virtual_node_id, const char* virtual_interface_id);
	
		// Finds the next link in the path returned by assign
		 xercesc::DOMElement* find_next_link_in_path (xercesc::DOMElement *prev, std::list<const char*>* links);
		
		// Copies the component spec from the source to the destination
		void annotate_rspec::copy_component_spec(const xercesc::DOMElement* src, xercesc::DOMElement* dst);
};

#endif //for __ANNOTATE_RSPEC_H
 
#endif // for WITH_XML

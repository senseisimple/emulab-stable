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

#ifndef __ANNOTATE_H

#define __ANNOTATE_H

#include <utility>
#include <string>
#include <list>
#include <map>

#include <xercesc/dom/DOM.hpp>

class annotate
{
	protected: 
		//xercesc::DOMDocument* doc;
		xercesc::DOMElement* virtual_root;
		std::map<std::string, xercesc::DOMElement*> *physical_elements;
	
	public:
		// Annotates nodes and direct links in the rspec
		virtual void annotate_element(const char* v_name, const char* p_name) = 0;
	
		// Annotates intraswitch and interswitch links in the rspec
		virtual void annotate_element(const char* v_name, std::list<const char*>* links) = 0;
	
		// Creates a hop from a switch till the next end point. Adds the hop to the vlink and returns the hop element that was created
		virtual xercesc::DOMElement* create_component_hop (const xercesc::DOMElement* plink, xercesc::DOMElement* vlink, int endpoint_interface, const xercesc::DOMElement* prev_component_hop) = 0;
			
		// Finds the next link in the path returned by assign
		virtual xercesc::DOMElement* find_next_link_in_path (xercesc::DOMElement *prev, std::list<const char*>* links) = 0;
		
		// Writes the annotated xml to disk
		void write_annotated_file(const char* filename);
};

#endif // for __ANNOTATE_H

#endif // for WITH_XML
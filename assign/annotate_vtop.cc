/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

static const char rcsid[] = "$Id: annotate_vtop.cc,v 1.2 2009-05-20 18:06:07 tarunp Exp $";

#ifdef WITH_XML

#include "annotate.h"
#include "annotate_vtop.h"

#include <xercesc/util/PlatformUtils.hpp>

#include <xercesc/dom/DOM.hpp>

#include "xstr.h"
#include "xmlhelpers.h"

#include <iostream>
#include <utility>
#include <list>
#include <string>
				 
#define XMLDEBUG(x) (cerr << x);
				 
extern DOMDocument* vtop_xml_document;
extern DOMElement* root;
extern map<string, DOMElement*>* ptop_elements;

XERCES_CPP_NAMESPACE_USE

using namespace std;

annotate_vtop :: annotate_vtop ()
{
// 	this->vtop_xml_document = vtop_xml_document;
	this->virtual_root = root;
	this->physical_elements = ptop_elements;
}

// This will get called when a node or a direct link needs to be annotated
void annotate_vtop::annotate_element (const char* v_name, const char* p_name)
{
	DOMElement* vnode = getElementByAttributeValue(this->virtual_root, "node", "name", v_name);
	// If a vnode by that name was found, then go ahead. If not, that element should be a link
	// We are not terribly concerned about having to scan the entire physical topology twice
	// because direct links are never really going to happen
	if (vnode != NULL)
	{
		vnode->setAttribute(XStr("assigned_to").x(), XStr(p_name).x());
	}
	else
	{
		DOMElement* vlink = getElementByAttributeValue(this->virtual_root, "link", "name", v_name);
		DOMElement* plink = (this->physical_elements->find(p_name))->second;
		
		annotate_interface(plink, vlink, "source_interface");
		annotate_interface(plink, vlink, "destination_interface");
		
		create_component_hop(plink, vlink, BOTH, NULL);
	}
}

// This is called when an intraswitch or interswitch link has to be annotated
void annotate_vtop::annotate_element (const char* v_name, list<const char*>* links)
{
	// These are the paths from the source to the first switch
	// and from the last switch to the destination
	const char* psrc_name = links->front();	
	const char* pdst_name = links->back();	
	DOMElement* p_src_switch_link = (this->physical_elements->find(psrc_name))->second;
	DOMElement* p_switch_dst_link = (this->physical_elements->find(pdst_name))->second;
	
	// Remove these links from the list
	// If it is an intra-switch link, the list should now be empty.
	links->pop_front();
	links->pop_back();
		
	DOMElement* vlink = getElementByAttributeValue (this->virtual_root, "link", "name", v_name);
	annotate_interface (p_src_switch_link, vlink, "source_interface");
	annotate_interface (p_switch_dst_link, vlink, "destination_interface");
	
	DOMElement* prev_component_hop = create_component_hop (p_src_switch_link, vlink, SOURCE, NULL);
	for (DOMElement *prev_link_in_path = p_src_switch_link; !links->empty(); )
	{
		DOMElement* p_switch_switch_link = find_next_link_in_path (prev_link_in_path, links);
		prev_component_hop = create_component_hop (p_switch_switch_link, vlink, NEITHER, prev_component_hop);
		prev_link_in_path = p_switch_switch_link;
	}
	create_component_hop (p_switch_dst_link, vlink, DESTINATION, prev_component_hop);
}

// Annotates the interface element on a link and updates the node which is the end point of the link as well
void annotate_vtop::annotate_interface (const DOMElement* plink, DOMElement* vlink, const char* interface_type)
{
	DOMElement* vlink_iface = getElementByTagName (vlink, interface_type);
			
	// Get the node name to which the interface is connected 
	// Use this to get the node from the partially annotated DOM
	XStr vlink_iface_node_name (vlink_iface->getAttribute(XStr("node_name").x()));
	DOMElement* v_src_node = getElementByAttributeValue(this->virtual_root, "node", "name", vlink_iface_node_name.c());
	XStr src_node_assigned_to (v_src_node->getAttribute(XStr("assigned_to").x()));
	
	// If the right interface is not the source_interface of the link, it must be the destination_interface
	DOMElement* p_iface = getElementByAttributeValue(plink, "source_interface", "node_name", src_node_assigned_to);
	if (p_iface == NULL)
		p_iface = getElementByAttributeValue(plink, "destination_interface", "node_name", src_node_assigned_to);
		
	vlink_iface->setAttribute(XStr("physical_node_name").x(), p_iface->getAttribute(XStr("node_name").x()));
	vlink_iface->setAttribute(XStr("physical_interface_name").x(), p_iface->getAttribute(XStr("interface_name").x()));
}

// Creates a hop from a switch till the next end point. Adds the hop to the vlink and returns the hop element that was created
DOMElement* annotate_vtop::create_component_hop (const DOMElement* plink, DOMElement* vlink, int endpoint_interface, const DOMElement* prev_component_hop)
{
	// Create a single_hop_link element
	DOMElement* component_hop = vtop_xml_document->createElement(XStr("component_hop").x());
	component_hop->setAttribute (XStr("assigned_to").x(), plink->getAttribute(XStr("name").x()));

	DOMElement* component_hop_interface = vtop_xml_document->createElement(XStr("interface").x());
		
	// We assume the first interface is the source and the second is the destination
	DOMElement* plink_src_iface = getElementByTagName(plink, "source_interface");
	DOMElement* plink_dst_iface = getElementByTagName(plink, "destination_interface");
				
	DOMElement* vlink_src_iface = getElementByTagName(vlink, "source_interface");
	DOMElement* vlink_dst_iface = getElementByTagName(vlink, "destination_interface");
		
	DOMElement* hop_src_iface = vtop_xml_document->createElement(XStr("interface").x());
	DOMElement* hop_dst_iface = vtop_xml_document->createElement(XStr("interface").x());
	set_component_hop_interface(hop_src_iface, plink_src_iface);
	set_component_hop_interface(hop_dst_iface, plink_dst_iface);
	
	// If the previous component hop is not specified (NULL),
	// then the link is either direct or the direction is guaranteed to be from the node to the switch
	if (prev_component_hop != NULL)
	{
		// Find the destination of the previous component hop
		DOMElement* prev_hop_dst_iface = dynamic_cast<DOMElement*>((prev_component_hop->getElementsByTagName(XStr("interface").x()))->item(1));
		XStr prev_hop_dst_uuid (prev_hop_dst_iface->getAttribute(XStr("physical_node_name").x()));
		
		// We need to do this because in advertisements, all links are from nodes to switches
		// and we need to reverse this order for the last hop of a multi-hop path
		if (strcmp(prev_hop_dst_uuid.c(), XStr(plink_dst_iface->getAttribute(XStr("node_name").x())).c()) == 0)
		{
			set_component_hop_interface(hop_src_iface, plink_dst_iface);
			set_component_hop_interface(hop_dst_iface, plink_src_iface);
		}
	}
	
	// If the source interface is an end point
	if (endpoint_interface == SOURCE || endpoint_interface == BOTH)
		set_interface_as_link_endpoint(hop_src_iface, vlink_src_iface);
	
	// If the destination interface is an end point
	if (endpoint_interface == DESTINATION || endpoint_interface == BOTH)
		set_interface_as_link_endpoint(hop_dst_iface, vlink_dst_iface);
		
	// Add interface specifications to the hop element
	component_hop->appendChild(hop_src_iface);
	component_hop->appendChild(hop_dst_iface);
		
	vlink->appendChild(component_hop);
	return (component_hop);
}

// Sets the attributes of an interface element in the component hop using the corresponding physical interface
void annotate_vtop::set_component_hop_interface (DOMElement* hop_interface, const DOMElement* physical_interface)
{
	hop_interface->setAttribute(XStr("physical_node_name").x(), physical_interface->getAttribute(XStr("node_name").x()));
	hop_interface->setAttribute(XStr("physical_interface_name").x(), physical_interface->getAttribute(XStr("interface_name").x()));
}

// If the interface belongs to an end point of the link, and additional virtual_id attribute has to be added to it
void annotate_vtop::set_interface_as_link_endpoint (DOMElement* interface, const DOMElement* vlink_interface)
{
	interface->setAttribute(XStr("virtual_node_name").x(), vlink_interface->getAttribute(XStr("node_name").x()));
	interface->setAttribute(XStr("virtual_interface_name").x(), vlink_interface->getAttribute(XStr("interface_name").x()));
}

// Finds the next link in the path returned by assign
// Assign sometimes reverses the links on the path from the source to the destination, 
// so you need to look at the entire path to find the next link
DOMElement* annotate_vtop::find_next_link_in_path (DOMElement *prev, list<const char*>* links)
{
	list<const char*>::iterator it;
	DOMElement* link = NULL;
	for (it = links->begin(); it != links->end(); ++it)
	{
		link = (this->physical_elements->find(*it))->second;
		XStr link_src(getElementByTagName(link, "source_interface")->getAttribute(XStr("node_name").x()));
		XStr link_dst(getElementByTagName(link, "destination_interface")->getAttribute(XStr("node_name").x()));
		XStr prev_dst(getElementByTagName(prev, "destination_interface")->getAttribute(XStr("node_name").x()));
		if (strcmp(link_src.c(), prev_dst.c()) == 0 || strcmp(link_dst.c(), prev_dst.c()) == 0)
		{
			links->remove(*it);
			break;
		}
	}
	return link;
}

#endif

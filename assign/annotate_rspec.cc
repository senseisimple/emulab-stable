/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003-2009 University of Utah and the Flux Group.
 * All rights reserved.
 */

static const char rcsid[] = "$Id: annotate_rspec.cc,v 1.6 2009-10-06 23:53:10 duerig Exp $";

#ifdef WITH_XML

#include "annotate.h"
#include "annotate_rspec.h"

#include <xercesc/util/PlatformUtils.hpp>

#include <xercesc/dom/DOM.hpp>

#include "xstr.h"
#include "xmlhelpers.h"

#include <iostream>
#include <utility>
#include <list>
#include <string>
				 
#define XMLDEBUG(x) (cerr << x);
				 
extern DOMDocument* doc;
extern DOMElement* request_root;
extern map<string, DOMElement*>* advertisement_elements;
				 
XERCES_CPP_NAMESPACE_USE

using namespace std;

annotate_rspec :: annotate_rspec ()
{
// 	this->doc = doc;
	this->virtual_root = request_root;
	this->physical_elements = advertisement_elements;
}

// This will get called when a node or a direct link needs to be annotated
void annotate_rspec::annotate_element (const char* v_name, const char* p_name)
{
	DOMElement* vnode = getElementByAttributeValue(this->virtual_root, "node", "virtual_id", v_name);
	// If a vnode by that name was found, then go ahead. If not, that element should be a link
	// We are not terribly concerned about having to scan the entire physical topology twice
	// because direct links are never really going to happen
	if (vnode != NULL)
	{
		DOMElement* pnode = (this->physical_elements->find(p_name))->second;
		copy_component_spec(pnode, vnode);
	}
	else
	{
		DOMElement* vlink = getElementByAttributeValue(this->virtual_root, "link", "virtual_id", v_name);
		DOMElement* plink = (this->physical_elements->find(p_name))->second;
		
		annotate_interface (plink, vlink, 0);
		annotate_interface (plink, vlink, 1);
		
		create_component_hop(plink, vlink, BOTH, NULL);
	}
}

// This is called when an intraswitch or interswitch link has to be annotated
void annotate_rspec::annotate_element (const char* v_name, list<const char*>* links)
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
		
	DOMElement* vlink = getElementByAttributeValue (this->virtual_root, "link", "virtual_id", v_name);
	annotate_interface(p_src_switch_link, vlink, 0);
	annotate_interface(p_switch_dst_link, vlink, 1);
	
	DOMElement* prev_component_hop = create_component_hop (p_src_switch_link, vlink, SOURCE, NULL);
#if 0
	for (DOMElement *prev_link_in_path = p_src_switch_link; !links->empty(); )
	{
		DOMElement* p_switch_switch_link = find_next_link_in_path (prev_link_in_path, links);
		prev_component_hop = create_component_hop (p_switch_switch_link, vlink, NEITHER, prev_component_hop);
		prev_link_in_path = p_switch_switch_link;
	}
#else
	{
	    static int gave_apology;

	    if( !gave_apology ) {
		gave_apology = 1;
		cout << "Warning: unable to locate interfaces on "
		    "switch/switch links; omitting those\n";
	    }
	}
#endif
	create_component_hop (p_switch_dst_link, vlink, DESTINATION, prev_component_hop);
}

// Creates a hop from a switch till the next end point. Adds the hop to the vlink and returns the hop element that was created
DOMElement* annotate_rspec::create_component_hop (const DOMElement* plink, DOMElement* vlink, int endpoint_interface, const DOMElement* prev_component_hop)
{
	// Create a single_hop_link element
	DOMElement* component_hop = doc->createElement(XStr("component_hop").x());
	copy_component_spec(plink, component_hop);

	DOMElement* component_hop_interface = doc->createElement(XStr("interface").x());
		
	// We assume the first interface is the source and the second is the destination
	DOMNodeList* pinterfaces = plink->getElementsByTagName(XStr("interface_ref").x());
	DOMElement* plink_src_iface = dynamic_cast<DOMElement*>(pinterfaces->item(0));
	DOMElement* plink_dst_iface = dynamic_cast<DOMElement*>(pinterfaces->item(1));
				
	DOMNodeList* vinterfaces = vlink->getElementsByTagName(XStr("interface_ref").x());
	DOMElement* vlink_src_iface = dynamic_cast<DOMElement*>(vinterfaces->item(0));
	DOMElement* vlink_dst_iface = dynamic_cast<DOMElement*>(vinterfaces->item(1));
		
	// If the previous component hop is not specified (NULL),
	// then the link is either direct or the direction is guaranteed to be from the node to the switch
	DOMElement* plink_src_iface_clone = dynamic_cast<DOMElement*>(doc->importNode(dynamic_cast<DOMNode*>(plink_src_iface), true));
	DOMElement* plink_dst_iface_clone = dynamic_cast<DOMElement*>(doc->importNode(dynamic_cast<DOMNode*>(plink_dst_iface), true));
	// If the previous component is specified,
	// the link specification could be the opposite of what we need
	if (prev_component_hop != NULL)
	{
		// Find the destination of the previous component hop
		DOMElement* prev_hop_dst_iface = dynamic_cast<DOMElement*>((prev_component_hop->getElementsByTagName(XStr("interface_ref").x()))->item(1));
		XStr prev_hop_dst_uuid (find_urn(prev_hop_dst_iface,
                                                 "component_node"));
		
		// We need to do this because in advertisements, all links are from nodes to switches
		// and we need to reverse this order for the last hop of a multi-hop path
		// This is slightly more expensive, but definitely more robust than checking based on whether a destination interface was specified
		if (strcmp(prev_hop_dst_uuid.c(),
                           XStr(find_urn(plink_dst_iface,
                                         "component_node")).c()) == 0)
		{
			plink_src_iface_clone = dynamic_cast<DOMElement*>(doc->importNode(dynamic_cast<DOMNode*>(plink_dst_iface), true));
			plink_dst_iface_clone = dynamic_cast<DOMElement*>(doc->importNode(dynamic_cast<DOMNode*>(plink_src_iface), true));
		}
	}
	
	// If the source interface is an end point
	if (endpoint_interface == SOURCE || endpoint_interface == BOTH)
		set_interface_as_link_endpoint(plink_src_iface_clone, XStr(vlink_src_iface->getAttribute(XStr("virtual_node_id").x())).c());
	
	// If the destination interface is an end point
	if (endpoint_interface == DESTINATION || endpoint_interface == BOTH)
		set_interface_as_link_endpoint(plink_dst_iface_clone, XStr(vlink_dst_iface->getAttribute(XStr("virtual_node_id").x())).c());
		
	// Add interface specifications to the link in the single hop element
	component_hop->appendChild(plink_src_iface_clone);
	component_hop->appendChild(plink_dst_iface_clone);
		
	vlink->appendChild(component_hop);
	return (component_hop);
}

// Annotates the interface element on a link and updates the node which is the end point of the link as well
void annotate_rspec::annotate_interface (const DOMElement* plink, DOMElement* vlink, int interface_number)
{
	DOMNodeList* vinterfaces = vlink->getElementsByTagName(XStr("interface_ref").x());
	DOMElement* vlink_iface = dynamic_cast<DOMElement*>(vinterfaces->item(interface_number));
			
	// Get the virtual_id on the end points of the interface
	XStr vlink_iface_virtual_id (vlink_iface->getAttribute(XStr("virtual_node_id").x()));
	DOMElement* vnode = getElementByAttributeValue(this->virtual_root, "node", "virtual_id", vlink_iface_virtual_id.c());
	XStr node_component_uuid (find_urn(vnode, "component"));
//        XStr node_component_uuid (vnode->getAttribute(XStr("component_uuid").x()));
	
	DOMElement* p_iface = getElementByAttributeValue(plink, "interface_ref", "component_node_uuid", node_component_uuid.c());
        if (p_iface == NULL)
        {
          p_iface = getElementByAttributeValue(plink, "interface_ref", "component_node_urn", node_component_uuid.c());
        }

	vlink_iface->setAttribute(XStr("component_node_uuid").x(), p_iface->getAttribute(XStr("component_node_uuid").x()));
	vlink_iface->setAttribute(XStr("component_interface_id").x(), p_iface->getAttribute(XStr("component_interface_id").x()));
	
	XStr component_interface_name (vlink_iface->getAttribute(XStr("component_interface_id").x()));
	XStr virtual_interface_name (vlink_iface->getAttribute(XStr("virtual_interface_id").x()));
	
	// Get the interface for the node and update 
	DOMElement* vnode_iface_decl = getElementByAttributeValue(vnode, "interface", "virtual_id", virtual_interface_name.c());
	vnode_iface_decl->setAttribute (XStr("component_name").x(), component_interface_name.x());
}

// Copies the component spec from the source to the destination
void annotate_rspec::copy_component_spec(const DOMElement* src, DOMElement* dst)
{
	if (src->hasAttribute (XStr("component_name").x()))
		dst->setAttribute (XStr("component_name").x(), XStr(src->getAttribute(XStr("component_name").x())).x());
	dst->setAttribute (XStr("component_uuid").x(), XStr(src->getAttribute(XStr("component_uuid").x())).x());
	dst->setAttribute (XStr("component_manager_uuid").x(), XStr(src->getAttribute(XStr("component_manager_uuid").x())).x());
}

// If the interface belongs to an end point of the link, and additional virtual_id attribute has to be added to it
void annotate_rspec::set_interface_as_link_endpoint (DOMElement* interface, const char* virtual_id)
{
	interface->setAttribute(XStr("virtual_id").x(), XStr(virtual_id).x());
}

// Finds the next link in the path returned by assign
// Assign sometimes reverses the links on the path from the source to the destination, 
// so you need to look at the entire path to find the next link
DOMElement* annotate_rspec::find_next_link_in_path (DOMElement *prev, list<const char*>* links)
{
	list<const char*>::iterator it;
	DOMElement* link = NULL;
	for (it = links->begin(); it != links->end(); ++it)
	{
		link = (this->physical_elements->find(*it))->second;
		XStr link_src(find_urn(getNthInterface(link,0),
                                       "component_node"));
		XStr link_dst(find_urn(getNthInterface(link,1),
                                       "component_node"));
		XStr prev_dst(find_urn(getNthInterface(prev,1),
                                       "component_node"));
		if (strcmp(link_src.c(), prev_dst.c()) == 0 || strcmp(link_dst.c(), prev_dst.c()) == 0)
		{
			links->remove(*it);
			break;
		}
	}
	return link;
}

#endif

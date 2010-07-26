/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003-2009 University of Utah and the Flux Group.
 * All rights reserved.
 */

static const char rcsid[] = "$Id: annotate_rspec_v2.cc,v 1.9 2009-10-21 20:49:26 tarunp Exp $";

#ifdef WITH_XML

#include "annotate.h"
#include "annotate_rspec_v2.h"

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

annotate_rspec_v2 :: annotate_rspec_v2 ()
{
  this->virtual_root = request_root;
  this->physical_elements = advertisement_elements;
  
  vector<DOMElement*> lan_links 
    = getElementsHavingAttribute(this->virtual_root, "link", "is_lan");
  vector<DOMElement*>::iterator it;
  for (it = lan_links.begin(); it < lan_links.end(); it++) {
    DOMElement* lan_link = *it;
    // Removing annotations inserted earlier
    lan_link->removeAttribute(XStr("is_lan").x());
    string lan_link_id 
      = string(XStr(lan_link->getAttribute(XStr("client_id").x())).c());
    set<string> virtual_interface_ids;
    DOMNodeList* interfaces 
      = lan_link->getElementsByTagName(XStr("interface_ref").x());
    for (int j = 0; j < interfaces->getLength(); j++) {
      DOMElement* interface = dynamic_cast<DOMElement*>(interfaces->item(j));
      virtual_interface_ids.insert
	(string(XStr(interface->getAttribute
		     (XStr("virtual_interface_id").x())).c()));
    }
    this->lan_links_map.insert
      (pair< string, set<string> >(lan_link_id, virtual_interface_ids));
  }
}

// Annotate a trivial link
void annotate_rspec_v2::annotate_element (const char* v_name)
{
  DOMElement* vlink 
    = getElementByAttributeValue(this->virtual_root, 
				 "link", "client_id", v_name);
  annotate_interface (vlink, 0);
  annotate_interface (vlink, 1);	
  create_component_hop(vlink);
}

// This will get called when a node or a direct link needs to be annotated
void annotate_rspec_v2::annotate_element (const char* v_name, const char* p_name)
{
  DOMElement* vnode 
    = getElementByAttributeValue(this->virtual_root, 
				 "node", "client_id", v_name);
  // If a vnode by that name was found, then go ahead. 
  // If not, that element should be a link
  // We are not terribly concerned about 
  // having to scan the entire physical topology twice
  // because direct links are never really going to happen
  if (vnode != NULL) {
    if (!vnode->hasAttribute(XStr("generated_by_assign").x())) {
      DOMElement* pnode = (this->physical_elements->find(p_name))->second;
      vnode->setAttribute(XStr("component_id").x(),
			  pnode->getAttribute(XStr("component_id").x()));
      vnode->setAttribute(XStr("component_manager_id").x(),
			  pnode->getAttribute(XStr("component_manager_id").x()));
    }
  }
  else {
    DOMElement* vlink 
      = getElementByAttributeValue(this->virtual_root, 
				   "link", "client_id", v_name);
    DOMElement* plink = (this->physical_elements->find(p_name))->second;
    
    // If plink is NULL, then it must be a trivial link
    if (plink == NULL) {
			
    }
    annotate_interface (plink, vlink, 0);
    annotate_interface (plink, vlink, 1);
    
    create_component_hop(plink, vlink, BOTH, NULL);
    
    if (vlink->hasAttribute(XStr("generated_by_assign").x())) {
      string str_lan_link 
	= string(XStr(vlink->getAttribute(XStr("lan_link").x())).c());
      DOMElement* lan_link 
	= getElementByAttributeValue(this->virtual_root, "link", 
				     "client_id", 
				     str_lan_link.c_str());
      DOMNodeList* component_hops 
	= vlink->getElementsByTagName(XStr("component_hop").x());
      for (int i = 0; i < component_hops->getLength(); i++) {
	DOMElement* component_hop 
	  = dynamic_cast<DOMElement*>(component_hops->item(i));
	copy_component_hop(lan_link, component_hop);
      }
    }
  }
}

// This is called when an intraswitch or interswitch link has to be annotated
void annotate_rspec_v2::annotate_element (const char* v_name, 
					  list<const char*>* links)
{
  // These are the paths from the source to the first switch
  // and from the last switch to the destination
  const char* psrc_name = links->front();	
  const char* pdst_name = links->back();	
  DOMElement* p_src_switch_link 
    = (this->physical_elements->find(psrc_name))->second;
  DOMElement* p_switch_dst_link 
    = (this->physical_elements->find(pdst_name))->second;
  
  // Remove these links from the list
  // If it is an intra-switch link, the list should now be empty.
  links->pop_front();
  links->pop_back();
  
  DOMElement* vlink 
    = getElementByAttributeValue (this->virtual_root, "link", 
				  "client_id", v_name);
  annotate_interface(p_src_switch_link, vlink, 0);
  annotate_interface(p_switch_dst_link, vlink, 1);
  
  DOMElement* prev_component_hop 
    = create_component_hop (p_src_switch_link, vlink, SOURCE, NULL);
#if 0
  for (DOMElement *prev_link_in_path = p_src_switch_link; !links->empty(); ) {
    DOMElement* p_switch_switch_link 
      = find_next_link_in_path (prev_link_in_path, links);
    prev_component_hop 
      = create_component_hop (p_switch_switch_link, 
			      vlink, NEITHER, prev_component_hop);
    prev_link_in_path = p_switch_switch_link;
  }
#else
  {
    static int gave_apology;

    if( !links->empty() && !gave_apology ) {
      gave_apology = 1;
      cerr << "Warning: unable to locate interfaces on "
	"switch/switch links; omitting those\n";
    }
  }
#endif
  {
    create_component_hop (p_switch_dst_link, 
			  vlink, DESTINATION, prev_component_hop);
  }
}

// Creates a component_hop for a trivial link
// Adds the hop to the vlink and returns the hop element that was created
DOMElement* annotate_rspec_v2::create_component_hop (DOMElement* vlink)
{
  DOMNodeList* interfaces 
    = vlink->getElementsByTagName(XStr("interface_ref").x());
  DOMElement* src_iface = dynamic_cast<DOMElement*>(interfaces->item(0));
  DOMElement* dst_iface = dynamic_cast<DOMElement*>(interfaces->item(1));
  
  const char* src_id 
    = XStr(src_iface->getAttribute(XStr("virtual_node_id").x())).c();
  const char* dst_id 
    = XStr(dst_iface->getAttribute(XStr("virtual_node_id").x())).c();
  
  DOMElement* src_vnode 
    = getElementByAttributeValue(this->virtual_root,
				 "node", "client_id", src_id);
  DOMElement* dst_vnode 
    = getElementByAttributeValue(this->virtual_root,
				 "node", "client_id", dst_id);
  
  XStr src_component_id (find_urn(src_vnode, "component"));
  XStr dst_component_id (find_urn(dst_vnode, "component"));
  
  DOMElement* component_hop = doc->createElement(XStr("component_hop").x());
  
  DOMElement* src_iface_clone 
    = dynamic_cast<DOMElement*>
    (doc->importNode(dynamic_cast<DOMNode*>(src_iface),true));
  src_iface_clone->setAttribute(XStr("component_node_id").x(),
				src_component_id.x());
  src_iface_clone->setAttribute(XStr("component_interface_id").x(),
				XStr("loopback").x()); 
  
  cerr << src_component_id.c() << " AND " << dst_component_id.c() << endl;
  
  DOMElement* dst_iface_clone 
    = dynamic_cast<DOMElement*>(doc->importNode
				(dynamic_cast<DOMNode*>(dst_iface),true));
  dst_iface_clone->setAttribute(XStr("component_node_id").x(),
				dst_component_id.x());
  dst_iface_clone->setAttribute(XStr("component_interface_id").x(),
				XStr("loopback").x());
  
  component_hop->appendChild(src_iface_clone);
  component_hop->appendChild(dst_iface_clone);
  vlink->appendChild(component_hop);
  return component_hop;
}

// Creates a hop from a switch till the next end point. 
// Adds the hop to the vlink and returns the hop element that was created
DOMElement* 
annotate_rspec_v2::create_component_hop (const DOMElement* plink, 
				      DOMElement* vlink, 
				      int endpoint_interface, 
				      const DOMElement* prev_component_hop)
{
  // Create a single_hop_link element
  DOMElement* component_hop = doc->createElement(XStr("component_hop").x());
  copy_component_spec(plink, component_hop);
  
  DOMElement* component_hop_interface 
    = doc->createElement(XStr("interface").x());
  
  // We assume the first interface is the source and the second the dest
  DOMNodeList* pinterfaces 
    = plink->getElementsByTagName(XStr("interface_ref").x());
  DOMElement* plink_src_iface =dynamic_cast<DOMElement*>(pinterfaces->item(0));
  DOMElement* plink_dst_iface =dynamic_cast<DOMElement*>(pinterfaces->item(1));
  
  DOMNodeList* vinterfaces 
    = vlink->getElementsByTagName(XStr("interface_ref").x());
  DOMElement* vlink_src_iface =dynamic_cast<DOMElement*>(vinterfaces->item(0));
  DOMElement* vlink_dst_iface =dynamic_cast<DOMElement*>(vinterfaces->item(1));
  
  // If the previous component hop is not specified (NULL),
  // then the link is either direct 
  // or the direction is guaranteed to be from the node to the switch
  DOMElement* plink_src_iface_clone 
    = dynamic_cast<DOMElement*>(doc->importNode
				(dynamic_cast<DOMNode*>(plink_src_iface),true));
  DOMElement* plink_dst_iface_clone 
    = dynamic_cast<DOMElement*>(doc->importNode
				(dynamic_cast<DOMNode*>(plink_dst_iface),true));
  // If the previous component is specified,
  // the link specification could be the opposite of what we need
  if (prev_component_hop != NULL)  {
    // Find the destination of the previous component hop
    DOMElement* prev_hop_dst_iface 
      = dynamic_cast<DOMElement*>((prev_component_hop->getElementsByTagName
				   (XStr("interface_ref").x()))->item(1));
    XStr prev_hop_dst_uuid (find_urn(prev_hop_dst_iface, "component_node"));
    
    // We need to do this because in advertisements, 
    // all links are from nodes to switches
    // and we need to reverse this for the last hop of a multi-hop path
    // This is slightly more expensive, 
    // but definitely more robust than checking based on 
    // whether a destination interface was specified
    if (strcmp(prev_hop_dst_uuid.c(),
	       XStr(find_urn(plink_dst_iface,
			     "component_node")).c()) == 0) {
      plink_src_iface_clone 
	= dynamic_cast<DOMElement*>
	(doc->importNode(dynamic_cast<DOMNode*>
			 (plink_dst_iface), true));
      plink_dst_iface_clone 
	= dynamic_cast<DOMElement*>
	(doc->importNode(dynamic_cast<DOMNode*>
			 (plink_src_iface), true));
    }
  }
  
  // If the source interface is an end point
  if (endpoint_interface == SOURCE || endpoint_interface == BOTH) {
    set_interface_as_link_endpoint
      (plink_src_iface_clone, 
       XStr(vlink_src_iface->getAttribute(XStr("virtual_node_id").x())).c(), 
       XStr(vlink_src_iface->getAttribute(XStr("virtual_interface_id").x())).c());
  }
  
  // If the destination interface is an end point
  if (endpoint_interface == DESTINATION || endpoint_interface == BOTH) {
    set_interface_as_link_endpoint
      (plink_dst_iface_clone, 
       XStr(vlink_dst_iface->getAttribute(XStr("virtual_node_id").x())).c(),
       XStr(vlink_dst_iface->getAttribute(XStr("virtual_interface_id").x())).c());
  }
  
  // Add interface specifications to the link in the single hop element
  component_hop->appendChild(plink_src_iface_clone);
  component_hop->appendChild(plink_dst_iface_clone);
  
  vlink->appendChild(component_hop);
  return (component_hop);
}

// Copies the component_hop from the generated_link to the requsted_link
// Also strips the unnecessary annotations from the component_hop after copying
// The "unnecessary annotations" 
// specify the end point of the link to be the auto-generated lan node
// We could just assume that the second interface_ref in the hop contains these
// "unnecessary annotations" since we have generated it in the first place
// and we can sure that it will always be in exactly that same position,
// but it sounds like a dirty way of doing it and is not exactly robust,
// so we shall use the slower, but more robust method
void annotate_rspec_v2::copy_component_hop(DOMElement* lan_link, 
					DOMElement* component_hop)
{
  string lan_link_id 
    = string(XStr(lan_link->getAttribute(XStr("client_id").x())).c());
  DOMNodeList* interfaces 
    = component_hop->getElementsByTagName(XStr("interface_ref").x());
  for (int i = 0; i < interfaces->getLength(); i++) {
    DOMElement* interface = dynamic_cast<DOMElement*>(interfaces->item(i));
    if (interface->hasAttribute(XStr("virtual_interface_id").x())) {
      string str_virtual_interface_id 
	= string(XStr(interface->getAttribute
		      (XStr("virtual_interface_id").x())).c());
      if (!has_interface_with_id (lan_link_id, str_virtual_interface_id)) {
	interface->removeAttribute(XStr("virtual_interface_id").x());
	interface->removeAttribute(XStr("virtual_node_id").x());
      }	
    }
  }
  lan_link->appendChild(dynamic_cast<DOMElement*>
			(doc->importNode(component_hop, true)));
}

// Checks if the link contains an interface with virtual_interface_id = id
bool annotate_rspec_v2::has_interface_with_id (string link_client_id, string id)
{
  map< string, set<string> >::iterator map_it;
  set<string>::iterator set_it;
  map_it = lan_links_map.find(link_client_id);
  if (map_it == this->lan_links_map.end())
    return false;
  set<string> client_ids = map_it->second;
  set_it = client_ids.find(id);
  if (set_it == client_ids.end())
    return false;
  return true;
}

// Annotates the interfaces on a node making up the end points of a trivial link
void annotate_rspec_v2::annotate_interface (const DOMElement* vlink, 
					 int interface_number)
{
  this->annotate_interface (NULL, vlink, interface_number, true);
}

// Annotates the interfaces on a non-trivial link
void annotate_rspec_v2::annotate_interface (const DOMElement* plink,
					 const DOMElement* vlink,
					 int interface_number)
{
  this->annotate_interface (plink, vlink, interface_number, false);
}

// Updates the node which is the end point of the link
// 1) Find the interface_ref from the list of interface_refs on the link 
//    and it's virtual_interface_id
// 2) Find the virtual node to which the interface_ref is referring 
//    and its corresponding interface
// 3) Use this to find the physical node (component_node_uuid/urn) to which 
//    the virtual node has been mapped
// 4) Find the corresponding interface_ref on the physical link to which 
//    the virtual link has been mapped and get its component_interface_id
// 5) Annotate the interface on the virtual node obtained in 2) 
//    with the interface_id obtained in 4)
void annotate_rspec_v2::annotate_interface (const DOMElement* plink, 
					    const DOMElement* vlink, 
					    int interface_number,
					    bool is_trivial_link)
{
  DOMNodeList* vinterfaces 
    = vlink->getElementsByTagName(XStr("interface_ref").x());
  DOMElement* vlink_iface 
    = dynamic_cast<DOMElement*>(vinterfaces->item(interface_number));
  XStr vlink_iface_virtual_interface_id 
    (vlink_iface->getAttribute(XStr("client_id").x()));
  
  // Get the client_id of the node to which the interface belongs
  XStr vlink_iface_virtual_node_id 
    (vlink_iface->getAttribute(XStr("virtual_node_id").x()));
  DOMElement* vnode 
    = getElementByAttributeValue(this->virtual_root, 
				 "node", 
				 "client_id", 
				 vlink_iface_virtual_node_id.c());
  DOMElement* vnode_iface_decl 
    = getElementByAttributeValue(vnode, "interface", "client_id", 
				 vlink_iface_virtual_interface_id.c());
  XStr component_node_uuid (find_urn(vnode, "component"));
  
  if (!is_trivial_link) {
    DOMElement* p_iface 
      = getElementByAttributeValue(plink, "interface_ref", 
				   "component_node_uuid", 
				   component_node_uuid.c());
    if (p_iface == NULL) {
      p_iface = getElementByAttributeValue(plink, "interface_ref", 
					   "component_node_urn", 
					   component_node_uuid.c());
    }
    
    XStr component_interface_id 
      (p_iface->getAttribute(XStr("component_interface_id").x()));
    vnode_iface_decl->setAttribute 
      (XStr("component_id").x(), component_interface_id.x());
  }
  else {
    vnode_iface_decl->setAttribute(XStr("component_id").x(), 
				   XStr("loopback").x());
  }
}

// Copies the component spec from the source to the destination
void annotate_rspec_v2::copy_component_spec(const DOMElement* src, DOMElement* dst)
{
  if (src->hasAttribute (XStr("component_name").x()))
    dst->setAttribute (XStr("component_name").x(), 
		       XStr(src->getAttribute
			    (XStr("component_name").x())).x());
  dst->setAttribute (XStr("component_uuid").x(), 
		     XStr(src->getAttribute(XStr("component_uuid").x())).x());
  dst->setAttribute (XStr("component_manager_uuid").x(), 
		     XStr(src->getAttribute
			  (XStr("component_manager_uuid").x())).x());
}

// If the interface belongs to an end point of the link, 
// and additional client_id attribute has to be added to it
void annotate_rspec_v2::set_interface_as_link_endpoint (DOMElement* interface, 
						     const char* virtual_node_id, 
						     const char* virtual_interface_id)
{
  interface->setAttribute(XStr("virtual_node_id").x(), 
			  XStr(virtual_node_id).x());
  interface->setAttribute(XStr("virtual_interface_id").x(), 
			  XStr(virtual_interface_id).x());
}

// Finds the next link in the path returned by assign
// Assign sometimes reverses the links on the path 
// from the source to destination, 
// so you need to look at the entire path to find the next link
DOMElement* annotate_rspec_v2::find_next_link_in_path (DOMElement *prev, 
						    list<const char*>* links)
{
  list<const char*>::iterator it;
  DOMElement* link = NULL;
  for (it = links->begin(); it != links->end(); ++it) {
    link = (this->physical_elements->find(*it))->second;
    XStr link_src(find_urn(getNthInterface(link,0),
			   "component_node"));
    XStr link_dst(find_urn(getNthInterface(link,1),
			   "component_node"));
    XStr prev_dst(find_urn(getNthInterface(prev,1),
			   "component_node"));
    if (strcmp(link_src.c(), prev_dst.c()) == 0 
	|| strcmp(link_dst.c(), prev_dst.c()) == 0) {
      links->remove(*it);
      break;
    }
  }
  return link;
}

void annotate_rspec_v2::cleanup()
{
  vector<DOMElement*>::iterator it;
  
  // Remove generated links
  vector<DOMElement*> generated_links 
    = getElementsHavingAttribute(this->virtual_root, "link", 
				 "generated_by_assign");
  for (it = generated_links.begin(); it < generated_links.end(); it++) {
    DOMNode* generated_link = dynamic_cast<DOMNode*>(*it);
    dynamic_cast<DOMNode*>(this->virtual_root)->removeChild(generated_link);
  }
  
  // Remove generated nodes
  vector<DOMElement*> generated_nodes 
    = getElementsHavingAttribute(this->virtual_root, "node", 
				 "generated_by_assign");
  for (it = generated_nodes.begin(); it < generated_nodes.end(); it++) {
    DOMNode* generated_link = dynamic_cast<DOMNode*>(*it);
    dynamic_cast<DOMNode*>(this->virtual_root)->removeChild(generated_link);
  }
  
  // Remove additional attributes added to elements
  
  // Remove the is_lan attribute
  vector<DOMElement*> lan_links 
    = getElementsHavingAttribute(this->virtual_root, "link", "is_lan");
  for (it = lan_links.begin(); it < lan_links.end(); it++) {
    DOMElement* lan_link = *it;
    lan_link->removeAttribute(XStr("is_lan").x());
  }
}

bool annotate_rspec_v2::is_generated_element(const char* tag, 
					     const char* attr_name, 
					     const char* attr_value)
{
  DOMElement* element 
    = getElementByAttributeValue(this->virtual_root, tag, 
				 attr_name, attr_value);
  if (element == NULL)
    return false;
  return (element->hasAttribute(XStr("generated_by_assign").x()));
}

#endif

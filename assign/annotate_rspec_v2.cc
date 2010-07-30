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
extern DOMElement* advt_root;

extern map<string, DOMElement*>* advertisement_elements;

extern map<string,string>* pIfacesMap;
extern map<string,string>* vIfacesMap;
				 
XERCES_CPP_NAMESPACE_USE
using namespace std;

annotate_rspec_v2 :: annotate_rspec_v2 ()
{
  this->document = doc;
  this->virtual_root = request_root;
  this->physical_root = advt_root;
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

  // Go through the entire virtual topology and create a map
  // of interfaces and their parent nodes
  // XXX: This is actually present in the rspecParser, but I can't think of 
  // a clean way of getting it in here without introducing version dependent
  // code in the parse_request_rspec.cc 
  //  this->vIfacesMap = this->populateIfaceMap(this->virtual_root, "client_id");
  this->vInterfaceMap = vIfacesMap;
  
  // Do the same for the physical topology
  // XXX: This is very inefficient. Need to find some way of using this data
  // since it's already present in the rspecParser. It's ok for the virtual 
  // topology since it's small, but the physical topology is big 
  //  this->pIfacesMap =this->populateIfaceMap(this->physical_root,"component_id");
  this->pInterfaceMap = pIfacesMap;
}

// Annotate a trivial link
void annotate_rspec_v2::annotate_element (const char* v_name)
{
  DOMElement* vlink 
    = getElementByAttributeValue(this->virtual_root, 
				 "link", "client_id", v_name);
  annotate_interface (vlink, 0);
  annotate_interface (vlink, 1);	
  DOMElement* hop = create_component_hop(vlink);
  vlink->appendChild(hop);
}

// This will get called when a node or a direct link needs to be annotated
void 
annotate_rspec_v2::annotate_element (const char* v_name, const char* p_name)
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
      if (pnode->hasAttribute(XStr("component_name").x())) {
	vnode->setAttribute(XStr("component_name").x(),
			    pnode->getAttribute(XStr("component_name").x()));
      }
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
    // XXX: Add trivial link support?
    if (plink == NULL) {
			
    }
    annotate_interface (plink, vlink, 0);
    annotate_interface (plink, vlink, 1);
    
    DOMElement* hop = create_component_hop(plink, vlink, BOTH, NULL);
    vlink->appendChild(hop);
    
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
  
  vector<DOMElement*> componentHops;
  DOMElement* prev_component_hop 
    = create_component_hop (p_src_switch_link, vlink, SOURCE, NULL);
  componentHops.push_back(prev_component_hop);
#if 0
  {
  for (DOMElement *prev_link_in_path = p_src_switch_link; !links->empty(); ) {
    DOMElement* p_switch_switch_link 
      = find_next_link_in_path (prev_link_in_path, links);
    prev_component_hop 
      = create_component_hop (p_switch_switch_link, 
			      vlink, NEITHER, prev_component_hop);
    prev_link_in_path = p_switch_switch_link;
    componentHops.push_back(prev_component_hop);
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
    DOMElement* hop 
      = create_component_hop (p_switch_dst_link, vlink, DESTINATION, 
			      prev_component_hop);
    componentHops.push_back(hop);
    for (int i = 0; i < componentHops.size(); i++) {
      vlink->appendChild(componentHops[i]);
    }
  }
}

// Creates a component_hop for a trivial link
DOMElement* annotate_rspec_v2::create_component_hop (DOMElement* vlink)
{
  DOMNodeList* ifaces = vlink->getElementsByTagName(XStr("interface_ref").x());
  DOMElement* srcIface = dynamic_cast<DOMElement*>(ifaces->item(0));
  DOMElement* dstIface = dynamic_cast<DOMElement*>(ifaces->item(1));

  string srcIfaceId = XStr(srcIface->getAttribute(XStr("client_id").x())).c();
  string dstIfaceId = XStr(dstIface->getAttribute(XStr("client_id").x())).c();

  bool fnd;
  string srcIfaceNodeId 
    = this->lookupIface(this->vInterfaceMap, srcIfaceId, fnd);
  string dstIfaceNodeId 
    = this->lookupIface(this->vInterfaceMap, dstIfaceId, fnd);

  DOMElement* srcVnode 
    = getElementByAttributeValue(this->virtual_root,
				 "node", "client_id", srcIfaceNodeId.c_str());
  DOMElement* dstVnode 
    = getElementByAttributeValue(this->virtual_root,
				 "node", "client_id", dstIfaceNodeId.c_str());
  
  DOMElement* srcIfaceClone 
    = dynamic_cast<DOMElement*>(doc->importNode
				(dynamic_cast<DOMNode*>(srcIface),true));
  srcIfaceClone->setAttribute(XStr("component_id").x(), XStr("loopback").x()); 
  
  DOMElement* dstIfaceClone
    = dynamic_cast<DOMElement*>(doc->importNode
				(dynamic_cast<DOMNode*>(dstIface),true));
  dstIfaceClone->setAttribute(XStr("component_id").x(), XStr("loopback").x());

  DOMElement* hop = doc->createElement(XStr("component_hop").x());  
  hop->appendChild(srcIfaceClone);
  hop->appendChild(dstIfaceClone);

  return hop;
}

// Creates a hop from a switch till the next end point. 
DOMElement* 
annotate_rspec_v2::create_component_hop (const DOMElement* plink, 
					 DOMElement* vlink, 
					 int endpointIface, 
					 const DOMElement* prevHop)
{
  // We assume the first interface is the source and the second the dest
  DOMNodeList* pIfaces =plink->getElementsByTagName(XStr("interface_ref").x());
  DOMElement* plinkSrcIface = dynamic_cast<DOMElement*>(pIfaces->item(0));
  DOMElement* plinkDstIface = dynamic_cast<DOMElement*>(pIfaces->item(1));
  
  DOMNodeList* vIfaces =vlink->getElementsByTagName(XStr("interface_ref").x());
  DOMElement* vlinkSrcIface = dynamic_cast<DOMElement*>(vIfaces->item(0));
  DOMElement* vlinkDstIface = dynamic_cast<DOMElement*>(vIfaces->item(1));

  string vlinkSrcIfaceId 
    = XStr(vlinkSrcIface->getAttribute(XStr("client_id").x())).c();

  string vlinkDstIfaceId 
    = XStr(vlinkDstIface->getAttribute(XStr("client_id").x())).c();
  
  // If the previous component hop is not specified (NULL),
  // then the link is either direct 
  // or the direction is guaranteed to be from the node to the switch
  DOMElement* plinkSrcIfaceClone 
    = dynamic_cast<DOMElement*>(doc->importNode
				(dynamic_cast<DOMNode*>(plinkSrcIface),true));
  DOMElement* plinkDstIfaceClone
    = dynamic_cast<DOMElement*>(doc->importNode
				(dynamic_cast<DOMNode*>(plinkDstIface),true));

  // If the previous component is specified,
  // the link specification could be the opposite of what we need
  if (prevHop != NULL)  {
    // Find the destination of the previous component hop
    DOMElement* prevHopDstIface
      = dynamic_cast<DOMElement*>((prevHop->getElementsByTagName
				   (XStr("interface_ref").x()))->item(1));

    bool found = false;
    string prevHopDstIfaceId 
      = XStr(prevHopDstIface->getAttribute(XStr("component_id").x())).c();
    string prevHopDstNodeId = this->lookupIface(this->pInterfaceMap, 
						prevHopDstIfaceId, found);

    // We need to do this because in advertisements, 
    // all links are from nodes to switches
    // and we need to reverse this for the last hop of a multi-hop path
    // This is slightly more expensive, 
    // but definitely more robust than checking based on 
    // whether a destination interface was specified
    string plinkDstIfaceId 
      = XStr(plinkDstIface->getAttribute(XStr("component_id").x())).c();
    string plinkDstNodeId = this->lookupIface(this->pInterfaceMap, 
					      plinkDstIfaceId, found);
    if (prevHopDstNodeId == plinkDstNodeId) {
      plinkSrcIfaceClone = dynamic_cast<DOMElement*>(doc->importNode
						     (dynamic_cast<DOMNode*>
						      (plinkDstIface), true));
      plinkDstIfaceClone = dynamic_cast<DOMElement*>(doc->importNode
						     (dynamic_cast<DOMNode*>
						      (plinkSrcIface), true));
    }
  }
  
  // If the source interface is an end point
  if (endpointIface == SOURCE || endpointIface == BOTH) {
    plinkSrcIfaceClone->setAttribute(XStr("client_id").x(),
				     XStr(vlinkSrcIfaceId.c_str()).x());
  }
  
  // If the destination interface is an end point
  if (endpointIface == DESTINATION || endpointIface == BOTH) {
    plinkDstIfaceClone->setAttribute(XStr("client_id").x(),
				     XStr(vlinkDstIfaceId.c_str()).x());
  }

  // Create a single_hop_link element
  DOMElement* hop = doc->createElement(XStr("component_hop").x());
  copy_component_spec(plink, hop);
  
  // Add interface specifications to the link in the single hop element
  hop->appendChild(plinkSrcIfaceClone);
  hop->appendChild(plinkDstIfaceClone);
  
  return hop;
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
					    int ifaceNumber,
					    bool isTrivialLink)
{
  DOMNodeList* refs = vlink->getElementsByTagName(XStr("interface_ref").x());
  DOMElement* ref = dynamic_cast<DOMElement*>(refs->item(ifaceNumber));
  string ifaceId = XStr(ref->getAttribute(XStr("client_id").x())).c();
  
  // Get the client_id of the node to which the interface belongs
  bool found = false;
  string nodeId = this->lookupIface(this->vInterfaceMap, ifaceId, found);
  DOMElement* vnode = getElementByAttributeValue(this->virtual_root, "node", 
						 "client_id", nodeId.c_str());
  DOMElement* decl = getElementByAttributeValue(vnode, "interface", 
						"client_id", ifaceId.c_str());

  string physNodeId = XStr(vnode->getAttribute(XStr("component_id").x())).c();

  if (!isTrivialLink) {
    // Find the interface on the physical link which is situated
    // on the physical node to which the vnode has been mapped
    const DOMElement* pIface = this->getIfaceOnNode(plink, physNodeId);
    if (pIface == NULL) {
      cerr << "*** No interface on " 
	   << XStr(plink->getAttribute(XStr("component_id").x())).c()
	   << " is declared on node " << physNodeId << endl;
      exit(EXIT_FATAL);
    }
    decl->setAttribute (XStr("component_id").x(), 
			pIface->getAttribute(XStr("component_id").x()));
  }
  else {
    decl->setAttribute(XStr("component_id").x(), XStr("loopback").x());
  }
}

// Copies the component spec from the source to the destination
void 
annotate_rspec_v2::copy_component_spec(const DOMElement* src, DOMElement* dst)
{
  if (src->hasAttribute (XStr("component_name").x()))
    dst->setAttribute(XStr("component_name").x(), 
		      XStr(src->getAttribute(XStr("component_name").x())).x());

  dst->setAttribute(XStr("component_id").x(), 
		    XStr(src->getAttribute(XStr("component_id").x())).x());
  dst->setAttribute(XStr("component_manager_id").x(), 
		    XStr(src->getAttribute
			 (XStr("component_manager_id").x())).x());
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

string annotate_rspec_v2::lookupIface (map<string,string>* ifacesMap, 
				       string ifaceId,
				       bool& found) 
{
  string nodeId = "";
  found = false;
  map<string, string>::iterator it = ifacesMap->find(ifaceId);
  if (it != ifacesMap->end()) {
    nodeId = it->second;
    found = true;
  }
  return nodeId;
}

// Returns that interface on the physical link which is declared in 
// the physical node nodeId
const DOMElement* 
annotate_rspec_v2::getIfaceOnNode(const DOMElement* plink, string nodeId)
{
  DOMNodeList* refs = plink->getElementsByTagName(XStr("interface_ref").x());
  for (int i = 0; i < refs->getLength(); i++) {
    bool found = false;
    DOMElement* ref = dynamic_cast<DOMElement*>(refs->item(i));
    string ifaceId = XStr(ref->getAttribute(XStr("component_id").x())).c();
    string declNodeId = this->lookupIface(this->pInterfaceMap, ifaceId, found);
    if (found && (declNodeId == nodeId)) {
      return ref;
    }
  }
  return NULL;
}

#endif

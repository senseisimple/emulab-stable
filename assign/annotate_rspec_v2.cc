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

// TODO: Link mapping has been disabled. Re-enable this when all bugs 
// related to interfaces on switches have been fixed.

annotate_rspec_v2 :: annotate_rspec_v2 ()
{
  this->document = doc;
  this->virtual_root = request_root;
  this->physical_root = advt_root;
  this->physical_elements = advertisement_elements;

#ifdef DISABLE_LINK_ANNOTATION
    cerr << "WARNING: Annotation code will execute, "
	 << "but links will not be annotated" << endl;
#endif
  
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

  this->vInterfaceMap = vIfacesMap;
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
#ifndef DISABLE_LINK_ANNOTATION
    vlink->appendChild(hop);
#endif
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
#ifndef DISABLE_LINK_ANNOTATION
      vlink->appendChild(hop);
#endif    
    
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
  // We can't locate interfaces on the switches, so we don't add those 
  // to the annotation. The warning is only given the one time
  static bool gave_apology = false;

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
  DOMElement* prevComponentHop 
    = create_component_hop (p_src_switch_link, vlink, SOURCE, NULL);
  componentHops.push_back(prevComponentHop);
    
  for (DOMElement *prevLinkInPath = p_src_switch_link; !links->empty(); ) {
#ifndef DISABLE_LINK_ANNOTATION
    if (!gave_apology) { 
      gave_apology = true;
      cerr << endl << "\nWARNING: Unable to locate interfaces on "
	"switch/switch links; omitting those from the annotation" << endl;
    }
#endif
    DOMElement* pSwitchSwitchLink 
      = find_next_link_in_path (prevLinkInPath, links);
    prevComponentHop = create_component_hop (pSwitchSwitchLink, vlink, 
					     NEITHER, prevComponentHop);
    prevLinkInPath = pSwitchSwitchLink;
    //    componentHops.push_back(prevComponentHop);
  }

  DOMElement* hop = create_component_hop (p_switch_dst_link, vlink, 
					  DESTINATION, prevComponentHop);
  componentHops.push_back(hop);
#ifndef DISABLE_LINK_ANNOTATION
  for (int i = 0; i < componentHops.size(); i++) {
    vlink->appendChild(componentHops[i]);
  }
#endif
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
  string srcIfaceNodeId =this->lookupIface(this->vInterfaceMap,srcIfaceId,fnd);
  string dstIfaceNodeId =this->lookupIface(this->vInterfaceMap,dstIfaceId,fnd);

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
  
  string plinkSrcIfaceId 
    = XStr(plinkSrcIface->getAttribute(XStr("component_id").x())).c();
  string plinkDstIfaceId 
    = XStr(plinkDstIface->getAttribute(XStr("component_id").x())).c();
  
  bool found = false;
  string plinkSrcNodeId = this->lookupIface(this->pInterfaceMap,
					    plinkSrcIfaceId, found);
  string plinkDstNodeId = this->lookupIface(this->pInterfaceMap, 
					    plinkDstIfaceId, found);

  // If the previous component is specified,
  // the link specification could be the opposite of what we need
  if (prevHop != NULL)  {
    // Find the destination of the previous component hop
    DOMElement* prevHopDstIface
      = dynamic_cast<DOMElement*>((prevHop->getElementsByTagName
				   (XStr("interface_ref").x()))->item(1));

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
    if (prevHopDstNodeId == plinkDstNodeId) {
      plinkSrcIfaceClone 
	= dynamic_cast<DOMElement*>(doc->importNode
				    (dynamic_cast<DOMNode*>(plinkDstIface), 
				     true));
      plinkDstIfaceClone 
	= dynamic_cast<DOMElement*>(doc->importNode
				    (dynamic_cast<DOMNode*>(plinkSrcIface), 
				     true));
    }
  }
  
  //Annotate the link endpoint 
  if (endpointIface != NEITHER) {
    bool annotated;
    annotated = this->annotate_endpoint(plinkSrcIfaceClone, vlinkSrcIfaceId);
    if (!annotated) {
      this->annotate_endpoint(plinkDstIfaceClone, vlinkSrcIfaceId);
    }
    annotated = this->annotate_endpoint(plinkDstIfaceClone, vlinkDstIfaceId);
    if (!annotated) {
      this->annotate_endpoint(plinkSrcIfaceClone, vlinkDstIfaceId);
    }
  }

  // Find the physical nodes for the source and destination interfaces
  // We need these to determine the component manager IDs.
  DOMElement* plinkSrcNode 
    = ((this->physical_elements)->find(plinkSrcNodeId))->second;
  DOMElement* plinkDstNode
    = ((this->physical_elements)->find(plinkDstNodeId))->second;

  plinkSrcIfaceClone->setAttribute
    (XStr("component_manager_id").x(),
     plinkSrcNode->getAttribute(XStr("component_manager_id").x()));
  plinkDstIfaceClone->setAttribute
    (XStr("component_manager_id").x(),
     plinkDstNode->getAttribute(XStr("component_manager_id").x()));

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

  // When the link is a trivial link, the interface name is loopback
  string interface = "loopback";
  string shortName = "";

  // This entire block is ifdef'ed because we have trouble annotating 
  // interfaces when a requested lan node is mapped to a switch
  // Once we have support for interfaces on switches, this problem will go away
  // XXX: While link mapping is disabled, node annotations will be 
  // inconsistent when any one node is mapped onto a switch
#ifndef DISABLE_LINK_ANNOTATION
  if (!isTrivialLink) {
    // Find the interface on the physical link which is situated
    // on the physical node to which the vnode has been mapped
    const DOMElement* pIface = this->getIfaceOnNode(plink, physNodeId);
    if (pIface == NULL) {
      cerr << "\n\n*** No interface on " 
	   << XStr(plink->getAttribute(XStr("component_id").x())).c()
	   << " is declared on node " << physNodeId << endl << endl;
      cerr << "*** Additional information: " << endl;
      cerr << "Virtual link ID: "
	   << XStr(vlink->getAttribute(XStr("client_id").x())).c() << endl;
      cerr << "Interface ID on vlink: " << ifaceId << endl;
      cerr << "Interface declared on vnode: " << nodeId << endl;
      cerr << "Virtual node mapped to: " << physNodeId << endl;
      cerr << "Physical link ID: " 
	   << XStr(plink->getAttribute(XStr("component_id").x())).c() << endl;
      exit(EXIT_FATAL);
    }
    interface = XStr(pIface->getAttribute(XStr("component_id").x())).c();
    shortName = this->getShortInterfaceName(interface);
  }

  // We also need to add the fixedinterface element to this
  // since we need to ensure that when a partially mapped request is fed
  // back into assign, the link isn't mapped to a different set of interfaces
  
  // XXX: This is not robust at all since it assumes that assign's output 
  // will always have the interface name at the end with the form: *:<iface>
  decl->setAttribute(XStr("component_id").x(), XStr(interface).x());
  this->addFixedInterface(decl, shortName);
#endif
}

// Copies the component spec from the source to the destination
void 
annotate_rspec_v2::copy_component_spec(const DOMElement* src, DOMElement* dst)
{
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

    // We assume as we have done throughout that the first interface 
    // represents the source of the link and the second interface
    // is the destination.
    string linkSrc = this->getNodeForNthInterface(link, 0);
    string linkDst = this->getNodeForNthInterface(link, 1);
    string prevDst = this->getNodeForNthInterface(prev, 1);

    if (linkSrc == prevDst || linkDst == prevDst) {
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

// Returns the node id for the nth interface on a link
string annotate_rspec_v2::getNodeForNthInterface (const DOMElement* link, 
						  int number)
{
  bool found  = false;
  DOMNodeList* ifaces = link->getElementsByTagName(XStr("interface_ref").x());
  if (ifaces->getLength() < number) {
    cerr << "*** Link " 
	 << XStr(link->getAttribute(XStr("component_id").x())).c()
	 << " has only " << ifaces->getLength() << " interfaces. "
	 << "Interface number " << number << " requested." << endl;
    exit(EXIT_FATAL);
  }
  DOMElement* iface = dynamic_cast<DOMElement*>(ifaces->item(number));
  string ifaceId = XStr(link->getAttribute(XStr("component_id").x())).c();
  return (this->lookupIface(this->pInterfaceMap, ifaceId, found));
}

// Annotates the end point interface if found
bool annotate_rspec_v2::annotate_endpoint(DOMElement* iface, string virtId) 
{
  bool found = false;
  bool annotated = false;
  string ifaceId = XStr(iface->getAttribute(XStr("component_id").x())).c();
  string physNodeId = this->lookupIface(this->pInterfaceMap, ifaceId, found);
  string virtNodeId = this->lookupIface(this->vInterfaceMap, virtId, found);
  DOMElement* virtNode
    = getElementByAttributeValue(this->virtual_root, "node", "client_id", 
				 virtNodeId.c_str());
  string mappedTo = XStr(virtNode->getAttribute(XStr("component_id").x())).c();
  if (mappedTo == physNodeId) {
    annotated = true;
    iface->setAttribute(XStr("client_id").x(), XStr(virtId).x());
  }
  return annotated;
}

// Adds a fixed interface element to an interface
void annotate_rspec_v2::addFixedInterface (DOMElement* iface, string shortName)
{
  DOMNodeList* fixedIfaces 
    = iface->getElementsByTagName(XStr("emulab:fixedinterface").x());
  // If a fixed interface is not present, we add one.
  if (fixedIfaces->getLength() == 0) {
    DOMElement* fixedIfaceNode 
      = doc->createElement(XStr("emulab:fixedinterface").x());
    fixedIfaceNode->setAttribute(XStr("name").x(), XStr(shortName).x());
    iface->appendChild(fixedIfaceNode);
  }
}

// Extracts the short interface name from the interface URN
string annotate_rspec_v2::getShortInterfaceName (string interface)
{
  int loc = interface.find_last_of(":");
  return interface.substr(loc+1);
}

#endif

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008, 2009 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * XML Parser for RSpec ptop files
 */

static const char rcsid[] = "$Id: parse_request_rspec.cc,v 1.16 2009-10-21 20:49:26 tarunp Exp $";

#ifdef WITH_XML

#include "parse_request_rspec.h"
#include "xmlhelpers.h"
#include "parse_error_handler.h"
#include "rspec_parser_v1.h"
#include "rspec_parser_v2.h"
#include "emulab_extensions_parser.h"

#include <fstream>
#include <sstream>
#include <sys/time.h>

#include "anneal.h"
#include "vclass.h"

#define XMLDEBUG(x) (cerr << x)
#define ISSWITCH(n) (n->types.find("switch") != n->types.end())

#ifdef TBROOT
	#define SCHEMA_LOCATION TBROOT"/lib/assign/rspec-request.xsd"
#else	
	#define SCHEMA_LOCATION "request.xsd"
#endif

using namespace rspec_emulab_extension;

/*
 * XXX: Do I have to release lists when done with them?
 */

/*
 * XXX: Global: This is really bad!
 */
extern name_pvertex_map pname2vertex;

/* --- Have to include the vnode data structures as well --- */
extern name_vvertex_map vname2vertex;
extern name_name_map fixed_nodes;
extern name_name_map node_hints;
extern name_count_map vtypes;
extern name_list_map vclasses;
extern vvertex_vector virtual_nodes;
extern name_vclass_map vclass_map;
/* --- end of vtop stuff --- */

DOMElement* request_root = NULL;
DOMDocument* doc = NULL;

int rspec_version = -1;

int bind_ptop_subnodes(tb_pgraph &pg);
int bind_vtop_subnodes(tb_vgraph &vg);

static rspec_parser* rspecParser;

/*
 * These are not meant to be used outside of this file,so they are only
 * declared in here
 */
static bool populate_nodes(DOMElement *root, 
			   tb_vgraph &vg, 
			   map< pair<string, string>,
			   pair<string, string> >* fixed_interfaces);
static bool populate_links(DOMElement *root, 
			   tb_vgraph &vg, 
			   map< pair<string, string>, 
			   pair<string, string> >* fixed_interfaces);
static bool populate_vclasses (DOMElement* root, tb_vgraph& vg);

DOMElement* appendChildTagWithData (DOMElement* parent, 
				    const char* tag_name, 
				    const char* child_value);
string generate_virtualNodeId (string virtual_id);
string generate_virtualIfaceId(string node_name, int interface_number);

int parse_request(tb_vgraph &vg, char *filename) {
  /* 
   * Fire up the XML domParser
   */
  XMLPlatformUtils::Initialize();
  
  XercesDOMParser *domParser = new XercesDOMParser;
  
  /*
   * Enable some of the features we'll be using: validation, namespaces, etc.
   */
  domParser->setValidationScheme(XercesDOMParser::Val_Always);
  domParser->setDoNamespaces(true);
  domParser->setDoSchema(true);
  domParser->setValidationSchemaFullChecking(true);
  //domParser->setExternalSchemaLocation("http://www.protogeni.net/resources/rspec/2 http://www.protogeni.net/resources/rspec/2/request.xsd");
  
  /*
   * Just use a custom error handler - must admin it's not clear to me why
   * we are supposed to use a SAX error handler for this, but this is what
   * the docs say....
   */    
  ParseErrorHandler* errHandler = new ParseErrorHandler();
  domParser->setErrorHandler(errHandler);
  
  /*
   * Do the actual parse
   */
  domParser->parse(filename);
  XMLDEBUG("XML parse completed" << endl);
  
  /* 
   * If there are any errors, do not go any further
   */
  if (errHandler->sawError()) {
    cerr << "*** There were " << domParser -> getErrorCount () 
	 << " errors in your file. " << endl;
    exit(EXIT_FATAL);
  }
  else {
    /*
     * Get the root of the document - we're going to be using the same root
     * for all subsequent calls
     */
    doc = domParser->getDocument();
    request_root = doc->getDocumentElement();
    
    bool is_physical;
    XStr type (request_root->getAttribute(XStr("type").x()));
    if (strcmp(type.c(), "request") != 0) {
      cerr << "*** RSpec type must be \"request\"" << endl;
      exit (EXIT_FATAL);
    } 
    
    // NOTE: Not sure about datetimes. They are strings for now
    XStr generated (request_root->getAttribute(XStr("generated").x()));
    XStr valid_until(request_root->getAttribute(XStr("valid_until").x()));
    
    // Initialize the rspec parser with the correct object depending
    // on the version of the rspec.
    int rspecVersion = rspec_parser_helper::getRspecVersion(request_root);
    switch (rspecVersion) {
    case 1:
      rspecParser = new rspec_parser_v1(RSPEC_TYPE_REQ);
      break;
    case 2:
      rspecParser = new rspec_parser_v2(RSPEC_TYPE_REQ);
      break;
    default:
      cerr << "*** Unsupported rspec ver. " << rspecVersion
	   << " ... Aborting " << endl;
      exit(EXIT_FATAL);
    }
    XMLDEBUG("Found rspec ver. " << rspecVersion << endl);
    
    map< pair<string, string>, pair<string, string> > fixed_interfaces;
    
    /*
     * These three calls do the real work of populating the assign data
     * structures
     */
    XMLDEBUG("starting vclass population" << endl);
    if (!populate_vclasses (request_root, vg)) {
      cerr << "*** Error reading vclasses from virtual topology "
	   << filename << endl;
      exit(EXIT_FATAL);
    }
    XMLDEBUG("finishing vclass population" << endl);
    XMLDEBUG("starting node population" << endl);
    if (!populate_nodes(request_root,vg, &fixed_interfaces)) {
      cerr << "*** Error reading nodes from virtual topology " 
	   << filename << endl;
      exit(EXIT_FATAL);
    }
    XMLDEBUG("finishing node population" << endl);
    
    XMLDEBUG("starting link population" << endl);
    if (!populate_links(request_root,vg, &fixed_interfaces)) {
      cerr << "*** Error reading links from virtual topology " 
	   << filename << endl;
      exit(EXIT_FATAL);
    }
    XMLDEBUG("finishing link population" << endl);
    
    /* TODO: We need to do something about policies at some point. */
    //populate_policies(root);
    
    cerr << "RSpec parsing finished" << endl; 
  }
  
  /*
   * All done, clean up memory
   */
//     XMLPlatformUtils::Terminate();
  free(rspecParser);
  return 0;
}

bool populate_node(DOMElement* elt, 
		   tb_vgraph &vg, map< pair<string,string>, 
		   pair<string,string> >* fixed_interfaces) 
{	
  bool hasVirtualId;
  string virtualId = rspecParser->readVirtualId(elt, hasVirtualId);
  
  bool hasComponentId;
  bool hasCMId;
  string componentId = rspecParser->readPhysicalId(elt, hasComponentId);
  string cmId = rspecParser->readComponentManagerId(elt, hasCMId);
  
  // If a node has a component_uuid, it is a fixed node
  if (hasComponentId) {
    if(hasCMId)
      fixed_nodes [virtualId] = componentId;	
    else
      cerr << "WARNING: Virtual node " << virtualId 
	   << " has a componentId specified "
	   << "but no component manager " << endl
	   << "The componentId will be ignored." << endl;
  }
  
  if (!hasVirtualId) {
    cerr << "*** Every node must have a virtual_id" << endl;
    return false;
  }
  
  bool allUnique;
  map< pair<string, string>, pair<string, string> > fixedIfacesOnNode;
  // XXX: This should not have to be called manually
  fixedIfacesOnNode = rspecParser->readInterfacesOnNode(elt, allUnique);
  if (!allUnique) {
    cerr << "*** The node-interface pairs in " << virtualId 
	 << " were not unique."	<< endl;
    return false;
  }
  fixed_interfaces->insert(fixedIfacesOnNode.begin(),fixedIfacesOnNode.end());
  
  /* Deal with the location tag */
  cerr << "WARNING: Country information will be ignored" << endl;
  
  /*
   * Add on types
   */
  tb_vclass *vclass = NULL;
  
  int typeCount;
  vector<struct node_type> types = rspecParser->readNodeTypes(elt, typeCount);
  bool no_type = (typeCount == 0);
  if (typeCount > 1) {
    cerr << "*** Too many node types (" << typeCount << ") on " 
	 << virtualId << " (allowed 1) ... Aborting " << endl;
    return false;
  }
  
  string typeName = types[0].typeName;
  int typeSlots = types[0].typeSlots;
  bool isStatic = types[0].isStatic;
  
  bool isUnlimited = (typeSlots == 1000);
  
  cerr << "Found req type " << typeName << endl;

  /*
   * Make a tb_ptype structure for this guy - or just add this node to
   * it if it already exists
   * XXX: This should not be "manual"!
   */
  const char* typeName_c = typeName.c_str();
  if (ptypes.find(typeName_c) == ptypes.end()) {
    ptypes[typeName_c] = new tb_ptype(typeName_c);
  }
  ptypes[typeName_c]->add_slots(typeSlots);
  tb_ptype *ptype = ptypes[typeName_c];
  
  name_vclass_map::iterator dit = vclass_map.find(fstring(typeName.c_str()));
  if (dit != vclass_map.end()) {
    no_type = true;
    vclass = (*dit).second;
  } 
  else {
    vclass = NULL;
    if (vtypes.find(typeName_c) == vtypes.end()) {
      vtypes[typeName_c] = typeSlots;
    } 
    else {
      vtypes[typeName_c] += typeSlots;
    }
  }

  // Read emulab extensions
  
  bool isSubnode;
  int subnodeCnt;
  string subnodeOf = rspecParser->readSubnodeOf(elt, isSubnode, subnodeCnt);
  if (isSubnode) {
    if (subnodeCnt > 1) {
      cerr << "*** To many \"subnode\" relations found in " 
	   << virtualId << ". Allowed 1 ... " << endl;
      return false;
    }
  }
  
  bool disallow_trivial_mix = rspecParser->readDisallowTrivialMix(elt);
 
  bool hasNodeHint = false;
  string nodeHint = rspecParser->readHintTo(elt, hasNodeHint);
  
  tb_vnode *v = NULL;
  if (no_type)
    // If they gave no type, just assume it's a PC for
    // now. This is not really a good assumption.
    v = new tb_vnode(virtualId.c_str(), "pc", typeSlots);
  else
    v = new tb_vnode(virtualId.c_str(), 
		     typeName.c_str(), typeSlots);
  
  // Construct the vertex
  if (disallow_trivial_mix) {
    v -> disallow_trivial_mix = true;
  }
  if (isSubnode) {
    v -> subnode_of_name = subnodeOf.c_str();
  }
  if (hasNodeHint) {
    node_hints[virtualId] = nodeHint;
  }
  
  bool hasExclusive;
  string exclusive = rspecParser->readExclusive(elt, hasExclusive);

  if (hasExclusive) {
    fstring desirename("shared");
    
    if (exclusive == "false" || exclusive == "0") {
      tb_node_featuredesire node_fd( desirename, 1.0,
				     true,featuredesire::FD_TYPE_NORMAL);
      node_fd.add_desire_user( 1.0 );
      v->desires.push_front( node_fd );
    } 
    else if(exclusive != "true" && exclusive != "1"){
      static int syntax_error;
      
      if( !syntax_error ) {
	syntax_error = 1;
	cout << "Warning: unrecognised exclusive "
	  "attribute \"" << exclusive << "\"; will "
	  "assume exclusive=\"true\"\n";
      }
    }
  }

  int fdsCount;
  vector<struct fd> fds = rspecParser->readFeaturesDesires(elt, fdsCount);
  for (int i = 0; i < fdsCount; i++) {
    struct fd desire = fds[i];
    featuredesire::fd_type fd_type;
    switch(desire.op.type) {
    case LOCAL_OPERATOR:
      fd_type = featuredesire::FD_TYPE_LOCAL_ADDITIVE;
      break;
    case GLOBAL_OPERATOR:
      if (desire.op.op == "OnceOnly") {
	fd_type = featuredesire::FD_TYPE_GLOBAL_ONE_IS_OKAY;
      }
      else {
	fd_type = featuredesire::FD_TYPE_GLOBAL_MORE_THAN_ONE;
      }
      break;
    default:
      fd_type = featuredesire::FD_TYPE_NORMAL;
      break;
    }
    tb_node_featuredesire node_fd (XStr(desire.fd_name.c_str()).f(),
				   desire.fd_weight,
				   desire.violatable,
				   fd_type);
    node_fd.add_desire_user(desire.fd_weight);
    (v->desires).push_front(node_fd);
  }
  
  v->vclass = vclass;
  vvertex vv = add_vertex(vg);
  vname2vertex[virtualId.c_str()] = vv;
  virtual_nodes.push_back(vv);
  put(vvertex_pmap,vv,v);
  
  // If a component manager has been specified, then the node must be 
  // managed by that CM. We implement this as a desire.
  if (hasCMId) {
    tb_node_featuredesire node_fd (XStr(cmId.c_str()).f(), 
				   1.0, true, featuredesire::FD_TYPE_NORMAL);
    node_fd.add_desire_user(0);
    (v->desires).push_front(node_fd);
  }
  
  v -> desires.sort();
  return true;
}

/*
 * Pull nodes from the document, and populate assign's own data structures
 */
bool populate_nodes(DOMElement *root, 
		    tb_vgraph &vg, map< pair<string, string>, 
		    pair<string, string> >* fixed_interfaces) {
  bool is_ok = true;
  /*
   * Get a list of all nodes in this document
   */
  DOMNodeList *nodes = root->getElementsByTagName(XStr("node").x());
  int nodeCount = nodes->getLength();
  XMLDEBUG("Found " << nodeCount << " nodes in rspec" << endl);
  clock_t times [nodeCount];
  
  for (size_t i = 0; i < nodeCount; i++)  {
    DOMNode *node = nodes->item(i);
    // This should not be able to fail, because all elements in
    // this list came from the getElementsByTagName() call
    DOMElement *elt = dynamic_cast<DOMElement*>(node);
    is_ok &= populate_node(elt, vg, fixed_interfaces);
  }
  
  /*
   * This post-pass binds subnodes to their parents
   */
  bind_vtop_subnodes(vg);
  
  /*
   * Indicate errors, if any
   */
  return is_ok;
}

bool populate_link (DOMElement* elt, 
		    tb_vgraph &vg, map< pair<string,string>, 
		    pair<string,string> >* fixed_interfaces) 
{
  bool hasVirtualId;
  string virtualId = rspecParser->readVirtualId(elt, hasVirtualId);
  
  bool hasVirtualizationType;
  string virtualizationType 
    = rspecParser->readVirtualizationType(elt, hasVirtualizationType);
  
  /*
   * Get the link type - we know there is at least one, and we
   * need it for the constructor
   * Note: Changed from element to attribute
   */
  int count;
  vector<struct link_type> linkTypes = rspecParser->readLinkTypes(elt, count);
  string linkType = "ethernet";
  if (count > 1) {
    cerr << "*** Too many link types specified (" << count 
	 << ") on " << virtualId << ". Allowed 1 ... Aborting" << endl;
    return false;
  }
  else if (count == 1){
    linkType = linkTypes[0].typeName;
  }
  
  /*
   * Get standard link characteristics
   */
  struct link_characteristics characteristics
    = rspecParser->readLinkCharacteristics(elt, count);
  if (count != 1) {
    cerr << "*** Incorrect number of link properties specified ("
	 << count << " on " << virtualId <<". Allowed 1 " << endl;
    return false;
  }
  int bandwidth = characteristics.bandwidth;
  int latency = characteristics.latency;
  double packetLoss = characteristics.packetLoss;

  struct link_interface src;
  struct link_interface dst;
  
  int ifaceCount = 0;
  vector<struct link_interface> interfaces
    = rspecParser->readLinkInterface(elt, ifaceCount);
  
  /* NOTE: In a request, we assume that each link has only two interfaces.
   * Although the order is immaterial, assign expects a source first 
   * and then destination and we assume the same ordering.
   * If more than two interfaces are provided, the link must be a lan 
   */
  if (ifaceCount > 2) {
    string str_lan_id = generate_virtualNodeId(virtualId);
    
    // NOTE: This is an attribute which is not in the rspec
    // it has been added to easily identify lan_links during annotation
    elt->setAttribute(XStr("is_lan").x(), XStr("true").x());
    
    // Create the lan node
    DOMElement* lan_node = doc->createElement(XStr("node").x());
    request_root->appendChild(lan_node);
    lan_node->setAttribute(XStr("virtualization_type").x(),
			   XStr("raw").x());
    lan_node->setAttribute(XStr("exclusive").x(), XStr("1").x());
    lan_node->setAttribute(XStr("virtual_id").x(),
			   XStr(str_lan_id.c_str()).x());
    
    // Create node type for the lan
    DOMElement* lan_node_type = doc->createElement(XStr("node_type").x());
    lan_node_type->setAttribute(XStr("type_name").x(), XStr("lan").x());
    lan_node_type->setAttribute(XStr("type_slots").x(), XStr("1").x());
    lan_node->appendChild(lan_node_type);
    
    // NOTE: This is an attribute which is not in the rspec
    // but which has been added to distinguish the element
    // from those explicitly specified by the user during annotation
    lan_node->setAttribute(XStr("generated_by_assign").x(),
			   XStr("true").x());
    
    // We need to store the dynamically created links in a list
    // and add them to the virtual graph later because the sanity checks
    // will fail if they are added before the lan node is added.
    list<DOMElement*> links;
    list<DOMElement*>::iterator it = links.begin();
    for (int i = 0; i < ifaceCount; ++i) {
      link_interface interface = interfaces[i];
      const XMLCh* virtualIfaceId = XStr(interface.virtualIfaceId.c_str()).x();
      const XMLCh* virtualNodeId = XStr(interface.virtualNodeId.c_str()).x();
      
      string str_lan_interface_id = generate_virtualIfaceId(str_lan_id, i);
      DOMElement* lan_interface = doc->createElement(XStr("interface").x());

      lan_node->appendChild(lan_interface);
      lan_interface->setAttribute(XStr("virtual_id").x(),
				  XStr(str_lan_interface_id.c_str()).x());
      
      DOMElement* link = doc->createElement(XStr("link").x());
      request_root->appendChild(link);
      link->setAttribute(XStr("virtual_id").x(), 
			 XStr(interface.virtualNodeId 
			      + string(":") + str_lan_id).x());
      appendChildTagWithData(link, "bandwidth",
			     rspecParser->numToString(bandwidth).c_str());
      appendChildTagWithData(link, "latency",
			     rspecParser->numToString(latency).c_str());
      appendChildTagWithData(link, "packet_loss",
			     rspecParser->numToString(packetLoss).c_str());
      
      DOMElement* src_interface_ref 
	= doc->createElement(XStr("interface_ref").x());
      src_interface_ref->setAttribute(XStr("clientId").x(),
				      virtualIfaceId);
      link->appendChild(src_interface_ref);
      
      DOMElement* dst_interface_ref 
	= doc->createElement(XStr("interface_ref").x());
      dst_interface_ref->setAttribute(XStr("clientId").x(),
				      XStr(str_lan_interface_id.c_str()).x());
      link->appendChild(dst_interface_ref);
      
      // Adding attributes to ensure that the element is handled
      // correctly during annotation.
      link->setAttribute(XStr("generated_by_assign").x(),
			 XStr("true").x());
      link->setAttribute(XStr("lan_link").x(),
			 XStr(virtualId.c_str()).x());
      
      links.insert(it, link);
    }
    
    populate_node(lan_node, vg, fixed_interfaces);
    for (it = links.begin(); it != links.end(); ++it)
      populate_link(*it, vg, fixed_interfaces);
    return true;
  }
  else if (ifaceCount == 2) {
    src = interfaces[0];
    dst = interfaces[1];
  }
  else {
    cerr << "*** Too few interfaces found (" << ifaceCount << ")" 
	 << " on " << virtualId << " at least 2 required ... Aborting" 
	 << endl;
    return false;
  }
  
  string srcNode = src.virtualNodeId;
  string srcIface = src.virtualIfaceId;
  string dstNode = dst.virtualNodeId;
  string dstIface = dst.virtualIfaceId;

  if (srcNode == "" || srcIface == "") {
    cerr << "*** No source node found on interface for link " 
	 << virtualId << endl;
    return false;
  }
  if (dstNode == "" || dstIface == "") {
    cerr << "*** No destination node found on interface for link " 
	 << virtualId << endl;
    return false;
  }
  
  if (vname2vertex.find(srcNode.c_str()) == vname2vertex.end()) {
    cerr << "*** Bad link " << virtualId 
	 << ", non-existent source node " << srcNode << endl;
    return false;
  }
  if (vname2vertex.find(dstNode.c_str()) == vname2vertex.end()) {
    cerr << "*** Bad link " << virtualId 
	 << ", non-existent destination node " << dstNode << endl;
    return false;
  }
  
  vvertex v_src_vertex = vname2vertex[srcNode.c_str()];
  vvertex v_dst_vertex = vname2vertex[dstNode.c_str()];
  tb_vnode *src_vnode = get(vvertex_pmap,v_src_vertex);
  tb_vnode *dst_vnode = get(vvertex_pmap,v_dst_vertex);
  
  // XXX: This is obsolete. We need to fix it ASAP
  bool emulated = false;
  if (virtualizationType == "raw" || virtualizationType == "")
    emulated = true;
  
  // Emulab extensions
  bool allow_delayed = !(rspecParser->readNoDelay(elt));
  bool allow_trivial = rspecParser->readTrivialOk(elt);
  
  map< pair<string,string>, pair<string,string> >::iterator it;
  
  bool fix_srcIface = false;
  fstring fixed_srcIface = "";
  it = fixed_interfaces->find(pair<string,string>(srcNode, srcIface));
  if (it != fixed_interfaces->end()) {
    cerr << "Found fixed source interface (" << (it->second).first << ","
	 << (it->second).second << ") on (" << (it->first).first << ","
	 << (it->first).second << ")" << endl;
    fix_srcIface = true;
    fixed_srcIface = (it->second).second;
  }
  
  
  bool fix_dstIface = false;
  fstring fixed_dstIface = "";
  it = fixed_interfaces->find(make_pair(dstNode, dstIface));
  if (it != fixed_interfaces->end()) {
    cerr << "Found fixed destination interface (" << (it->second).first 
	 << "," << (it->second).second << ") on (" << (it->first).first
	 << "," << (it->first).second << ")" << endl;
    fix_dstIface = true;
    fixed_dstIface = (it->second).second;
  }
  
  if (emulated) {
    if (!allow_trivial) {
      src_vnode->total_bandwidth += bandwidth;
      dst_vnode->total_bandwidth += bandwidth;
    }
  } 
  else {
    src_vnode->num_links++;
    dst_vnode->num_links++;
    src_vnode->link_counts[linkType.c_str()]++;
    dst_vnode->link_counts[linkType.c_str()]++;
  }
  
  vedge virt_edge = (add_edge(v_src_vertex,v_dst_vertex,vg)).first;
  
  tb_vlink *virt_link = new tb_vlink();
  
  virt_link-> name = virtualId;
  virt_link-> type = fstring(linkType.c_str());

  virt_link-> fix_src_iface = fix_srcIface;
  if (fix_srcIface) {
    virt_link-> src_iface = (fixed_srcIface);//.f();
  }
  
  virt_link-> fix_dst_iface = fix_dstIface;
  if (fix_dstIface) {
    virt_link-> dst_iface = (fixed_dstIface);//.f();
  }
  
  virt_link-> emulated = emulated;
  virt_link-> allow_delayed = allow_delayed;
  virt_link-> allow_trivial = allow_trivial;
  virt_link-> no_connection = false;
  virt_link->delay_info.bandwidth = bandwidth;
  virt_link->delay_info.delay = latency;
  virt_link->delay_info.loss = packetLoss;
  virt_link->src = v_src_vertex;
  virt_link->dst = v_dst_vertex;
  
  // XXX: Should not be manual
  put(vedge_pmap, virt_edge, virt_link);
  
  return true;
}

/*
 * Pull the links from the vtop file, and populate assign's own data sturctures
 */
bool populate_links(DOMElement *root, tb_vgraph &vg, map< pair<string, string>, pair<string, string> >* fixed_interfaces) {
    
    bool is_ok = true;
    
    /*
     * TODO: Support the "PENALIZE_BANDWIDTH" option?
     * TODO: Support the "FIX_PLINK_ENDPOINTS" and "FIX_PLINKS_DEFAULT"?
     */
    DOMNodeList *links = root->getElementsByTagName(XStr("link").x());
    int linkCount = links->getLength();
    XMLDEBUG("Found " << links->getLength()  << " links in rspec" << endl);
    for (size_t i = 0; i < linkCount; i++) {
        DOMNode *link = links->item(i);
        DOMElement *elt = dynamic_cast<DOMElement*>(link);
	is_ok &= populate_link(elt, vg, fixed_interfaces);	
    }
    return is_ok;
}

bool populate_vclass (struct vclass vclass, tb_vgraph& vg)
{
  tb_vclass *v = NULL;
  const char* name = vclass.name.c_str();
  // We don't have support for hard vclasses yet
  if (vclass.type.type == SOFT_VCLASS) {
    cerr << "Soft vclass " << name << " with weight " 
	 << vclass.type.weight << endl;
    v = new tb_vclass (XStr(name).f(), vclass.type.weight);
    if (v == NULL) {
      cerr << "*** Could not create vclass " << vclass.name << endl;
      return false;
    }
    vclass_map[name] = v;
  }
  
  for (int i = 0; i < vclass.physicalTypes.size(); i++) {
    fstring physType = XStr(vclass.physicalTypes[i].c_str()).f();
    cerr << "vclass physType: " << physType << endl;
    v->add_type(physType);
    vclasses[name].push_back(physType);
  }
  return true;
}

/*
 * Populate the vclasses
 */
bool populate_vclasses (DOMElement* root, tb_vgraph& vg)
{
  bool isOk = true;
  vector<struct vclass> vclasses = rspecParser->readVClasses(root);
  cerr << "Found " << vclasses.size() << " vclasses." << endl;
  for (int i = 0; i < vclasses.size(); i++) {
    isOk &= populate_vclass(vclasses[i], vg);
  }
  return isOk;
}
    

DOMElement* appendChildTagWithData (DOMElement* parent, 
				    const char* tag_name, 
				    const char* child_value)
{
  DOMElement* child = doc->createElement(XStr(tag_name).x());
  child->appendChild(doc->createTextNode(XStr(child_value).x()));
  parent->appendChild(child);
  return child;
}

string generate_virtualNodeId (string virtual_id) 
{
 std:ostringstream oss;
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);
  oss << virtual_id << tv.tv_sec << tv.tv_usec;
  return oss.str();
}

string generate_virtualIfaceId (string lan_name, int interface_number)
{ 
 std:ostringstream oss;
  oss << lan_name << ":" << interface_number;
  return oss.str();
}

#endif

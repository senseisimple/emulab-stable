/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * XML Parser for RSpec ptop files
 */

static const char rcsid[] = "$Id: parse_advertisement_rspec.cc,v 1.7 2009-10-21 20:49:26 tarunp Exp $";

#ifdef WITH_XML

#include "parse_advertisement_rspec.h"
#include "xmlhelpers.h"
#include "parse_error_handler.h"
#include "rspec_parser_helper.h"
#include "rspec_parser_v1.h"
#include "rspec_parser_v2.h"

#include <fstream>
#include <map>
#include <set>

#include "anneal.h"
#include "vclass.h"

#define XMLDEBUG(x) (cerr << x)
#define ISSWITCH(n) (n->types.find("switch") != n->types.end())

#ifdef TBROOT
	#define SCHEMA_LOCATION TBROOT"/lib/assign/ad.xsd"
#else
	#define SCHEMA_LOCATION "ad.xsd"
#endif

using namespace rspec_emulab_extension;
 
/*
 * XXX: Do I have to release lists when done with them?
 */

/*
 * XXX: Global: This is really bad!
 */
extern name_pvertex_map pname2vertex;

/* ------------------- Have to include the vnode data structures as well-----*/
extern name_vvertex_map vname2vertex;
extern name_name_map fixed_nodes;
extern name_name_map node_hints;
extern name_count_map vtypes;
extern name_list_map vclasses;
extern vvertex_vector virtual_nodes;
extern name_vclass_map vclass_map;
/* -------------------------- end of vtop stuff------------------ */

// This is a hash map of the entire physical topology 
// because it takes far too long for it to search the XML DOM tree.
map<string,DOMElement*>* advertisement_elements= new map<string,DOMElement*>();

map<string, string>* pIfacesMap = new map<string, string>();
map<string, string> shortNodeNames;

DOMElement* advt_root = NULL;

/*
 * TODO: This should be moved out of parse_top.cc, where it currently
 * resides.
 */
int bind_ptop_subnodes(tb_pgraph &pg);
int bind_vtop_subnodes(tb_vgraph &vg);

static rspec_parser* rspecParser;

/*
 * These are not meant to be used outside of this file, so they are only
 * declared in here
 */
static bool populate_nodes(DOMElement *root, tb_pgraph &pg, tb_sgraph &sg,
			   set<string> &unavailable);
static bool populate_links(DOMElement *root, tb_pgraph &pg, tb_sgraph &sg,
			   set<string> &unavailable);
static bool populate_type_limits(DOMElement *root,tb_pgraph &pg,tb_sgraph &sg);
static bool populate_policies (DOMElement*root, tb_pgraph &pg, tb_sgraph &sg);

int parse_advertisement(tb_pgraph &pg, tb_sgraph &sg, char *filename) {
  /* 
   * Fire up the XML parser
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
  XMLDEBUG("Begin XML parse" << endl);
  domParser->parse(filename);
  XMLDEBUG("XML parse completed" << endl);
  
  /* 
   * If there are any errors, do not go any further
   */
  if (errHandler->sawError()) {
    cout << "*** There were " << domParser -> getErrorCount () 
	 << " errors in " << filename << endl;
    exit(EXIT_FATAL);
  }
  else {
    /*
     * Get the root of the document - we're going to be using the same root
        * for all subsequent calls
        */
    DOMDocument* doc = domParser->getDocument();
    advt_root = doc->getDocumentElement();
    set<string> unavailable; // this should really be an unordered_set,
    // but that's not very portable yet
    
    int rspecVersion = rspec_parser_helper::getRspecVersion(advt_root);
    switch (rspecVersion) {
    case 1:
      rspecParser = new rspec_parser_v1(RSPEC_TYPE_ADVT);
      break;
    case 2:
      rspecParser = new rspec_parser_v2(RSPEC_TYPE_ADVT);
      break;
    default:
      cout << "*** Unsupported rspec ver. " << rspecVersion
	   << " ... Aborting " << endl;
      exit(EXIT_FATAL);
    }
    XMLDEBUG("Found rspec ver. " << rspecVersion << endl);
    
    string type = XStr(advt_root->getAttribute(XStr("type").x())).c();
    if (type != "advertisement") {
      cout << "*** Rspec type must be \"advertisement\" in " << filename
	   << " (found " << type << ")" << endl;
      exit(EXIT_FATAL);
    }
    
    /*
     * These three calls do the real work of populating the assign data
     * structures
     */
    XMLDEBUG("Starting node population" << endl);
    if (!populate_nodes(advt_root,pg,sg,unavailable)) {
      cout << "*** Error reading nodes from physical topology "
	   << filename << endl;
      exit(EXIT_FATAL);
    }
    XMLDEBUG("Finishing node population" << endl);
    XMLDEBUG("Starting link population" << endl);
    if (!populate_links(advt_root,pg,sg,unavailable)) {
      cout << "*** Error reading links from physical topology "
	   << filename << endl;
      exit(EXIT_FATAL);
    }
    XMLDEBUG("Finishing link population" << endl);
    XMLDEBUG("Setting type limits" << endl);
    if (!populate_type_limits(advt_root, pg, sg)) {
      cout << "*** Error setting type limits " << filename << endl;
      exit(EXIT_FATAL);
    }
    XMLDEBUG("Finishing setting type limits" << endl);
    XMLDEBUG("Starting policy population" << endl);
    if (!populate_policies(advt_root, pg, sg)) {
      cout << "*** Error setting policies " << filename << endl;
      exit(EXIT_FATAL);
    }
    XMLDEBUG("Finishing setting policies" << endl);
    
    XMLDEBUG("RSpec parsing finished" << endl); 
  }
  
  /*
   * All done, clean up memory
   */
  //     XMLPlatformUtils::Terminate();
  delete rspecParser;
  return 0;
}

/*
 * Pull nodes from the document, and populate assign's own data structures
 */
bool populate_nodes(DOMElement *root, 
		    tb_pgraph &pg, tb_sgraph &sg, set<string> &unavailable) {
  static bool displayedWarning = false;
  bool is_ok = true;
  pair<map<string, DOMElement*>::iterator, bool> insert_ret;
  /*
   * Get a list of all nodes in this document
   */
  DOMNodeList *nodes = root->getElementsByTagName(XStr("node").x());
  int nodeCount = nodes->getLength();
  XMLDEBUG("Found " << nodeCount << " nodes in rspec" << endl);
  
  int availableCount = 0;
  for (size_t i = 0; i < nodeCount; i++) {
    DOMNode *node = nodes->item(i);
    // This should not be able to fail, because all elements in
    // this list came from the getElementsByTagName() call
    DOMElement *elt = dynamic_cast<DOMElement*>(node);
    
    bool hasComponentId;
    bool hasCMId;
    string componentId = rspecParser->readPhysicalId(elt, hasComponentId);
    string componentManagerId = rspecParser->readComponentManagerId(elt,hasCMId);

    bool hasComponentName;
    string componentName 
      = rspecParser->readComponentName (elt, hasComponentName);
    shortNodeNames.insert(pair<string,string>(componentId, componentName));

    if (!hasComponentId || !hasCMId) {
      is_ok = false;
      continue;
    }
    
   // Maintain a list of componentId's seen so far to ensure no duplicates
    insert_ret = advertisement_elements->insert
      (pair<string, DOMElement*>(componentId, elt));
    if (insert_ret.second == false) {
      cout << "*** " << componentId << " already exists" << endl;
      is_ok = false;
    }
    
    // XXX: This should not have to be called manually
    bool allUnique;
    rspecParser->readInterfacesOnNode(elt, allUnique);

    // XXX: We don't ever do anything with this, so I am commenting it out
    if (!displayedWarning) {
      cout << "WARNING: Country information will be ignored" << endl;
      displayedWarning = true;
    }
    
    pvertex pv;
    
    tb_pnode *p;
    /*
     * TODO: These three steps shouldn't be 'manual'
     */
    pv = add_vertex(pg);
    // XXX: This is wrong!
    p = new tb_pnode(componentId);
    // XXX: Global
    put(pvertex_pmap,pv,p);
    /*
     * XXX: This shouldn't be "manual"
     */
    pname2vertex[componentId.c_str()] = pv;
    
    int typeCount;
    vector<struct node_type>types = rspecParser->readNodeTypes(elt, typeCount);
    for (int i = 0; i < typeCount; i++) {
      node_type type = types[i];
      string typeName = type.typeName;
      int typeSlots = type.typeSlots;
      bool isStatic = type.isStatic;

      // Add the type into assign's data structures
      if (ptypes.find(typeName) == ptypes.end()) {
	ptypes[typeName] = new tb_ptype(typeName);
      }
      ptypes[typeName]->add_slots(typeSlots);
      tb_ptype *ptype = ptypes[typeName];
      
      /*
       * For the moment, we treat switches specially - when we get the
       * "forwarding" code working correctly, this special treatment
       * will go away.
       * TODO: This should not be in the parser, it should be somewhere
       * else!
       */
      if (typeName == "switch") {
	p->is_switch = true;
	p->types["switch"] = new tb_pnode::type_record(1,false,ptype);
	svertex sv = add_vertex(sg);
	tb_switch *s = new tb_switch();
	put(svertex_pmap,sv,s);
	s->mate = pv;
	p->sgraph_switch = sv;
	p->switches.insert(pv);
      } 
      else {
	p->types[typeName.c_str()]
	  = new tb_pnode::type_record(typeSlots,isStatic,ptype);
      }
      p->type_list.push_back(p->types[typeName.c_str()]);
    }
    
    bool hasExclusive;
    string exclusive = rspecParser->readExclusive(elt, hasExclusive);
    
    if (hasExclusive) {
      fstring feature ("shared");
      if (exclusive == "false" || exclusive == "0") {
	p->features.push_front
	  (tb_node_featuredesire(feature, 1.0, true,
				 featuredesire::FD_TYPE_NORMAL));
      }
      else if (exclusive != "true" && exclusive != "1") {
	static int syntax_error;
	if ( !syntax_error ) { 
	  syntax_error = 1;
	  cout << "WARNING: unrecognized exclusive attribute \""
	       << exclusive << "\"; Asuming exclusive = \"true\"" << endl;
	}
      }
    }
    
    // Add the component_manager_uuid as a feature.
    // We need to do this to handle external references
    tb_node_featuredesire node_fd (XStr(componentManagerId.c_str()).f(), 
				   0.9);//, false, featuredesire::FD_TYPE_NORMAL);
    (p->features).push_front(node_fd);

    // This has to be at the end becuase if we don't populate
    // at least the interfaces, we get all kinds of crappy errors
    bool isAvailable;
    string available = rspecParser->readAvailable(elt, isAvailable);
    if (available == "false") {
      unavailable.insert(componentId);
      continue;
    }
    ++availableCount;

    bool isSubnode = false;
    int subnodeCnt;
    string subnodeOf = rspecParser->readSubnodeOf (elt, isSubnode, subnodeCnt);
    if (isSubnode) {
      if (subnodeCnt > 1) {
	cout << "*** Too many \"subnode\" relations found in "
	     << componentId << "Allowed 1 ... " << endl;
	is_ok = false;
	continue;
      }
      else { 
	// Just store the name for now, we'll do late binding to
	// an actual pnode later
	p->subnode_of_name = XStr(subnodeOf.c_str()).f();
      }
    }

    // Deal with features
    int fdsCount;
    vector<struct fd> fds = rspecParser->readFeaturesDesires(elt, fdsCount);
    for (int i = 0; i < fdsCount; i++) {
      struct fd feature = fds[i];
      featuredesire::fd_type fd_type;
      switch(feature.op.type) {
      case LOCAL_OPERATOR:
	fd_type = featuredesire::FD_TYPE_LOCAL_ADDITIVE;
	break;
      case GLOBAL_OPERATOR:
	if (feature.op.op == "OnceOnly") {
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
      tb_node_featuredesire node_fd (XStr(feature.fd_name.c_str()).f(),
				     feature.fd_weight,
				     feature.violatable,
				     fd_type);
      (p->features).push_front(node_fd);
    }
    
    // Read extensions for emulab-specific flags
    bool hasTrivialBw;
    int trivialBw = rspecParser->readTrivialBandwidth(elt, hasTrivialBw);
    if (hasTrivialBw) {
      p->trivial_bw = trivialBw;
    }

    if (rspecParser->readUnique(elt)) {
      p->unique = true;
    }

    /*
     * XXX: Is this really necessary?
     */
    p->features.sort();
  }
  
  /*
   * This post-pass binds subnodes to their parents
   */
  bind_ptop_subnodes(pg);
  XMLDEBUG ("Found " << availableCount << " available nodes" << endl);
  /*
   * Indicate no errors
   */
  
  return is_ok;
}

/*
 * Pull the links from the ptop file, and populate assign's own data sturctures
 */
bool populate_links(DOMElement *root, tb_pgraph &pg, tb_sgraph &sg,
			  set<string> &unavailable) {
    
  bool is_ok = true;
  pair<map<string, DOMElement*>::iterator, bool> insert_ret;

  // It should be safe to read it now because all the short interface names
  // have been populated once all the nodes have been read.
  map<string, string> shortNames = rspecParser->getShortNames();
  
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
    
    bool hasComponentId;
    bool hasCMId;
    string componentId = rspecParser->readPhysicalId(elt, hasComponentId);
    string cmId = rspecParser->readComponentManagerId(elt, hasCMId);
    
    if (!hasComponentId || !hasCMId) {
      cout << "*** Require component ID and component manager ID" << endl;
      is_ok = false;
    }
    else {
      insert_ret = 
	advertisement_elements->insert(pair<string, DOMElement*>
				       (componentId, elt));
      if (insert_ret.second == false) {
	cout << "*** " << componentId << " already exists" << endl;
	is_ok = false;
      }
    }

    /*
     * Get source and destination interfaces - we use knowledge of the
     * schema that there is awlays exactly one source and one destination
     */
    int ifaceCount = 0;
    vector<struct link_interface> interfaces 
      = rspecParser->readLinkInterface(elt, ifaceCount);
    
    // Error handling
    switch (ifaceCount) {
    case RSPEC_ERROR_UNSEEN_NODEIFACE_SRC:
      cout << "*** Unseen node-interface pair on the source interface ref"
	   << endl;
      is_ok = false;
      continue;
      
    case RSPEC_ERROR_UNSEEN_NODEIFACE_DST:
      cout << "*** Unseen node-interface pair on the destination interface ref"
	   << endl;
      is_ok = false;
      continue;
    }
    
    if (ifaceCount != 2) {
      cout << "*** Incorrect number of interfaces found on link " 
	   << componentId << ". Expected 2 (found " 
	   << ifaceCount << ")" << endl;
      is_ok = false;
      continue;
    }

    string src_node = interfaces[0].physicalNodeId;
    string src_iface = interfaces[0].physicalIfaceId;
    string dst_node = interfaces[1].physicalNodeId;
    string dst_iface = interfaces[1].physicalIfaceId;

    if (src_node == "" || src_iface == "") {
      cout << "*** Physical link " << componentId 
	   << " must have a component id and component interface id "
	   << " specified for the source node" << endl;
      is_ok = false;
      continue;
    }
    if (dst_node == "" || dst_iface == "") {
      cout << "*** Physical link " << componentId 
	   << " must have a component id and component interface id"
	   << " specified for the destination node" << endl;
      is_ok = false;
      continue;
    }

    pIfacesMap->insert(pair<string, string>(src_iface, src_node));
    pIfacesMap->insert(pair<string, string>(dst_iface, dst_node));

    if( unavailable.count( src_node ) || 
	unavailable.count( dst_node ) )
      //one or both of the endpoints are unavailable; silently
      //ignore the link
      continue;

    /*
     * Get standard link characteristics
     */
    int count;
    link_characteristics characteristics
      = rspecParser->readLinkCharacteristics (elt, count);

    if (count == RSPEC_ASYMMETRIC_LINK) {
      cout << "*** Disallowed asymmetric link found on " << componentId
           << ". Links must be symmetric" << endl;
      is_ok = false;
    }
    else if (count > 2) {
      cout << "*** Too many link properties found on " << componentId
           << ". Max. allowed: 2" << endl;
      is_ok = false;
    }

    int bandwidth = characteristics.bandwidth;
    int latency = characteristics.latency;
    double packetLoss = characteristics.packetLoss;
      
    /*
     * Find the nodes in the existing data structures
     */
    pvertex src_vertex = pname2vertex[src_node.c_str()];
    pvertex dst_vertex = pname2vertex[dst_node.c_str()];
    tb_pnode *src_pnode = get(pvertex_pmap,src_vertex);
    tb_pnode *dst_pnode = get(pvertex_pmap,dst_vertex);
    
    /*
     * Start getting link types - we know there is at least one, and we
     * need it for the constructor
     */
    int typeCount;
    vector<link_type> types = rspecParser->readLinkTypes(elt, typeCount);
    string str_first_type = types[0].typeName;

    /* Create srcmac and dstmacs which are needed by the code which 
     * parsers assign's output
     */

    string srcMac = "(null)", dstMac = "(null)";
    if (shortNames[src_iface] != "(null)") {
      srcMac = shortNodeNames[src_node] + "/" + shortNames[src_iface];
    }
    if (shortNames[dst_iface] != "(null)") {
      dstMac = shortNodeNames[dst_node] + "/" + shortNames[dst_iface];
    }
      
    /*
     * Create the actual link object
     */
    pedge phys_edge = (add_edge(src_vertex,dst_vertex,pg)).first;
    // XXX: Link type!?
    // XXX: Don't want to use (null) src and dest macs, but would break
    // other stuff if I remove them... bummer!
    tb_plink *phys_link =
      new tb_plink(componentId.c_str(), 
		   tb_plink::PLINK_NORMAL, str_first_type.c_str(),
                   src_pnode->name, srcMac.c_str(), shortNames[src_iface],
                   dst_pnode->name, dstMac.c_str(), shortNames[dst_iface]);
    
    phys_link->delay_info.bandwidth = bandwidth;
    phys_link->delay_info.delay = latency;
    phys_link->delay_info.loss = packetLoss;
		
    // XXX: Should not be manual
    put(pedge_pmap, phys_edge, phys_link);
    
    
    // ******************************************
    // XXX: This is weird. It has to be clarified
    // ******************************************
#ifdef PENALIZE_BANDWIDTH
    float penalty = 1.0
      phys_link -> penalty = penalty;
#endif
    
    // XXX: Likewise, should happen automatically, but the current tb_plink
    // strucutre doesn't actually have pointers to the physnode endpoints
    src_pnode->total_interfaces++;
    dst_pnode->total_interfaces++;
    src_pnode->link_counts[str_first_type.c_str()]++;
    dst_pnode->link_counts[str_first_type.c_str()]++;
    
    /*
     * Add in the rest of the link types we found
     */
    for (int i = 1; i < typeCount; i++) {
      const char *str_type_name = types[i].typeName.c_str();
      ////XMLDEBUG("  Link has type " << type_name << endl);
      // XXX: Should not be manual
      phys_link->types.insert(str_type_name);
      src_pnode->link_counts[str_type_name]++;
      dst_pnode->link_counts[str_type_name]++;
    }
    
    if (ISSWITCH(src_pnode) && ISSWITCH(dst_pnode)) {
      svertex src_switch = get(pvertex_pmap,src_vertex)->sgraph_switch;
      svertex dst_switch = get(pvertex_pmap,dst_vertex)->sgraph_switch;
      sedge swedge = add_edge(src_switch,dst_switch,sg).first;
      tb_slink *sl = new tb_slink();
      put(sedge_pmap,swedge,sl);
      sl->mate = phys_edge;
      phys_link->is_type = tb_plink::PLINK_INTERSWITCH;
    }
    
    else if (ISSWITCH(src_pnode) && ! ISSWITCH(dst_pnode)) {
      dst_pnode->switches.insert(src_vertex);
#ifdef PER_VNODE_TT
      dst_pnode->total_bandwidth += bandwidth;
#endif
    }
    
    else if (ISSWITCH(dst_pnode) && ! ISSWITCH(src_pnode)) {
	src_pnode->switches.insert(dst_vertex);
#ifdef PER_VNODE_TT
	src_pnode->total_bandwidth += bandwidth;
#endif
    }
    
  }
  return is_ok;
}

bool populate_type_limits(DOMElement *root, tb_pgraph &pg, tb_sgraph &sg)
{
  int count;
  vector<struct type_limit> rv = rspecParser->readTypeLimits(root, count);
  XMLDEBUG ("Found " << count << " type limits" << endl);
  for (int i = 0; i < count; i++) {
    fstring type = XStr(rv[i].typeClass.c_str()).f();
    int max = rv[i].typeCount;

    if (ptypes.find(type) == ptypes.end()) {
      ptypes[type] = new tb_ptype(type);
    }
    ptypes[type]->set_max_users(max);
  }  
  return true;
}

bool populate_policies (DOMElement* root, tb_pgraph &pg, tb_sgraph &sg)
{
  int count;
  bool is_ok = true;
  vector<struct policy> policies = rspecParser->readPolicies(root, count);
  XMLDEBUG ("Found " << count << " policies" << endl);
  for (int i = 0; i < count; i++) {
    struct policy policy = policies[i];
    string name = policy.name;
    string limit = policy.limit;
    tb_featuredesire *fd 
      = tb_featuredesire::get_featuredesire_by_name(((fstring)XStr(name).c()));
    if (fd == NULL) {
      cerr << "*** Desire " << name << " not found" << endl;
      is_ok &= false;
    }
    else {
      if (limit == "disallow") {
	fd->disallow_desire();
      }
      else {
	fd->limit_desire(rspec_parser_helper::stringToNum(limit));
      }
    }
  }
  return is_ok;
}   

#endif

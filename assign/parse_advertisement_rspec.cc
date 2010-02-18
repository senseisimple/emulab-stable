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

#include <fstream>
#include <map>
#include <set>

#include "anneal.h"
#include "vclass.h"

#define XMLDEBUG(x) (cerr << x)
#define ISSWITCH(n) (n->types.find("switch") != n->types.end())

#ifdef TBROOT
	#define SCHEMA_LOCATION TBROOT"/lib/assign/rspec-ad.xsd"
#else
	#define SCHEMA_LOCATION "rspec-ad.xsd"
#endif
				 
/*
 * XXX: Do I have to release lists when done with them?
 */

/*
 * XXX: Global: This is really bad!
 */
extern name_pvertex_map pname2vertex;

/* ------------------- Have to include the vnode data structures as well -------------------- */
extern name_vvertex_map vname2vertex;
extern name_name_map fixed_nodes;
extern name_name_map node_hints;
extern name_count_map vtypes;
extern name_list_map vclasses;
extern vvertex_vector virtual_nodes;
extern name_vclass_map vclass_map;
/* ---------------------------------- end of vtop stuff ----------------------------------- */

// This is a hash map of the entire physical topology because it takes far too long for it to search the XML DOM tree.
map<string, DOMElement*>* advertisement_elements = new map<string, DOMElement*>();
DOMElement* advertisement_root = NULL;

/*
 * TODO: This should be moved out of parse_top.cc, where it currently
 * resides.
 */
int bind_ptop_subnodes(tb_pgraph &pg);
int bind_vtop_subnodes(tb_vgraph &vg);

/*
 * These are not meant to be used outside of this file, so they are only
 * declared in here
 */
bool populate_nodes_rspec(DOMElement *root, tb_pgraph &pg, tb_sgraph &sg,
			  set<string> &unavailable);
bool populate_links_rspec(DOMElement *root, tb_pgraph &pg, tb_sgraph &sg,
			  set<string> &unavailable);

void populate_policies(DOMElement *root);

int parse_fds_xml (const DOMElement* tag, node_fd_set *fd_set);

int parse_ptop_rspec(tb_pgraph &pg, tb_sgraph &sg, char *filename) {
    /* 
     * Fire up the XML parser
     */
    XMLPlatformUtils::Initialize();
    
    XercesDOMParser *parser = new XercesDOMParser;
    
    /*
     * Enable some of the features we'll be using: validation, namespaces, etc.
     */
    parser->setValidationScheme(XercesDOMParser::Val_Always);
    parser->setDoNamespaces(true);
    parser->setDoSchema(true);
    parser->setValidationSchemaFullChecking(true);
    
    /*
     * Must validate against the ptop schema
     */
	parser -> setExternalSchemaLocation
			 ("http://www.protogeni.net/resources/rspec/0.1 " SCHEMA_LOCATION);
    
    /*
     * Just use a custom error handler - must admin it's not clear to me why
     * we are supposed to use a SAX error handler for this, but this is what
     * the docs say....
     */    
    ParseErrorHandler* errHandler = new ParseErrorHandler();
    parser->setErrorHandler(errHandler);
    
    /*
     * Do the actual parse
     */
    parser->parse(filename);
    //XMLDEBUG("XML parse completed" << endl);
	    
    /* 
     * If there are any errors, do not go any further
     */
    if (errHandler->sawError()) {
        cerr << "There were " << parser -> getErrorCount () 
			 << " errors in your file. " << endl;
        exit(EXIT_FATAL);
    }
    else {
        /*
        * Get the root of the document - we're going to be using the same root
        * for all subsequent calls
        */
        DOMDocument *doc = parser->getDocument();
        advertisement_root = doc->getDocumentElement();
        set<string> unavailable; // this should really be an unordered_set,
	    // but that's not very portable yet

        bool is_physical;
        XStr type (advertisement_root->getAttribute(XStr("type").x()));
        if (strcmp(type.c(), "advertisement") == 0)
        	is_physical = true;
		else if (strcmp(type.c(), "request") == 0)
			is_physical = false;
        
        // XXX: Not sure about datetimes, so they are strings for now
        XStr generated
				(advertisement_root->getAttribute(XStr("generated").x()));
        XStr valid_until
				(advertisement_root->getAttribute(XStr("valid_until").x()));
        
        /*
        * These three calls do the real work of populating the assign data
        * structures
        */
        XMLDEBUG("starting node population" << endl);
        if (!populate_nodes_rspec(advertisement_root,pg,sg,unavailable)) {
        cerr << "Error reading nodes from physical topology " 
			 << filename << endl;
        exit(EXIT_FATAL);
        }
        XMLDEBUG("finishing node population" << endl);
        
		XMLDEBUG("starting link population" << endl);
        if (!populate_links_rspec(advertisement_root,pg,sg,unavailable)) {
        cerr << "Error reading links from physical topology " 
			 << filename << endl;
        exit(EXIT_FATAL);
        }
        XMLDEBUG("finishing link population" << endl);
        
        // TODO: We need to do something about these policies
		//populate_policies(root);
        
        cerr << "RSpec parsing finished" << endl; 
    }
    
    /*
    * All done, clean up memory
    */
//     XMLPlatformUtils::Terminate();
    return 0;
}

/*
 * Pull nodes from the document, and populate assign's own data structures
 */
bool populate_nodes_rspec(DOMElement *root, tb_pgraph &pg, tb_sgraph &sg,
			  set<string> &unavailable) {
	bool is_ok = true;
	pair<map<string, DOMElement*>::iterator, bool> insert_ret;
    /*
     * Get a list of all nodes in this document
     */
    DOMNodeList *nodes = root->getElementsByTagName(XStr("node").x());
    int nodeCount = nodes->getLength();
    XMLDEBUG("Found " << nodeCount << " nodes in rspec" << endl);
	clock_t times [nodeCount];

	int availableCount = 0;
	int counter = 0;
    for (size_t i = 0; i < nodeCount; i++) 
	{
		DOMNode *node = nodes->item(i);
		// This should not be able to fail, because all elements in
		// this list came from the getElementsByTagName() call
		DOMElement *elt = dynamic_cast<DOMElement*>(node);
				
		component_spec componentSpec = parse_component_spec(elt);
		string str_component_manager_uuid =
				                string(componentSpec.component_manager_uuid);
		string str_component_name = string(componentSpec.component_name);
		string str_component_uuid = string(componentSpec.component_uuid);

		if (str_component_manager_uuid == "")
		{
			is_ok = false;
			continue;
		}
		
		XStr available (getChildValue(elt, "available"));
		if (strcmp(available, "false") == 0)
		{
		    unavailable.insert( str_component_uuid );
		    continue;
		}
		
		++availableCount;
		
		if (str_component_uuid == "")
		{
			is_ok = false;
			continue;
		}
		else
		{
			insert_ret = advertisement_elements->insert(
					       pair<string, DOMElement*>(str_component_uuid, elt));
			if (insert_ret.second == false)
			{
				cerr << str_component_uuid << " already exists" << endl;
				is_ok = false;
			}
		}

		XStr virtualization_type
				        (elt->getAttribute(XStr("virtualization_type").x()));
		
		DOMNodeList *interfaces =
				            elt->getElementsByTagName(XStr("interface").x());
		string *str_component_interface_names = 
				                        new string [interfaces->getLength()];
		for (int index = 0; index < interfaces->getLength(); ++index)
			str_component_interface_names[index] = string(
					XStr((dynamic_cast<DOMElement*>(interfaces->item(index)))
					           ->getAttribute(XStr("component_id").x())).c());

		/* Deal with the location tag */
		string country = string("");
		string latitude = string ("");
		string longitude = string("");
		if (hasChildTag(elt, "location"))
		{
			country = string(
							XStr(elt->getAttribute (XStr("country").x())).c());
			if (elt->hasAttribute (XStr("latitude").x()))
				latitude = string
						   (XStr(elt->getAttribute(XStr("latitude").x())).c());
			if (elt->hasAttribute (XStr("longitude").x()))
				longitude = string
						  (XStr(elt->getAttribute(XStr("longitude").x())).c());
		}

		pvertex pv;

		tb_pnode *p;
		/*
		* TODO: These three steps shouldn't be 'manual'
		*/
		pv = add_vertex(pg);
		// XXX: This is wrong!
		p = new tb_pnode(str_component_uuid);
		// XXX: Global
		put(pvertex_pmap,pv,p);
		/*
		* XXX: This shouldn't be "manual"
		*/
		pname2vertex[str_component_uuid.c_str()] = pv;
		
		/*
		* Add on types
		*/
		int type_slots = 0;
		bool no_type = false;
		tb_vclass *vclass;
		const char* str_type_name;
		// XXX: This a ghastly hack. Find a way around it ASAP.
		string s_type_name = string("");
		DOMNodeList *types = elt->getElementsByTagName(XStr("node_type").x());
		for (int i = 0; i < types->getLength(); i++) 
		{
			DOMElement *typetag = dynamic_cast<DOMElement*>(types->item(i));
// 			XStr type_name(getChildValue(typetag, "type_name"));
			XStr type_name(typetag->getAttribute(XStr("type_name").x()));
			
			/*
			* Check to see if it's a static type
			*/
			bool is_static = false;
			if (typetag->hasAttribute(XStr("static").x()))
				is_static = true;
			
			/*
			* ... and how many slots it has
			* XXX: Need a real 'unlimited' value!
			*/
			int type_slots;
			XStr type_slot_string
					           (typetag->getAttribute(XStr("type_slots").x()));
			if (strcmp(type_slot_string.c(), "unlimited") == 0) {
				type_slots = 1000;
			} 
			else {
				type_slots = type_slot_string.i();
			}
			
			/*
			* Make a tb_ptype structure for this guy - or just add this node to
			* it if it already exists
			* XXX: This should not be "manual"!
			*/
			str_type_name = type_name.c();
			s_type_name = string (str_type_name);
			if (ptypes.find(str_type_name) == ptypes.end()) {
				ptypes[str_type_name] = new tb_ptype(str_type_name);
			}
			ptypes[str_type_name]->add_slots(type_slots);
			tb_ptype *ptype = ptypes[str_type_name];
			
			/*
			* For the moment, we treat switches specially - when we get the
			* "forwarding" code working correctly, this special treatment
			* will go away.
			* TODO: This should not be in the parser, it should be somewhere
			* else!
			*/
			if (type_name == "switch") {
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
				p->types[str_type_name] = 
					new tb_pnode::type_record(type_slots,is_static,ptype);
			}
			p->type_list.push_back(p->types[str_type_name]);
		}

		if( hasChildTag( elt, "exclusive" ) ) {
		    XStr exclusive( getChildValue( elt, "exclusive" ) );
		    fstring feature( "shared" );

		    if( !strcmp( exclusive, "false" ) )
			p->features.push_front( 
							tb_node_featuredesire( feature, 
								1.0, true, featuredesire::FD_TYPE_NORMAL) );
		}

	   /*
		* Parse out the features
		* TODO: I am pretty sure that we don't have features in Protogeni.
		*/
		//parse_fds_xml(elt,&(p->features));
		
		/*
		* Finally, pull out any special node flags
		*/
// 		int trivial_bw = 0;
// 		if (hasChildTag(elt,"trivial_bw")) {
// 			trivial_bw = XStr(getChildValue(elt,"trivial_bw")).i();
// 			////XMLDEBUG("  Trivial bandwidth: " << trivial_bw << endl);
// 		}
// 		p->trivial_bw = trivial_bw;
		
		// Add the component_manager_uuid as a feature.
		// We need to do this to handle external references
		(p->features).push_front(
		                  tb_node_featuredesire(
								XStr(str_component_manager_uuid.c_str()).f(), 
								1.0, false, featuredesire::FD_TYPE_NORMAL));
		
 		if (hasChildTag(elt,"subnode_of")) {
			XStr subnode_of_name(getChildValue(elt,"subnode_of"));
			if (!p->subnode_of_name.empty()) 
			{
		    	is_ok = false;
		    	continue;
			} 
			else 
			{
				// Just store the name for now, we'll do late binding to
				// an actual pnode later
		    	p->subnode_of_name = subnode_of_name.f();
			}
			//XMLDEBUG("  Subnode of: " << subnode_of_name << endl);
		}
// 		
// 		if (hasChildTag(elt,"unique")) {
// 			p->unique = true;
// 			////XMLDEBUG("  Unique" << endl);
// 		}
	
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
bool populate_links_rspec(DOMElement *root, tb_pgraph &pg, tb_sgraph &sg,
			  set<string> &unavailable) {
    
    bool is_ok = true;
    pair<map<string, DOMElement*>::iterator, bool> insert_ret;
	
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
        
/*        interface_spec *pathSrcs;
        interface_spec *pathDsts;*/
		
		component_spec linkComponentSpec = parse_component_spec(
				                            dynamic_cast<DOMElement*>(link));
		string str_component_manager_uuid = 
				                    linkComponentSpec.component_manager_uuid;
		string str_component_uuid = linkComponentSpec.component_uuid;
		string str_component_name = linkComponentSpec.component_name;
		string str_sliver_uuid = linkComponentSpec.sliver_uuid;
		if (str_component_uuid == "" || str_component_manager_uuid == "")
			is_ok = false;
		else
		{
			insert_ret = advertisement_elements->insert(
					       pair<string, DOMElement*>(str_component_uuid, elt));
			if (insert_ret.second == false)
			{
				cerr << str_component_uuid << " already exists" << endl;
				is_ok = false;
			}
		}
		
        /*
        * Get source and destination interfaces - we use knowledge of the
        * schema that there is awlays exactly one source and one destination
        */
        string src_node;
        string src_iface;
        string dst_node;
        string dst_iface;
		DOMNodeList *interfaces = elt->getElementsByTagName(
				                                    XStr("interface_ref").x());
		/* NOTE: In a request, we assume that each link has only two interfaces
		 * Although the order is immaterial, assign expects a source first 
		 * and a destination second and we assume the same
		 */
		if (interfaces->getLength() != 2)
		{
			cerr << "Incorrect number of interfaces found on link " 
				 << str_component_uuid << ". Expected 2 (found " 
				 << interfaces->getLength() << ")" << endl;
			is_ok = false;
			continue;
		}
		interface_spec source = parse_interface_rspec_xml(
							dynamic_cast<DOMElement*>(interfaces->item(0)));
		interface_spec dest = 				parse_interface_rspec_xml(
				            dynamic_cast<DOMElement*>(interfaces->item(1)));

		src_node = source.component_node_id;
		src_iface = source.component_interface_id;
		dst_node = dest.component_node_id;
		dst_iface = dest.component_interface_id;
		
		if (src_node.compare("") == 0 || src_iface.compare("") == 0)
		{
			cerr << "Physical link " << str_component_uuid 
				 << " must have a component uuid and "
				 << "component interface name specified for the source node" 
				 << endl;
			is_ok = false;
			continue;
		}
		if (dst_node.compare ("") == 0 || dst_iface.compare("") == 0)
		{
			cerr << "Physical link " << str_component_uuid 
				 << " must have a component uuid and component interface name"
				 << " specified for the destination node" << endl;
			is_ok = false;
			continue;
		}

		if( unavailable.count( src_node ) || 
		    unavailable.count( dst_node ) )
		    // one or both of the endpoints are unavailable; silently
		    // ignore the link
		    continue;

        /*
        * Get standard link characteristics
        */
        XStr bandwidth(getChildValue(elt,"bandwidth"));
        XStr latency(getChildValue(elt,"latency"));
        XStr packet_loss(getChildValue(elt,"packet_loss"));
        
        /*
        * Check to make sure the referenced nodes actually exist
        */
		if (pname2vertex.find(src_node.c_str()) == pname2vertex.end()) {
			cerr << "Bad link, non-existent source node " 
				 << src_node << endl;
			is_ok = false;
			continue;
		}
		if (pname2vertex.find(dst_node.c_str()) == pname2vertex.end()) {
			cerr << "Bad link, non-existent destination node " 
				 << dst_node << endl;
			is_ok = false;
			continue;
		}
		
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
		DOMNodeList *types = elt->getElementsByTagName(XStr ("link_type").x());
		DOMElement *first_type_tag = dynamic_cast<DOMElement*>(types->item(0));
		XStr first_type (first_type_tag->getAttribute(XStr("type_name").x()));
		const char* str_first_type = first_type.c();
        		
		/*
		* Create the actual link object
		*/
		pedge phys_edge = (add_edge(src_vertex,dst_vertex,pg)).first;
		// XXX: Link type!?
		// XXX: Don't want to use (null) src and dest macs, but would break
		// other stuff if I remove them... bummer!
		tb_plink *phys_link =
			new tb_plink(str_component_uuid.c_str(), 
						 tb_plink::PLINK_NORMAL, str_first_type,
	   				    "(null)", "(null)", 
			             src_iface.c_str(), dst_iface.c_str());
		
		phys_link->delay_info.bandwidth = bandwidth.i();
		phys_link->delay_info.delay = latency.i();
		phys_link->delay_info.loss = packet_loss.d();
	
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
		src_pnode->link_counts[str_first_type]++;
		dst_pnode->link_counts[str_first_type]++;
		
		/*
		* Add in the rest of the link types we found
		*/
		for (int i = 1; i < types->getLength(); i++) 
		{
			DOMElement *link_type = dynamic_cast<DOMElement*>(types->item(i));
			const char *str_type_name = XStr(link_type->getAttribute(
					           				   XStr("type_name").x())).c();
			////XMLDEBUG("  Link has type " << type_name << endl);
			// XXX: Should not be manual
			phys_link->types.insert(str_type_name);
			src_pnode->link_counts[str_type_name]++;
			dst_pnode->link_counts[str_type_name]++;
		}
	
		if (ISSWITCH(src_pnode) && ISSWITCH(dst_pnode)) 
		{
			svertex src_switch = get(pvertex_pmap,src_vertex)->sgraph_switch;
			svertex dst_switch = get(pvertex_pmap,dst_vertex)->sgraph_switch;
			sedge swedge = add_edge(src_switch,dst_switch,sg).first;
			tb_slink *sl = new tb_slink();
			put(sedge_pmap,swedge,sl);
			sl->mate = phys_edge;
			phys_link->is_type = tb_plink::PLINK_INTERSWITCH;
		}
			
		else if (ISSWITCH(src_pnode) && ! ISSWITCH(dst_pnode)) 
		{
			dst_pnode->switches.insert(src_vertex);
			#ifdef PER_VNODE_TT
				dst_pnode->total_bandwidth += bandwidth.i();
			#endif
		}
			
		else if (ISSWITCH(dst_pnode) && ! ISSWITCH(src_pnode)) 
		{
			src_pnode->switches.insert(dst_vertex);
			#ifdef PER_VNODE_TT
				src_pnode->total_bandwidth += bandwidth.i();
			#endif
		}

    }
    return is_ok;
}

#endif

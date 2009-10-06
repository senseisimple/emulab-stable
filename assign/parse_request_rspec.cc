/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * XML Parser for RSpec ptop files
 */

static const char rcsid[] = "$Id: parse_request_rspec.cc,v 1.13 2009-10-06 23:53:11 duerig Exp $";

#ifdef WITH_XML

#include "parse_request_rspec.h"
#include "xmlhelpers.h"
#include "parse_error_handler.h"

#include <fstream>

#include "anneal.h"
#include "vclass.h"

#define XMLDEBUG(x) (cerr << x)
#define ISSWITCH(n) (n->types.find("switch") != n->types.end())

#ifdef TBROOT
	#define SCHEMA_LOCATION TBROOT"/lib/assign/rspec-request.xsd"
#else
	#define SCHEMA_LOCATION "rspec-request.xsd"
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

DOMElement* request_root = NULL;
DOMDocument* doc = NULL;

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
bool populate_nodes_rspec(DOMElement *root, tb_vgraph &vg);
bool populate_links_rspec(DOMElement *root, tb_vgraph &vg);
bool populate_vclasses_rspec (DOMElement *root, tb_vgraph &vg);

bool hasComponentSpec (DOMElement* element);

void populate_policies(DOMElement *root);

int parse_fds_xml (const DOMElement* tag, node_fd_set *fd_set);

int parse_vtop_rspec(tb_vgraph &vg, char *filename) {
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
    parser -> setExternalSchemaLocation ("http://www.protogeni.net/resources/rspec/0.1 " SCHEMA_LOCATION);
    
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
        cerr << "There were " << parser -> getErrorCount () << " errors in your file. Please correct the errors and try again." << endl;
        exit(EXIT_FATAL);
    }
    else {
        /*
        * Get the root of the document - we're going to be using the same root
        * for all subsequent calls
        */
        doc = parser->getDocument();
        request_root = doc->getDocumentElement();
        
        bool is_physical;
        XStr type (request_root->getAttribute(XStr("type").x()));
		if (strcmp(type.c(), "request") != 0)
		{
			cerr << "Error: RSpec type must be \"request\"" << endl;
			exit (EXIT_FATAL);
		}
        
        // XXX: Not sure what to do with the datetimes, so they are strings for now
        XStr generated (request_root->getAttribute(XStr("generated").x()));
        XStr valid_until(request_root->getAttribute(XStr("valid_until").x()));
        
        /*
        * These three calls do the real work of populating the assign data
        * structures
        */
        // clock_t startNode = clock();
        XMLDEBUG("starting node population" << endl);
        if (!populate_nodes_rspec(request_root,vg)) {
        cerr << "Error reading nodes from virtual topology " << filename << endl;
        exit(EXIT_FATAL);
        }
        XMLDEBUG("finishing node population" << endl);
        // //cerr << "Time taken : " << (clock() - startNode) / CLOCKS_PER_SEC << endl;

		// clock_t startLink = clock();
        XMLDEBUG("starting link population" << endl);
        if (!populate_links_rspec(request_root,vg)) {
        cerr << "Error reading links from virtual topology " << filename << endl;
        exit(EXIT_FATAL);
        }
        XMLDEBUG("finishing link population" << endl);
		// //cerr << "Time taken : " << (clock() - startLink) / CLOCKS_PER_SEC << endl;
        
        /* TODO: We need to do something about policies at some point. */
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
bool populate_nodes_rspec(DOMElement *root, tb_vgraph &vg) {
	bool is_ok = true;
    /*
     * Get a list of all nodes in this document
     */
    DOMNodeList *nodes = root->getElementsByTagName(XStr("node").x());
    int nodeCount = nodes->getLength();
    XMLDEBUG("Found " << nodeCount << " nodes in rspec" << endl);
	clock_t times [nodeCount];

	int counter = 0;
    for (size_t i = 0; i < nodeCount; i++) 
	{
		DOMNode *node = nodes->item(i);
		// This should not be able to fail, due to the fact that all elements in
		// this list came from the getElementsByTagName() call
		DOMElement *elt = dynamic_cast<DOMElement*>(node);
				
		
		// TODO: There ought to be a check here to do some of this stuff only if it is a physical node
		
		string str_virtual_uuid = string("");
		if (elt->hasAttribute(XStr("virtual_id").x()))
			str_virtual_uuid = string (XStr(elt->getAttribute(XStr("virtual_id").x())).c());
			
		XStr virtualization_type(elt->getAttribute(XStr("virtualization_type").x()));
		bool available = hasChildTag (elt, "available");
        
        string str_aggregate_uuid = string("");
        string str_component_name = string("");
        string str_component_uuid = string("");
		string str_component_manager_uuid = string("");
        string str_sliver_uuid = string("");
        if (hasComponentSpec(elt))
        {
        	component_spec componentSpec = parse_component_spec(elt);
        	str_component_manager_uuid = string(componentSpec.component_manager_uuid);
        	str_component_name = string(componentSpec.component_name);
        	str_component_uuid = string(componentSpec.component_uuid);
        	str_sliver_uuid = string(componentSpec.sliver_uuid);
			// If a node has a component_uuid, it is a fixed node
			fixed_nodes [str_virtual_uuid] = str_component_uuid;	
        }
		
		if (str_virtual_uuid == "")
		{
			cerr << "Every node must have a virtual_id" << endl;
			is_ok = false;
			continue;
		}

		DOMNodeList *interfaces = elt->getElementsByTagName(XStr("interface").x());
		string *str_virtual_interface_names = new string [interfaces->getLength()];
		string *str_component_interface_names = new string [interfaces->getLength()];
		for (int index = 0; index < interfaces->getLength(); ++index)
		{
			str_virtual_interface_names[index] = string(XStr((dynamic_cast<DOMElement*>(interfaces->item(index)))->getAttribute(XStr("virtual_name").x())).c());
			str_component_interface_names[index] = string(XStr((dynamic_cast<DOMElement*>(interfaces->item(index)))->getAttribute(XStr("component_name").x())).c());
		}

		/* Deal with the location tag */
		string country = string("");
		string latitude = string ("");
		string longitude = string("");
		if (hasChildTag(elt, "location"))
		{
			country = string(XStr(elt->getAttribute (XStr("country").x())).c());
			if (elt->hasAttribute (XStr("latitude").x()))
				latitude = string(XStr(elt->getAttribute(XStr("latitude").x())).c());
			if (elt->hasAttribute (XStr("longitude").x()))
				longitude = string(XStr(elt->getAttribute(XStr("longitude").x())).c());
		}

		
		/*
		* Add on types
		*/
		int type_slots = 1;
		tb_vclass *vclass = NULL;
		const char* str_type_name;
		// XXX: This a ghastly hack. Find a way around it ASAP.
		string s_type_name = string("");
		DOMNodeList *types = elt->getElementsByTagName(XStr("node_type").x());
		int num_types = types->getLength();
		bool no_type = !num_types;

		for (int i = 0; i < num_types; i++) 
		{
			DOMElement *node_type = dynamic_cast<DOMElement*>(types->item(i));
			XStr node_type_name (node_type->getAttribute(XStr("type_name").x()));
			
			XStr type_slots_str (node_type->getAttribute(XStr("type_slots").x()));
			int node_type_slots = 1;
			bool is_unlimited = false;
			if (strcmp(type_slots_str.c(), "unlimited") == 0)
				is_unlimited = true;
			else
				type_slots = node_type_slots = type_slots_str.i();

			bool is_static = node_type->hasAttribute(XStr("static").x());
			
			/*
			* Make a tb_ptype structure for this guy - or just add this node to
			* it if it already exists
			* XXX: This should not be "manual"!
			*/
			str_type_name = node_type_name.c();
			s_type_name = string (str_type_name);
			if (ptypes.find(str_type_name) == ptypes.end()) {
				ptypes[str_type_name] = new tb_ptype(str_type_name);
			}
			ptypes[str_type_name]->add_slots(node_type_slots);
			tb_ptype *ptype = ptypes[str_type_name];
			
			name_vclass_map::iterator dit = vclass_map.find(node_type_name.f());
			if (dit != vclass_map.end()) {
				no_type = true;
				vclass = (*dit).second;
			} 
			else {
				vclass = NULL;
				if (vtypes.find(str_type_name) == vtypes.end()) {
					vtypes[str_type_name] = node_type_slots;
				} 
				else {
					vtypes[str_type_name] += node_type_slots;
				}
			}
		}

		/*
		* Parse out the features
		* TODO: We are still not sure how to add features and desires in Protogeni.
		* Commenting this out for now, because we might handle it in some way later.
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
		
// 		if (hasChildTag(elt,"subnode_of")) {
// 			XStr subnode_of_name(getChildValue(elt,"subnode_of"));
// 			if (!p->subnode_of_name.empty()) 
// 			{
// 		    	is_ok = false;
// 		    	continue;
// 			} 
// 			else 
// 			{
// 				// Just store the name for now, we'll do late binding to
// 				// an actual pnode later
// 		    	p->subnode_of_name = subnode_of_name.f();
// 			}
// 			////XMLDEBUG("  Subnode of: " << subnode_of_name << endl);
// 		}
// 		
// 		if (hasChildTag(elt,"unique")) {
// 			p->unique = true;
// 			////XMLDEBUG("  Unique" << endl);
// 		}
	
		/*
		* XXX: Is this really necessary?
		* TODO: We don't really have features, so this shouldn't be here.
		* Commented pending final confirmation.
		*/
		// p->features.sort();
		

		// We might add hint_to to Geni later, but I am not sure that will ever happen. 
		// Am leaving this here for the moment anyway - TP
// 		XStr node_hint_to(elt->getAttribute(XStr("hint_to").x()));

// 		XStr *subnode_of_name = NULL;
// 		if (hasChildTag (elt, "subnode_of"))
// 			subnode_of_name = new XStr(getChildValue (elt, "subnode_of"));
// 
// 		bool is_unique = hasChildTag (elt, "unique");
// 		bool is_disallow_trivial_mix = hasChildTag (elt, "disallow_trivial_mix");

		// A whole bunch of defaults for stuff that assign wants and Protogeni probably doesn't
		// The code to read the values from the request is still there in case we change our minds
		XStr *subnode_of_name = NULL;
		bool is_unique = false;
		bool is_disallow_trivial_mix = false;
		//--------------------------------------------------------------------------------------
		
		tb_vnode *v = NULL;
		if (no_type)
                        // If they gave no type, just assume it's a PC for
                        // now. This is not really a good assumption.
			v = new tb_vnode(str_virtual_uuid.c_str(), "pc", type_slots);
		else
			v = new tb_vnode(str_virtual_uuid.c_str(), s_type_name.c_str(), type_slots);
		
		// Construct the vertex
		v -> disallow_trivial_mix = is_disallow_trivial_mix;
		if (subnode_of_name != NULL)
			v -> subnode_of_name = (*subnode_of_name).c();
		
		if( elt->hasAttribute( XStr( "exclusive" ).x() ) ) {
		    XStr exclusive( elt->getAttribute( XStr(
			"exclusive" ).x() ) );
		    fstring desirename( "shared" );

		    if( !strcmp( exclusive, "false" ) ||
			!strcmp( exclusive, "0" ) ) {
			tb_node_featuredesire node_fd( desirename, 1.0,
                                true, featuredesire::FD_TYPE_NORMAL);
			node_fd.add_desire_user( 1.0 );
			v->desires.push_front( node_fd );
		    } else if( strcmp( exclusive, "true" ) &&
			       strcmp( exclusive, "1" ) ) {
			static int syntax_error;

			if( !syntax_error ) {
			    syntax_error = 1;

			    cout << "Warning: unrecognised exclusive "
				"attribute \"" << exclusive << "\"; will "
				"assume exclusive=\"true\"\n";
			}
		    }
		}
		
		v->vclass = vclass;
		vvertex vv = add_vertex(vg);
		vname2vertex[str_virtual_uuid.c_str()] = vv;
		virtual_nodes.push_back(vv);
		put(vvertex_pmap,vv,v);
		
		parse_fds_vnode_xml (elt, &(v -> desires));
		v -> desires.sort();
			
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

/*
 * Pull the links from the ptop file, and populate assign's own data sturctures
 */
bool populate_links_rspec(DOMElement *root, tb_vgraph &vg) {
    
    bool is_ok = true;
    
    /*
     * TODO: Support the "PENALIZE_BANDWIDTH" option?
     * TODO: Support the "FIX_PLINK_ENDPOINTS" and "FIX_PLINKS_DEFAULT" options?
     */
    DOMNodeList *links = root->getElementsByTagName(XStr("link").x());
    int linkCount = links->getLength();
    XMLDEBUG("Found " << links->getLength()  << " links in rspec" << endl);
    for (size_t i = 0; i < linkCount; i++) {
        DOMNode *link = links->item(i);
        DOMElement *elt = dynamic_cast<DOMElement*>(link);
        
		string str_virtual_uuid = string("");
		if (elt->hasAttribute(XStr("virtual_id").x()))
			str_virtual_uuid = string (XStr(elt->getAttribute(XStr("virtual_id").x())).c());
		
		string str_virtualization_type = string("");
		if (elt->hasAttribute(XStr("virtualization_type").x()))
			str_virtualization_type = string(XStr(elt->getAttribute(XStr("virtualization_type").x())).c());
			
//         string str_component_manager_uuid = string("");
//         string str_component_name = string("");
//         string str_component_uuid = string("");
//         string str_sliver_uuid = string("");
//         
//         component_spec linkComponentSpec;
// 		linkComponentSpec = parse_component_spec(dynamic_cast<DOMElement*>(link));
// 		str_component_manager_uuid = linkComponentSpec.component_manager_uuid;
// 		str_component_uuid = linkComponentSpec.component_uuid;
// 		str_component_name = linkComponentSpec.component_name;
// 		str_sliver_uuid = linkComponentSpec.sliver_uuid;

        /*
        * Get source and destination interfaces - we use knowledge of the
        * schema that there is awlays exactly one source and one destination
        */
        string src_node;
        string src_iface;
        string dst_node;
        string dst_iface;
		DOMNodeList *interfaces = elt->getElementsByTagName(XStr("interface_ref").x());
		/* NOTE: In a request, we assume that each link has only two interfaces specified. 
		 * Although the order is immaterial, assign expects a source and a destination and we assume 
		 * that the first is the source and the second is the destination. */
		if (interfaces->getLength() != 2)
		{
			cerr << "Incorrect number of interfaces found on link " << str_virtual_uuid 
					<< ". Expected 2 (found " << interfaces->getLength() << ")" << endl;
			is_ok = false;
			continue;
		}
        interface_spec source = parse_interface_rspec_xml(dynamic_cast<DOMElement*>(interfaces->item(0)));
        interface_spec dest = parse_interface_rspec_xml(dynamic_cast<DOMElement*>(interfaces->item(1)));

		src_node = source.virtual_node_uuid;
		src_iface = source.virtual_interface_name;
		dst_node = dest.virtual_node_uuid;
		dst_iface = dest.virtual_interface_name;
		if (src_node.compare("") == 0 || src_iface.compare("") == 0)
		{
			cerr << "No source node found on interface for link " << str_virtual_uuid << endl;
			is_ok = false;
			continue;
		}
		if (dst_node.compare("") == 0 || dst_iface.compare("") == 0)
		{
			cerr << "No destination node found on interface for link " << str_virtual_uuid << endl;
			is_ok = false;
			continue;
		}
        
		/*
		 * Get standard link characteristics
		 */
		XStr bandwidth( hasChildTag( elt, "bandwidth" ) ?
				getChildValue(elt,"bandwidth") :
				XStr( "100000" ).x() );
		XStr latency( hasChildTag( elt, "latency" ) ?
			      getChildValue(elt,"latency") :
			      XStr( "0" ).x() );
		XStr packet_loss( hasChildTag( elt, "packet_loss" ) ?
				  getChildValue(elt,"packet_loss") :
				  XStr( "0" ).x() );
	
		if (vname2vertex.find(src_node.c_str()) == vname2vertex.end()) {
			cerr << "Bad link, non-existent source node " << src_node << " which has length " << src_node.length() << endl;
			is_ok = false;
			continue;
		}
		if (vname2vertex.find(dst_node.c_str()) == vname2vertex.end()) {
			cerr << "Bad link, non-existent destination node " << dst_node;
			is_ok = false;
			continue;
		}
		
		/*
		* Get the link type - we know there is at least one, and we
		* need it for the constructor
                * Note: Changed from element to attribute
		*/
                /*
		DOMNodeList *type = elt->getElementsByTagName(XStr ("link_type").x());
		DOMElement *type_tag = dynamic_cast<DOMElement*>(type->item(0));
		XStr link_type(type_tag->getAttribute(XStr("type_name").x()));
                */
		string str_link_type(XStr(elt->getAttribute(XStr("link_type").x())).c());
                if (str_link_type == "")
                {
                  str_link_type = "ethernet";
                }
		
		
		/* ------------------- vtop stuff goes here --------------------------- */
		
		vvertex v_src_vertex = vname2vertex[src_node.c_str()];
		vvertex v_dst_vertex = vname2vertex[dst_node.c_str()];
		tb_vnode *src_vnode = get(vvertex_pmap,v_src_vertex);
		tb_vnode *dst_vnode = get(vvertex_pmap,v_dst_vertex);

		bool emulated = false;
		if (str_virtualization_type.compare("raw") == 0 || str_virtualization_type.compare("") == 0)
			emulated = true;
		
// 		bool allow_delayed = !hasChildTag (elt, "nodelay");
		
		//This section has a whole bunch of defaults for tags that were present earlier
		// but are not in Protogeni. We will eventually decide whether or not we want them.
		bool allow_delayed = true;
		//bool allow_trivial = false;
		bool allow_trivial = true;
		bool fix_src_iface = false;
		bool fix_dst_iface = false;
		fstring fixed_src_iface = "";
		fstring fixed_dst_iface = "";


/*		bool allow_trivial = false;
		#ifdef ALLOW_TRIVIAL_DEFAULT
			allow_trivial = true;
		#else
			allow_trivial = false;
		#endif	
		allow_trivial = hasChildTag(elt, "trivial_ok");*/
		
/*		bool fix_src_iface = false;
		bool fix_dst_iface = false;*/
/*		fstring fixed_src_iface = "";
		fstring fixed_dst_iface = "";*/
// 		if ((fix_src_iface = hasChildTag(elt, "fixsrciface")))
// 			fixed_src_iface = XStr(getChildValue (elt, "fixsrciface")).f();
// 		if ((fix_dst_iface = hasChildTag(elt, "fixdstiface")))
// 			fixed_dst_iface = XStr(getChildValue (elt, "fixdstiface")).f();

		if (emulated) 
		{
			if (!allow_trivial) 
			{
				src_vnode->total_bandwidth += bandwidth.i();
				dst_vnode->total_bandwidth += bandwidth.i();
			}
		} 
		else 
		{
			src_vnode->num_links++;
			dst_vnode->num_links++;
			src_vnode->link_counts[str_link_type.c_str()]++;
			dst_vnode->link_counts[str_link_type.c_str()]++;
		}

		/*
		* Create the actual link object
		*/
		vedge virt_edge = (add_edge(v_src_vertex,v_dst_vertex,vg)).first;
		
		tb_vlink *virt_link = new tb_vlink();
		
		virt_link-> name = str_virtual_uuid;
		virt_link-> type = fstring(str_link_type.c_str());

		virt_link-> fix_src_iface = fix_src_iface;
		virt_link-> src_iface = (fixed_src_iface);//.f();

		virt_link-> fix_dst_iface = fix_dst_iface;
		virt_link-> dst_iface = (fixed_dst_iface);//.f();

		virt_link-> emulated = emulated;
		virt_link-> allow_delayed = allow_delayed;
		virt_link-> allow_trivial = allow_trivial;
		virt_link-> no_connection = true;
		virt_link->delay_info.bandwidth = bandwidth.i();
		virt_link->delay_info.delay = latency.i();
		virt_link->delay_info.loss = packet_loss.d();
		virt_link->src = v_src_vertex;
		virt_link->dst = v_dst_vertex;
	
		// XXX: Should not be manual
		put(vedge_pmap, virt_edge, virt_link);

    // XXX: Special treatment for switches
    }
    return is_ok;
}

#endif

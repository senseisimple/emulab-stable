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

#include <fstream>
#include <sstream>
#include <sys/time.h>

#include "anneal.h"
#include "vclass.h"

#define XMLDEBUG(x) (cerr << x)
#define ISSWITCH(n) (n->types.find("switch") != n->types.end())

#ifdef TBROOT
	#define SCHEMA_LOCATION TBROOT"/lib/assign/request.xsd"
#else
	#define SCHEMA_LOCATION "request.xsd"
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
bool populate_nodes_rspec(DOMElement *root, tb_vgraph &vg, map< pair<string, string>, pair<string, string> >* fixed_interfaces);
bool populate_links_rspec(DOMElement *root, tb_vgraph &vg, map< pair<string, string>, pair<string, string> >* fixed_interfaces);
bool populate_vclasses_rspec (DOMElement *root, tb_vgraph &vg);

string generate_virtual_node_id (string virtual_id);
string generate_virtual_interface_id(string node_name, int interface_number);
DOMElement* appendChildTagWithData (DOMElement* parent, const char*tag_name, const char* child_value);

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
        
		map< pair<string, string>, pair<string, string> > fixed_interfaces;
				//map< pair<string, string>, pair<string, string> >();
				
        /*
        * These three calls do the real work of populating the assign data
        * structures
        */
        // clock_t startNode = clock();
        XMLDEBUG("starting node population" << endl);
        if (!populate_nodes_rspec(request_root,vg, &fixed_interfaces)) {
			cerr << "Error reading nodes from virtual topology " << filename << endl;
			exit(EXIT_FATAL);
        }
        XMLDEBUG("finishing node population" << endl);
        // //cerr << "Time taken : " << (clock() - startNode) / CLOCKS_PER_SEC << endl;

		// clock_t startLink = clock();
        XMLDEBUG("starting link population" << endl);
        if (!populate_links_rspec(request_root,vg, &fixed_interfaces)) {
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

bool populate_node(DOMElement* elt, tb_vgraph &vg, map< pair<string,string>, pair<string,string> >* fixed_interfaces) 
{	
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
		cerr << "ERROR: Every node must have a virtual_id" << endl;
		return false;
	}

	DOMNodeList *interfaces = elt->getElementsByTagName(XStr("interface").x());
	string *str_interface_virtual_ids = new string [interfaces->getLength()];
	string *str_interface_component_ids = new string [interfaces->getLength()];
	for (int index = 0; index < interfaces->getLength(); ++index)
	{
		DOMElement* interface = dynamic_cast<DOMElement*>(interfaces->item(index));
		str_interface_virtual_ids[index] = string(XStr(interface->getAttribute(XStr("virtual_id").x())).c());
		if (interface->hasAttribute(XStr("component_id").x()))
		{
			string component_id = string(XStr(interface->getAttribute(XStr("component_id").x())).c());
			string component_uuid = str_component_uuid;
			pair<map< pair<string, string>, pair<string, string> > :: iterator, bool> rv 
					= fixed_interfaces->insert(make_pair(
											   	make_pair(str_virtual_uuid, str_interface_virtual_ids[index]),
												make_pair(str_component_uuid, component_id)));
			if (rv.second == false) 
			{
				cerr << "The node-interface pair (" << str_virtual_uuid << "," << str_interface_virtual_ids[index] << ") was not unique.";
				cerr << "Interfaces within a node must have unique identifiers."<< endl;
				return false;
			}
		}
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

		if( !strcmp( exclusive, "false" ) || !strcmp( exclusive, "0" ) ) {
			tb_node_featuredesire node_fd( desirename, 1.0,
										   true, featuredesire::FD_TYPE_NORMAL);
			node_fd.add_desire_user( 1.0 );
			v->desires.push_front( node_fd );
		} else if( strcmp( exclusive, "true" ) && strcmp( exclusive, "1" ) ) {
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
	return true;
}

/*
 * Pull nodes from the document, and populate assign's own data structures
 */
bool populate_nodes_rspec(DOMElement *root, tb_vgraph &vg, map< pair<string, string>, pair<string, string> >* fixed_interfaces) {
	bool is_ok = true;
    /*
     * Get a list of all nodes in this document
     */
    DOMNodeList *nodes = root->getElementsByTagName(XStr("node").x());
    int nodeCount = nodes->getLength();
    XMLDEBUG("Found " << nodeCount << " nodes in rspec" << endl);
	clock_t times [nodeCount];

    for (size_t i = 0; i < nodeCount; i++) 
	{
		DOMNode *node = nodes->item(i);
		// This should not be able to fail, due to the fact that all elements in
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

bool populate_link (DOMElement* elt, tb_vgraph &vg, map< pair<string,string>, pair<string,string> >* fixed_interfaces) 
{
	string str_virtual_id = string("");
	if (elt->hasAttribute(XStr("virtual_id").x()))
		str_virtual_id = string (XStr(elt->getAttribute(XStr("virtual_id").x())).c());
		
	string str_virtualization_type = string("");
	if (elt->hasAttribute(XStr("virtualization_type").x()))
		str_virtualization_type = string(XStr(elt->getAttribute(XStr("virtualization_type").x())).c());
		
		/*
	* Get the link type - we know there is at least one, and we
	* need it for the constructor
	* Note: Changed from element to attribute
		*/
	string str_link_type(XStr(elt->getAttribute(XStr("link_type").x())).c());
	if (str_link_type == "")
		str_link_type = "ethernet";

		/*
	* Get standard link characteristics
		*/
	bool bandwidth_specified = hasChildTag(elt, "bandwidth");
	XStr bandwidth( bandwidth_specified ? 
						getChildValue(elt,"bandwidth") : 
						XStr( "100000" ).x() );
	bool latency_specified = hasChildTag(elt, "latency");
	XStr latency( latency_specified ?
			      			getChildValue(elt,"latency") :
							XStr( "0" ).x() );
	bool packet_loss_specified = hasChildTag(elt, "packet_loss");
	XStr packet_loss( packet_loss_specified ?
				  			getChildValue(elt,"packet_loss") :
							XStr( "0" ).x() );
		
	string src_node;
	string src_iface;
	string dst_node;
	string dst_iface;
		
	DOMNodeList *interfaces = elt->getElementsByTagName(XStr("interface_ref").x());
	interface_spec source;
	interface_spec dest;
		
		/* NOTE: In a request, we assume that each link has only two interfaces specified. 
	* Although the order is immaterial, assign expects a source and a destination and we assume 
	* that the first is the source and the second is the destination. 
		* If more than two interfaces are provided, the link must be a lan */
	if (interfaces->getLength() != 2)
	{
		string str_lan_id = generate_virtual_node_id(str_virtual_id);
		
		// NOTE: This is an attribute which is not in the rspec
		// it has been added to easily identify lan_links during annotation
		elt->setAttribute(XStr("is_lan").x(), XStr("true").x());
			
		// Create the lan node
		DOMElement* lan_node = doc->createElement(XStr("node").x());
		request_root->appendChild(lan_node);
		lan_node->setAttribute(XStr("virtualization_type").x(), XStr("raw").x());
		lan_node->setAttribute(XStr("exclusive").x(), XStr("1").x());
		lan_node->setAttribute(XStr("virtual_id").x(), XStr(str_lan_id.c_str()).x());
			
		// Create node type for the lan
		DOMElement* lan_node_type = doc->createElement(XStr("node_type").x());
		lan_node_type->setAttribute(XStr("type_name").x(), XStr("lan").x());
		lan_node_type->setAttribute(XStr("type_slots").x(), XStr("1").x());
		lan_node->appendChild(lan_node_type);
		
		// NOTE: This is an attribute which is not in the rspec
		// but which has been added to distinguish the element
		// from those explicitly specified by the user during annotation
		lan_node->setAttribute(XStr("generated_by_assign").x(), XStr("true").x());
			
		// We need to store the dynamically created links in a list
		// and add them to the virtual graph later because the sanity checks
		// will fail if they are added before the lan node is added.
		list<DOMElement*> links;
		list<DOMElement*>::iterator it = links.begin();
		for (int i = 0; i < interfaces->getLength(); ++i)
		{
			DOMElement* interface = dynamic_cast<DOMElement*>(interfaces->item(i));
			XStr virtual_interface_id (interface->getAttribute(XStr("virtual_interface_id").x()));
			XStr virtual_node_id(interface->getAttribute(XStr("virtual_node_id").x()));
				
			string str_lan_interface_id = generate_virtual_interface_id(str_lan_id, i);
			DOMElement* lan_interface = doc->createElement(XStr("interface").x());
			lan_node->appendChild(lan_interface);
			lan_interface->setAttribute(XStr("virtual_id").x(), XStr(str_lan_interface_id.c_str()).x());
				
			interface_spec interface_ref = parse_interface_rspec_xml(dynamic_cast<DOMElement*>(interfaces->item(i)));
			DOMElement* link = doc->createElement(XStr("link").x());
			request_root->appendChild(link);
			link->setAttribute(XStr("virtual_id").x(), XStr(interface_ref.virtual_node_id + string(":") + str_lan_id).x());
			appendChildTagWithData(link, "bandwidth", bandwidth.c());
			appendChildTagWithData(link, "latency", latency.c());
			appendChildTagWithData(link, "packet_loss", packet_loss.c());
				
			DOMElement* src_interface_ref = doc->createElement(XStr("interface_ref").x());
			src_interface_ref->setAttribute(XStr("virtual_interface_id").x(), virtual_interface_id.x());
			src_interface_ref->setAttribute(XStr("virtual_node_id").x(), virtual_node_id.x());
			link->appendChild(src_interface_ref);
				
			DOMElement* dst_interface_ref = doc->createElement(XStr("interface_ref").x());
			dst_interface_ref->setAttribute(XStr("virtual_interface_id").x(), XStr(str_lan_interface_id.c_str()).x());
			dst_interface_ref->setAttribute(XStr("virtual_node_id").x(), XStr(str_lan_id.c_str()).x());
			link->appendChild(dst_interface_ref);
			
			// Adding attributes to ensure that the element is handled
			// correctly during annotation.
			link->setAttribute(XStr("generated_by_assign").x(), XStr("true").x());
			link->setAttribute(XStr("lan_link").x(), XStr(str_virtual_id.c_str()).x());
			
			links.insert(it, link);
		}
			
		populate_node(lan_node, vg, fixed_interfaces);
		for (it = links.begin(); it != links.end(); ++it)
			populate_link(*it, vg, fixed_interfaces);
		return true;
	}
	else 
	{
		source = parse_interface_rspec_xml(dynamic_cast<DOMElement*>(interfaces->item(0)));
		dest = parse_interface_rspec_xml(dynamic_cast<DOMElement*>(interfaces->item(1)));
	}

	src_node = source.virtual_node_id;
	src_iface = source.virtual_interface_id;
	dst_node = dest.virtual_node_id;
	dst_iface = dest.virtual_interface_id;
	if (src_node.compare("") == 0 || src_iface.compare("") == 0)
	{
		cerr << "No source node found on interface for link " << str_virtual_id << endl;
		return false;
	}
	if (dst_node.compare("") == 0 || dst_iface.compare("") == 0)
	{
		cerr << "No destination node found on interface for link " << str_virtual_id << endl;
		return false;
	}
        
	if (vname2vertex.find(src_node.c_str()) == vname2vertex.end()) {
		cerr << "Bad link " << str_virtual_id << ", non-existent source node " << src_node << endl;
		return false;
	}
	if (vname2vertex.find(dst_node.c_str()) == vname2vertex.end()) {
		cerr << "Bad link " << str_virtual_id << ", non-existent destination node " << dst_node << endl;
		return false;
	}
		
	vvertex v_src_vertex = vname2vertex[src_node.c_str()];
	vvertex v_dst_vertex = vname2vertex[dst_node.c_str()];
	tb_vnode *src_vnode = get(vvertex_pmap,v_src_vertex);
	tb_vnode *dst_vnode = get(vvertex_pmap,v_dst_vertex);

        // If the virtualization type on the string is missing or "raw", then
        // we leave the emulated flag off - we want the whole physical
        // interface. If anything else, we assume that it's some kind of
        // virtualized link and the emulated flag should be set.
	bool emulated = true;
	if (str_virtualization_type.compare("raw") == 0 ||
                str_virtualization_type.compare("") == 0) {
            emulated = false;
            cerr << "Set emulated=false" << endl;
        }
		
// 		bool allow_delayed = !hasChildTag (elt, "nodelay");
		
		//This section has a whole bunch of defaults for tags that were present earlier
		// but are not in Protogeni. We will eventually decide whether or not we want them.
	bool allow_delayed = true;
		//bool allow_trivial = false;
	bool allow_trivial = true;
		
	map< pair<string,string>, pair<string,string> >::iterator it;
		
	bool fix_src_iface = false;
	fstring fixed_src_iface = "";
	it = fixed_interfaces->find(pair<string,string>(src_node.c_str(), src_iface.c_str()));
	if (it != fixed_interfaces->end())
	{
		cerr << "Found fixed source interface (" << (it->second).first << "," << (it->second).second << ") on (" << (it->first).first << "," << (it->first).second << ")" << endl;
		fix_src_iface = true;
		fixed_src_iface = (it->second).second;
	}
			
		
	bool fix_dst_iface = false;
	fstring fixed_dst_iface = "";
	it = fixed_interfaces->find(make_pair(dst_node, src_iface));
	if (it != fixed_interfaces->end())
	{
		cerr << "Found fixed destination interface (" << (it->second).first << "," << (it->second).second << ") on (" << (it->first).first << "," << (it->first).second << ")" << endl;
		fix_dst_iface = true;
		fixed_dst_iface = (it->second).second;
	}

/*		bool allow_trivial = false;
#ifdef ALLOW_TRIVIAL_DEFAULT
	allow_trivial = true;
#else
	allow_trivial = false;
#endif	
	allow_trivial = hasChildTag(elt, "trivial_ok");*/
		
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
		
	virt_link-> name = str_virtual_id;
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
	
	return true;
}

/*
 * Pull the links from the ptop file, and populate assign's own data sturctures
 */
bool populate_links_rspec(DOMElement *root, tb_vgraph &vg, map< pair<string, string>, pair<string, string> >* fixed_interfaces) {
    
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
        
		is_ok &= populate_link(elt, vg, fixed_interfaces);	
    }
    return is_ok;
}

string generate_virtual_node_id (string virtual_id) 
{
	std:ostringstream oss;
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	oss << virtual_id << tv.tv_sec << tv.tv_usec;
	return oss.str();
}

DOMElement* appendChildTagWithData (DOMElement* parent, const char*tag_name, const char* child_value)
{
	DOMElement* child = doc->createElement(XStr(tag_name).x());
	child->appendChild(doc->createTextNode(XStr(child_value).x()));
	parent->appendChild(child);
	return child;
}

string generate_virtual_interface_id (string lan_name, int interface_number)
{
	std:ostringstream oss;
	oss << lan_name << ":" << interface_number;
	return oss.str();
}

#endif

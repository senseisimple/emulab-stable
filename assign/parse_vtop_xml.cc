/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

static const char rcsid[] = "$Id: parse_vtop_xml.cc,v 1.2 2009-05-20 18:06:08 tarunp Exp $";

#ifdef WITH_XML

#include "port.h"

#include <boost/config.hpp>
#include <boost/utility.hpp>
#include <boost/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>

#include <iostream>

using namespace boost;

#include "common.h"
#include "vclass.h"
#include "delay.h"
#include "physical.h"
#include "virtual.h"
#include "parser.h"
#include "anneal.h"
#include "string.h"
#include "parse_vtop_xml.h"
#include "xmlhelpers.h"
#include "xstr.h"
#include "parse_error_handler.h"

extern name_vvertex_map vname2vertex;
extern name_name_map fixed_nodes;
extern name_name_map node_hints;
extern name_count_map vtypes;
extern name_list_map vclasses;
extern vvertex_vector virtual_nodes;
extern name_vclass_map vclass_map;

#define XMLDEBUG(x) (cout << x);


#define top_error(s) errors++;cout << "TOP:" << line << ": " << s << endl
#define top_error_noline(s) errors++;cout << "TOP: " << s << endl

#ifdef TBROOT
	#define SCHEMA_LOCATION TBROOT"/lib/assign/vtop.xsd"
#else
	#define SCHEMA_LOCATION "vtop.xsd"
#endif

// extern name_vclass_map vclass_map;
// extern name_name_map fixed_nodes;
// extern name_name_map node_hints;

DOMElement *root = NULL;
DOMDocument *vtop_xml_document = NULL;

bool populate_nodes (DOMElement* root, tb_vgraph &vg);
bool populate_links (DOMElement* root, tb_vgraph &vg);
bool populate_vclasses (DOMElement* root, tb_vgraph &vg);

int bind_vtop_subnodes (tb_vgraph &vg);

int parse_vtop_xml(tb_vgraph &vg, char* filename) {
    
    /*
     * Initialize the XML parser
     */
    XMLPlatformUtils::Initialize();
    
    //XMLReader xerces = XMLReaderFactory::createXMLReader("org.apache.xerces.parsers.SAXParser");
    
    XercesDOMParser *parser = new XercesDOMParser;
    parser->setValidationScheme(XercesDOMParser::Val_Always);
    parser->setDoNamespaces(true);
    parser->setDoSchema(true);
    parser->setValidationSchemaFullChecking(true);
    
	parser -> setExternalSchemaLocation ("http://emulab.net/resources/vtop/0.2 " SCHEMA_LOCATION);
        
    ParseErrorHandler *handler = new ParseErrorHandler();
    parser->setErrorHandler(handler);
    
    /*
     * Do the actual parsing
     */
    parser->parse(filename);
    
	if (handler->sawError()) {
		cerr << "There were " << parser -> getErrorCount() << " errors. Please correct the errors and try again." << endl;
		exit(EXIT_FATAL);
	}

    vtop_xml_document = parser->getDocument();
    root = vtop_xml_document->getDocumentElement();
    
	XMLDEBUG("Starting vclass population ... " << endl);
    if (!populate_vclasses(root, vg))
	{
		cerr << "Error reading vclasses from virtual topology " << filename << endl;
		exit (EXIT_FATAL);
	}	
	XMLDEBUG ("Finishing vclass population ... " << endl);
	

	XMLDEBUG("Starting node population ... " << endl);
    if (!populate_nodes(root, vg))
	{
		cerr << "Error reading nodes from virtual topology " << filename << endl;
		exit (EXIT_FATAL);
	}
	XMLDEBUG("Finishing node population ..." << endl);
 
	XMLDEBUG("Starting link population ... " << endl);
	if (!populate_links(root, vg))
	{	
		cerr << "Error reading links from virtual topology " << filename << endl;
		exit (EXIT_FATAL);
	}
	XMLDEBUG ("Finishing link population ... " << endl);
					

    // Clean up parser memory
    // delete parser;
    //XMLPlatformUtils::Terminate();
    return 0;
}

bool populate_nodes (DOMElement *root, tb_vgraph &vg) {

	bool is_ok = true;

	DOMNodeList *nodes = root->getElementsByTagName(XStr("node").x());
    int nodeCount = nodes->getLength();
    XMLDEBUG("Found " << nodeCount << " nodes in vtop" << endl);

    for (size_t i = 0; i < nodeCount; i++) 
	{
		DOMNode *node = nodes->item(i);
		// This should not be able to fail, due to the fact that all elements in
		// this list came from the getElementsByTagName() call
		DOMElement *elt = dynamic_cast<DOMElement*>(node);
		XStr node_name(elt->getAttribute(XStr("name").x()));
		XStr node_assigned_to(elt->getAttribute(XStr("assigned_to").x()));
		XStr node_hint_to(elt->getAttribute(XStr("hint_to").x()));
		//XMLDEBUG("Got node " << name << endl);

		//XMLDEBUG("Node " << node_name << " assigned to " << node_assigned_to << endl);
		if (strcmp(node_assigned_to.c(), "") != 0)
			fixed_nodes[node_name.f()] = node_assigned_to.f();
		
		if (strcmp(node_hint_to.c(), "") != 0)
			node_hints[node_name.f()] = node_hint_to.f();
			
		// For a virtual node, it will always return only 1 node. 
		// In any other case, the error would have been caught by the validator earlier
		DOMElement *node_type = dynamic_cast<DOMElement*>(elt -> getElementsByTagName(XStr("node_type").x()) -> item(0));

		// If there is a type_slots node, it has the number of slots
		// Else there has to be an "unlimited" node.
		// If neither is present, the parser will have complained earlier.
		XStr node_type_name (node_type->getAttribute(XStr("type_name").x()));
		
		XStr type_slots (node_type->getAttribute(XStr("type_slots").x()));
		int node_type_slots = 1;
		bool is_unlimited = false;
		if (strcmp(type_slots.c(), "unlimited") == 0)
			is_unlimited = true;
		else
			node_type_slots = type_slots.i();

		bool is_static = node_type->hasAttribute(XStr("static").x());
				
		XStr *subnode_of_name = NULL;
		if (hasChildTag (elt, "subnode_of"))
			subnode_of_name = new XStr(getChildValue (elt, "subnode_of"));

		bool is_unique = hasChildTag (elt, "unique");
		bool is_disallow_trivial_mix = hasChildTag (elt, "disallow_trivial_mix");
		
		tb_vclass *vclass;
	
		const char *str_node_type_name = node_type_name.c();
		bool no_type = false;
		name_vclass_map::iterator dit = vclass_map.find(node_type_name.f());
		if (dit != vclass_map.end()) {
			no_type = true;
			cout<<"Found a type called " << node_type_name.f() << " ?"<<endl;
			vclass = (*dit).second;
		} 
		else {
			vclass = NULL;
			if (vtypes.find(str_node_type_name) == vtypes.end()) {
				vtypes[str_node_type_name] = node_type_slots;
			} 
			else {
				vtypes[str_node_type_name] += node_type_slots;
			}
		}
		
		tb_vnode *v = NULL;
		if (no_type)
			v = new tb_vnode(node_name.c(), "", node_type_slots);
		else
			v = new tb_vnode(node_name.c(), str_node_type_name, node_type_slots);
		
		// Construct the vertex
		v -> disallow_trivial_mix = is_disallow_trivial_mix;
		if (subnode_of_name != NULL)
			v -> subnode_of_name = (*subnode_of_name).c();
		
		v->vclass = vclass;
		vvertex vv = add_vertex(vg);
		vname2vertex[node_name.c()] = vv;
		virtual_nodes.push_back(vv);
		put(vvertex_pmap,vv,v);
		
		parse_fds_vnode_xml (elt, &(v -> desires));
		v -> desires.sort();
	}	

	int errors = bind_vtop_subnodes (vg);
	if (errors > 0)
	{
		cerr << "Errors occurred while binding the subnodes. Check the virutal topology file." << endl;
		is_ok = false;
	}
	return is_ok;
}

bool populate_links (DOMElement *root, tb_vgraph &vg) {

	bool is_ok = true;
	
	DOMNodeList *links = root->getElementsByTagName(XStr("link").x());
	int linkCount = links->getLength();
	XMLDEBUG("Found " << links->getLength()  << " links in vtop" << endl);
	
	for (size_t i = 0; i < linkCount; i++) {
		DOMNode *link = links->item(i);
		DOMElement *elt = dynamic_cast<DOMElement*>(link);
        
		XStr link_name(elt->getAttribute(XStr("name").x()));
        
        /*
		* Get source and destination interfaces - we use knowledge of the
		* schema that there is awlays exactly one source and one destination
		*/
		DOMNodeList *src_iface_container =
				elt->getElementsByTagName(XStr("source_interface").x());
		node_interface_pair source =
				parse_interface_xml(dynamic_cast<DOMElement*>
				(src_iface_container->item(0)));
		XStr link_src_node(source.first);
		XStr link_src_iface(source.second);
        
        DOMNodeList *dst_iface_container =
				elt->getElementsByTagName(XStr("destination_interface").x());
		node_interface_pair dest =
				parse_interface_xml(dynamic_cast<DOMElement*>
				(dst_iface_container->item(0)));
		XStr link_dst_node(dest.first);
		XStr link_dst_iface(dest.second);
        
        /*
		* Check to make sure the referenced nodes actually exist
		*/
		if (vname2vertex.find(link_src_node.c()) == vname2vertex.end()) {
			cerr << "Bad link, non-existent source node " << link_src_node;
			is_ok = false;
			continue;
		}
		if (vname2vertex.find(link_dst_node.c()) == vname2vertex.end()) {
			cerr << "Bad link, non-existent destination node " << link_dst_node;
			is_ok = false;
			continue;
		}
    
        /*
		* Find the nodes in the existing data structures
		*/
		vvertex src_vertex = vname2vertex[link_src_node.c()];
		vvertex dst_vertex = vname2vertex[link_dst_node.c()];
		tb_vnode *src_vnode = get(vvertex_pmap,src_vertex);
		tb_vnode *dst_vnode = get(vvertex_pmap,dst_vertex);
        
        /*
		* Get standard link characteristics
		*/
		XStr link_bandwidth(getChildValue(elt,"bandwidth"));
		XStr link_latency(getChildValue(elt,"latency"));
		XStr link_packet_loss(getChildValue(elt,"packet_loss"));
        
		/* 
		 * Get extra link characteristics
		 */
		bool emulated = hasChildTag (elt, "multiplex_ok");
		bool allow_delayed = !hasChildTag (elt, "nodelay");
		
		bool allow_trivial = false;
		#ifdef ALLOW_TRIVIAL_DEFAULT
			allow_trivial = true;
		#else
			allow_trivial = false;
		#endif	
		allow_trivial = hasChildTag(elt, "trivial_ok");
		
		bool fix_src_iface = false;
		bool fix_dst_iface = false;
		fstring fixed_src_iface = "";
		fstring fixed_dst_iface = "";
		if ((fix_src_iface = hasChildTag(elt, "fixsrciface")))
			fixed_src_iface = XStr(getChildValue (elt, "fixsrciface")).f();
		if ((fix_dst_iface = hasChildTag(elt, "fixdstiface")))
			fixed_dst_iface = XStr(getChildValue (elt, "fixdstiface")).f();
		
        //XMLDEBUG("  bw = " << link_bandwidth << " latency = " << link_latency << " loss = " << link_packet_loss << endl);
        
        /*
		 * Start getting link types - we know there is at least one, and we
		 * need it for the constructor
		 */
		DOMNodeList *type = elt->getElementsByTagName(XStr ("link_type").x());
		DOMElement *type_tag = dynamic_cast<DOMElement*>(type->item(0));
		XStr link_type(type_tag->getAttribute(XStr("type_name").x()));
        
        //XMLDEBUG ("type_name = " << link_type << endl);
		if (emulated) 
		{
			if (!allow_trivial) 
			{
				src_vnode->total_bandwidth += link_bandwidth.i();
				dst_vnode->total_bandwidth += link_bandwidth.i();
			}
		} 
		else 
		{
			src_vnode->num_links++;
			dst_vnode->num_links++;
			src_vnode->link_counts[link_type.c()]++;
			dst_vnode->link_counts[link_type.c()]++;
		}

		//XMLDEBUG ("Got here" << endl);
        /*
		* Create the actual link object
		*/
		vedge virt_edge = (add_edge(src_vertex,dst_vertex,vg)).first;
        
		tb_vlink *virt_link = new tb_vlink();
        
		virt_link-> name = link_name.f();
		virt_link-> type = link_type.f();
		//if (fix_src_iface)
		//{
			virt_link-> fix_src_iface = fix_src_iface;
			virt_link-> src_iface = (fixed_src_iface);//.f();
		//}
		//if (fix_dst_iface)
		//{
			virt_link-> fix_dst_iface = fix_dst_iface;
			virt_link-> dst_iface = (fixed_dst_iface);//.f();
		//}
		virt_link-> emulated = emulated;
		virt_link-> allow_delayed = allow_delayed;
		virt_link-> allow_trivial = allow_trivial;
		virt_link-> no_connection = true;
		virt_link->delay_info.bandwidth = link_bandwidth.i();
		virt_link->delay_info.delay = link_latency.i();
		virt_link->delay_info.loss = link_packet_loss.d();
		virt_link->src = src_vertex;
		virt_link->dst = dst_vertex;
	
        // XXX: Should not be manual
		put(vedge_pmap, virt_edge, virt_link);
		
	}
	return is_ok;
}

bool populate_vclasses (DOMElement *root, tb_vgraph &vg)
{
	bool is_ok = true;

	DOMNodeList *vclass_elements = root->getElementsByTagName(XStr("vclass").x());
	int vclassCount = vclass_elements->getLength();
	XMLDEBUG("Found " << vclassCount << " vclasses in vtop" << endl);

	for (size_t i = 0; i < vclassCount; i++) 
	{
		DOMNode *vclass = vclass_elements->item(i);
		
		// This should not be able to fail, due to the fact that all elements in
		// this list came from the getElementsByTagName() call
		DOMElement *elt = dynamic_cast<DOMElement*>(vclass);
		
		XStr vclass_name (elt->getAttribute(XStr("name").x()));
		const char *str_vclass_name = vclass_name.c();
		
		tb_vclass *v = NULL;
		/* 
		 * XXX: Have not dealt with the case when the hard tag is present.
		 * Will have to do this before we can use this correctly
		 */
		if (hasChildTag (elt, "hard"))
		{
			// Deal with it here
		}
		else if (hasChildTag (elt, "soft"))
		{
			XStr vclass_weight (getChildValue(elt, "weight"));
			v = new tb_vclass(vclass_name.f(),vclass_weight.d());
			if (v == NULL)
			{
				cerr << "Could not create vclass: " << vclass_name << endl;
				is_ok = false;
				continue;
			}
			vclass_map[str_vclass_name] = v;
		}
		
		/* Get all the physical types for the vclass */
		DOMNodeList *phys_types = elt->getElementsByTagName(XStr("physical_type").x());
		for (int j = 0; j < phys_types->getLength(); j++) 
		{
			DOMElement* phys_type = dynamic_cast<DOMElement*>(phys_types -> item(j));
			XStr phys_type_name (phys_type -> getFirstChild() -> getNodeValue());
			v->add_type(phys_type_name.f());
			vclasses[str_vclass_name].push_back(phys_type_name.f());
		}
	}
	return is_ok;
}

int bind_vtop_subnodes(tb_vgraph &vg) {
    int errors = 0;

    // Iterate through all vnodes looking for ones that are subnodes
    vvertex_iterator vit,vendit;
    tie(vit,vendit) = vertices(vg);
    for (;vit != vendit;++vit) {
	tb_vnode *vnode = get(vvertex_pmap, *vit);
	if (!vnode->subnode_of_name.empty()) {
	    if (vname2vertex.find(vnode->subnode_of_name)
		    == vname2vertex.end()) {
		top_error_noline(vnode->name << " is a subnode of a " <<
			"non-existent node, " << vnode->subnode_of_name << ".");
		continue;
	    }
	    vvertex parent_vv = vname2vertex[vnode->subnode_of_name];
	    vnode->subnode_of = get(vvertex_pmap,parent_vv);
	    vnode->subnode_of->subnodes.push_back(vnode);
	}
    }

    return errors;
}

#endif 

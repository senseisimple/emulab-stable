/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * XML Parser for ptop files
 */

static const char rcsid[] = "$Id: parse_ptop_xml.cc,v 1.4 2009-05-20 18:06:08 tarunp Exp $";

#ifdef WITH_XML

#include "parse_ptop_xml.h"
#include "xmlhelpers.h"
#include "parse_error_handler.h"

#include <fstream>

#include <time.h>

#define XMLDEBUG(x) (cerr << x)
				 
#define ISSWITCH(n) (n->types.find("switch") != n->types.end())

#ifdef TBROOT
	#define SCHEMA_LOCATION TBROOT"/lib/assign/ptop.xsd"
#else
	#define SCHEMA_LOCATION "ptop.xsd"
#endif

/*
 * XXX: Do I have to release lists when done with them?
 */

/*
 * XXX: Global: This is really bad!
 */
extern name_pvertex_map pname2vertex;

/*
 * TODO: This should be moved out of parse_top.cc, where it currently
 * resides.
 */
int bind_ptop_subnodes(tb_pgraph &pg);

/*
 * These are not meant to be used outside of this file, so they are only
 * declared in here
 */
bool populate_nodes(DOMElement *root, tb_pgraph &pg, tb_sgraph &sg);
bool populate_links(DOMElement *root, tb_pgraph &pg, tb_sgraph &sg);
void populate_policies(DOMElement *root);

int parse_fds_xml (const DOMElement* tag, node_fd_set *fd_set);

map<string, DOMElement*>* ptop_elements = new map<string, DOMElement*>();

int parse_ptop_xml(tb_pgraph &pg, tb_sgraph &sg, char *filename) {
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
	parser -> setExternalSchemaLocation ("http://emulab.net/resources/ptop/0.1 " SCHEMA_LOCATION);
    
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
	clock_t start_time;
	start_time = clock ();
    parser->parse(filename);
	cerr << endl << "Completed parsing XML in " << (clock() - start_time) / CLOCKS_PER_SEC << " seconds." << endl << endl;		
    XMLDEBUG("XML parse completed" << endl);

	    
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
        DOMDocument *doc = parser->getDocument();
        DOMElement *root = doc->getDocumentElement();
        
        /*
        * These three calls do the real work of populating the assign data
        * structures
        */
		XMLDEBUG("starting node population" << endl);
		start_time = clock ();
        if (!populate_nodes(root,pg,sg)) {
        cerr << "Error reading nodes from physical topology " << filename
            << endl;
        exit(EXIT_FATAL);
        }
		cerr << endl << "Completed populating nodes in " << (clock() - start_time) / CLOCKS_PER_SEC << " seconds." << endl << endl;		
        XMLDEBUG("finishing node population" << endl);
		
        XMLDEBUG("starting link population" << endl);
		start_time = clock ();
        if (!populate_links(root,pg, sg)) {
        cerr << "Error reading links from physical topology " << filename
            << endl;
        exit(EXIT_FATAL);
        }
		cerr << endl << "Completed populating links in " << (clock() - start_time) / CLOCKS_PER_SEC << " seconds." << endl << endl;		
        XMLDEBUG("finishing link population" << endl);
        
		//populate_policies(root);
        
        cerr << "Ptop parsing finished" << endl; 
    }
    
    /*
    * All done, clean up memory
    */
    XMLPlatformUtils::Terminate();
    return 0;
}

/*
 * Pull nodes from the document, and populate assign's own data structures
 */
bool populate_nodes(DOMElement *root, tb_pgraph &pg, tb_sgraph &sg) {
	bool is_ok = true;
    /*
     * Get a list of all nodes in this document
     */
    DOMNodeList *nodes = root->getElementsByTagName(XStr("node").x());
    int nodeCount = nodes->getLength();
    XMLDEBUG("Found " << nodeCount << " nodes in ptop" << endl);

	int counter = 0;
    for (size_t i = 0; i < nodeCount; i++) 
	{
		DOMNode *node = nodes->item(i);
		// This should not be able to fail, due to the fact that all elements in
		// this list came from the getElementsByTagName() call
		DOMElement *elt = dynamic_cast<DOMElement*>(node);
		XStr name(elt->getAttribute(XStr("name").x()));
		
		ptop_elements->insert(pair<string, DOMElement*>(string(name.c()), elt));
		
		//XMLDEBUG("Got node " << name << endl);
		
		/*
		* TODO: These three steps shouldn't be 'manual'
		*/
		pvertex pv = add_vertex(pg);
		// XXX: This is wrong!
		tb_pnode *p = new tb_pnode(name.f());
		// XXX: Global
		put(pvertex_pmap,pv,p);
		
		/*
		* Add on types
		*/
		DOMNodeList *types = elt->getElementsByTagName(XStr("node_type").x());
		for (int i = 0; i < types->getLength(); i++) 
		{
			
			DOMElement *typetag = dynamic_cast<DOMElement*>(types->item(i));
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
			XStr type_slot_string(typetag->getAttribute(XStr("type_slots").x()));
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
			const char* str_type_name = type_name.c();
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
		
		/*
		* Parse out the features
		*/
		clock_t start = clock ();
		parse_fds_xml(elt,&(p->features));
		
		/*
		* Finally, pull out any special node flags
		*/
		int trivial_bw = 0;
		if (hasChildTag(elt,"trivial_bw")) {
			trivial_bw = XStr(getChildValue(elt,"trivial_bw")).i();
			//XMLDEBUG("  Trivial bandwidth: " << trivial_bw << endl);
		}
		p->trivial_bw = trivial_bw;
		
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
		
		if (hasChildTag(elt,"unique")) {
			p->unique = true;
			//XMLDEBUG("  Unique" << endl);
		}
	
		/*
		* XXX: Is this really necessary?
		*/
		p->features.sort();
		
		/*
		* XXX: This shouldn't be "manual"
		*/
		pname2vertex[name.c()] = pv;
		
		
    }
   		 
    /*
     * This post-pass binds subnodes to their parents
     */
    bind_ptop_subnodes(pg);
    
    /*
     * Indicate no errors
     */
     
    return is_ok;
}

/*
 * Pull the links from the ptop file, and populate assign's own data sturctures
 */
bool populate_links(DOMElement *root, tb_pgraph &pg, tb_sgraph &sg) {
    
    bool errors = false;
    
    /*
     * TODO: Support the "PENALIZE_BANDWIDTH" option?
     * TODO: Support the "FIX_PLINK_ENDPOINTS" and "FIX_PLINKS_DEFAULT" options?
     */
    DOMNodeList *links = root->getElementsByTagName(XStr("link").x());
    int linkCount = links->getLength();
    XMLDEBUG("Found " << links->getLength()  << " links in ptop" << endl);
    for (size_t i = 0; i < linkCount; i++) {
        DOMNode *link = links->item(i);
        DOMElement *elt = dynamic_cast<DOMElement*>(link);
        
        XStr name(elt->getAttribute(XStr("name").x()));
        
		ptop_elements->insert(pair<string, DOMElement*>(string(name.c()), elt));
        //XMLDEBUG("Got link " << name << endl);
    
        /*
        * Get source and destination interfaces - we use knowledge of the
        * schema that there is awlays exactly one source and one destination
        */
        DOMNodeList *src_iface_container =
            elt->getElementsByTagName(XStr("source_interface").x());
        node_interface_pair source =
            parse_interface_xml(dynamic_cast<DOMElement*>
                    (src_iface_container->item(0)));
        XStr src_node(source.first);
        XStr src_iface(source.second);
        
        //XMLDEBUG("  Source: " << src_node << " / " << src_iface << endl);
        
        DOMNodeList *dst_iface_container =
            elt->getElementsByTagName(XStr("destination_interface").x());
        node_interface_pair dest =
            parse_interface_xml(dynamic_cast<DOMElement*>
                    (dst_iface_container->item(0)));
        XStr dst_node(dest.first);
        XStr dst_iface(dest.second);
        
        //XMLDEBUG("  Destination: " << src_node << " / " << src_iface << endl);
        
        /*
        * Check to make sure the referenced nodes actually exist
        */
        if (pname2vertex.find(src_node.c()) == pname2vertex.end()) {
            cerr << "Bad link, non-existent source node " << src_node << endl;
            errors = true;
            continue;
        }
        if (pname2vertex.find(dst_node.c()) == pname2vertex.end()) {
            cerr << "Bad link, non-existent destination node " << dst_node << endl;
            errors = true;
            continue;
        }
    
        /*
        * Find the nodes in the existing data structures
        */
        pvertex src_vertex = pname2vertex[src_node.c()];
        pvertex dst_vertex = pname2vertex[dst_node.c()];
        tb_pnode *src_pnode = get(pvertex_pmap,src_vertex);
        tb_pnode *dst_pnode = get(pvertex_pmap,dst_vertex);
        
        /*
        * Get standard link characteristics
        */
        XStr bandwidth(getChildValue(elt,"bandwidth"));
        XStr latency(getChildValue(elt,"latency"));
        XStr packet_loss(getChildValue(elt,"packet_loss"));
        
        //XMLDEBUG("  bw = " << bandwidth << " latency = " << latency << " loss = " << packet_loss << endl);
        
        /*
        * Start getting link types - we know there is at least one, and we
        * need it for the constructor
        */
        DOMNodeList *types = elt->getElementsByTagName(XStr ("link_type").x());
        DOMElement *first_type_tag = dynamic_cast<DOMElement*>(types->item(0));
		XStr first_type (first_type_tag->getAttribute(XStr("type_name").x()));
		const char* str_first_type = first_type.c();
        
        //XMLDEBUG ("type_name = " << first_type << endl;);
        
        /*
        * Create the actual link object
        */
        pedge phys_edge = (add_edge(src_vertex,dst_vertex,pg)).first;
        // XXX: Link type!?
        // XXX: Don't want to use (null) src and dest macs, but would break
        // other stuff if I remove them... bummer!
        tb_plink *phys_link =
            new tb_plink(name.c(), tb_plink::PLINK_NORMAL, str_first_type,
                         src_pnode->name, "(null)", src_iface.c(),
                         dst_pnode->name, "(null)", dst_iface.c());
            
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
			const char *str_type_name = XStr(link_type->getAttribute(XStr("type_name").x())).c();
            //XMLDEBUG("  Link has type " << type_name << endl);
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
		} else {
                    // Neither is a switch - a direct node->node link
#ifdef PER_VNODE_TT
                    dst_pnode->total_bandwidth += bandwidth.i();
                    src_pnode->total_bandwidth += bandwidth.i();
#endif
                }

	//XMLDEBUG("created link " << *phys_link << endl);
    // XXX: Special treatment for switches
    }
    return !errors;
}

#endif

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "port.h"

#include <boost/graph/adjacency_list.hpp>
using namespace boost;

#include <iostream>

#include "delay.h"
#include "physical.h"
#include "parse_ptop_xml.h"

#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/util/PlatformUtils.hpp>
XERCES_CPP_NAMESPACE_USE

#include <string>
using namespace std;

extern name_pvertex_map pname2vertex;

#define XMLDEBUG(x) (cout << x);

unsigned int PtopParserHandlers::count_pnodes() const {
    return this->pnode_count;
}

/*
 * Implementation of the PtopParserHandlers class
 */
PtopParserHandlers::PtopParserHandlers(tb_pgraph &_PG, tb_sgraph &_SG)
    : PG(_PG), SG(_SG), current_pnode(NULL), current_ptype(NULL),
      current_uri(""), current_localname(""), current_qname(""),
      pnode_count(0) { ; }
PtopParserHandlers::~PtopParserHandlers() { ; }

void PtopParserHandlers::startElement(const XMLCh* const uri,
                                      const XMLCh* const localname,
                                      const XMLCh* const qname,
                                      const Attributes& attrs) {
    
    this->current_uri = uri;
    this->current_localname = localname;
    this->current_qname = qname;
    
    XStr element(localname);
    XMLDEBUG("Starting element " << element << endl);
    /*
     * Dispatch on the element type - note: We can get away with little
     * error checking here thanks to the validating parser - it will make
     * sure required elements are always present
     */

    /*
     * Top-level strings
     */
    if (element == "node") {
        this->startNode(attrs);
    } else if (element == "link") {
        // Nothing for now
    } else if (element == "node_type") {
	this->startNodeType(attrs);
    } else if (element == "limit") {
	this->handleLimit(attrs);
    } else if (element == "unlimited") {
	this->handleUnlimited(attrs);
    } else if (element == "static") {
	this->handleStatic(attrs);
    } else if (element == "node_feature") {
    } else if (element == "feature_weight") {
    } else if (element == "node_option") {
    } else if (element == "option_name") {
    } else if (element == "option_value") {
    } else if (element == "endpoint") {
    } else if (element == "delay_spec") {
    } else if (element == "bandwidth") {
    } else if (element == "delay") {
    } else if (element == "loss") {
    } else if (element == "link_type") {
    } else {
        cerr << "Unknown element type " << element.c() << endl;
    }
}

void PtopParserHandlers::endElement(const XMLCh* const uri,
                                      const XMLCh* const localname,
                                      const XMLCh* const qname) {
    XStr element(localname);
    XMLDEBUG("Ending element " << element << endl);
    
    this->current_uri = uri;
    this->current_localname = localname;
    this->current_qname = qname;
    /* I really should clear the attributes somehow */
    
    /* 
     * Dispatch on element name - note, we don't have to do anything at
     * the end of most tags
     */
    if (element == "node_type") {
	this->endNodeType();
    } else if (element == "node") {
	this->endNode();
    }
}

void PtopParserHandlers::startNode(const Attributes& attrs) {
    /*
     * Create the objects that represent this pnode
     */
    XStr name = attrs.getValue( XStr("name").x() );
    XMLDEBUG("Starting node " << name << endl);
    fstring node_name(name);
    pvertex pv = add_vertex(PG);
    this->current_pnode = new tb_pnode(node_name);
    put(pvertex_pmap,pv,current_pnode);
    pname2vertex[name.c()] = pv;
}

void PtopParserHandlers::endNode() {
    this->pnode_count++;
}

void PtopParserHandlers::startNodeType(const Attributes& attrs) {
    /*
     * Make a tb_ptype structure for this guy - or just add this node to
     * it if it already exists
     */
    XStr nodetype = attrs.getValue( XStr("name").x() );
    XMLDEBUG("  Starting ptype " << nodetype << endl);
    if (ptypes.find(nodetype.c()) == ptypes.end()) {
	ptypes[nodetype.c()] = new tb_ptype(nodetype.c());
    }
    
    this->current_ptype = ptypes[nodetype.c()];
    this->slots = 0;
    this->is_static = false;
}

void PtopParserHandlers::endNodeType() {
    fstring name = this->current_ptype->name();
    ptypes[name]->add_slots(this->slots);
    this->current_pnode->types[name] = 
	new tb_pnode::type_record(this->slots,is_static,current_ptype);
    
}

void PtopParserHandlers::handleLimit(const Attributes& attrs) {
    XStr count = attrs.getValue( XStr("slots").x() );
     if (sscanf(count.c(),"%i",&this->slots) != 1) {
	// XXX: Better error handling
	cout << "*** ERROR: Bad integer " << count << endl;
	exit(EXIT_FATAL);
    }
}

void PtopParserHandlers::handleUnlimited(const Attributes& attrs){
    this->slots = 1000;
}

void PtopParserHandlers::handleStatic(const Attributes& attrs) {
    this->is_static = true;
}

/*
void PtopParserHandlers::characters(const XMLCh* const chars,
                                    const unsigned int length) {
    // Nothing for now
}
*/

/*
 * Handle errors we get from the parser (validation errors, etc.)
 */
void PtopParserHandlers::error(const SAXParseException& exc) {
    this->fatalError(exc);
    
}
void PtopParserHandlers::fatalError(const SAXParseException& exc) {
    cout << "*** Fatal Error while parsing " << XStr(exc.getSystemId()) 
	 << " line <" << exc.getLineNumber() << ", character "
	 << exc.getColumnNumber() << ":" << endl 
	 << XStr(exc.getMessage()) << endl;
    exit(EXIT_FATAL);
}

int parse_ptop_xml(tb_pgraph &PG, tb_sgraph &SG, char *filename) {
	char buffer[16384];

	/*
	 * Initialize the XML parser
	 */
        XMLPlatformUtils::Initialize();
        SAX2XMLReader* parser = XMLReaderFactory::createXMLReader();
	
	// Set up our handlers for the elements
        PtopParserHandlers handler(PG,SG);
        parser->setContentHandler(&handler);
	parser->setErrorHandler(&handler);
	
	// Turn on validation	
        parser->setFeature(XMLUni::fgSAX2CoreValidation, true);
        parser->setFeature(XMLUni::fgXercesDynamic, false);

	// XXX : Figure out to get xerces to parse an istream
        parser->parse(filename);

	/*
	 * Finish up the parser, freeing up memory it allocated
	 * XXX: For some reason, I get double free warnings.
	 * It's possible the Xerces guys haven't caught this because AFAIK, BSD's
	 * malloc (phkmalloc) is the only one that prints warnings about this.
	 * Or, it could be my fault, as a result of a bug in XStr or something.
	 */
	delete parser;
	XMLPlatformUtils::Terminate();
	
	return handler.count_pnodes();
}

/*
 * This is the old ptop parsing code, which I'm cannibalizing. I'm trying to
 * delete code from here as I move it above.
 */
#if 0    
    if (command == "node") {
 
	bool isswitch = false;
	
	  if (type == "switch") {
	    isswitch = true;
	    p->is_switch = true;
	    p->types["switch"] = new tb_pnode::type_record(1,false,ptype);
	    svertex sv = add_vertex(SG);
	    tb_switch *s = new tb_switch();
	    put(svertex_pmap,sv,s);
	    s->mate = pv;
	    p->sgraph_switch = sv;
	    p->switches.insert(pv);
	  } else {
	    p->types[type] = new tb_pnode::type_record(iload,is_static,ptype);
	  }
	}
	for (i=i+1;(i<parsed_line.size()) && (parsed_line[i] != "-") ;++i) {
	  string sfeature,cost;
	  if (split_two(parsed_line[i],':',sfeature,cost,"0") != 0) {
	    ptop_error("Bad node line, no cost for feature: " <<
		       sfeature << ".");
	  }
      fstring feature(sfeature);

	  double gcost;
	  if (sscanf(cost.c_str(),"%lg",&gcost) != 1) {
	    ptop_error("Bad node line, bad cost: " << gcost << ".");
	    gcost = 0;
	  }

	  p->features.push_front(tb_node_featuredesire(feature,gcost));

	}
	/*
	 * Parse any other node options or flags
	 */
	for (i=i+1; i < parsed_line.size(); ++i) {
	    string flag,value;
	    split_two(parsed_line[i],':',flag,value,"(null)");
	    if (flag == "trivial_bw") {
		// Handle trivial bandwidth spec
		int trivial_bw;
		if (sscanf(value.c_str(),"%i",&trivial_bw) != 1) {
		    ptop_error("Bad bandwidth given for trivial_bw: " << value
			    << endl);
		    trivial_bw = 0;
		}
		p->trivial_bw = trivial_bw;
	    } else if (flag == "subnode_of") {
		// Handle subnode relationships
		if (!p->subnode_of_name.empty()) {
		    ptop_error("Can't be a subnode of two nodes");
		    continue;
		} else {
		    // Just store the name for now, we'll do late binding to
		    // an actual pnode later
		    p->subnode_of_name = value;
		}
	    } else if (flag == "unique") {
		// Means that we should never put this pnode into a ptype with
		// other pnodes
		p->unique = true;
	    } else {
		ptop_error("Bad flag given: " << flag << ".");
	    }
	}
	p->features.sort();
	pname2vertex[name] = pv;
      }
    } else if (command == "link") {
      if (parsed_line.size() < 8) {
	ptop_error("Bad link line, too few arguments.");
      }
      int num = 1;
#ifdef PENALIZE_BANDWIDTH
      float penalty;
      if (parsed_line.size() == 9) {
	if (sscanf(parsed_line[8].c_str(),"%f",&penalty) != 1) {
	  ptop_error("Bad number argument: " << parsed_line[8] << ".");
	  penalty=1.0;
	}
      }
#endif

#if 0
#ifdef FIX_PLINK_ENDPOINTS
      bool fixends;
#ifdef FIX_PLINKS_DEFAULT
      fixends = true;
#else
      fixends = false;
#endif
      if (parsed_line.size() == 10) {
	  if (parsed_line[9] == "fixends") {
	      fixends = true;
	  }
      }
#else
      if (parsed_line.size() > 9) {
	ptop_error("Bad link line, too many arguments.");
      }
#endif
#endif

      fstring name = parsed_line[1];
      string ssrc,ssrcmac;
      split_two(parsed_line[2],':',ssrc,ssrcmac,"(null)");
      fstring src = ssrc;
      fstring srcmac = ssrcmac;
      string sdst,sdstmac;
      split_two(parsed_line[3],':',sdst,sdstmac,"(null)");
      fstring dst(sdst), dstmac(sdstmac);
      string bw = parsed_line[4];
      string delay = parsed_line[5];
      string loss = parsed_line[6];
      fstring link_type = parsed_line[7];
      int ibw,idelay;
      double gloss;

      
      if ((sscanf(bw.c_str(),"%d",&ibw) != 1) ||
	  (sscanf(delay.c_str(),"%d",&idelay) != 1) ||
	  (sscanf(loss.c_str(),"%lg",&gloss) != 1)) {
	ptop_error("Bad link line, bad delay characteristics.");
      }

#define ISSWITCH(n) (n->types.find("switch") != n->types.end())
      // Check to make sure the nodes in the link actually exist
      if (pname2vertex.find(src) == pname2vertex.end()) {
	  ptop_error("Bad link line, non-existent src node " << src);
	  continue;
      }
      if (pname2vertex.find(dst) == pname2vertex.end()) {
	  ptop_error("Bad link line, non-existent dst node " << dst);
	  continue;
      }

      pvertex srcv = pname2vertex[src];
      pvertex dstv = pname2vertex[dst];
      tb_pnode *srcnode = get(pvertex_pmap,srcv);
      tb_pnode *dstnode = get(pvertex_pmap,dstv);
      
      for (int cur = 0;cur<num;++cur) {
	pedge pe = (add_edge(srcv,dstv,PG)).first;
	tb_plink *pl = new
	    tb_plink(name,tb_plink::PLINK_NORMAL,link_type,srcmac,dstmac);
	put(pedge_pmap,pe,pl);
	pl->delay_info.bandwidth = ibw;
	pl->delay_info.delay = idelay;
	pl->delay_info.loss = gloss;
#if 0
#ifdef FIX_PLINK_ENDPOINTS
	pl->fixends = fixends;
#endif
#endif
#ifdef PENALIZE_BANDWIDTH
	pl->penalty = penalty;
#endif
	if (ISSWITCH(srcnode) && ISSWITCH(dstnode)) {
	  if (cur != 0) {
	    cout <<
	      "Warning: Extra links between switches will be ignored. (" <<
	      name << ")" << endl;
	  } else {
	    svertex src_switch = get(pvertex_pmap,srcv)->sgraph_switch;
	    svertex dst_switch = get(pvertex_pmap,dstv)->sgraph_switch;
	    sedge swedge = add_edge(src_switch,dst_switch,SG).first;
	    tb_slink *sl = new tb_slink();
	    put(sedge_pmap,swedge,sl);
	    sl->mate = pe;
	    pl->is_type = tb_plink::PLINK_INTERSWITCH;
	  }
	}
	srcnode->total_interfaces++;
	dstnode->total_interfaces++;
	srcnode->link_counts[link_type]++;
	dstnode->link_counts[link_type]++;
	for (int i = 8; i < parsed_line.size(); i++) {
	  fstring link_type = parsed_line[i];
	  pl->types.insert(link_type);
	  srcnode->link_counts[link_type]++;
	  dstnode->link_counts[link_type]++;
	}
	if (ISSWITCH(srcnode) &&
	    ! ISSWITCH(dstnode)) {
	  dstnode->switches.insert(srcv);
#ifdef PER_VNODE_TT
	  dstnode->total_bandwidth += ibw;
#endif
	}
	else if (ISSWITCH(dstnode) &&
		 ! ISSWITCH(srcnode)) {
	  srcnode->switches.insert(dstv);
#ifdef PER_VNODE_TT
	  srcnode->total_bandwidth += ibw;
#endif
	}
      }
    } else if (command == "set-type-limit") {
      if (parsed_line.size() != 3) {
	ptop_error("Bad set-type-limit line, requires two arguments.");
      }
      fstring type = parsed_line[1];
      int max;
      if (sscanf(parsed_line[2].c_str(),"%u",&max) != 1) {
	  ptop_error("Bad number argument: " << parsed_line[2] << ".");
      }

      // Look for a record for this ptype - create it if it doesn't exist
      if (ptypes.find(type) == ptypes.end()) {
	  ptypes[type] = new tb_ptype(type);
      }

      ptypes[type]->set_max_users(max);

    } else {
      ptop_error("Unknown directive: " << command << ".");
    }

// Used to do late binding of subnode names to pnodes, so that we're no
// dependant on their ordering in the ptop file, which can be annoying to get
// right.
// Returns the number of errors found
int xml_bind_ptop_subnodes() {
    int errors = 0;

    // Iterate through all pnodes looking for ones that are subnodes
    pvertex_iterator vit,vendit;
    tie(vit,vendit) = vertices(PG);
    for (;vit != vendit;++vit) {
	tb_pnode *pnode = get(pvertex_pmap,*vit);
	if (!pnode->subnode_of_name.empty()) {
	    if (pname2vertex.find(pnode->subnode_of_name) ==
		    pname2vertex.end()) {
		ptop_error_noline(pnode->name << " is a subnode of a " <<
			"non-existent node, " << pnode->subnode_of_name << ".");
			continue;
		}
		pvertex parent_pv = pname2vertex[pnode->subnode_of_name];
		pnode->subnode_of = get(pvertex_pmap,parent_pv);
		pnode->subnode_of->has_subnode = true;
	}
    }

    return errors;
}
#endif

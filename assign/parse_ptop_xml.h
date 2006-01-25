#ifndef __PARSE_PTOP_XML_H
#define __PARSE_PTOP_XML_h

#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
XERCES_CPP_NAMESPACE_USE

#include "physical.h"
#include "xmlhelpers.h"

int parse_ptop_xml(tb_pgraph &PG, tb_sgraph &SG, char *filename);
    
class PtopParserHandlers : public DefaultHandler {
    public:
        /*
         * Constructors, destructors
         */
        PtopParserHandlers(tb_pgraph &_PG, tb_sgraph &_SG);
        ~PtopParserHandlers();

        /*
         * Functions that allow us to be a SAX ContentHandler
         */
        void startElement(const XMLCh* const uri, const XMLCh* const localname,
                const XMLCh* const qname, const Attributes& attrs);
	void endElement(const XMLCh* const uri, const XMLCh* const localname,
                const XMLCh* const qname);	
        //void characters(const XMLCh* const chars, const unsigned int length);
        //void resetDocument();

        /*
         * Functions that allow us to be a SAX ErrorHandler
         */
	void warning(const SAXParseException& exc) {;}
        void error(const SAXParseException& exc);
        void fatalError(const SAXParseException& exc);
	void resetErrors() {;}
	
	/*
	 * Helpers for my own information
	 */
	unsigned int count_pnodes() const;
    private:
	/*
	 * Parser state
	 */
	
	// Information about the currently-parsed element, from the startElement
	// and endElement handlers
	// TODO: about memory management! May have memory leaks.
	XStr current_uri;
	XStr current_localname;
        XStr current_qname;
	    
	// State pertaining to the current physical node
        tb_pnode *current_pnode;
	
	// State pertaining to the current physical nodes' type
	tb_ptype *current_ptype;
	unsigned int slots;
	bool is_static;
	
	unsigned int pnode_count;
	
        tb_pgraph PG;
        tb_sgraph SG;

	// Element handling functions
        void startNode(const Attributes& attrs);
	void endNode();
	void startNodeType(const Attributes& attrs);
	void endNodeType();
	void handleLimit(const Attributes& attrs);
	void handleUnlimited(const Attributes& attrs);
	void handleStatic(const Attributes& attrs);

};

#endif

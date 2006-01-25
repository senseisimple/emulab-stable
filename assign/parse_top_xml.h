#ifndef __PARSE_TOP_XML_H
#define __PARSE_TOP_XML_H

#include <xercesc/util/PlatformUtils.hpp>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMWriter.hpp>

#include <xercesc/framework/StdOutFormatTarget.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLUni.hpp>

#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>

#include <xercesc/util/OutOfMemoryException.hpp>

#include <xercesc/sax2/XMLReaderFactory.hpp>
XERCES_CPP_NAMESPACE_USE

/*
 * Our error reporter - this gets called if the parser runs into any errors
 * while inside the parse() function.
 */

class ParsePtopErrorHandler : public ErrorHandler {
    public:
    ParsePtopErrorHandler() : hadError(false) { ; };
    ~ParsePtopErrorHandler() { ; };
    
    /*
     * Implementation of the ErrorHandler functions
     */
    void warning(const SAXParseException& toCatch) { /* Ignore for now */ };
    void error(const SAXParseException& toCatch);
    void fatalError(const SAXParseException& toCatch);
    void resetErrors() { hadError = false; }
    
    bool sawError() const { return hadError; }
    
    private:
    bool hadError;
    
};

int parse_top_xml(tb_vgraph &VG, char* filename);


#endif

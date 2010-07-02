/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Implements the annotate methods which are independent of the type of file being annotated.
 */

static const char rcsid[] = "$Id: annotate.cc,v 1.2 2009-05-20 18:06:07 tarunp Exp $";

#ifdef WITH_XML

#include "annotate.h"

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMWriter.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>

XERCES_CPP_NAMESPACE_USE
using namespace std;

void annotate::write_annotated_file (const char* filename)
{
	// Get the current implementation
	DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(NULL);
		
	// Construct the DOMWriter
	DOMWriter* writer = ((DOMImplementationLS*)impl)->createDOMWriter();
		
	// Make the output look pretty
	if (writer->canSetFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true))
		writer->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true);
		
	// Set the byte-order-mark feature      
	if (writer->canSetFeature(XMLUni::fgDOMWRTBOM, true))
		writer->setFeature(XMLUni::fgDOMWRTBOM, true);
		
	// Construct the LocalFileFormatTarget
	XMLFormatTarget *outputFile = new xercesc::LocalFileFormatTarget(filename);
		
	// Serialize a DOMNode to the local file "<some-file-name>.xml"
	writer->writeNode(outputFile, *dynamic_cast<DOMNode*>(this->virtual_root));
		
	// Flush the buffer to ensure all contents are written
	outputFile->flush();
		
	// Release the memory
	writer->release();
}

#endif

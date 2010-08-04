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
#include "common.h"
#include "xstr.h"
#include <string>

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
  if (!impl) { 
    cout << "*** Could not create DOMImplementationRegistry" << endl;
    exit(EXIT_FATAL);
  }

  // Construct the DOMWriter
  DOMWriter* writer = ((DOMImplementationLS*)impl)->createDOMWriter();
  if (!writer) {
    cout << "*** Could not create DOMWriter" << endl;
    exit(EXIT_FATAL);
  }
  
  // Make the output look pretty
  if (writer->canSetFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true))
    writer->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true);
  
  // Set the byte-order-mark feature      
  if (writer->canSetFeature(XMLUni::fgDOMWRTBOM, true))
    writer->setFeature(XMLUni::fgDOMWRTBOM, true);

  // Set the XML declaration feature
  if (writer->canSetFeature(XMLUni::fgDOMXMLDeclaration, true))
    writer->setFeature(XMLUni::fgDOMXMLDeclaration, true);

  // Construct the LocalFileFormatTarget
  XMLFormatTarget *outputFile = new xercesc::LocalFileFormatTarget(filename);
  if (!outputFile) {
    cout << "*** Could not create output file " << filename << endl;
    exit(EXIT_FATAL);
  }
  
  // Serialize a DOMNode to the local file "<some-file-name>.xml"
  writer->writeNode(outputFile, *dynamic_cast<DOMNode*>(this->document));
  
  // Flush the buffer to ensure all contents are written
  outputFile->flush();
  
  // Release the memory
  writer->release();
}

string annotate::printXML (const DOMElement* input)
{
  DOMElement* tag = (DOMElement*)(input);

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

  // Serialize a DOMNode to the local file "<some-file-name>.xml"
  string rv = XStr(writer->writeToString(*dynamic_cast<DOMNode*>(tag))).c();
  
  // Release the memory
  writer->release();

  return rv;
}


#endif

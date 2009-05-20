/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008-2009 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Parsing for the (experimental) policy XML format
 */

#ifdef WITH_XML

#ifndef __PARSE_POLICY_XML_H
#define __PARSE_POLICY_XML_H

#include "physical.h"
#include "port.h"

#include "xmlhelpers.h"
#include "xstr.h"

//#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax/HandlerBase.hpp>
XERCES_CPP_NAMESPACE_USE

int parse_policy_xml(char *filename);

#endif // for __PARSE_POLICY_XML

#endif // for WITH_XML

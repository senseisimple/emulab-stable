/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Parsing for the (experimental) ptop XML format
 */

#ifdef WITH_XML

#ifndef __PARSE_REQUEST_RSPEC_H
#define __PARSE_REQUEST_RSPEC_H

#include "physical.h"
#include "virtual.h"
#include "port.h"

#include "xmlhelpers.h"
#include "xstr.h"

//#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax/HandlerBase.hpp>

int parse_request(tb_vgraph &VG, char *filename);

#endif // for __PARSE_REQUEST_RSPEC_H

#endif // for WITH_XML

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * XML Parser for policy files
 */

static const char rcsid[] = "$Id: parse_policy_xml.cc,v 1.2 2009-05-20 18:06:08 tarunp Exp $";

#ifdef WITH_XML

#include "parse_policy_xml.h"
#include "xmlhelpers.h"
#include "parse_error_handler.h"

#define XMLDEBUG(x) (cerr << x)

bool populate_type_limits (DOMElement *root);
bool populate_desire_limits (DOMElement *root);

int parse_policy_xml(char *filename) 
{
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
	* Must validate against the policy schema
	*/		
	parser -> setExternalSchemaLocation ("http://emulab.net/resources/ptop/0.1 policy.xsd");

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

		/* These calls extract the policies from the policy document */
		XMLDEBUG("starting type limit population" << endl);
		if (!populate_type_limits(root)) {
			cerr << "Error reading type limits from policy " << filename
					<< endl;
			exit(EXIT_FATAL);
		}
		XMLDEBUG("finishing type limit population" << endl);

		XMLDEBUG("starting desire limit population" << endl);
		if (!populate_desire_limits(root)) {
			cerr << "Error reading desire limits from policy " << filename
					<< endl;
			exit(EXIT_FATAL);
		}
		XMLDEBUG("finishing desire limit processing" << endl);

		cerr << "Policy parsing finished" << endl; 
	}

	/*
	 * All done, clean up memory
	 */
	XMLPlatformUtils::Terminate();
	return 0;
}

/*
 * Get the type limits from the document
 */
bool populate_type_limits(DOMElement *root) 
{
	/*
	* Get a list of all type_limits in this document
	*/
	DOMNodeList *type_limits = root->getElementsByTagName(XStr("type_limit").x());
	int typelimitCount = type_limits->getLength();
	
	for (size_t i = 0; i < typelimitCount; i++) 
	{
		DOMNode *type_limit = type_limits->item(i);
		// This should not be able to fail, due to the fact that all elements in
		// this list came from the getElementsByTagName() call
		DOMElement *elt = dynamic_cast<DOMElement*>(type_limit);
		
		XStr type_name (getChildValue (elt, "type_name"));
		XStr type_limit_value (getChildValue (elt, "type_limit"));
		
		const char * str_type_name = type_name.c();
		// Look for a record for this ptype - create it if it doesn't exist
		if (ptypes.find(str_type_name) == ptypes.end()) 
		{
			ptypes[str_type_name] = new tb_ptype(str_type_name);
		}
		ptypes[str_type_name]->set_max_users(type_limit_value.i());
	}
	return true;	
}

/*
 * Get the desire limits from the document
 */
bool populate_desire_limits(DOMElement *root) 
{
	bool is_ok = true;

	DOMNodeList *desire_limits = root->getElementsByTagName(XStr("desire_policy").x());
	int desirelimitCount = desire_limits->getLength();
	
	for (size_t i = 0; i < desirelimitCount; i++) 
	{
		DOMNode *desire = desire_limits->item(i);
		DOMElement *elt = dynamic_cast<DOMElement*>(desire);

		XStr desire_name (getChildValue (elt, "desire_name"));
		tb_featuredesire *fd_obj = tb_featuredesire::get_featuredesire_by_name(desire_name.c());
		if (fd_obj == NULL)
		{
			cerr << "Desire not found : " << desire_name << endl;
			is_ok = false;
			continue;
		}
		else 
		{
			if (hasChildTag (elt, "disallow"))
				fd_obj->disallow_desire ();
			else
			{
				XStr limit (getChildValue (elt, "limit"));
				fd_obj -> limit_desire(limit.d());
			}
		}
	}
	return is_ok;
}

#endif

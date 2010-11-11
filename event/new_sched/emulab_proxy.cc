/*
 * emulab_proxy.cpp
 *
 * Copyright (c) 2004 The University of Utah and the Flux Group.
 * All rights reserved.
 *
 * This file is licensed under the terms of the GNU Public License.  
 * See the file "license.terms" for restrictions on redistribution 
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/client.hpp>

#include <assert.h>

#include <xmlrpc-c/girerr.hpp>
#include <map>
#include <iostream>

#include "emulab_proxy.h"

using namespace emulab;

/**
 * The Emulab XML-RPC protocol version number.
 */
static const double PROTOCOL_VERSION = 0.1;

EmulabResponse::EmulabResponse(const er_code_t code,
			       const xmlrpc_c::value value,
			       const xmlrpc_c::value output)
    : er_Code(code), er_Value(value), er_Output(output)
{
}

EmulabResponse::EmulabResponse(xmlrpc_c::value result)
{
	std::map<std::string, xmlrpc_c::value> result_map;
	std::map<std::string, xmlrpc_c::value>::iterator p;

	if (!result.type() == xmlrpc_c::value::TYPE_STRUCT)
	{
		throw girerr::error("Invalid response from server");
	}

	result_map = (xmlrpc_c::value_struct)result;

	/* Make sure the result has what we expect before */
	p = result_map.find("code");
	if (p == result_map.end()) {
		throw girerr::error("Invalid response from server");
	}

	/* ... decoding its contents. */
	this->er_Code = (er_code_t)(int)(xmlrpc_c::value_int)result_map["code"];
	this->er_Value = result_map["value"];
	this->er_Output = result_map["output"];
}

EmulabResponse::~EmulabResponse()
{
}


ServerProxy::ServerProxy(xmlrpc_c::clientXmlTransportPtr transport,
			 bool wbxml_mode,
			 const char *url)
{
	this->transport = transport;
	this->server_url = url;
}

ServerProxy::~ServerProxy()
{
}

xmlrpc_c::value ServerProxy::call(xmlrpc_c::rpcPtr rpc)
{
	xmlrpc_c::client_xml client(this->transport);
	xmlrpc_c::carriageParm_curl0 carriage_parm(this->server_url);

	rpc->call(&client, &carriage_parm);

	assert(rpc->isFinished());

	/* This will throw an exception if the RPC call failed */
	return rpc->getResult();
}

EmulabResponse ServerProxy::invoke(const char *method_name,
				   spa_attr_t tag,
				   ...)
{
    va_list args;

    va_start(args, tag);
    EmulabResponse retval = this->invoke(method_name, tag, args);
    va_end(args);

    return( retval );
}

EmulabResponse ServerProxy::invoke(const char *method_name,
				   spa_attr_t tag,
				   va_list args)
{
    xmlrpc_c::paramList params;
    std::map <std::string, xmlrpc_c::value> struct_data;
    
    /* Iterate over the tag list to build the argument structure, */
    while( tag != SPA_TAG_DONE )
    {
	    std::string key;

	    key = std::string(va_arg(args, const char *));
	    switch( tag )
	    {
	    case SPA_Boolean:
		    struct_data[key] =
			    xmlrpc_c::value_boolean((bool)va_arg(args, int));
		    break;
	    case SPA_Integer:
		    struct_data[key] =
			    xmlrpc_c::value_int(va_arg(args, int));
		    break;
	    case SPA_Double:
		    struct_data[key] =
			    xmlrpc_c::value_double(va_arg(args, double));
		break;
	    case SPA_String:
		    struct_data[key] =
			    xmlrpc_c::value_string(
				    std::string(va_arg(args, const char *)));
		break;
	    default:
		    break;
	    }

	    tag = (spa_attr_t)va_arg(args, int);
    }
    
    /* ... add the parameters, and */
    params.add(xmlrpc_c::value_double(PROTOCOL_VERSION));
    params.add(xmlrpc_c::value_struct(struct_data));
    xmlrpc_c::rpcPtr rpc(std::string(method_name), params);
    
    /* ... call the method. */
    return EmulabResponse(this->call(rpc));
}

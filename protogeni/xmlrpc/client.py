#! /usr/bin/env python
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004, 2008 University of Utah and the Flux Group.
# All rights reserved.
# 
# Permission to use, copy, modify and distribute this software is hereby
# granted provided that (1) source code retains these copyright, permission,
# and disclaimer notices, and (2) redistributions including binaries
# reproduce the notices in supporting documentation.
#
# THE UNIVERSITY OF UTAH ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
# CONDITION.  THE UNIVERSITY OF UTAH DISCLAIMS ANY LIABILITY OF ANY KIND
# FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
#

#
#
import sys
import pwd
import getopt
import os
import re
import xmlrpclib

# Default server
XMLRPC_SERVER   = "boss"
SERVER_PATH     = ":443/protogeni/xmlrpc/"

# Path to my certificate
CERTIFICATE     = "/users/stoller/.ssl/encrypted.pem"
PASSPHRASEFILE  = "/users/stoller/.ssl/password"
passphrase      = ""

# Debugging output.
debug           = 1
impotent        = 0


def Fatal(message):
    print message
    sys.exit(1)
    return

def PassPhraseCB(v, prompt1='Enter passphrase:', prompt2='Verify passphrase:'):
    return passphrase

#
# Process a single command line
#
def do_method(module, method, params):
    if debug:
        print module + " " + method + " " + str(params);
        pass
    if impotent:
        return 0;

    if not os.path.exists(CERTIFICATE):
        return Fatal("error: missing emulab certificate: %s\n" % CERTIFICATE)
    
    from M2Crypto.m2xmlrpclib import SSL_Transport
    from M2Crypto import SSL
    
    URI = "https://" + XMLRPC_SERVER + SERVER_PATH + module
    
    ctx = SSL.Context("sslv23")
    ctx.load_cert(CERTIFICATE, CERTIFICATE, PassPhraseCB)
    ctx.set_verify(SSL.verify_none, 16)
    ctx.set_allow_unknown_ca(0)
    
    # Get a handle on the server,
    server = xmlrpclib.ServerProxy(URI, SSL_Transport(ctx), verbose=0)
        
    # Get a pointer to the function we want to invoke.
    meth      = getattr(server, method)
    meth_args = [ params ]

    #
    # Make the call. 
    #
    try:
        response = apply(meth, meth_args)
        pass
    except xmlrpclib.Fault, e:
        print e.faultString
        return (-1, None)

    #
    # Parse the Response, which is a Dictionary. See EmulabResponse in the
    # emulabclient.py module. The XML standard converts classes to a plain
    # Dictionary, hence the code below. 
    # 
    if len(response["output"]):
        print response["output"],
        pass

    rval = response["code"]

    #
    # If the code indicates failure, look for a "value". Use that as the
    # return value instead of the code. 
    # 
    if rval:
        if response["value"]:
            rval = response["value"]
            pass
        pass
    return (rval, response)

#
# Read my passphrase from a file to avoid typing it over and over.
#
passphrase = open(PASSPHRASEFILE).readline()
passphrase = passphrase.strip()

#
# Get a credential for myself, that allows me to do things at the SA.
#
params = {}
params["uuid"] = "0b2eb97e-ed30-11db-96cb-001143e453fe"
rval,response = do_method("sa", "GetCredential", params)
if rval:
    Fatal("Could not get my credential")
    pass
mycredential = response["value"]

#
# Look me up just for the hell of it. I can see why the hrn is "useful"
#
params = {}
params["uuid"]       = "7450199a-b6eb-102b-a5ad-001143e43770"
params["credential"] = mycredential
params["type"]       = "User"
rval,response = do_method("sa", "Resolve", params)
if rval:
    Fatal("Could not resolve myself")
    pass

#
# Lookup slice, delete before proceeding.
#
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = "myslice1"
rval,response = do_method("sa", "Resolve", params)
if rval == 0:
    myslice = response["value"]
    myuuid  = myslice["uuid"]

    params = {}
    params["credential"] = mycredential
    params["type"]       = "Slice"
    params["uuid"]       = myuuid
    rval,response = do_method("sa", "Remove", params)
    if rval:
        Fatal("Could not remove slice record")
        pass
    pass

#
# Create a slice. 
#
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = "myslice1"
rval,response = do_method("sa", "Register", params)
if rval:
    Fatal("Could not get my slice")
    pass
myslice = response["value"]

#
# Send a stub rspec over to the SA and let it decide what to do.
#
rspec = "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> " +\
        " <node uuid=\"00000000-0000-0000-0000-000000000002\" " +\
        "       virtualization_type=\"raw\"> " +\
        " </node>" +\
        "</rspec>"
params = {}
params["credential"] = myslice
params["rspec"]      = rspec
rval,response = do_method("sa", "DiscoverResources", params)
if rval:
    Fatal("Could not do resource discovery")
    pass
nspec = response["value"]
print str(nspec)



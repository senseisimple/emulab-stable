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
# Got tired of typing this over and over so I stuck it in a file.
PASSPHRASEFILE  = "/users/stoller/.ssl/password"
passphrase      = ""

# Debugging output.
debug           = 0
impotent        = 0

def Fatal(message):
    print message
    sys.exit(1)
    return

def PassPhraseCB(v, prompt1='Enter passphrase:', prompt2='Verify passphrase:'):
    passphrase = open(PASSPHRASEFILE).readline()
    passphrase = passphrase.strip()
    return passphrase

#
# Call the rpc server.
#
def do_method(module, method, params, URI=None):
    if debug:
        print module + " " + method + " " + str(params);
        pass

    if not os.path.exists(CERTIFICATE):
        return Fatal("error: missing emulab certificate: %s\n" % CERTIFICATE)
    
    from M2Crypto.m2xmlrpclib import SSL_Transport
    from M2Crypto import SSL

    if URI == None:
        URI = "https://" + XMLRPC_SERVER + SERVER_PATH + module
    else:
        URI = URI + "/" + module
        pass
    
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
# Get a credential for myself, that allows me to do things at the SA.
#
params = {}
params["uuid"] = "0b2eb97e-ed30-11db-96cb-001143e453fe"
rval,response = do_method("sa", "GetCredential", params)
if rval:
    Fatal("Could not get my credential")
    pass
mycredential = response["value"]
print "Got my SA credential"
#print str(mycredential);
#sys.exit(0);

#
# Look me up just for the hell of it. I can see why the hrn is "useful"
#
params = {}
#params["uuid"]       = "7450199a-b6eb-102b-a5ad-001143e43770"
params["hrn"]       = "stoller"
params["credential"] = mycredential
params["type"]       = "User"
rval,response = do_method("sa", "Resolve", params)
if rval:
    Fatal("Could not resolve myself")
    pass
print "Found my record at the SA"
#print str(response)

#
# Look up leebee alter ego so I can bind him to the slice.
#
params = {}
params["hrn"]       = "leebee"
params["credential"] = mycredential
params["type"]       = "User"
rval,response = do_method("sa", "Resolve", params)
if rval:
    Fatal("Could not resolve leebee")
    pass
print "Found leebee's record at the SA"
leebee = response["value"]

#
# Lookup a node at the component. 
#
params = {}
params["credential"] = mycredential;
params["hrn"]        = "pc41";
params["type"]       = "Node";
rval,response = do_method("cm", "Resolve", params,
         URI="https://myboss.myelab.testbed.emulab.net:443/protogeni/xmlrpc")
if rval:
    Fatal("Could not lookup info for pc41")
    pass
#print str(response["value"]);
print "Found pc41's record at the CM"

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

    print "Deleting previous slice called myslice1";
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
print "Creating new slice called myslice1";
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = "myslice1"
params["userbindings"] = (leebee["uuid"],)
rval,response = do_method("sa", "Register", params)
if rval:
    Fatal("Could not get my slice")
    pass
myslice = response["value"]
print "New slice created"

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
print "Bogus DiscoverResources returned"

#
# Okay, we do not actually have anything like resource discovery yet,
# so lets fake it.
#
rspec = "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> " +\
        " <node uuid=\"de9803c2-773e-102b-8eb4-001143e453fe\" " +\
        "       hrn=\"geni1\" "+\
        "       virtualization_type=\"emulab-vnode\"> " +\
        " </node>" +\
        " <node uuid=\"de995217-773e-102b-8eb4-001143e453fe\" " +\
        "       virtualization_type=\"emulab-vnode\"> " +\
        " </node>" +\
        " <link name=\"link0\" link_name=\"link0\"> " +\
        "  <LinkEndPoints name=\"destination_interface\" " +\
        "            iface_name=\"eth0\" " +\
        "            node_uuid=\"de9803c2-773e-102b-8eb4-001143e453fe\" /> " +\
        "  <LinkEndPoints name=\"source_interface\" " +\
        "            iface_name=\"eth0\" " +\
        "            node_uuid=\"de995217-773e-102b-8eb4-001143e453fe\" /> " +\
        " </link> " +\
        "</rspec>"
params = {}
params["credential"] = myslice
params["rspec"]      = rspec
params["impotent"]   = impotent
rval,response = do_method("cm", "GetTicket", params,
         URI="https://myboss.myelab.testbed.emulab.net:443/protogeni/xmlrpc")
if rval:
    Fatal("Could not get ticket")
    pass
ticket = response["value"]
print "Got a ticket from the CM"
#print str(ticket)

#
# Create the sliver.
#
params = {}
params["ticket"]   = ticket
params["impotent"] = impotent
rval,response = do_method("cm", "RedeemTicket", params,
         URI="https://myboss.myelab.testbed.emulab.net:443/protogeni/xmlrpc")
if rval:
    Fatal("Could not redeem ticket")
    pass
sliver = response["value"]
print "Created a sliver"
print str(sliver)

#
# Split the sliver since its an aggregate of resources
#
params = {}
params["credential"] = sliver
rval,response = do_method("cm", "SplitSliver", params,
         URI="https://myboss.myelab.testbed.emulab.net:443/protogeni/xmlrpc")
if rval:
    Fatal("Could not split sliver")
    pass
slivers = response["value"]
print "Split a sliver"
print str(slivers)

#
# Start the sliver.
#
params = {}
params["credential"] = sliver
params["impotent"]   = impotent
rval,response = do_method("cm", "StartSliver", params,
         URI="https://myboss.myelab.testbed.emulab.net:443/protogeni/xmlrpc")
if rval:
    Fatal("Could not start sliver")
    pass

print "Sliver has been started, waiting for input to delete it"
print "You should be able to log into the sliver after a little bit"
sys.stdin.readline();
print "Deleting sliver now"

#
# Delete the sliver.
#
params = {}
params["credential"] = sliver
params["impotent"]   = impotent
rval,response = do_method("cm", "DeleteSliver", params,
         URI="https://myboss.myelab.testbed.emulab.net:443/protogeni/xmlrpc")
if rval:
    Fatal("Could not stop sliver")
    pass
print "Sliver has been deleted"

#
# Delete the slice.
#
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = "myslice1"
rval,response = do_method("sa", "Remove", params)
if rval:
    Fatal("Could not delete slice")
    pass
pass
print "Slice has been deleted"


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

#
# Lookup slice, delete before proceeding.
#
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = "myslice3"
rval,response = do_method("sa", "Resolve", params)
if rval == 0:
    myslice = response["value"]
    myuuid  = myslice["uuid"]

    print "Deleting previous slice called myslice3";
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
print "Creating new slice called myslice3";
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = "myslice3"
rval,response = do_method("sa", "Register", params)
if rval:
    Fatal("Could not get my slice")
    pass
myslice = response["value"]
print "New slice created"
#print str(myslice);

#
# Get a ticket for a node on Emulab.
#
rspec1 = "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> " +\
        " <node uuid=\"de9a0b2e-773e-102b-8eb4-001143e453fe\" " +\
        "       nickname=\"geni1\" "+\
        "       virtualization_type=\"emulab-vnode\"> " +\
        " </node>" +\
        " <link name=\"link0\" nickname=\"link0\" link_type=\"tunnel\"> " +\
        "  <linkendpoints nickname=\"destination_interface\" " +\
        "            tunnel_ip=\"192.168.1.1\" " +\
        "            node_uuid=\"de9a0b2e-773e-102b-8eb4-001143e453fe\" /> " +\
        "  <linkendpoints nickname=\"source_interface\" " +\
        "            tunnel_ip=\"192.168.1.2\" " +\
        "            node_uuid=\"de995217-773e-102b-8eb4-001143e453fe\" /> " +\
        " </link> " +\
        "</rspec>"

params = {}
params["credential"] = myslice
params["rspec"]      = rspec1
params["impotent"]   = impotent
rval,response = do_method("cm", "GetTicket", params,
         URI="https://boss.emulab.net:443/protogeni/xmlrpc")
if rval:
    Fatal("Could not get ticket")
    pass
ticket1 = response["value"]
print "Got a ticket from the CM on Emulab"
print str(ticket)

#
# Get a ticket for a node on Inner Emulab.
#
rspec2 = "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> " +\
        " <node uuid=\"de9803c2-773e-102b-8eb4-001143e453fe\" " +\
        "       nickname=\"geni2\" "+\
        "       virtualization_type=\"emulab-vnode\"> " +\
        " </node>" +\
        " <link name=\"link0\" nickname=\"link0\" link_type=\"tunnel\"> " +\
        "  <linkendpoints nickname=\"destination_interface\" " +\
        "            tunnel_ip=\"192.168.1.2\" " +\
        "            node_uuid=\"de9803c2-773e-102b-8eb4-001143e453fe\" /> " +\
        "  <linkendpoints nickname=\"source_interface\" " +\
        "            tunnel_ip=\"192.168.1.1\" " +\
        "            node_uuid=\"de9a0b2e-773e-102b-8eb4-001143e453fe\" /> " +\
        " </link> " +\
        "</rspec>"

params = {}
params["credential"] = myslice
params["rspec"]      = rspec2
params["impotent"]   = impotent
rval,response = do_method("cm", "GetTicket", params,
         URI="https://myboss.myelab.testbed.emulab.net:443/protogeni/xmlrpc")
if rval:
    Fatal("Could not get ticket")
    pass
ticket2 = response["value"]
print "Got a ticket from the CM on Inner Emulab"
#print str(ticket2)

#
# Create the slivers.
#
params = {}
params["ticket"]   = ticket2
params["impotent"] = impotent
rval,response = do_method("cm", "RedeemTicket", params,
         URI="https://myboss.myelab.testbed.emulab.net:443/protogeni/xmlrpc")
if rval:
    Fatal("Could not redeem ticket on inner emulab")
    pass
sliver2 = response["value"]
print "Created a sliver on Inner Emulab"
sys.exit(0);

params = {}
params["ticket"]   = ticket1
params["impotent"] = impotent
rval,response = do_method("cm", "RedeemTicket", params,
         URI="https://boss.emulab.net:443/protogeni/xmlrpc")
if rval:
    Fatal("Could not redeem ticket on emulab")
    pass
sliver1 = response["value"]
print "Created a sliver on Emulab"
print str(sliver1)

print "Sliver has been started, waiting for input to delete it"
print "You should be able to log into the sliver after a little bit"
sys.stdin.readline();
print "Deleting sliver now"

#
# Delete the slice.
#
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = "myslice3"
rval,response = do_method("sa", "Remove", params)
if rval:
    Fatal("Could not delete slice")
    pass
pass
print "Slice has been deleted"


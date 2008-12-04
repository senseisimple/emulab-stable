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
import urllib
from xml.sax.handler import ContentHandler
import xml.sax
import string

# Default server
XMLRPC_SERVER   = "boss"
SERVER_PATH     = ":443/protogeni/xmlrpc"
HOME            = os.environ["HOME"]

# Path to my certificate
CERTIFICATE     = HOME + "/.ssl/encrypted.pem"
# Got tired of typing this over and over so I stuck it in a file.
PASSPHRASEFILE  = HOME + "/.ssl/password"
passphrase      = ""

# Debugging output.
debug           = 0

def Fatal(message):
    print message
    sys.exit(1)
    return

def PassPhraseCB(v, prompt1='Enter passphrase:', prompt2='Verify passphrase:'):
    passphrase = open(PASSPHRASEFILE).readline()
    passphrase = passphrase.strip()
    return passphrase

class findElement(ContentHandler):
    name       = None
    value      = None
    string     = None
    attributes = None
    data       = None
    
    def __init__(self, name, stuff):
        self.name = name
        xml.sax.parseString(stuff, self)
        pass
    def startElement(self, name, attrs):
        if self.name == name:
            self.data = []
            self.attributes = attrs
        elif self.data != None:
            self.data.append("<" + name + ">")
            pass
        pass
    def characters(self, content):
        if self.data != None:
            self.data.append(content)
            pass
        pass
    def endElement(self, name):
        if self.name == name:
            self.value  = string.join(self.data, "");
            self.string = "<" + name + ">" + self.value + "</" + name + ">"
            self.data   = None;
        elif self.data != None:
            self.data.append("</" + name + ">")
            pass
        pass
    pass

#
# Call the rpc server.
#
def do_method(module, method, params, URI=None):
    if not os.path.exists(CERTIFICATE):
        return Fatal("error: missing emulab certificate: %s\n" % CERTIFICATE)
    
    from M2Crypto.m2xmlrpclib import SSL_Transport
    from M2Crypto import SSL

    if URI == None:
        URI = "https://" + XMLRPC_SERVER + SERVER_PATH
        pass
    else:
        # This is silly; bug down in M2crypto. https should imply port.
        type,string = urllib.splittype(URI)
        hostport,path = urllib.splithost(string)
        host,port = urllib.splitport(hostport)
        if port == None:
            URI = type + "://" + host + ":443/" + path
        pass
    if module != None:
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
# Lookup slice.
#
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = "mytestslice"
rval,response = do_method("sa", "Resolve", params)
if rval:
    #
    # Create a slice. 
    #
    print "Creating new slice called mytestslice";
    params = {}
    params["credential"] = mycredential
    params["type"]       = "Slice"
    params["hrn"]        = "mytestslice"
    rval,response = do_method("sa", "Register", params)
    if rval:
        Fatal("Could not create new slice")
        pass
    myslice = response["value"]
    print "New slice created"
    pass
else:
    #
    # Get the slice credential.
    #
    print "Asking for slice credential for mytestslice";
    myslice = response["value"]
    myuuid  = myslice["uuid"]
    params = {}
    params["credential"] = mycredential
    params["type"]       = "Slice"
    params["uuid"]       = myuuid
    rval,response = do_method("sa", "GetCredential", params)
    if rval:
        Fatal("Could not get Slice credential")
        pass
    myslice = response["value"]
    print "Got the slice credential"
    pass

#
# Ask the clearinghouse for a list of component managers. 
#
params = {}
params["credential"] = mycredential
rval,response = do_method("ch", "ListComponents", params,
                          URI="https://boss.emulab.net:443/protogeni/xmlrpc")
if rval:
    Fatal("Could not get a list of components from the ClearingHouse")
    pass
components = response["value"];
url1 = components[0]["url"]
url2 = components[1]["url"]

#
# Get a ticket for a node on a CM.
#
rspec1 = "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> " +\
        " <node uuid=\"*\" " +\
        "       nickname=\"geni1\" "+\
        "       virtualization_type=\"emulab-vnode\"> " +\
        " </node>" +\
        "</rspec>"

params = {}
params["credential"] = myslice
params["rspec"]      = rspec1
rval,response = do_method(None, "GetTicket", params, URI=url1)
if rval:
    Fatal("Could not get ticket")
    pass
ticket1 = response["value"]
print "Got a ticket from CM1"

#
# Get the uuid of the node assigned so we can specify it in the tunnel.
#
ticket_element = findElement("ticket", ticket1)
node_element   = findElement("node", str(ticket_element.string))
uuid_element   = findElement("uuid", str(node_element.string))
node1_uuid     = uuid_element.value

#
# Get a ticket for a node on another CM.
#
rspec2 = "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> " +\
        " <node uuid=\"*\" " +\
        "       nickname=\"geni1\" "+\
        "       virtualization_type=\"emulab-vnode\"> " +\
        " </node>" +\
        "</rspec>"

params = {}
params["credential"] = myslice
params["rspec"]      = rspec2
rval,response = do_method(None, "GetTicket", params, URI=url2)
if rval:
    Fatal("Could not get ticket")
    pass
ticket2 = response["value"]
print "Got a ticket from CM2"

#
# Get the uuid of the node assigned so we can specify it in the tunnel.
#
ticket_element = findElement("ticket", ticket2)
node_element   = findElement("node", str(ticket_element.string))
uuid_element   = findElement("uuid", str(node_element.string))
node2_uuid     = uuid_element.value

#
# Create the slivers.
#
params = {}
params["ticket"]   = ticket1
rval,response = do_method(None, "RedeemTicket", params, url1)
if rval:
    Fatal("Could not redeem ticket on CM1")
    pass
sliver1 = response["value"]
print "Created a sliver CM1"

params = {}
params["ticket"]   = ticket2
rval,response = do_method(None, "RedeemTicket", params, url2)
if rval:
    Fatal("Could not redeem ticket on CM2")
    pass
sliver2 = response["value"]
print "Created a sliver on CM2"

#
# Now add the tunnel part since we have the uuids for the two nodes.
#
rspec1 = "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> " +\
        " <node uuid=\"" + node1_uuid + "\" " +\
        "       nickname=\"geni1\" "+\
        "       virtualization_type=\"emulab-vnode\"> " +\
        " </node>" +\
        " <link name=\"link0\" nickname=\"link0\" link_type=\"tunnel\"> " +\
        "  <linkendpoints nickname=\"destination_interface\" " +\
        "            tunnel_ip=\"192.168.1.1\" " +\
        "            node_nickname=\"geni1\" " +\
        "            node_uuid=\"" + node1_uuid + "\" /> " +\
        "  <linkendpoints nickname=\"source_interface\" " +\
        "            tunnel_ip=\"192.168.1.2\" " +\
        "            node_nickname=\"geni2\" " +\
        "            node_uuid=\"" + node2_uuid + "\" /> " +\
        " </link> " +\
        "</rspec>"

params = {}
params["credential"] = sliver1
params["rspec"]      = rspec1
rval,response = do_method(None, "UpdateSliver", params, url1)
if rval:
    Fatal("Could not update sliver on CM1")
    pass
print "Updated sliver on CM1 with tunnel stuff"

#
# And again for the second sliver.
#
rspec2 = "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> " +\
        " <node uuid=\"" + node2_uuid + "\" " +\
        "       nickname=\"geni2\" "+\
        "       virtualization_type=\"emulab-vnode\"> " +\
        " </node>" +\
        " <link name=\"link0\" nickname=\"link0\" link_type=\"tunnel\"> " +\
        "  <linkendpoints nickname=\"destination_interface\" " +\
        "            tunnel_ip=\"192.168.1.1\" " +\
        "            node_nickname=\"geni1\" " +\
        "            node_uuid=\"" + node1_uuid + "\" /> " +\
        "  <linkendpoints nickname=\"source_interface\" " +\
        "            tunnel_ip=\"192.168.1.2\" " +\
        "            node_nickname=\"geni2\" " +\
        "            node_uuid=\"" + node2_uuid + "\" /> " +\
        " </link> " +\
        "</rspec>"

params = {}
params["credential"] = sliver2
params["rspec"]      = rspec2
rval,response = do_method(None, "UpdateSliver", params, url2)
if rval:
    Fatal("Could not update sliver on CM2")
    pass
print "Updated sliver on CM2 with tunnel stuff"

#
# Start the slivers.
#
params = {}
params["credential"] = sliver1
rval,response = do_method(None, "StartSliver", params, url1)
if rval:
    Fatal("Could not start sliver on CM1")
    pass

params = {}
params["credential"] = sliver2
rval,response = do_method(None, "StartSliver", params, url2)
if rval:
    Fatal("Could not start sliver on CM2")
    pass

print "Slivers have been started, waiting for input to delete it"
print "You should be able to log into the sliver after a little bit"
sys.stdin.readline();

#
# Delete the slivers.
#
print "Deleting sliver1 now"
params = {}
params["credential"] = sliver1
rval,response = do_method(None, "DeleteSliver", params, url1)
if rval:
    Fatal("Could not delete sliver on CM1")
    pass
print "Sliver1 has been deleted"

print "Deleting sliver2 now"
params = {}
params["credential"] = sliver2
rval,response = do_method(None, "DeleteSliver", params, url2)
if rval:
    Fatal("Could not delete sliver on CM2")
    pass
print "Sliver2 has been deleted"


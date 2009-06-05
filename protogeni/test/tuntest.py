#! /usr/bin/env python
#
# GENIPUBLIC-COPYRIGHT
# Copyright (c) 2008-2009 University of Utah and the Flux Group.
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
from M2Crypto import X509

ACCEPTSLICENAME=1

def Usage():
    print "usage: " + sys.argv[ 0 ] + " [option...] \
[component-manager-1 component-manager-2]"
    print """Options:
    -c file, --credentials=file         read self-credentials from file
                                            [default: query from SA]
    -d, --debug                         be verbose about XML methods invoked
    -f file, --certificate=file         read SSL certificate from file
                                            [default: ~/.ssl/encrypted.pem]
    -h, --help                          show options and usage
    -n name, --slicename=name           specify human-readable name of slice
                                            [default: mytestslice]
    -p file, --passphrase=file          read passphrase from file
                                            [default: ~/.ssl/password]
    -r file, --read-commands=file       specify additional configuration file
    -s file, --slicecredentials=file    read slice credentials from file
                                            [default: query from SA]"""

execfile( "test-common.py" )

if len( args ) == 2:
    managers = ( args[ 0 ], args[ 1 ] )
elif len( args ):
    Usage()
    sys.exit( 1 )
else:
    managers = None

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
# Get a credential for myself, that allows me to do things at the SA.
#
mycredential = get_self_credential()
print "Got my SA credential"

#
# Lookup slice.
#
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = SLICENAME
rval,response = do_method("sa", "Resolve", params)
if rval:
    #
    # Create a slice. 
    #
    print "Creating new slice called " + SLICENAME
    params = {}
    params["credential"] = mycredential
    params["type"]       = "Slice"
    params["hrn"]        = SLICENAME
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
    print "Asking for slice credential for " + SLICENAME
    myslice = response["value"]
    myslice = get_slice_credential( myslice, mycredential )
    print "Got the slice credential"
    pass

#
# Ask the clearinghouse for a list of component managers. 
#
params = {}
params["credential"] = mycredential
rval,response = do_method("ch", "ListComponents", params)
if rval:
    Fatal("Could not get a list of components from the ClearingHouse")
    pass
components = response["value"];

if managers:
    def FindCM( name, cmlist ):
        for cm in cmlist:
            hrn = cm[ "hrn" ]
            if hrn == name or hrn == name + ".cm":
                return cm[ "url" ]
        Fatal( "Could not find component manager " + name )

    url1 = FindCM( managers[ 0 ], components )
    url2 = FindCM( managers[ 1 ], components )
else:
    url1 = components[0]["url"]
    url2 = components[1]["url"]

#url1 = "https://boss.emulab.net/protogeni/stoller/xmlrpc/cm"
#url2 = "https://myboss.myelab.testbed.emulab.net/protogeni/xmlrpc/cm"

#
# Get a ticket for a node on a CM.
#
rspec1 = "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> " +\
        " <node virtual_id=\"geni1\" "+\
        "       virtualization_type=\"emulab-vnode\"> " +\
        " </node>" +\
        "</rspec>"

print "Asking for a ticket from CM1 ..."
params = {}
params["credential"] = myslice
params["rspec"]      = rspec1
rval,response = do_method(None, "GetTicket", params, URI=url1)
if rval:
    Fatal("Could not get ticket")
    pass
ticket1 = response["value"]
print "Got a ticket from CM1, asking for a ticket from CM2 ..."

#
# Get the uuid of the node assigned so we can specify it in the tunnel.
#
ticket_element = findElement("ticket", ticket1)
node_element   = findElement("node", str(ticket_element.string))
node1_rspec    = str(node_element.string);

#
# Get a ticket for a node on another CM.
#
rspec2 = "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> " +\
        " <node virtual_id=\"geni2\" "+\
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
print "Got a ticket from CM2, redeeming ticket on CM1 ..."

#
# Get the uuid of the node assigned so we can specify it in the tunnel.
#
ticket_element = findElement("ticket", ticket2)
node_element   = findElement("node", str(ticket_element.string))
node2_rspec    = str(node_element.string);

#
# Create the slivers.
#
params = {}
params["credential"] = myslice
params["ticket"]   = ticket1
rval,response = do_method(None, "RedeemTicket", params, url1)
if rval:
    Fatal("Could not redeem ticket on CM1")
    pass
sliver1,manifest1 = response["value"]
print "Created a sliver on CM1, redeeming ticket on CM2 ..."
print str(manifest1);

params = {}
params["credential"] = myslice
params["ticket"]   = ticket2
rval,response = do_method(None, "RedeemTicket", params, url2)
if rval:
    Fatal("Could not redeem ticket on CM2")
    pass
sliver2,manifest2 = response["value"]
print "Created a sliver on CM2"
print str(manifest2)

#
# Now add the tunnel part since we have the uuids for the two nodes.
#
rspec = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" +\
        "<rspec xmlns=\"http://www.protogeni.net/resources/rspec/0.1\" " +\
        "        type=\"request\"> " + node1_rspec + " " + node2_rspec + " " +\
        " <link virtual_id=\"link0\" link_type=\"tunnel\"> " +\
        "  <interface_ref virtual_node_id=\"geni1\" " +\
        "                 virtual_interface_id=\"virt0\" "+\
        "                 tunnel_ip=\"192.168.1.1\" />" +\
        "  <interface_ref virtual_node_id=\"geni2\" " +\
        "                 virtual_interface_id=\"virt0\" "+\
        "                 tunnel_ip=\"192.168.1.2\" />" +\
        " </link> " +\
        "</rspec>"

#print str(rspec)

print "Updating ticket on CM1 with tunnel stuff ..."
params = {}
params["credential"] = myslice
params["ticket"]     = ticket1
params["rspec"]      = rspec
rval,response = do_method(None, "UpdateTicket", params, url1)
if rval:
    Fatal("Could not update ticket on CM1")
    pass
ticket1 = response["value"]
print "Updated ticket on CM1. Updating ticket on CM2 with tunnel stuff ..."

#
# And again for the second ticket.
#
params = {}
params["credential"] = myslice
params["ticket"]     = ticket2
params["rspec"]      = rspec
rval,response = do_method(None, "UpdateTicket", params, url2)
if rval:
    Fatal("Could not update ticket on CM2")
    pass
ticket2 = response["value"]
print "Updated ticket on CM2. Updating sliver on CM1 with new ticket ..."

#
# Update the slivers with the new tickets, to create the tunnels
#
params = {}
params["credential"] = sliver1
params["ticket"]     = ticket1
rval,response = do_method(None, "UpdateSliver", params, url1)
if rval:
    Fatal("Could not update sliver on CM1")
    pass
manifest1 = response["value"]
print "Updated sliver on CM1. Updating sliver on CM2 with new ticket ..."
#print str(manifest1);

params = {}
params["credential"] = sliver2
params["ticket"]     = ticket2
rval,response = do_method(None, "UpdateSliver", params, url2)
if rval:
    Fatal("Could not start sliver on CM2")
    pass
manifest2 = response["value"]
print "Updated sliver on CM2. Starting sliver on CM1 ..."
#print str(manifest1);

#
# Start the slivers.
#
params = {}
params["credential"] = sliver1
rval,response = do_method(None, "StartSliver", params, url1)
if rval:
    Fatal("Could not start sliver on CM1")
    pass
print "Started sliver on CM1. Starting sliver on CM2 ..."

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


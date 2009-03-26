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

execfile( "test-common.py" )

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
#url1 = components[0]["url"]
#url2 = components[1]["url"]

#url1 = "https://myboss.myelab.testbed.emulab.net/protogeni/xmlrpc/cm"
#url2 = "https://www.emulab.net/protogeni/stoller/xmlrpc/cm"

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
uuid_element   = findElement("sliver_uuid", str(node_element.string))
node1_sliveruuid = uuid_element.value

#
# Get a ticket for a node on another CM.
#
rspec2 = "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> " +\
        " <node uuid=\"*\" " +\
        "       nickname=\"geni2\" "+\
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
uuid_element   = findElement("sliver_uuid", str(node_element.string))
node2_sliveruuid = uuid_element.value

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
        "       sliver_uuid=\"" + node1_sliveruuid + "\" " +\
        "       nickname=\"geni1\" "+\
        "       virtualization_type=\"emulab-vnode\"> " +\
        " </node>" +\
        " <link name=\"link0\" nickname=\"link0\" link_type=\"tunnel\"> " +\
        "  <linkendpoints nickname=\"destination_interface\" " +\
        "            tunnel_ip=\"192.168.1.1\" " +\
        "            node_nickname=\"geni1\" " +\
        "            sliver_uuid=\"" + node1_sliveruuid + "\" " +\
        "            node_uuid=\"" + node1_uuid + "\" /> " +\
        "  <linkendpoints nickname=\"source_interface\" " +\
        "            tunnel_ip=\"192.168.1.2\" " +\
        "            node_nickname=\"geni2\" " +\
        "            sliver_uuid=\"" + node2_sliveruuid + "\" " +\
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
        "       sliver_uuid=\"" + node2_sliveruuid + "\" " +\
        "       nickname=\"geni2\" "+\
        "       virtualization_type=\"emulab-vnode\"> " +\
        " </node>" +\
        " <link name=\"link0\" nickname=\"link0\" link_type=\"tunnel\"> " +\
        "  <linkendpoints nickname=\"destination_interface\" " +\
        "            tunnel_ip=\"192.168.1.1\" " +\
        "            node_nickname=\"geni1\" " +\
        "            sliver_uuid=\"" + node1_sliveruuid + "\" " +\
        "            node_uuid=\"" + node1_uuid + "\" /> " +\
        "  <linkendpoints nickname=\"source_interface\" " +\
        "            tunnel_ip=\"192.168.1.2\" " +\
        "            node_nickname=\"geni2\" " +\
        "            sliver_uuid=\"" + node2_sliveruuid + "\" " +\
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


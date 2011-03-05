#! /usr/bin/env python
#
# GENIPUBLIC-COPYRIGHT
# Copyright (c) 2008-2011 University of Utah and the Flux Group.
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
import xml.dom.minidom
import string
from M2Crypto import X509

ACCEPTSLICENAME=1

def Usage():
    print "usage: " + sys.argv[ 0 ] + " [option...] \
[component-manager-1 component-manager-2 [rspec1-file rspec2-file tunnel-file]]"
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
                                            [default: query from SA]

    component-manager-1 and component-manager-2 are hrns
    rspec1-file and rspec2-file are the rspecs to be sent to the two component managers.
    tunnel-file contains just link tags for the tunnels between them."""

execfile( "test-common.py" )

def makeRequest(mainText, otherText, tun):
  main = xml.dom.minidom.parseString(mainText)
  other = xml.dom.minidom.parseString(otherText)
  result = '''<?xml version="1.0" encoding="UTF-8"?>
<rspec xmlns="http://www.protogeni.net/resources/rspec/0.2" type="request">'''
  result += "\n"
  for node in main.getElementsByTagName("node"):
    result += node.toxml()
    result += "\n"
  for node in other.getElementsByTagName("node"):
    result += node.toxml()
    result += "\n"
  for link in main.getElementsByTagName("link"):
    result += link.toxml()
    result += "\n"
  result += tun
  result += "</rspec>"
  return result

if len( args ) >= 2:
    managers = ( args[ 0 ], args[ 1 ] )
    if len ( args ) == 3:
      try:
        rspecfile = open(args[ 2 ])
        rspec = rspecfile.read()
        rspecfile.close()
      except IOError, e:
        print >> sys.stderr, args[ 0 ] + ": " + e.strerror
        sys.exit( 1 )
    else:
      rspec = None
elif len( args ):
    Usage()
    sys.exit( 1 )
else:
    managers = None
    rspec = None

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
                return (cm[ "url" ], cm[ "urn" ])
        Fatal( "Could not find component manager " + name )

    url1,urn1 = FindCM( managers[ 0 ], components )
    url2,urn2 = FindCM( managers[ 1 ], components )
else:
    url1 = components[0]["url"]
    url2 = components[1]["url"]
    urn1 = components[0]["urn"]
    urn2 = components[1]["urn"]

#url1 = "https://boss.emulab.net:443/protogeni/stoller/xmlrpc/cm"
#url2 = "https://myboss.myelab.testbed.emulab.net:443/protogeni/xmlrpc/cm"

def DeleteSlivers():
    #
    # Delete the slivers.
    #
    print "Deleting sliver1 now"
    params = {}
    params["credentials"] = (myslice,)
    params["slice_urn"]   = SLICEURN
    rval,response = do_method(None, "DeleteSlice",
                              params, URI=url1, version="2.0")
    if rval:
        Fatal("Could not delete sliver on CM1")
        pass
    print "Sliver1 has been deleted"
    
    print "Deleting sliver2 now"
    params = {}
    params["credentials"] = (myslice,)
    params["slice_urn"]   = SLICEURN
    rval,response = do_method(None, "DeleteSlice",
                              params, URI=url2, version="2.0")
    if rval:
        Fatal("Could not delete sliver on CM2")
        pass
    print "Sliver2 has been deleted"
    sys.exit(0);
    pass

if DELETE:
    DeleteSlivers()
    sys.exit(1)
    pass

#
# Lookup my ssh keys.
#
params = {}
params["credential"] = mycredential
rval,response = do_method("sa", "GetKeys", params)
if rval:
    Fatal("Could not get my keys")
    pass
mykeys = response["value"]
if debug: print str(mykeys)

#
# Build an rspec.
#
if not rspec:
  rspec = ("<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> " +\
       	   " <node virtual_id=\"geni1\" "+\
	   "       component_manager_urn=\"" + urn1 + "\" " +\
           "       virtualization_type=\"emulab-vnode\"> " +\
           " </node>" +\
       	   " <node virtual_id=\"geni2\" "+\
	   "       component_manager_urn=\"" + urn2 + "\" " +\
           "       virtualization_type=\"emulab-vnode\"> " +\
           " </node>" +\
	   " <link virtual_id=\"link0\" link_type=\"tunnel\"> " +\
           "  <interface_ref virtual_node_id=\"geni1\" " +\
           "                 virtual_interface_id=\"virt0\" "+\
           "                 tunnel_ip=\"192.168.1.1\" />" +\
           "  <interface_ref virtual_node_id=\"geni2\" " +\
           "                 virtual_interface_id=\"virt0\" "+\
           "                 tunnel_ip=\"192.168.1.2\" />" +\
           " </link>" +\
           "</rspec>")

print "Asking for a ticket from CM1 ..."
params = {}
params["slice_urn"]   = SLICEURN
params["credentials"] = (myslice,)
params["rspec"]       = rspec
rval,response = do_method(None, "GetTicket", params, URI=url1, version="2.0")
if rval:
    if response and response["value"]:
        print >> sys.stderr, ""
        print >> sys.stderr, str(response["value"])
        print >> sys.stderr, ""
        pass
    Fatal("Could not get ticket")
    pass
ticket1 = response["value"]
print "Got a ticket from CM1, asking for a ticket from CM2 ..."

#
# Get a ticket for a node on another CM.
#
params = {}
params["slice_urn"]   = SLICEURN
params["credentials"] = (myslice,)
params["rspec"]       = rspec
rval,response = do_method(None, "GetTicket", params, URI=url2, version="2.0")
if rval:
    if response and response["value"]:
        print >> sys.stderr, ""
        print >> sys.stderr, str(response["value"])
        print >> sys.stderr, ""
        pass
    Fatal("Could not get ticket")
    pass
ticket2 = response["value"]
print "Got a ticket from CM2, redeeming ticket on CM1 ..."

#
# Create the slivers.
#
params = {}
params["credentials"] = (myslice,)
params["ticket"]      = ticket1
params["slice_urn"]   = SLICEURN
params["keys"]        = mykeys
rval,response = do_method(None, "RedeemTicket", params,
                          URI=url1, version="2.0")
if rval:
    Fatal("Could not redeem ticket on CM1")
    pass
sliver1,manifest1 = response["value"]
print "Created a sliver on CM1, redeeming ticket on CM2 ..."
print str(manifest1);

params = {}
params["credentials"] = (myslice,)
params["ticket"]      = ticket2
params["slice_urn"]   = SLICEURN
params["keys"]        = mykeys
rval,response = do_method(None, "RedeemTicket", params,
                          URI=url2, version="2.0")
if rval:
    Fatal("Could not redeem ticket on CM2")
    pass
sliver2,manifest2 = response["value"]
print "Created a sliver on CM2"
print str(manifest2)

#
# Start the slivers.
#
params = {}
params["credentials"] = (sliver1,)
params["slice_urn"]   = SLICEURN
rval,response = do_method(None, "StartSliver", params, URI=url1, version="2.0")
if rval:
    Fatal("Could not start sliver on CM1")
    pass
print "Started sliver on CM1. Starting sliver on CM2 ..."

params = {}
params["credentials"] = (sliver2,)
params["slice_urn"]   = SLICEURN
rval,response = do_method(None, "StartSliver", params, URI=url2, version="2.0")
if rval:
    Fatal("Could not start sliver on CM2")
    pass

print "Slivers have been started"
print "You should be able to log into the sliver after a little bit"

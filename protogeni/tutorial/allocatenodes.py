#! /usr/bin/env python
#
# GENIPUBLIC-COPYRIGHT
# Copyright (c) 2008-2010 University of Utah and the Flux Group.
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
import time
import re
import xml.dom.minidom

ACCEPTSLICENAME=1

debug    = 0
impotent = 1

execfile( "test-common.py" )

if len(REQARGS) > 1:
    Usage()
    sys.exit( 1 )
elif len(REQARGS) == 1:
    try:
        rspecfile = open(REQARGS[0])
        rspec = rspecfile.read()
        rspecfile.close()
    except IOError, e:
        print >> sys.stderr, args[ 0 ] + ": " + e.strerror
        sys.exit( 1 )
else:
    rspec = "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> " +\
            " <node virtual_id=\"geni1\" "+\
            "       virtualization_type=\"emulab-vnode\" " +\
            "       startup_command=\"/bin/ls > /tmp/foo\"> " +\
            " </node>" +\
            "</rspec>"    

#
# Get a credential for myself, that allows me to do things at the SA.
#
print "Obtaining SA credential...",
sys.stdout.flush()
mycredential = get_self_credential()
print " "

#
# Lookup my ssh keys.
#
params = {}
params["credential"] = mycredential
print "Looking up SSH keys...",
sys.stdout.flush()
rval,response = do_method("sa", "GetKeys", params)
print " "
if rval:
    Fatal("Could not get my keys")
    pass
mykeys = response["value"]
if debug: print str(mykeys)
keyfile = open( os.environ[ "HOME" ] + "/.ssh/id_rsa.pub" )
emulabkey = keyfile.readline().strip()
keyfile.close()
mykeys.append( { 'type' : 'ssh', 'key' : emulabkey } )
if debug: print str(mykeys)

#
# Lookup slice.
#
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = SLICENAME
print "Looking up slice...",
sys.stdout.flush()
rval,response = do_method("sa", "Resolve", params)
print " "
if rval:
    #
    # Create a slice. 
    #
    print "Creating new slice called " + SLICENAME + "...",
    sys.stdout.flush()
    params = {}
    params["credential"] = mycredential
    params["type"]       = "Slice"
    params["hrn"]        = SLICENAME
    rval,response = do_method("sa", "Register", params)
    print " "
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
    print "Asking for slice credential for " + SLICENAME + "...",
    sys.stdout.flush()
    myslice = response["value"]
    myslice = get_slice_credential( myslice, mycredential )
    print " "
    pass

#
# Create the sliver.
#
print "Creating the sliver...",
sys.stdout.flush()
params = {}
params["credentials"] = (myslice,)
params["slice_urn"]   = SLICEURN
params["rspec"]       = rspec
params["keys"]        = mykeys
params["impotent"]    = impotent
rval,response = do_method("cm", "CreateSliver", params, version="2.0")
print " "
if rval:
    Fatal("Could not create sliver")
    pass
sliver,manifest = response["value"]
print "Received the manifest:"

doc = xml.dom.minidom.parseString( manifest )

for node in doc.getElementsByTagName( "node" ):
    print "    Node " + node.getAttribute( "virtual_id" ) + " is " + node.getAttribute( "hostname" )

print "Waiting until sliver is ready",
sys.stdout.flush()

#
# Poll for the sliver status.  It would be nice to use WaitForStatus
# here, but SliverStatus is more general (since it falls in the minimal
# subset).
#
params = {}
params["slice_urn"]   = SLICEURN
params["credentials"] = (sliver,)
while True:
    rval,response = do_method("cm", "SliverStatus", params, quiet=True, version="2.0")
    if rval:
        sys.stdout.write( "*" )
        sys.stdout.flush()
	time.sleep( 3 )
    elif response[ "value" ][ "status" ] == "ready":
        break
    elif response[ "value" ][ "status" ] == "changing" or response[ "value" ][ "status" ] == "mixed":
        sys.stdout.write( "." )
        sys.stdout.flush()
        time.sleep( 3 )
    else:
	print
        Fatal( "Sliver status is " + response[ "value" ][ "status" ] )

print
print "Sliver is ready."

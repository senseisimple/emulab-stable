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
import time
import re

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
mycredential = get_self_credential()
print "Got my SA credential"

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
# Lookup slice and get credential.
#
myslice = resolve_slice( SLICENAME, mycredential )

print "Asking for slice credential for " + SLICENAME
slicecredential = get_slice_credential( myslice, mycredential )
print "Got the slice credential"

#
# Create the sliver.
#
print "Creating the Sliver ..."
params = {}
params["credentials"] = (slicecredential,)
params["slice_urn"]   = myslice["urn"]
params["rspec"]       = rspec
params["keys"]        = mykeys
params["impotent"]    = impotent
rval,response = do_method("cm", "CreateSliver", params, version="2.0")
if rval:
    Fatal("Could not create sliver")
    pass
sliver,manifest = response["value"]
print "Created the sliver"
#print str(manifest)


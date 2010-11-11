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

ACCEPTSLICENAME=1

debug    = 0
impotent = 1
rspec    = None

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
    pass

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
# Lookup slice.
#
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = SLICENAME
rval,response = do_method("sa", "Resolve", params)
if rval:
    Fatal("No such slice at SA");
    pass
else:
    #
    # Get the slice credential.
    #
    print "Asking for slice credential for " + SLICENAME
    myslice = response["value"]
    slicecred = get_slice_credential( myslice, mycredential )
    print "Got the slice credential"
    pass

#
# Do a resolve to get the sliver urn.
#
print "Resolving the slice at the CM"
params = {}
params["credentials"] = (slicecred,)
params["urn"]         = myslice["urn"]
rval,response = do_method("cm", "Resolve", params, version="2.0")
if rval:
    Fatal("Could not get resolve slice")
    pass
myslice = response["value"]
print str(myslice)

#
# Get the sliver credential.
#
print "Asking for sliver credential"
params = {}
params["slice_urn"] = SLICEURN
params["credentials"] = (slicecred,)
rval,response = do_method("cm", "GetSliver", params, version="2.0")
if rval:
    Fatal("Could not get Sliver credential")
    pass
slivercred = response["value"]
print "Got the sliver credential"

if rspec == None:
    #
    # Do a resolve to get the manifest urn.
    #
    print "Resolving the sliver at the CM to get the manifest"
    params = {}
    params["credentials"] = (slicecred,)
    params["urn"]         = myslice["sliver_urn"]
    rval,response = do_method("cm", "Resolve", params, version="2.0")
    if rval:
        Fatal("Could not get resolve slice")
        pass
    mysliver = response["value"]
    rspec = mysliver["manifest"];
    pass

#
# Update the sliver, getting a ticket back
#
print "Updating the Sliver ..."
params = {}
params["sliver_urn"]  = myslice["sliver_urn"]
params["credentials"] = (slicecred,)
params["rspec"]       = rspec
params["impotent"]    = impotent
rval,response = do_method("cm", "UpdateSliver", params, version="2.0")
if rval:
    Fatal("Could not update sliver")
    pass
ticket = response["value"]
print "Updated the sliver, got a new ticket"
print str(ticket)


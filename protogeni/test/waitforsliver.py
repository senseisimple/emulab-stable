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
import time

ACCEPTSLICENAME=1

execfile( "test-common.py" )

#
# Get a credential for myself, that allows me to do things at the SA.
#
mycredential = get_self_credential()
if debug:
    print "Got my SA credential. Looking for slice ..."

#
# Lookup slice.
#
myslice = resolve_slice( SLICENAME, mycredential )
if debug:
    print "Found the slice, asking for a credential ..."

#
# Get the slice credential.
#
slicecred = get_slice_credential( myslice, mycredential )
if debug:
    print "Got the slice credential, asking for a sliver credential ..."

#
# Get the sliver credential.
#
params = {}
params["credentials"] = (slicecred,)
params["slice_urn"]   = SLICEURN
rval,response = do_method("cmv2", "GetSliver", params)
if rval:
    Fatal("Could not get Sliver credential")
    pass
slivercred = response["value"]
if debug:
    print "Got the sliver credential.  Polling sliver status..."

#
# Poll for the sliver status.  It would be nice to use WaitForStatus
# here, but SliverStatus is more general (since it falls in the minimal
# subset).
#
params = {}
params["slice_urn"]   = SLICEURN
params["credentials"] = (slivercred,)
while True: # #@(%ing Python doesn't have do loops
    rval,response = do_method("cmv2", "SliverStatus", params)
    if rval:
        Fatal("Could not get sliver status")
    if response[ "value" ][ "status" ] == "ready": # no #@(%ing switch, either
        break
    elif response[ "value" ][ "status" ] == "changing":
        time.sleep( 3 )
    else:
        Fatal( "Sliver state is " + response[ "value" ][ "status" ] )

if debug:
    print "Sliver is ready.  Resolving slice at the CM..."

#
# Resolve the slice at the CM, to find the sliver URN.
#
params = {}
params["urn"] = SLICEURN
params["credentials"] = (slicecred,)
rval,response = do_method("cmv2", "Resolve", params)
if rval:
    Fatal("Could not resolve slice")
    pass
sliver_urn = response[ "value" ][ "sliver_urn" ]

if debug:
    print "Got the sliver URN.  Resolving manifest..."

#
# Resolve the sliver at the CM, to find the manifest.
#
params = {}
params["urn"] = sliver_urn
params["credentials"] = (slicecred,)
rval,response = do_method("cmv2", "Resolve", params)
if rval:
    Fatal("Could not resolve sliver")
    pass

print response[ "value" ][ "manifest" ]

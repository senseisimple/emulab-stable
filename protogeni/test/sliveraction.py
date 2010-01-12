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
import re

ACCEPTSLICENAME=1
URN = None

execfile( "test-common.py" )

if len(REQARGS) < 1:
    print >> sys.stderr, "Must provide the action (start/stop/restart)"
    sys.exit(1)
else:
    action = REQARGS[0]
    if action != "start" and action != "stop" and action != "restart":
        print >> sys.stderr, "Action must be one of start/stop/restart"
        sys.exit(1)
        pass
    if len(REQARGS) == 2:
        URN = REQARGS[1]
        pass
    pass

#
# Get a credential for myself, that allows me to do things at the SA.
#
mycredential = get_self_credential()
print "Got my SA credential. Looking for slice ..."

#
# Lookup slice, delete before proceeding.
#
myslice = resolve_slice( SLICENAME, mycredential )
print "Found the slice, asking for a credential ..."

#
# Get the slice credential.
#
slicecred = get_slice_credential( myslice, mycredential )
print "Got the slice credential, asking for a sliver credential ..."

#
# Get the sliver credential.
#
params = {}
params["credentials"] = (slicecred,)
params["slice_urn"]   = SLICEURN
rval,response = do_method("cm", "GetSliver", params, version="2.0")
if rval:
    Fatal("Could not get Sliver credential")
    pass
slivercred = response["value"]

if action == "start":
    method = "StartSliver"
elif action == "stop":
    method = "StopSliver"
else:
    method = "RestartSliver"
    pass

#
# Start the sliver.
#
print "Got the sliver credential, calling " + method + " on the sliver";
params = {}
params["credentials"] = (slivercred,)
if URN:
    params["component_urns"] = (URN,)
else:
    params["slice_urn"]   = SLICEURN
    pass
rval,response = do_method("cm", method, params, version="2.0")
if rval:
    Fatal("Could not start sliver")
    pass
print "Sliver has been " + action + "'ed."




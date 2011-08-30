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

ACCEPTSLICENAME=1

execfile( "test-common.py" )

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
# Do a resolve to get the sliver urn.
#
print "Resolving the slice at the CM"
params = {}
params["credentials"] = (slicecred,)
params["urn"]         = myslice["urn"]
rval,response = do_method("cm", "Resolve", params, version="2.0")
if rval:
    Fatal("Could not resolve slice")
    pass
mysliver = response["value"]
print str(mysliver)

if not "sliver_urn" in mysliver:
    Fatal("No sliver exists for slice")
    pass

#
# Get the sliver credential.
#
params = {}
params["credentials"] = (slicecred,)
params["slice_urn"]   = myslice["urn"]
rval,response = do_method("cm", "GetSliver", params, version="2.0")
if rval:
    Fatal("Could not get Sliver credential")
    pass
slivercred = response["value"]
print "Got the sliver credential, deleting the sliver";

#
# Delete the sliver.
#
params = {}
params["credentials"] = (slivercred,)
params["sliver_urn"]  = mysliver["sliver_urn"]
rval,response = do_method("cm", "DeleteSliver", params, version="2.0")
if rval:
    Fatal("Could not delete sliver")
    pass
print "Sliver has been deleted. Ticket for remaining time:"
ticket = response["value"]
print str(ticket);



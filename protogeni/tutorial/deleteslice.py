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

execfile( "test-common.py" )

#
# Get a credential for myself, that allows me to do things at the SA.
#
print "Obtaining SA credential...",
sys.stdout.flush()
mycredential = get_self_credential()
print " "

#
# Lookup slice, delete before proceeding.
#
print "Looking up slice...",
sys.stdout.flush()
myslice = resolve_slice( SLICENAME, mycredential )
print " "

#
# Get the slice credential.
#
print "Obtaining slice credential...",
sys.stdout.flush()
slicecred = get_slice_credential( myslice, mycredential )
print " "

#
# Delete the Slice
#
print "Deleting the slice...",
sys.stdout.flush()
params = {}
params["credentials"] = (slicecred,)
params["slice_urn"]   = SLICEURN
rval,response = do_method("cm", "DeleteSlice", params, version="2.0")
print " "
if rval:
    Fatal("Could not delete slice")
    pass
print "Slice has been deleted."



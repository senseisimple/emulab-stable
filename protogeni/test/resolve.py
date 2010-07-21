#! /usr/bin/env python
#
# GENIPUBLIC-COPYRIGHT
# Copyright (c) 2009-2010 University of Utah and the Flux Group.
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

execfile("test-common.py")

URN = None

if len(REQARGS) == 1:
    URN = REQARGS[0]
else:
    print "You must supply a URN to resolve"
    sys.exit(1)
    pass

#
# Get a credential for myself, that allows me to do things at the SA.
#
mycredential = get_self_credential()
print "Got my SA credential"

type = URN.split('+')[2]  
if type == 'slice':
    SLICENAME = URN
    pass
    
if type == 'slice' or type == 'sliver':
    myslice = resolve_slice( SLICENAME, mycredential )
    print "Found the slice, asking for a credential ..."
    
    mycredential = get_slice_credential( myslice, mycredential )
    print "Got the slice credential"
    pass

print "Resolving at the local CM"
params = {}
params["credentials"] = (mycredential,)
params["urn"]         = URN
rval,response = do_method("cm", "Resolve", params, version="2.0")
if rval:
    Fatal("Could not resolve")
    pass
value = response["value"]
print str(value)



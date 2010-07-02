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
import time
import re
import xmlrpclib
from M2Crypto import X509

ACCEPTSLICENAME=1

execfile( "test-common.py" )

#
# Get a credential for myself, that allows me to do things at the SA.
#
mycredential = get_self_credential()
print "Got self credential"

#
# Lookup slice.
#
myslice = resolve_slice( SLICENAME, mycredential )
print "Resolved slice " + SLICENAME

#
# Get the slice credential.
#
slicecredential = get_slice_credential( myslice, mycredential )
print "Got slice credential"

#
# Bind to slice at the CM.
#
params = {}
params["credential"] = slicecredential
rval,response = do_method("cm", "BindToSlice", params)
if rval:
    Fatal("Could not bind myself to slice")
    pass
print "Bound myself to slice"

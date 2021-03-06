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
import xmlrpclib
from M2Crypto import X509

ACCEPTSLICENAME=1

execfile( "test-common.py" )

if len(REQARGS) != 1 or REQARGS[0] != "verified":
    print "Are you sure you want to use this script? It freezes the slice."
    print "You probably want to use deleteslice.py instead."
    print "If you really want to use this script, add 'verified'"
    sys.exit(1)
    pass

#
# Get a credential for myself, that allows me to do things at the SA.
#
mycredential = get_self_credential()
print "Got my SA credential"

#
# Lookup slice, get credential.
#
myslice = resolve_slice( SLICENAME, mycredential )
myslice = get_slice_credential( myslice, mycredential )
print "Got the slice credential"

print "Shutting down slice";
params = {}
params["credential"] = myslice
rval,response = do_method("sa", "Shutdown", params)
if rval:
    Fatal("Could not shutdown slice")
    pass
print "Slice has been shutdown";

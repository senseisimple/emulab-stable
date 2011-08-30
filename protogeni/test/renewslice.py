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
# Renew the slice at the SA *and* the sliver at the CM.
# Also see renewsliver.py ... to just renew the sliver. 
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

minutes = 60

execfile( "test-common.py" )

if len(REQARGS) != 1:
    print >> sys.stderr, "Must provide number of minutes to renew for"
    sys.exit(1)
else:
    minutes = REQARGS[0]
    pass
#
# Get a credential for myself, that allows me to do things at the SA.
#
mycredential = get_self_credential()
print "Got my SA credential"

#
# Lookup slice
#
myslice = resolve_slice( SLICENAME, mycredential )
print "Found the slice, asking for a credential ..."

#
# Get the slice credential.
#
slicecred = get_slice_credential( myslice, mycredential )
print "Got the slice credential, renewing the slice at the SA ..."

#
# Bump the expiration time.
#
valid_until = time.strftime("%Y%m%dT%H:%M:%S",
                            time.gmtime(time.time() + (60 * int(minutes))))

#
# Renew the slice at the SA.
#
params = {}
params["credential"] = slicecred
params["expiration"] = valid_until
rval,response = do_method("sa", "RenewSlice", params)
if rval:
    Fatal("Could not renew slice at the SA")
    pass
print "The slice has been renewed successfully."
print "Now asking for slice credential again...";

#
# Get the slice credential again so we have the new time in it.
#
slicecred = get_slice_credential( myslice, mycredential )
print "Got the slice credential, attempting to renew the sliver...";

params = {}
params["credentials"]  = (slicecred,)
params["slice_urn"]    = myslice["urn"]
params["valid_until"]  = valid_until
rval,response = do_method("cm", "RenewSlice", params, version="2.0")
if rval:
    Fatal("Renewed slice, but not sliver.")
    pass
print "Sliver has been renewed until " + valid_until



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
OtherUser = None

execfile( "test-common.py" )

if len(REQARGS) == 1:
    OtherUser = REQARGS[0]
    pass

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
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = SLICENAME
print "Looking for existing slice...",
sys.stdout.flush()
rval,response = do_method("sa", "Resolve", params)
print " "
if rval == 0:
    print "Deleting previously registered slice";
    params = {}
    params["credential"] = mycredential
    params["type"]       = "Slice"
    params["hrn"]        = SLICENAME
    rval,response = do_method("sa", "Remove", params)
    if rval:
        Fatal("Could not remove slice record")
        pass
    pass

#
# Create a slice. 
#
print "Creating new slice called " + SLICENAME + "...",
sys.stdout.flush()
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = SLICENAME
rval,response = do_method("sa", "Register", params)
print " "
if rval:
    Fatal("Could not get my slice")
    print str(rval)
    print str(response)
    pass
myslice = response["value"]
print "New slice created: " + SLICEURN
if debug: print str(myslice)

#
# Lookup another user so we can bind them to the slice.
#
if OtherUser:
    params = {}
    params["hrn"]       = OtherUser;
    params["credential"] = mycredential
    params["type"]       = "User"
    rval,response = do_method("sa", "Resolve", params)
    if rval:
        Fatal("Could not resolve other user")
        pass
    user = response["value"]
    print "Found other user record at the SA, binding to slice ..."
    
    #
    # And bind the user to the slice so that he can get his own cred.
    #
    params = {}
    params["urn"]        = user["urn"]
    params["credential"] = myslice
    rval,response = do_method("sa", "BindToSlice", params)
    if rval:
        Fatal("Could not bind other user to slice")
        pass
    binding = response["value"]
    print "Bound other user to slice at the SA"
    pass


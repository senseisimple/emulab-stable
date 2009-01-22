#! /usr/bin/env python
#
# EMULAB-COPYRIGHT
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
import xmlrpclib
from M2Crypto import X509

execfile( "test-common.py" )

#
# Get a credential for myself, that allows me to do things at the SA.
#
mycredential = get_self_credential()
print "Got my SA credential"

#
# Lookup slice, delete before proceeding.
#
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = SLICENAME
rval,response = do_method("sa", "Resolve", params)
if rval == 0:
    myslice = response["value"]
    myuuid  = myslice["uuid"]

    print "Deleting previously registered slice";
    params = {}
    params["credential"] = mycredential
    params["type"]       = "Slice"
    params["uuid"]       = myuuid
    rval,response = do_method("sa", "Remove", params)
    if rval:
        Fatal("Could not remove slice record")
        pass
    pass

#
# Create a slice. 
#
print "Creating new slice called " + SLICENAME
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = SLICENAME
rval,response = do_method("sa", "Register", params)
if rval:
    Fatal("Could not get my slice")
    print str(rval)
    print str(response)
    pass
myslice = response["value"]
print "New slice created"
#print str(myslice);

#
# Lookup another user so we can bind them to the slice.
#
params = {}
params["hrn"]       = "leebee"
params["credential"] = mycredential
params["type"]       = "User"
rval,response = do_method("sa", "Resolve", params)
if rval:
    Fatal("Could not resolve leebee")
    pass
leebee = response["value"]
print "Found leebee record at the SA, binding to slice ..."

#
# And bind the user to the slice so that he can get his own cred.
#
params = {}
params["uuid"]       = leebee["uuid"]
params["credential"] = myslice
rval,response = do_method("sa", "BindToSlice", params)
if rval:
    Fatal("Could not bind leebee to slice")
    pass
leebee = response["value"]
print "Bound leebee to slice at the SA"

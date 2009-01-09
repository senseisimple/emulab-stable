#! /usr/bin/env python
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004, 2008 University of Utah and the Flux Group.
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

execfile( "test-common.py" )

#
# Get a credential for myself, that allows me to do things at the SA.
#
params = {}
rval,response = do_method("sa", "GetCredential", params)
if rval:
    Fatal("Could not get my credential")
    pass
mycredential = response["value"]
print "Got my SA credential, looking up mytestslice"
#print str(mycredential);

#
# Lookup slice.
#
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = "mytestslice"
rval,response = do_method("sa", "Resolve", params)
if rval:
    #
    # Create a slice. 
    #
    Fatal("Slice does not exist");
    pass
else:
    #
    # Get the slice credential.
    #
    print "Asking for slice credential for mytestslice";
    myslice = response["value"]
    myuuid  = myslice["uuid"]
    params = {}
    params["credential"] = mycredential
    params["type"]       = "Slice"
    params["uuid"]       = myuuid
    rval,response = do_method("sa", "GetCredential", params)
    if rval:
        Fatal("Could not get Slice credential")
        pass
    myslice = response["value"]
    print "Got the slice credential, binding myself to it"
    pass

#
# Bind to slice at the CM.
#
params = {}
params["credential"] = myslice
rval,response = do_method("cm", "BindToSlice", params)
if rval:
    Fatal("Could not bind myself to slice")
    pass
print "Bound myself to slice"

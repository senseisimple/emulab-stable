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
import re
import xmlrpclib
from M2Crypto import X509

execfile( "test-common.py" )

#
# Get a credential for myself, that allows me to do things at the SA.
#
params = {}
params["uuid"] = "0b2eb97e-ed30-11db-96cb-001143e453fe"
rval,response = do_method("sa", "GetCredential", params)
if rval:
    Fatal("Could not get my credential")
    pass
mycredential = response["value"]
print "Got my SA credential"
#print str(mycredential);

#
# Lookup slice, delete before proceeding.
#
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = "mytestslice"
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


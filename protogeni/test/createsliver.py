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
print "Got my SA credential"
#print str(mycredential);

#
# Lookup my ssh keys.
#
params = {}
params["credential"] = mycredential
rval,response = do_method("sa", "GetKeys", params)
if rval:
    Fatal("Could not get my keys")
    pass
mykeys = response["value"]
#print str(mykeys);

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
    print "Creating new slice called mytestslice";
    params = {}
    params["credential"] = mycredential
    params["type"]       = "Slice"
    params["hrn"]        = "mytestslice"
    rval,response = do_method("sa", "Register", params)
    if rval:
        Fatal("Could not create new slice")
        pass
    myslice = response["value"]
    print "New slice created"
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
    print "Got the slice credential"
    pass

#
# Get a ticket. We do not have a real resource discovery tool yet, so
# as a debugging aid, you can wildcard the uuid, and the CM will find
# a free node and fill it in.
#
print "Asking for a ticket from the local CM"

rspec = "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> " +\
        " <node uuid=\"*\" " +\
        "       nickname=\"geni1\" "+\
        "       virtualization_type=\"emulab-vnode\"> " +\
        " </node>" +\
        "</rspec>"
params = {}
params["credential"] = myslice
params["rspec"]      = rspec
rval,response = do_method("cm", "GetTicket", params)
if rval:
    Fatal("Could not get ticket")
    pass
ticket = response["value"]
print "Got a ticket from the CM. Redeeming the ticket ..."

#
# Create the sliver.
#
params = {}
params["ticket"]   = ticket
params["keys"]     = mykeys
params["impotent"] = impotent
rval,response = do_method("cm", "RedeemTicket", params)
if rval:
    Fatal("Could not redeem ticket")
    pass
sliver = response["value"]
print "Created the sliver. Starting the sliver ..."

#
# Start the sliver.
#
params = {}
params["credential"] = sliver
params["impotent"]   = impotent
rval,response = do_method("cm", "StartSliver", params)
if rval:
    Fatal("Could not start sliver")
    pass
print "Sliver has been started."


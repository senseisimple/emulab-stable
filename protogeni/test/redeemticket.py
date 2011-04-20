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
#
import sys
import pwd
import getopt
import os
import time
import re

ACCEPTSLICENAME=1

debug    = 0
impotent = 1

execfile( "test-common.py" )

#
# Get a credential for myself, that allows me to do things at the SA.
#
mycredential = get_self_credential()
print "Got my SA credential"

#
# Lookup my ssh keys.
#
params = {}
params["credential"] = mycredential
params["version"]    = 2
rval,response = do_method("sa", "GetKeys", params)
if rval:
    Fatal("Could not get my keys")
    pass
mykeys = response["value"]
if debug: print str(mykeys)

#
# Lookup slice.
#
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = SLICENAME
rval,response = do_method("sa", "Resolve", params)
if rval:
    Fatal("No such slice at SA");
    pass
else:
    #
    # Get the slice credential.
    #
    print "Asking for slice credential for " + SLICENAME
    myslice = response["value"]
    slicecred = get_slice_credential( myslice, mycredential )
    print "Got the slice credential"
    pass

#
# Do a resolve to get the ticket urn.
#
print "Resolving the slice at the CM"
params = {}
params["credentials"] = (slicecred,)
params["urn"]         = myslice["urn"]
rval,response = do_method("cm", "Resolve", params, version="2.0")
if rval:
    Fatal("Could not resolve slice")
    pass
myslice = response["value"]
print str(myslice)

if not "ticket_urn" in myslice:
    Fatal("No ticket exists for slice")
    pass

#
# Get the ticket with another call to resolve.
#
print "Asking for a copy of the ticket"
params = {}
params["credentials"] = (slicecred,)
params["urn"]         = myslice["ticket_urn"]
rval,response = do_method("cm", "Resolve", params, version="2.0")
if rval:
    Fatal("Could not get the ticket")
    pass
ticket = response["value"]
print "Got the ticket"

redeemcred = slicecred;
#
# Get the sliver credential.
#
print "Asking for sliver credential"
params = {}
params["slice_urn"] = SLICEURN
params["credentials"] = (slicecred,)
rval,response = do_method("cm", "GetSliver", params, version="2.0")
if not rval:
  redeemcred = response["value"]
  print "Got the sliver credential"

#
# And redeem the ticket.
#
print "Redeeming the ticket"
params = {}
params["credentials"] = (redeemcred,)
params["ticket"]      = ticket
params["slice_urn"]   = SLICEURN
params["keys"]        = mykeys
rval,response = do_method("cm", "RedeemTicket", params, version="2.0")
if rval:
    Fatal("Could not redeem the ticket")
    pass
(sliver, manifest) = response["value"]
print "Created the sliver"
print str(manifest)

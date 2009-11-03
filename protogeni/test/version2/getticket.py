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

execfile( "../test-common.py" )

if len(REQARGS) > 1:
    Usage()
    sys.exit( 1 )
elif len(REQARGS) == 1:
    try:
        rspecfile = open(REQARGS[0])
        rspec = rspecfile.read()
        rspecfile.close()
    except IOError, e:
        print >> sys.stderr, args[ 0 ] + ": " + e.strerror
        sys.exit( 1 )
else:
    rspec = "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> " +\
            " <node virtual_id=\"geni1\" "+\
            "       virtualization_type=\"emulab-vnode\"> " +\
            " </node>" +\
            "</rspec>"
    pass

#
# Get a credential for myself, that allows me to do things at the SA.
#
mycredential = get_self_credential()
print "Got my SA credential, looking up " + SLICENAME

#
# Lookup slice.
#
params = {}
params["credential"] = mycredential
params["type"]       = "Slice"
params["hrn"]        = SLICENAME
rval,response = do_method("sa", "Resolve", params)
if rval:
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
        Fatal("Could not create new slice")
        pass
    myslice = response["value"]
    print "New slice created"
    pass
else:
    #
    # Get the slice credential.
    #
    print "Asking for slice credential for " + SLICENAME
    myslice = get_slice_credential( response[ "value" ], mycredential )
    print "Got the slice credential"
    pass

#
# Get a ticket. We do not have a real resource discovery tool yet, so
# as a debugging aid, you can wildcard the uuid, and the CM will find
# a free node and fill it in.
#
print "Asking for a ticket from the local CM"

params = {}
params["slice_urn"]   = SLICEURN
params["credentials"] = (myslice,)
params["rspec"]       = rspec
params["impotent"]    = 0
rval,response = do_method("cmv2", "GetTicket", params)
if rval:
    Fatal("Could not get ticket")
    pass
ticket = response["value"]
#print str(ticket)

#
# Update the ticket.
#
print "Got the ticket, doing a update on it. "
params = {}
params["slice_urn"]   = SLICEURN
params["ticket"]      = ticket
params["credentials"] = (myslice,)
params["rspec"]       = rspec
params["impotent"]    = 0
rval,response = do_method("cmv2", "UpdateTicket", params)
if rval:
    Fatal("Could not update ticket")
    pass
ticket = response["value"]
print str(ticket)

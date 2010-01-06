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
print "Asking for a ticket from the CM"

rspec = "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> " +\
        " <node virtual_id=\"geni1\" "+\
        "       virtualization_type=\"emulab-vnode\"> " +\
        " </node>" +\
        "</rspec>"
params = {}
params["credential"] = myslice
params["rspec"]      = rspec
params["impotent"]   = 0
rval,response = do_method("cm", "GetTicket", params)
if rval:
    if response and response["value"]:
        print >> sys.stderr, ""
        print >> sys.stderr, str(response["value"])
        print >> sys.stderr, ""
        pass
    Fatal("Could not get ticket")
    pass
ticket = response["value"]
print "Got a ticket from the CM. Delaying a moment ..."
if debug: print str(ticket)
#print str(ticket)

time.sleep(2)

print "Doing a ticket update ..."
params = {}
params["credential"] = myslice
params["ticket"]  = ticket
params["rspec"]   = rspec
rval,response = do_method("cm", "UpdateTicket", params)
if rval:
    Fatal("Could not update ticket")
    pass
ticket = response["value"]
print "Got an updated ticket from the CM. Delaying a moment ..."
if debug: print str(ticket)
#print str(ticket)

time.sleep(2)

print "Getting a list of all your tickets ..."
params = {}
params["credential"] = myslice
rval,response = do_method("cm", "ListTickets", params)
if rval:
    Fatal("Could not get ticket list")
    pass
tickets = response["value"]
print str(tickets);

print "Asking for a copy of the ticket ..."
params = {}
params["credential"] = myslice
params["uuid"]       = tickets[0]["uuid"]
rval,response = do_method("cm", "GetTicket", params)
if rval:
    Fatal("Could not get ticket list")
    pass
ticketcopy = response["value"]

print "Releasing the ticket now ..."
params = {}
params["credential"] = myslice
params["ticket"]     = ticketcopy
rval,response = do_method("cm", "ReleaseTicket", params)
if rval:
    Fatal("Could not release ticket")
    pass
print "Ticket has been released"


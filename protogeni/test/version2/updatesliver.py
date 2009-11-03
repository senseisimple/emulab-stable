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

ACCEPTSLICENAME=1

debug    = 0
impotent = 1

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
            "       virtualization_type=\"emulab-vnode\" " +\
            "       startup_command=\"/bin/ls > /tmp/foo\"> " +\
            " </node>" +\
            "</rspec>"    

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
# Do a resolve to get the sliver urn.
#
print "Resolving the slice at the CM"
params = {}
params["credentials"] = (slicecred,)
params["urn"]         = myslice["urn"]
rval,response = do_method("cmv2", "Resolve", params)
if rval:
    Fatal("Could not get resolve slice")
    pass
myslice = response["value"]
print str(myslice)

#
# Get the sliver credential.
#
print "Asking for sliver credential"
params = {}
params["credentials"] = (slicecred,)
rval,response = do_method("cmv2", "GetSliver", params)
if rval:
    Fatal("Could not get Sliver credential")
    pass
slivercred = response["value"]
print "Got the sliver credential"

#
# Renew the sliver, for kicks
#
valid_until = time.strftime("%Y%m%dT%H:%M:%S",time.gmtime(time.time() + 6000));

print "Renewing the Sliver until " + valid_until
params = {}
params["credentials"]  = (slivercred,)
params["slice_urn"]    = SLICEURN
params["valid_until"]  = valid_until
rval,response = do_method("cmv2", "RenewSliver", params)
if rval:
    Fatal("Could not renew sliver")
    pass
print "Sliver has been renewed"

#
# Update the sliver, getting a ticket back
#
print "Updating the Sliver ..."
params = {}
params["sliver_urn"]  = myslice["sliver_urn"]
params["credentials"] = (slicecred,)
params["rspec"]       = rspec
params["impotent"]    = impotent
rval,response = do_method("cmv2", "UpdateSliver", params)
if rval:
    Fatal("Could not update sliver")
    pass
ticket = response["value"]
print "Updated the sliver, got a new ticket"
print str(ticket)


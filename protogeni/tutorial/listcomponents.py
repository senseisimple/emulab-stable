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
import zlib

execfile( "test-common.py" )

#
# Get a credential for myself, that allows me to do things at the SA.
#
mycredential = get_self_credential()

#
# Ask the clearinghouse for a list of component managers. 
#
params = {}
params["credential"] = mycredential
rval,response = do_method("ch", "ListComponents", params)
if rval:
    Fatal("Could not get a list of components from the ClearingHouse")
    pass
if debug: print str(response["value"])

#
# Ask each manager for its list.
#
for manager in response["value"]:
    print manager[ "urn" ] + ": " + manager["url"]

    # Skip the discover resources. Too much.
    continue
    
    #
    # manager for resource list.
    #
    params = {}
    params["credential"] = mycredential
    params["available"] = True
    params["compress"] = True
    rval,response = do_method(None,
                              "DiscoverResources", params, manager["url"])
    if rval:
        print "Could not get a list of resources"
    else:
        if isinstance( response[ "value" ], xmlrpclib.Binary ):
            response[ "value" ] = zlib.decompress( str( response[ "value" ] ) )

        if debug:
            print response[ "value" ]

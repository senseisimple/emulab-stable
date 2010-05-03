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

execfile( "test-common.py" )

if len(REQARGS) > 1:
    Usage()
    sys.exit( 1 )
elif len(REQARGS) == 1:
    CMURI = REQARGS[0]
    if CMURI[-2:] == "cm":
        CMURI = CMURI[:-3]
        pass
pass

#
# Get a credential for myself, that allows me to do things at the SA.
#
mycredential = get_self_credential()

#
# Ask manager for its list.
#
params = {}
params["credentials"] = (mycredential,)
rval,response = do_method("cm", "DiscoverResources", params, version="2.0")
if rval:
    Fatal("Could not get a list of resources")
    pass
print response[ "value" ]

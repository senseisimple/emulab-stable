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
import re
import xmlrpclib
from M2Crypto import X509

execfile( "test-common.py" )

TYPE = None

if len(REQARGS) == 1:
    TYPE = REQARGS[0]
else:
    print "You must supply a TYPE (users|slices|authorities) to list"
    sys.exit(1)
    pass

# sanity check on TYPE
if TYPE not in ["users", "slices", "authorities"]:
    print "TYPE must be one of users|slices|authorities"
    sys.exit(1)

#
# Get a credential for myself, that allows me to do things at the SA.
#
mycredential = get_self_credential()

#
# Ask the clearinghouse for a list of users|slices|authorities as specified
# by TYPE
params = {}
params["credential"] = mycredential
params["type"]       = TYPE
rval,response = do_method("ch", "List", params)
if rval:
    Fatal("Could not get the list from the ClearingHouse")
    pass
print str(response["value"])


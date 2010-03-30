#! /usr/bin/env python
#
# GENIPUBLIC-COPYRIGHT
# Copyright (c) 2009-2010 University of Utah and the Flux Group.
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

execfile("test-common.py")

HRN = None
TYPE = None
if len(REQARGS) == 2:
    HRN = REQARGS[0]
    TYPE = REQARGS[1]
else:
    print "You must supply a HRN and TYPE to resolve"
    sys.exit(1)
    pass

# sanity check on specified type
if TYPE not in ["SA", "CM", "Component", "Slice", "User"]:
    print "TYPE must be one of SA|CM|Component|Slice|User"
    sys.exit(1)

#
# Get special credentials from the command line, that allows me to do things at the CH.
#
if not selfcredentialfile:
    print "Please specify special credentials with -c option."
    sys.exit(1)


mycredential = get_self_credential()


print "Resolving at the CH"
params = {}
params["credential"] = mycredential
params["hrn"]        = HRN
params["type"]       = TYPE
rval,response = do_method("ch", "Resolve", params, version="2.0")
if rval:
    Fatal("Could not resolve")
    pass
value = response["value"]
print str(value)



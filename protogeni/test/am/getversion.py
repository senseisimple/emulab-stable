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

def Usage():
    print "usage: " + sys.argv[ 0 ] + " [option...] [authority]"
    print """Options:
    -d, --debug                         be verbose about XML methods invoked
    -h, --help                          show options and usage
    -r file, --read-commands=file       specify additional configuration file"""

execfile( "test-common.py" )

if len( args ) > 1:
    Usage()
    sys.exit( 1 )
elif len( args ) == 1:
    authority = args[ 0 ]
else:
    authority = "am"

try:
    response = do_method(authority, "GetVersion", [],
                         response_handler=geni_am_response_handler)
    print response
except xmlrpclib.Fault, e:
    Fatal("Could not obtain the API version: %s" % (str(e)))

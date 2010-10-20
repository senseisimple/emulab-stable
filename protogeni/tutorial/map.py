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
import zlib
from M2Crypto import X509

def Usage():
    print "usage: " + sys.argv[ 0 ] + " [option...] <advertisement> <request>"
    print """Options:
    -c file, --credentials=file         read self-credentials from file
                                            [default: query from SA]
    -d, --debug                         be verbose about XML methods invoked
    -f file, --certificate=file         read SSL certificate from file
                                            [default: ~/.ssl/encrypted.pem]
    -h, --help                          show options and usage
    -p file, --passphrase=file          read passphrase from file
                                            [default: ~/.ssl/password]
    -r file, --read-commands=file       specify additional configuration file
    -s file, --slicecredentials=file    read slice credentials from file
                                            [default: query from SA]"""

execfile( "test-common.py" )

if len( args ) != 2:
    Usage()
    sys.exit( 1 )

adfile = open( args[ 0 ] )
ad = adfile.read()
adfile.close()

reqfile = open( args[ 1 ] )
req = reqfile.read()
reqfile.close()

#
# Get a credential for myself, that allows me to do things at the SA.
#
mycredential = get_self_credential()

#
# Ask the slice embedding service to map the rspec for us.
#
params = {}
params[ "credential" ] = mycredential
params[ "advertisement" ] = xmlrpclib.Binary( zlib.compress( ad ) )
params[ "request" ] = req
rval,response = do_method("ses", "Map", params)
if rval:
    Fatal("Could not map to physical topology")

print str(response["value"])

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
import stat
import xmlrpclib
from M2Crypto import SSL, X509

def RememberCB( c, prompt1 = '', prompt2 = '' ):
    return passphrase

execfile( "test-common.py" )

if os.path.exists( PASSPHRASEFILE ):
    Fatal( "A passphrase has already been stored." )

from M2Crypto.util import passphrase_callback
while True:
    passphrase = passphrase_callback(0)
    if not os.path.exists(CERTIFICATE):
        print >> sys.stderr, "Warning:", CERTIFICATE, "not found; cannot " \
            "verify passphrase."
        break

    try:
        ctx = SSL.Context( "sslv23" )
        ctx.load_cert( CERTIFICATE, CERTIFICATE, RememberCB )
    except M2Crypto.SSL.SSLError, err:
        print >> sys.stderr, "Could not decrypt key.  Please try again."
        continue

    break

f = open( PASSPHRASEFILE, "w" )
os.chmod( PASSPHRASEFILE, stat.S_IRUSR | stat.S_IWUSR )
f.write( passphrase )

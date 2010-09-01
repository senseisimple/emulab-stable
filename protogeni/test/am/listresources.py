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
import zlib

execfile( "test-common.py" )

available_key = "geni_available"
compress_key = "geni_compressed"

#
# Get a credential for myself, that allows me to do things at the SA.
#
mycredential = get_self_credential()

#
# Ask manager for its list.
#
options = {}
options[available_key] = True
options[compress_key] = True
params = [[mycredential], options]

try:
    response = do_method("am", "ListResources", params,
                         response_handler=geni_am_response_handler)
    if compress_key in options and options[compress_key]:
        # decode and decompress the result
        #
        # response is a string whose content is a base64 encoded
        # representation of a zlib compressed rspec
        print zlib.decompress(response.decode('base64'))
    else:
        print response
except xmlrpclib.Fault, e:
    Fatal("Could not get a list of resources: %s" % (str(e)))


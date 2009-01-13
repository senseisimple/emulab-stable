# EMULAB-COPYRIGHT
# Copyright (c) 2004-2009 University of Utah and the Flux Group.
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

from urlparse import urlsplit, urlunsplit
from urllib import splitport

HOME            = os.environ["HOME"]
# Path to my certificate
CERTIFICATE     = HOME + "/.ssl/encrypted.pem"
# Got tired of typing this over and over so I stuck it in a file.
PASSPHRASEFILE  = HOME + "/.ssl/password"
passphrase      = ""

SLICENAME       = "mytestslice"

cert = X509.load_cert( CERTIFICATE )

# XMLRPC server: use www.emulab.net for the clearinghouse, and
# the issuer of the certificate we'll identify with for everything else
XMLRPC_SERVER   = { "ch" : "www.emulab.net",
                    "default" : cert.get_issuer().CN }
SERVER_PATH     = { "default" : ":443/protogeni/xmlrpc/" }

# Debugging output.
debug           = 0
impotent        = 0

CONFIGFILE      = ".protogeni-config.py"
GLOBALCONF      = HOME + "/" + CONFIGFILE
LOCALCONF       = CONFIGFILE

if os.path.exists( GLOBALCONF ):
    execfile( GLOBALCONF )
if os.path.exists( LOCALCONF ):
    execfile( LOCALCONF )

def Fatal(message):
    print message
    sys.exit(1)
    return

def PassPhraseCB(v, prompt1='Enter passphrase:', prompt2='Verify passphrase:'):
    passphrase = open(PASSPHRASEFILE).readline()
    passphrase = passphrase.strip()
    return passphrase

#
# Call the rpc server.
#
def do_method(module, method, params, URI=None):
    if debug:
        print module + " " + method + " " + str(params);
        pass

    if not os.path.exists(CERTIFICATE):
        return Fatal("error: missing emulab certificate: %s\n" % CERTIFICATE)
    
    from M2Crypto.m2xmlrpclib import SSL_Transport
    from M2Crypto import SSL

    if URI == None:
        if module in XMLRPC_SERVER:
            addr = XMLRPC_SERVER[ module ]
        else:
            addr = XMLRPC_SERVER[ "default" ]

        if module in SERVER_PATH:
            path = SERVER_PATH[ module ]
        else:
            path = SERVER_PATH[ "default" ]

        URI = "https://" + addr + path + module
    elif module:
        URI = URI + "/" + module
        pass

    scheme, netloc, path, query, fragment = urlsplit(URI)
    if scheme == "https":
        host,port = splitport(netloc)
        if not port:
            netloc = netloc + ":443"
            URI = urlunsplit((scheme, netloc, path, query, fragment));
            pass
        pass
    
    ctx = SSL.Context("sslv23")
    ctx.load_cert(CERTIFICATE, CERTIFICATE, PassPhraseCB)
    ctx.set_verify(SSL.verify_none, 16)
    ctx.set_allow_unknown_ca(0)
    
    # Get a handle on the server,
    server = xmlrpclib.ServerProxy(URI, SSL_Transport(ctx), verbose=0)
        
    # Get a pointer to the function we want to invoke.
    meth      = getattr(server, method)
    meth_args = [ params ]

    #
    # Make the call. 
    #
    try:
        response = apply(meth, meth_args)
        pass
    except xmlrpclib.Fault, e:
        print e.faultString
        return (-1, None)

    #
    # Parse the Response, which is a Dictionary. See EmulabResponse in the
    # emulabclient.py module. The XML standard converts classes to a plain
    # Dictionary, hence the code below. 
    # 
    if len(response["output"]):
        print response["output"],
        print ": ",
        pass

    rval = response["code"]

    #
    # If the code indicates failure, look for a "value". Use that as the
    # return value instead of the code. 
    # 
    if rval:
        if response["value"]:
            rval = response["value"]
            pass
        pass
    return (rval, response)

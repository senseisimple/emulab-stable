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

# Debugging output.
debug           = 0
impotent        = 0

HOME            = os.environ["HOME"]
# Path to my certificate
CERTIFICATE     = HOME + "/.ssl/encrypted.pem"
# Got tired of typing this over and over so I stuck it in a file.
PASSPHRASEFILE  = HOME + "/.ssl/password"
passphrase      = ""

CONFIGFILE      = ".protogeni-config.py"
GLOBALCONF      = HOME + "/" + CONFIGFILE
LOCALCONF       = CONFIGFILE

SLICENAME       = "mytestslice"

selfcredentialfile = None
slicecredentialfile = None

def Usage():
    print "usage: " + sys.argv[ 0 ] + " [option...]"
    print """Options:
    -c file, --credentials=file         read self-credentials from file
                                            [default: query from SA]
    -d, --debug                         be verbose about XML methods invoked
    -f file, --certificate=file         read SSL certificate from file
                                            [default: ~/.ssl/encrypted.pem]
    -h, --help                          show options and usage
    -p file, --passphrase=file          read passphrase from file
                                            [default: ~/.ssl/password]
    -s file, --slicecredentials=file    read slice credentials from file
                                            [default: query from SA]"""

try:
    opts, args = getopt.getopt( sys.argv[ 1: ], "c:df:hp:s:",
                                [ "credentials=", "debug", "certificate=",
                                  "help", "passphrase=", "slicecredentials=" ] )
except getopt.GetoptError, err:
    print str( err )
    Usage()
    sys.exit( 1 )

for opt, arg in opts:
    if opt in ( "-c", "--credentials" ):
        selfcredentialfile = arg
    elif opt in ( "-d", "--debug" ):
        debug = 1
    elif opt in ( "-f", "--certificate" ):
        CERTIFICATE = arg
    elif opt in ( "-h", "--help" ):
        Usage()
        sys.exit( 0 )
    elif opt in ( "-p", "--passphrase" ):
        PASSPHRASEFILE = arg
    elif opt in ( "-s", "--slicecredentials" ):
        slicecredentialfile = arg

cert = X509.load_cert( CERTIFICATE )

# XMLRPC server: use www.emulab.net for the clearinghouse, and
# the issuer of the certificate we'll identify with for everything else
XMLRPC_SERVER   = { "ch" : "www.emulab.net",
                    "default" : cert.get_issuer().CN }
SERVER_PATH     = { "default" : ":443/protogeni/xmlrpc/" }

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

def get_self_credential():
    if selfcredentialfile:
        f = open( selfcredentialfile )
        c = f.read()
        f.close()
        return c
    params = {}
    params["uuid"] = "0b2eb97e-ed30-11db-96cb-001143e453fe"
    rval,response = do_method("sa", "GetCredential", params)
    if rval:
        Fatal("Could not get my credential")
        pass
    return response["value"]

def resolve_slice( name, selfcredential ):
    params = {}
    params["credential"] = mycredential
    params["type"]       = "Slice"
    params["hrn"]        = SLICENAME
    rval,response = do_method("sa", "Resolve", params)
    if rval:
        Fatal("Slice does not exist");
        pass
    else:
        return response["value"]

def get_slice_credential( slice, selfcredential ):
    if slicecredentialfile:
        f = open( slicecredentialfile )
        c = f.read()
        f.close()
        return c

    params = {}
    params["credential"] = selfcredential
    params["type"]       = "Slice"
    params["uuid"]       = slice["uuid"]
    rval,response = do_method("sa", "GetCredential", params)
    if rval:
        Fatal("Could not get Slice credential")
        pass
    return response["value"]

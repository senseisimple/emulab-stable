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

from urlparse import urlsplit, urlunsplit
from urllib import splitport
import xmlrpclib
import M2Crypto
from M2Crypto import X509
import socket

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
EXTRACONF       = None
SLICENAME       = "mytestslice"
REQARGS         = None
CMURI           = None
DELETE          = 0

selfcredentialfile = None
slicecredentialfile = None
admincredentialfile = None

if "Usage" not in dir():
    def Usage():
        print "usage: " + sys.argv[ 0 ] + " [option...]"
        print """Options:
    -c file, --credentials=file         read self-credentials from file
                                            [default: query from SA]
    -d, --debug                         be verbose about XML methods invoked
    -f file, --certificate=file         read SSL certificate from file
                                            [default: ~/.ssl/encrypted.pem]
    -h, --help                          show options and usage"""
        if "ACCEPTSLICENAME" in globals():
            print """    -n name, --slicename=name           specify human-readable name of slice
                                            [default: mytestslice]"""
            pass
        print """    -m uri, --cm=uri           specify uri of component manager
                                            [default: local]"""
        print """    -p file, --passphrase=file          read passphrase from file
                                            [default: ~/.ssl/password]
    -r file, --read-commands=file       specify additional configuration file
    -s file, --slicecredentials=file    read slice credentials from file
                                            [default: query from SA]
    -a file, --admincredentials=file    read admin credentials from file"""

try:
    opts, REQARGS = getopt.getopt( sys.argv[ 1: ], "c:df:hn:p:r:s:m:a:",
                                   [ "credentials=", "debug", "certificate=",
                                     "help", "passphrase=", "read-commands=",
                                     "slicecredentials=","admincredentials",                                     "slicename=", "cm=", "delete"] )
except getopt.GetoptError, err:
    print >> sys.stderr, str( err )
    Usage()
    sys.exit( 1 )

args = REQARGS

if "PROTOGENI_CERTIFICATE" in os.environ:
    CERTIFICATE = os.environ[ "PROTOGENI_CERTIFICATE" ]
if "PROTOGENI_PASSPHRASE" in os.environ:
    PASSPHRASEFILE = os.environ[ "PROTOGENI_PASSPHRASE" ]

for opt, arg in opts:
    if opt in ( "-c", "--credentials" ):
        selfcredentialfile = arg
    elif opt in ( "-d", "--debug" ):
        debug = 1
    elif opt in ( "--delete" ):
        DELETE = 1
    elif opt in ( "-f", "--certificate" ):
        CERTIFICATE = arg
    elif opt in ( "-h", "--help" ):
        Usage()
        sys.exit( 0 )
    elif opt in ( "-n", "--slicename" ):
        SLICENAME = arg
    elif opt in ( "-m", "--cm" ):
        CMURI = arg
        if CMURI[-2:] == "cm":
            CMURI = CMURI[:-3]
        elif CMURI[-4:] == "cmv2":
            CMURI = CMURI[:-5]
            pass
        pass
    elif opt in ( "-p", "--passphrase" ):
        PASSPHRASEFILE = arg
    elif opt in ( "-r", "--read-commands" ):
        EXTRACONF = arg
    elif opt in ( "-s", "--slicecredentials" ):
        slicecredentialfile = arg
    elif opt in ( "-a", "--admincredentials" ):
        admincredentialfile = arg

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
if EXTRACONF and os.path.exists( EXTRACONF ):
    execfile( EXTRACONF )

HOSTNAME = XMLRPC_SERVER[ "default" ]
DOMAIN   = HOSTNAME[HOSTNAME.find('.')+1:]
SLICEURN = "urn:publicid:IDN+" + DOMAIN + "+slice+" + SLICENAME

def Fatal(message):
    print >> sys.stderr, message
    sys.exit(1)

def PassPhraseCB(v, prompt1='Enter passphrase:', prompt2='Verify passphrase:'):
    """Acquire the encrypted certificate passphrase by reading a file
    or prompting the user.

    This is an M2Crypto callback. If the passphrase file exists and is
    readable, use it. If the passphrase file does not exist or is not
    readable, delegate to the standard M2Crypto passphrase
    callback. Return the passphrase.
    """
    if os.path.exists(PASSPHRASEFILE):
        try:
            passphrase = open(PASSPHRASEFILE).readline()
            passphrase = passphrase.strip()
            return passphrase
        except IOError, e:
            print 'Error reading passphrase file %s: %s' % (PASSPHRASEFILE,
                                                            e.strerror)
    else:
        if debug:
            print 'passphrase file %s does not exist' % (PASSPHRASEFILE)
    # Prompt user if PASSPHRASEFILE does not exist or could not be read.
    from M2Crypto.util import passphrase_callback
    return passphrase_callback(v, prompt1, prompt2)

def geni_am_response_handler(method, method_args):
    """Handles the GENI AM responses, which are different from the
    ProtoGENI responses. ProtoGENI always returns a dict with three
    keys (code, value, and output. GENI AM operations return the
    value, or an XML RPC Fault if there was a problem.
    """
    return apply(method, method_args)

#
# Call the rpc server.
#
def do_method(module, method, params, URI=None, quiet=False, version=None,
              response_handler=None):
    if not os.path.exists(CERTIFICATE):
        return Fatal("error: missing emulab certificate: %s\n" % CERTIFICATE)
    
    from M2Crypto.m2xmlrpclib import SSL_Transport
    from M2Crypto import SSL

    if URI == None and CMURI and (module == "cm" or module == "cmv2"):
        URI = CMURI
        pass

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

    if version:
        URI = URI + "/" + version
        pass

    scheme, netloc, path, query, fragment = urlsplit(URI)
    if not scheme:
        URI = "https://" + URI
        pass
    
    scheme, netloc, path, query, fragment = urlsplit(URI)
    if scheme == "https":
        host,port = splitport(netloc)
        if not port:
            netloc = netloc + ":443"
            URI = urlunsplit((scheme, netloc, path, query, fragment));
            pass
        pass

    if debug:
#        print URI + " " + method + " " + str(params);
        print URI + " " + method
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

    if response_handler:
        # If a response handler was passed, use it and return the result.
        # This is the case when running the GENI AM.
        return response_handler(meth, params)

    #
    # Make the call. 
    #
    try:
        response = apply(meth, meth_args)
        pass
    except xmlrpclib.Fault, e:
        if not quiet: print >> sys.stderr, e.faultString
        return (-1, None)
    except xmlrpclib.ProtocolError, e:
        if not quiet: print >> sys.stderr, e.errmsg
        return (-1, None)
    except M2Crypto.SSL.Checker.WrongHost, e:
        if not quiet:
            print >> sys.stderr, "Warning: certificate host name mismatch."
            print >> sys.stderr, "Please consult:"
            print >> sys.stderr, "    http://www.protogeni.net/trac/protogeni/wiki/HostNameMismatch"            
            print >> sys.stderr, "for recommended solutions."
            print >> sys.stderr, e
        return (-1, None)

    #
    # Parse the Response, which is a Dictionary. See EmulabResponse in the
    # emulabclient.py module. The XML standard converts classes to a plain
    # Dictionary, hence the code below. 
    # 
    if response[ "code" ] and len(response["output"]):
        if not quiet: print >> sys.stderr, response["output"] + ":",
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
    rval,response = do_method("sa", "GetCredential", params)
    if rval:
        Fatal("Could not get my credential")
        pass
    return response["value"]

def resolve_slice( name, selfcredential ):
    params = {}
    params["credential"] = mycredential
    params["type"]       = "Slice"
    params["hrn"]        = name
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
    if "urn" in slice:
        params["urn"]       = slice["urn"]
    else:
        params["uuid"]      = slice["uuid"]
    rval,response = do_method("sa", "GetCredential", params)
    if rval:
        Fatal("Could not get Slice credential")
        pass
    return response["value"]

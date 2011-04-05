#! /usr/bin/env python
#
# GENIPUBLIC-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
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

import datetime
import getopt
import os
import random
import re
import sys
import tempfile
import uuid
import xml.dom.minidom
import xmlrpclib
from M2Crypto import X509

XMLSEC1 = "xmlsec1"

def Usage():
    print "usage: " + sys.argv[ 0 ] + " [option...] object principal [privilege...]"
    print """
where "object" specifies the entity for which privileges are to be delegated;
"principal" identifies the agent to whom those privileges are granted; and
"privilege" lists which classes of operations the delegate may invoke.

Each of "object" and "principal" may be specified as a URN (or a deprecated
UUID or HRN), or a filename.  Each "privilege" must be of the form <name>[-],
where "name" is a privilege identifier and the optional "-" symbol indicates
that the privilege cannot be re-delegated.  If no privileges are specified,
then all possible privileges held are delegated.

Options:
    -c file, --credentials=file         read self-credentials from file
                                            [default: query from SA]
    -d, --debug                         be verbose about XML methods invoked
    -f file, --certificate=file         read SSL certificate from file
                                            [default: ~/.ssl/encrypted.pem]
    -h, --help                          show options and usage
    -p file, --passphrase=file          read passphrase from file
                                            [default: ~/.ssl/password]
    -r file, --read-commands=file       specify additional configuration file"""

execfile( "test-common.py" )

if len( args ) < 2:
    Usage()
    sys.exit( 1 )

def is_urn( u ):
    # Reject %00 sequences (see RFC 3986, section 7.3).
    if re.search( "%00", u ): return None

    # We accept ANY other %-encoded octet (following RFC 3987, section 5.3.2.3
    # in favour of RFC 2141, section 5, which specifies the opposite).
    # You would think that urllib.unquote and urllib.quote could do this
    # for us, but it can't; it's not safe to unquote ALL characters yet,
    # since we must still maintain a distinction between encoded delimiter
    # characters and real (unencoded) delimiters, and urllib.unquote would
    # conflate the two.
    urn = ""
    m = re.match( "([^%]*)%([0-9A-Fa-f]{2})(.*)", u )
    while m:
        urn += m.group( 1 )
        val = int( m.group( 2 ), 16 )
	# Transform %-encoded sequences back to unreserved characters
	# where possible (see RFC 3986, section 2.3).
        if( val == 0x2D or val == 0x2E or
            ( val >= 0x30 and val <= 0x39 ) or
	    ( val >= 0x41 and val <= 0x5A ) or
	    val == 0x5F or
	    ( val >= 0x61 and val <= 0x7A ) or
	    val == 0x7E ): urn += chr( val )
        else: urn += '%' + m.group( 2 )
        u = m.group( 3 )
        # this is only so $#&*(#%ing ugly because Python does not know
        # that assignments are expressions
        m = re.match( "([^%]*)%([0-9A-Fa-f]{2})(.*)", u )
    urn += u

    # The "urn" prefix is case-insensitive (see RFC 2141, section 2).
    # The "publicid" NID is case-insensitive (see RFC 2141, section 3).
    # The "IDN" specified by Viecco is believed to be case-sensitive (no
    #   authoritative reference known).
    # We regard Viecco's optional resource-type specifier as being
    #   mandatory: partly to avoid ambiguity between resource type
    #   namespaces, and partly to avoid ambiguity between a resource-type
    #   and a resource-name containing (escaped) whitespace.
    return re.match( r'^[uU][rR][nN]:[pP][uU][bB][lL][iI][cC][iI][dD]:IDN\+[A-Za-z0-9.-]+(?::[A-Za-z0-9.-]+)*\+\w+\+(?:[-!$()*,.0-9=@A-Z_a-z]|(?:%[0-9A-Fa-f][0-9A-Fa-f]))+', urn )

def is_uuid( u ):
    return re.match( "\w+\-\w+\-\w+\-\w+\-\w+$", u )

def is_hrn( h ):
    return re.match( "[^./]", h )
    
# Look up an element (which must exist exactly once) within a node.
def Lookup( node, name ):
    cnodes = filter( lambda x: x.nodeName == name, node.childNodes )

    if len( cnodes ) != 1:
        print >> sys.stderr, sys.argv[ 0 ] + ": invalid credential\n"
        if debug:
            print >> sys.stderr, "(Element \"" + name + "\" was found",
            print >> sys.stderr, str( len( cnodes ) ) + " times (expected once)"
            
        sys.exit( 1 )
    
    return cnodes[ 0 ]

def SimpleNode( d, elem, body ):
    n = d.createElement( elem );
    n.appendChild( d.createTextNode( body ) );
    return n;

mycredential = None
slice_urn = None
slice_uuid = None

if is_urn( args[ 0 ] ): slice_urn = args[ 0 ]
elif is_uuid( args[ 0 ] ): slice_uuid = args[ 0 ]
elif is_hrn( args[ 0 ] ):
    if not mycredential: mycredential = get_self_credential()
    rval, response = do_method( "sa", "Resolve", 
                                dict( credential = mycredential, 
                                      type = "Slice", 
                                      hrn = args[ 0 ] ) )
    if not rval and "value" in response and "uuid" in response[ "value" ]:
        slice_uuid = response[ "value" ][ "uuid" ]

if slice_urn:
    # we were given a URN, and can request credentials from the SA
    if not mycredential: mycredential = get_self_credential()
    rval, response = do_method( "sa", "GetCredential", 
                                dict( credential = mycredential, 
                                      type = "Slice", 
                                      urn = slice_urn ) )
    if rval:
        Fatal( sys.argv[ 0 ] + ": could not get slice credential" )

    doc = xml.dom.minidom.parseString( response[ "value" ] )
elif slice_uuid:
    # we were given a UUID, or a slice HRN which resolved to one: use
    # that UUID to request credentials from the SA (deprecated)
    if not mycredential: mycredential = get_self_credential()
    rval, response = do_method( "sa", "GetCredential", 
                                dict( credential = mycredential, 
                                      type = "Slice", 
                                      uuid = slice_uuid ) )
    if rval:
        Fatal( sys.argv[ 0 ] + ": could not get slice credential" )

    doc = xml.dom.minidom.parseString( response[ "value" ] )
else:
    # we didn't get any kind of slice identifier; assume the parameter is
    # the name of a file containing slice credentials
    doc = xml.dom.minidom.parse( args[ 0 ] )

delegate = None

if is_urn( args[ 1 ] ):
    if not mycredential: mycredential = get_self_credential()
    rval, response = do_method( "sa", "Resolve", 
                                dict( credential = mycredential, 
                                      type = "User", 
                                      urn = args[ 1 ] ) )
    if not rval and "value" in response and "gid" in response[ "value" ]:
        delegate = response[ "value" ][ "gid" ]
if is_uuid( args[ 1 ] ):
    if not mycredential: mycredential = get_self_credential()
    rval, response = do_method( "sa", "Resolve", 
                                dict( credential = mycredential, 
                                      type = "User", 
                                      uuid = args[ 1 ] ) )
    if not rval and "value" in response and "gid" in response[ "value" ]:
        delegate = response[ "value" ][ "gid" ]
elif is_hrn( args[ 1 ] ):
    if not mycredential: mycredential = get_self_credential()
    rval, response = do_method( "sa", "Resolve", 
                                dict( credential = mycredential, 
                                      type = "User", 
                                      hrn = args[ 1 ] ) )
    if not rval and "value" in response and "gid" in response[ "value" ]:
        delegate = response[ "value" ][ "gid" ]

if not delegate:
    # we didn't get any kind of user identifier; assume the parameter is
    # the name of a file containing the delegate's GID
    principal = open( args[ 1 ] )
    delegate = re.search( r"^-----BEGIN CERTIFICATE-----\s*(.*)" +
                          "^-----END CERTIFICATE-----$", principal.read(),
                          re.MULTILINE | re.DOTALL ).group( 1 )
    principal.close()

old = Lookup( doc.documentElement, "credential" )

c = doc.createElement( "credential" )

# I really want do loops in Python...
while True:
    id = "ref" + '%016X' % random.getrandbits( 64 )
    if not filter( lambda x: x.getAttribute( "xml:id" ) == "ref" + str( id ),
                   doc.getElementsByTagName( "credential" ) ):
        break

c.setAttribute( "xml:id", str( id ) )
c.appendChild( Lookup( old, "type" ).cloneNode( True ) )

c.appendChild( SimpleNode( doc, "serial", "1" ) )

c.appendChild( SimpleNode( doc, "owner_gid", delegate ) )

c.appendChild( Lookup( old, "target_gid" ).cloneNode( True ) )

c.appendChild( SimpleNode( doc, "uuid", str( uuid.uuid4() ) ) )

for n in old.childNodes:
    if n.nodeName in ( "privileges", "capabilities" ):
        if len( args ) > 2:
            # A list of privileges was given: add them each to the credential.
            if n.nodeName == "capabilities": type = "capability"
            else: type = "privilege"
        
            privs = n.cloneNode( False )
            for arg in args[ 2 : ]:
                if arg[ -1 ] == '-':
                    argname = arg[ : -1 ]
                    argdel = "0"
                else:
                    argname = arg
                    argdel = "1"

                priv = doc.createElement( type )
                privname = doc.createElement( "name" )
                privname.appendChild( doc.createTextNode( argname ) )
                privdel = doc.createElement( "can_delegate" )
                privdel.appendChild( doc.createTextNode( argdel ) )
                priv.appendChild( privname )
                priv.appendChild( privdel )
                privs.appendChild( priv )
            c.appendChild( privs )
        else:
            clone = n.cloneNode( True )
            c.appendChild( clone )
            for child in clone.childNodes:
                if child.nodeName in ( "privilege", "capability" ) and \
                    Lookup( child, "can_delegate" ).firstChild.nodeValue == "0":
                    # a privilege which cannot be delegated: delete it
                    # from the clone
                    clone.removeChild( child )
    elif n.nodeName in ( "ticket", "extensions", "expires" ):
        c.appendChild( n.cloneNode( True ) )

doc.documentElement.replaceChild( c, old )
p = doc.createElement( "parent" )
p.appendChild( old )
c.appendChild( p )

signature = doc.createElement( "Signature" );
signature.setAttribute( "xml:id", "Sig_" + str( id ) )
signature.setAttribute( "xmlns", "http://www.w3.org/2000/09/xmldsig#" )
Lookup( doc.documentElement, "signatures" ).appendChild( signature )
signedinfo = doc.createElement( "SignedInfo" )
signature.appendChild( signedinfo )
canonmeth = doc.createElement( "CanonicalizationMethod" );
canonmeth.setAttribute( "Algorithm", 
                        "http://www.w3.org/TR/2001/REC-xml-c14n-20010315" )
signedinfo.appendChild( canonmeth )
sigmeth = doc.createElement( "SignatureMethod" );
sigmeth.setAttribute( "Algorithm", 
                      "http://www.w3.org/2000/09/xmldsig#rsa-sha1" )
signedinfo.appendChild( sigmeth )
reference = doc.createElement( "Reference" );
reference.setAttribute( "URI", "#" + str( id ) )
signedinfo.appendChild( reference )
transforms = doc.createElement( "Transforms" )
reference.appendChild( transforms )
transform = doc.createElement( "Transform" );
transform.setAttribute( "Algorithm", 
                        "http://www.w3.org/2000/09/xmldsig#" +
                        "enveloped-signature" )
transforms.appendChild( transform )
digestmeth = doc.createElement( "DigestMethod" );
digestmeth.setAttribute( "Algorithm", 
                         "http://www.w3.org/2000/09/xmldsig#sha1" )
reference.appendChild( digestmeth )
digestvalue = doc.createElement( "DigestValue" );
reference.appendChild( digestvalue )
signaturevalue = doc.createElement( "SignatureValue" )
signature.appendChild( signaturevalue )
keyinfo = doc.createElement( "KeyInfo" )
signature.appendChild( keyinfo )
x509data = doc.createElement( "X509Data" )
keyinfo.appendChild( x509data )
x509subname = doc.createElement( "X509SubjectName" )
x509data.appendChild( x509subname )
x509serial = doc.createElement( "X509IssuerSerial" )
x509data.appendChild( x509serial )
x509cert = doc.createElement( "X509Certificate" )
x509data.appendChild( x509cert )
keyvalue = doc.createElement( "KeyValue" )
keyinfo.appendChild( keyvalue )

# Grrr... it would be much nicer to open a pipe to xmlsec1, but it can't
# read from standard input, so we have to use a temporary file.
tmpfile = tempfile.NamedTemporaryFile()
doc.writexml( tmpfile )
tmpfile.flush()

ret = os.spawnlp( os.P_WAIT, XMLSEC1, XMLSEC1, "--sign", "--node-id",
                  "Sig_" + str( id ), "--privkey-pem",
                  CERTIFICATE + "," + CERTIFICATE, tmpfile.name )
if ret == 127:
    print >> sys.stderr, XMLSEC1 + ": invocation error\n"

tmpfile.close()

sys.exit( ret )

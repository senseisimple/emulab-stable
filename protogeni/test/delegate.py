#! /usr/bin/env python
#
# EMULAB-COPYRIGHT
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
import os
import re
import sys
import tempfile
import uuid
import xml.dom.minidom

if len( sys.argv ) != 2:
    print >> sys.stderr, "usage: " + sys.argv[ 0 ] + " principal"
    sys.exit( 1 )

HOME            = os.environ["HOME"]
# Path to my certificate
CERTIFICATE     = HOME + "/.ssl/encrypted.pem"
XMLSEC1         = "xmlsec1"

CONFIGFILE      = ".protogeni-config.py"
GLOBALCONF      = HOME + "/" + CONFIGFILE
LOCALCONF       = CONFIGFILE

if os.path.exists( GLOBALCONF ):
    execfile( GLOBALCONF )
if os.path.exists( LOCALCONF ):
    execfile( LOCALCONF )

# Look up an element (which must exist exactly once) within a node.
def Lookup( node, name ):
    cnodes = filter( lambda x: x.nodeName == name, node.childNodes )

    if len( cnodes ) != 1:
        print >> sys.stderr, sys.argv[ 0 ] + ": invalid credential\n"
        sys.exit( 1 )
    
    return cnodes[ 0 ]

def SimpleNode( d, elem, body ):
    n = d.createElement( elem );
    n.appendChild( d.createTextNode( body ) );
    return n;

principal = open( sys.argv[ 1 ] )
owner = re.search( r"^-----BEGIN CERTIFICATE-----\s*(.*)" +
                   "^-----END CERTIFICATE-----$", principal.read(),
                   re.MULTILINE | re.DOTALL ).group( 1 )
principal.close()

doc = xml.dom.minidom.parse( sys.stdin )

old = Lookup( doc.documentElement, "credential" )

c = doc.createElement( "credential" )

id = 1
while filter( lambda x: x.getAttribute( "xml:id" ) == "ref" + str( id ),
              doc.getElementsByTagName( "credential" ) ):    
    id = id + 1
c.setAttribute( "xml:id", "ref" + str( id ) )
c.appendChild( Lookup( old, "type" ).cloneNode( True ) )

c.appendChild( SimpleNode( doc, "serial", "1" ) )

c.appendChild( SimpleNode( doc, "owner_gid", owner ) )

c.appendChild( Lookup( old, "target_gid" ).cloneNode( True ) )

c.appendChild( SimpleNode( doc, "uuid", str( uuid.uuid4() ) ) )

t = datetime.datetime.utcnow() + datetime.timedelta( hours = 6 )
t = t.replace( microsecond = 0 )
c.appendChild( SimpleNode( doc, "expires", t.isoformat() ) )

# FIXME allow an option to specify that only a proper subset of privileges
# are propagated (or even a a different set specified, even though that would
# presumably cause the credentials to be rejected).
for n in old.childNodes:
    if n.nodeName in ( "privileges", "capabilities", "ticket", "extensions" ):
        c.appendChild( n.cloneNode( True ) )

doc.documentElement.replaceChild( c, old )
p = doc.createElement( "parent" )
p.appendChild( old )
c.appendChild( p )

signature = doc.createElement( "Signature" );
signature.setAttribute( "xml:id", "Sig_ref" + str( id ) )
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
reference.setAttribute( "URI", "#ref" + str( id ) )
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
                  "Sig_ref" + str( id ), "--privkey-pem",
                  CERTIFICATE + "," + CERTIFICATE, tmpfile.name )
if ret == 127:
    print >> sys.stderr, XMLSEC1 + ": invocation error\n"

tmpfile.close()

sys.exit( ret )

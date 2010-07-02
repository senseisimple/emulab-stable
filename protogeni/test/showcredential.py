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

import os
import re
import sys
import xml.dom.minidom

OPENSSL = "/usr/local/bin/openssl"

# Look up an element (which must exist exactly once) within a node.
def Lookup( node, name ):
    cnodes = filter( lambda x: x.nodeName == name, node.childNodes )

    if len( cnodes ) != 1:
        raise Exception( "invalid credential" )
    
    return cnodes[ 0 ]

def Text( node ):
    nodes = filter( lambda x: x.nodeType == x.TEXT_NODE, node.childNodes )
    return reduce( lambda x, y: x + y, map( lambda x: x.nodeValue, nodes ), "" )

def Decode( gid ):
    ( p1r, p1w ) = os.pipe()
    if not os.fork():
        p = os.fdopen( p1w, "w" )
        print >> p, "-----BEGIN CERTIFICATE-----"
        print >> p, gid,
        print >> p, "-----END CERTIFICATE-----"
        p.close()
        os._exit( 0 )
    os.close( p1w )
    ( p2r, p2w ) = os.pipe()
    if not os.fork():
        os.close( p2r )
        os.dup2( p1r, 0 )
        os.dup2( p2w, 1 )
        os.close( p1r )
        os.close( p2w )
        os.execl( OPENSSL, OPENSSL, "x509", "-text", "-noout" )
        os._exit( 1 )
    os.close( p1r )
    os.close( p2w )

    f = os.fdopen( p2r, "r" )
    s = f.read()
    f.close()
    return s

def SubjectName( cert ):
    return ( re.search( r"X509v3 Subject Alternative Name:[ \t]*\n[ \t]*.*URI:"
                        "(urn:publicid:[-!$%()*+.0-9:;=?@A-Z_a-z~]+)", \
                        cert ) or \
             re.search( r"Subject: .*OU=([-\w.]+)", cert ) ).group( 1 )

def ShowCredential( cred, level ):

    if level == 0:
        print "Credential:"
    elif level == 1:
        print "Parent:"
    elif level == 2:
        print "Grandparent:"
    elif level == 3:
        print "Great-grandparent:"
    else:
        print "Great(" + str( level - 2 ) + ")-grandparent:"

    type = Text( Lookup( cred, "type" ) )
    print "    Type: " + type
    print "    Serial: " + Text( Lookup( cred, "serial" ) )
    owner = Decode( Text( Lookup( cred, "owner_gid" ) ) )
    target = Decode( Text( Lookup( cred, "target_gid" ) ) )

    print "    Owner: " + SubjectName( owner )
    print "    Target: " + SubjectName( target )
    try:
        # look for deprecated UUID
        print "    UUID: " + Text( Lookup( cred, "uuid" ) )
    except Exception:
        pass
    print "    Expires: " + Text( Lookup( cred, "expires" ) )

    if type == "privilege":
        print "    Privileges:"
        privs = Lookup( cred, "privileges" )
        plist = filter( lambda x: x.nodeName == "privilege", privs.childNodes )
        for p in plist:
            name = Text( Lookup( p, "name" ) )
            delegate = Text( Lookup( p, "can_delegate" ) )
            print "        " + name,
            if delegate == "1":
                print "(can be delegated)"
            else:
                print "(cannot be delegated)"
    elif type == "capability":
        pass
    elif type == "ticket":
        pass

    print

    try:
        parent = Lookup( cred, "parent" )
    except Exception:
        return
    
    # Pretend Python has better tail recursion support than it really does.
    ShowCredential( Lookup( parent, "credential" ), level + 1 )

if len( sys.argv ) > 2:
    print "usage: " + sys.argv[ 0 ] + " [filename]"
    sys.exit( 1 )

if len( sys.argv ) > 1:
    docsrc = sys.argv[ 1 ]
else:
    docsrc = sys.stdin

doc = xml.dom.minidom.parse( docsrc )

scred = Lookup( doc, "signed-credential" )
cred = Lookup( scred, "credential" )
ShowCredential( cred, 0 )

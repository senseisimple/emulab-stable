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

import getopt
import os
import re
import socket
import sys
import xml.dom.minidom
import xmlrpclib
from M2Crypto import X509

ACCEPTSLICENAME=1

def Usage():
    print "usage: " + sys.argv[ 0 ] + " [option...] [command...]"
    print """Options:
    -c file, --credentials=file         read self-credentials from file
                                            [default: query from SA]
    -d, --debug                         be verbose about XML methods invoked
    -f file, --certificate=file         read SSL certificate from file
                                            [default: ~/.ssl/encrypted.pem]
    -h, --help                          show options and usage
    -n name, --slicename=name           specify human-readable name of slice
                                            [default: mytestslice]
    -p file, --passphrase=file          read passphrase from file
                                            [default: ~/.ssl/password]
    -r file, --read-commands=file       specify additional configuration file
    -s file, --slicecredentials=file    read slice credentials from file
                                            [default: query from SA]
Commands:
    copy <source> <dest>        copy local dir/file <source> to remote <dest>
    exec <command>              run <command> on slice nodes
    install <file>              extract tape archive <file> onto slice nodes
    list                        show IP addresses of node control interfaces"""

execfile( "test-common.py" )

NOTREADY = "---notready---"
RSYNC = "/usr/local/bin/rsync"
SSH = "/usr/bin/ssh"

#
# Parse command line.
#
commands = []
while args:
    command = args.pop( 0 )
    if command == "copy":
        if len( args ) < 2: Fatal( sys.argv[ 0 ] +
                                ": command \"copy\" requires two parameters" )
        source = args.pop( 0 )
        dest = args.pop( 0 )
        commands.append( ( command, source, dest ) )
    elif command == "install":
        if not args: Fatal( sys.argv[ 0 ] + 
                            ": command \"install\" requires a parameter" )
        file = args.pop( 0 )
        commands.append( ( command, file ) )
    elif command == "exec":
        if not args: Fatal( sys.argv[ 0 ] +
                            ": command \"exec\" requires a parameter" )
        remotecmd = args.pop( 0 )
        commands.append( ( command, remotecmd ) )
    elif command == "list":
        commands.append( ( command, ) )
    else:
        Fatal( sys.argv[ 0 ] + ": unknown command " + command )

#
# Get a credential for the SA.
#
mycredential = get_self_credential()
print "Got SA credential.  Looking up slice " + SLICENAME + "..."

#
# Look up the slice and obtain credentials.
#
slice = resolve_slice( SLICENAME, mycredential )
print "Found slice " + SLICENAME + ", asking for a credential..."

slicecred = get_slice_credential( slice, mycredential )
print "Got a slice credential.  Searching for component managers..."

#
# Ask the clearinghouse for a list of component managers. 
#
params = {}
params[ "credential" ] = mycredential
rval,response = do_method( "ch", "ListComponents", params )
if rval:
    Fatal( "Could not get a list of components from the clearing house." )
    pass
if debug: print str( response[ "value" ] )

#
# Ask each manager for its relevant nodes.  We talk to the component
# managers in parallel, but send each of its requests serially: this
# ought to allow a reasonable amount of concurrency without excessively
# burdening any individual server.
#
params = { "slice_urn" : slice[ "urn" ], "credentials" : [ slicecred ] }
( pr, pw ) = os.pipe()

for cm in response[ "value" ]:
    if debug: print cm[ "url" ]
    if not os.fork():
        # Child process: give a list of nodes to our parent, or complain
        # that we're not ready.
        os.close( pr )
        p = os.fdopen( pw, "w" )

        def Say( text ):
            if len( text ) + 1 > os.fpathconf( pw, "PC_PIPE_BUF" ):
                # This should never happen.  POSIX guarantees that at least
                # 512 bytes can be sent atomically, and we only ever need
                # to send dotted decimal IPv4 addresses and short error
                # messages.  Neither of those should be anywhere near
                # the limit.
                Fatal( "Message is too long for pipe atomicity" )

            print >> p, text

        try:
            rval, response = do_method( None, "SliverStatus", params, \
                                            cm[ "url" ], True )
        except:
            print >> sys.stderr, "Warning: could not obtain sliver status " + \
                "from " + cm[ "url" ]
            os._exit( 1 )

        if not rval:
            if response[ "value" ][ "status" ] == "ready":
                # There is a sliver here for us.  Look up its name...
                rval, response = do_method( None, "Resolve", 
                                            { "urn" : slice[ "urn" ],
                                              "credentials" : [ slicecred ] },
                                            cm[ "url" ], True )
                if rval:
                    print >> sys.stderr, "Could not resolve slice " + \
                        slice[ "urn" ]
                    os._exit( 1 )

                sliver = response[ "value" ][ "sliver_urn" ];

                # Now we have the sliver name, and can resolve it to
                # obtain the manifest.
                rval, response = do_method( None, "Resolve", 
                                            { "urn" : sliver,
                                              "credentials" : [ slicecred ] },
                                            cm[ "url" ], True )
                if rval:
                    print >> sys.stderr, "Could not resolve sliver " + sliver
                    os._exit( 1 )

                # Wade through the manifest, and look for all node elements,
                # noting the host name of each one.
                doc = xml.dom.minidom.parseString( response[ "value" ] \
                                                       [ "manifest" ] )

                for node in filter( lambda x: x.nodeName == "node",
                                    doc.documentElement.childNodes ):
                    if node.hasAttribute( "hostname" ):
                        Say( socket.gethostbyname( node.getAttribute( \
                                    "hostname" ) ) )
            else:
                Say( NOTREADY )

        p.close()
        os._exit( 0 )

os.close( pw )
p = os.fdopen( pr, "r" )
nodes = []
for line in p:
    addr = line.rstrip()
    if addr == NOTREADY: Fatal( "Slice is not ready" )
    # Check for RFC 1918 private address space
    m = re.match( r"([0-9]+)\.([0-9]+)\.[0-9]+\.[0-9]+", addr )
    if not m:
        print >> sys.stderr, "Warning: malformed address " + addr
        continue
    octet0 = m.group( 1 )
    octet1 = m.group( 2 )
    if ( octet0 == 10 ) or ( octet0 == 172 and 16 <= octet1 < 32 ) or \
            ( octet0 == 192 and octet1 == 168 ):
        # private address for control interface; silently ignore the node
        continue
    nodes.append( addr )
p.close()

#
# Clean up our zombies.
#
for cm in response[ "value" ]:
    os.wait()

# Sort the list of nodes: not really necessary, but perhaps it will help
# the user get the same output on each run even though the list was
# generated in non-deterministic order.
nodes.sort()

for command in commands:
    if command[ 0 ] == "copy":
        print "Copying \"" + command[ 1 ] + "\" to \"" + command[ 2 ] + "\"..."
                
        child = {}
        for node in nodes:
            pid = os.fork()
            if pid:
                child[ pid ] = node
            else:
                os.execl( RSYNC, "rsync", "-a", "-e",
                          SSH + " -T -o 'BatchMode yes' " +
                          "-o 'StrictHostKeyChecking no' " +
                          "-o 'EscapeChar none'",
                          command[ 1 ], node + ":" + command[ 2 ] )
                os._exit( 1 )
        for node in nodes:
            ( pid, status ) = os.wait()
            print "  (finished on node " + child[ pid ] + ")"
    elif command[ 0 ] == "exec":
        print "Executing command \"" + command[ 1 ] + "\"..."
        for node in nodes:
            if not os.fork():
                devnull = os.open( "/dev/null", os.O_WRONLY )
                os.dup2( devnull, 1 )
                os.dup2( devnull, 2 )
                os.close( devnull )
                os.execl( SSH, "ssh", "-n", "-T", "-o", "BatchMode yes", 
                          "-o", "StrictHostKeyChecking no", node, command[ 1 ] )
                os._exit( 1 )
        for node in nodes:
            os.wait()
    elif command[ 0 ] == "install":
        print "Installing file \"" + command[ 1 ] + "\"..."
        # This is a bit ugly.  Sufficiently recent versions of GNU tar can
        # do all the decompression stuff transparently, but it's too hard
        # for us to figure out what is installed at the remote end, so we
        # have to do it as primitively as possible.
        remotecmd = "tar xpPf -"
        try:
            payload = open( command[ 1 ] )            
        except IOError, e:
            print >> sys.stderr, command[ 1 ] + ": " + e.strerror
            continue
        header = payload.read( 3 )
        payload.close()
        if header[ 0:2 ] == "\037\235": 
            remotecmd = "uncompress -c | " + remotecmd
        elif header[ 0:2 ] == "\037\213":
            remotecmd = "gzip -c -d | " + remotecmd
        elif header[ 0:3 ] == "BZh":
            remotecmd = "bzip2 -c -d | " + remotecmd

        child = {}
        for node in nodes:
            pid = os.fork()
            if pid:
                child[ pid ] = node
            else:
                infile = os.open( command[ 1 ], os.O_RDONLY )
                os.dup2( infile, 0 )
                devnull = os.open( "/dev/null", os.O_WRONLY )
                os.dup2( devnull, 1 )
                os.dup2( devnull, 2 )
                os.close( devnull )
                os.execl( SSH, "ssh", "-T", "-o", "BatchMode yes", 
                          "-o", "StrictHostKeyChecking no", 
                          "-o", "EscapeChar none", node,
                          remotecmd )
                os._exit( 1 )
        for node in nodes:
            ( pid, status ) = os.wait()
            print "  (finished on node " + child[ pid ] + ")"
    elif command[ 0 ] == "list":
        for node in nodes:
            print node

"""
Copyright (c) 2002 Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met: 

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
      
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
      
    * Neither the name of the Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.
      
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

EXPORT LAWS: THIS LICENSE ADDS NO RESTRICTIONS TO THE EXPORT LAWS OF
YOUR JURISDICTION. It is licensee's responsibility to comply with any
export regulations applicable in licensee's jurisdiction. Under
CURRENT (May 2000) U.S. export regulations this software is eligible
for export from the U.S. and can be downloaded by or otherwise
exported or reexported worldwide EXCEPT to U.S. embargoed destinations
which include Cuba, Iraq, Libya, North Korea, Iran, Syria, Sudan,
Afghanistan and any other country to which the U.S. has embargoed
goods and services.

DESCRIPTION: Node manager XML-RPC server that handles requests that
don't require authentication and encryption.

AUTHOR: Brent Chun (bnc@intel-research.net)

$Id: nodemgrsvr.py,v 1.1 2003-08-19 17:17:22 aclement Exp $

"""
import threading
import SocketServer
from xmlrpcserver import RequestHandler

class nodemgrhandler(RequestHandler):
    def call(self, method, args):
        if method == "getconfig":
            result = self.server.getconfig()
        elif method == "getleases":
            result = self.server.getleases()
        elif method == "getslivers":
            result = self.server.getslivers()
        elif method == "getprinciple":
            result = self.server.getprinciple(args)
        elif method == "getsshkeys":
            result = self.server.getsshkeys(args)
        else:
            raise NotImplementedError, method + " not implemented"
        return result 

    def finish(self):
        self.request.close()

class nodemgrsvr(SocketServer.TCPServer):
    def __init__(self, server_address, RequestHandlerClass, nodemgr):
        import hacks
        SocketServer.TCPServer.allow_reuse_address = 1
        try:
            method = SocketServer.TCPServer.__init__
            args = [ self, server_address, RequestHandlerClass ]
            hacks.retryapply(method, args, 10, 1)
        except:
            raise "Could not bind to TCP port %d" % server_address[1]
        self.nodemgr = nodemgr
        self.dbthr = nodemgr.dbthr

    def getconfig(self):
        import nodemgr
        config = {}
        config["port"] = nodemgr.PORT
        config["sslport"] = nodemgr.SSLPORT
        config["httpport"] = nodemgr.HTTPPORT
        return config

    def getleases(self):
        return self.dbthr.getleases()

    def getslivers(self):
        return self.dbthr.getslivers()

    def getprinciple(self, args):
        slice = args[0]["slice"]
        principle = self.dbthr.getprinciple(slice)
        if not principle:
            raise "Slice %s does not have sliver on node" % slice
        return principle

    def getsshkeys(self, args):
        """Return list of stripped SSH pubkeys""" 
        import digest
        from nodemgrsslsvr import slurpkeys
        slice = args[0]["slice"]
        if not self.dbthr.getprinciple(slice):
            raise "Slice %s does not have sliver on node" % slice
        keys = slurpkeys(slice)
        if keys:
            keys = map(lambda key: key.strip(), keys)
            rv = { "numkeys" : len(keys), "keys" : keys }
        else:
            rv = { "numkeys" : 0 }
        return rv
        
class nodemgrsvrthr(threading.Thread):
    def __init__(self, nodemgr):
        threading.Thread.__init__(self)
        self.server = nodemgrsvr(("", nodemgr.conf.xmlport), nodemgrhandler, nodemgr)
        
    def run(self):
        self.server.serve_forever()

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

DESCRIPTION: Agent XML-RPC server that handles requests that don't
require authentication and encryption.

AUTHOR: Brent Chun (bnc@intel-research.net)

$Id: agentsvr.py,v 1.1 2003-08-19 17:17:19 aclement Exp $

"""
import threading
import SocketServer
from xmlrpcserver import RequestHandler
import agent

class agenthandler(RequestHandler):
    def call(self, method, args):
        if method == "getconfig":
            result = self.server.getconfig()
        elif method == "getads":
            result = self.server.getads()
        elif method == "gettickets":
            result = self.server.gettickets()
        elif method == "getslices":
            result = self.server.getslices()
        else:
            raise NotImplementedError, method + " not implemented"
        return result 

    def finish(self):
        self.request.close()

class agentsvr(SocketServer.TCPServer):
    def __init__(self, server_address, RequestHandlerClass, agent):
        import hacks
        SocketServer.TCPServer.allow_reuse_address = 1
        try:
            method = SocketServer.TCPServer.__init__
            args = [ self, server_address, RequestHandlerClass ]
            hacks.retryapply(method, args, 10, 1)
        except:
            raise "Could not bind to TCP port %d" % server_address[1]
        self.agent = agent

    def getconfig(self):
        config = {}
        config["port"] = agent.PORT
        config["sslport"] = agent.SSLPORT
        config["httpport"] = agent.HTTPPORT
        config["maxlease"] = self.agent.conf.maxlease
        return config

    def getads(self):
        return self.agent.gmetadthr.getips()

    def gettickets(self):
        return self.agent.dbthr.gettickets()

    def getslices(self):
        return self.agent.dbthr.getslices()

class agentsvrthr(threading.Thread):
    def __init__(self, agent):
        threading.Thread.__init__(self)
        self.server = agentsvr(("", agent.conf.xmlport), agenthandler, agent)

    def run(self):
        self.server.serve_forever()

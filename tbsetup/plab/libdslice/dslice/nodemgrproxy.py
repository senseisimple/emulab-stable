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

DESCRIPTION: Proxy class that implements client end of XML-RPC
communication with a node manager.

AUTHOR: Brent Chun (bnc@intel-research.net)

$Id: nodemgrproxy.py,v 1.1 2003-08-19 17:17:22 aclement Exp $

"""
from xmlrpclib import ServerProxy
from M2Crypto.m2xmlrpclib import SSL_Transport
from sslctx import clictxinit

class nodemgrproxy:
    def __init__(self, host, port, sslport=None, key=None, cert=None, cacert=None):
        self.host = host
        self.port = port
        self.sslport = sslport
        self.key = key
        self.cert = cert
        self.cacert = cacert

    def getconfig(self):
        s = ServerProxy("http://%s:%d" % (self.host, self.port))
        return s.getconfig()

    def getleases(self):
        s = ServerProxy("http://%s:%d" % (self.host, self.port))
        return s.getleases()

    def getslivers(self):
        s = ServerProxy("http://%s:%d" % (self.host, self.port))
        return s.getslivers()

    def getprinciple(self, slice):
        s = ServerProxy("http://%s:%d" % (self.host, self.port))
        params = { "slice" : slice }
        return s.getprinciple(params)

    def getsshkeys(self, slice):
        s = ServerProxy("http://%s:%d" % (self.host, self.port))
        params = { "slice" : slice }
        return s.getsshkeys(params)

    def newlease(self, ticketdata):
        ctx = clictxinit(self.cert, self.key, self.cacert)
        s = ServerProxy("https://%s:%d" % (self.host, self.sslport), SSL_Transport(ctx))
        params = { "ticketdata" : ticketdata }
        return s.newlease(params)

    def newvm(self, leasedata, privatekey, publickey):
        ctx = clictxinit(self.cert, self.key, self.cacert)
        s = ServerProxy("https://%s:%d" % (self.host, self.sslport), SSL_Transport(ctx))
        params = { "leasedata" : leasedata, "privatekey" : privatekey,
                   "publickey" : publickey }
        return s.newvm(params)

    def newleasevm(self, ticketdata, privatekey, publickey):
        ctx = clictxinit(self.cert, self.key, self.cacert)
        s = ServerProxy("https://%s:%d" % (self.host, self.sslport), SSL_Transport(ctx))
        params = { "ticketdata" : ticketdata, "privatekey" : privatekey,
                   "publickey" : publickey }
        return s.newleasevm(params)

    def deletelease(self, slice):
        ctx = clictxinit(self.cert, self.key, self.cacert)
        s = ServerProxy("https://%s:%d" % (self.host, self.sslport), SSL_Transport(ctx))
        params = { "slice" : slice }
        return s.deletelease(params)

    def renewlease(self, slice):
        ctx = clictxinit(self.cert, self.key, self.cacert)
        s = ServerProxy("https://%s:%d" % (self.host, self.sslport), SSL_Transport(ctx))
        params = { "slice" : slice }
        return s.renewlease(params)

    def addkey(self, slice, key):
        ctx = clictxinit(self.cert, self.key, self.cacert)
        s = ServerProxy("https://%s:%d" % (self.host, self.sslport), SSL_Transport(ctx))
        params = { "slice" : slice, "key" : key }
        return s.addkey(params)

    def delkey(self, slice, key):
        ctx = clictxinit(self.cert, self.key, self.cacert)
        s = ServerProxy("https://%s:%d" % (self.host, self.sslport), SSL_Transport(ctx))
        params = { "slice" : slice, "key" : key }
        return s.delkey(params)

    def nukekeys(self, slice):
        ctx = clictxinit(self.cert, self.key, self.cacert)
        s = ServerProxy("https://%s:%d" % (self.host, self.sslport), SSL_Transport(ctx))
        params = { "slice" : slice }
        return s.nukekeys(params)

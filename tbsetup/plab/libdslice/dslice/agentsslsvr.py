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

DESCRIPTION: Agent XML-RPC server that handles requests that require
authentication and encryption over SSL.

AUTHOR: Brent Chun (bnc@intel-research.net)

$Id: agentsslsvr.py,v 1.2 2003-09-13 00:23:03 ricci Exp $

"""
import threading
from xmlrpcserver import RequestHandler
from M2Crypto import SSL
import sslhandler

class agentsslhandler(sslhandler.sslhandler):
    def call(self, method, args):
        principle = self.getpeerkeysha1()
        subject = self.getpeersubject()
        if method == "newtickets":
            result = self.server.newtickets(principle, subject, args)
        else:
            raise NotImplementedError, method + " not implemented"
        return result 

class agentsslsvr(SSL.ThreadingSSLServer):
    def __init__(self, conf, agent):
        from sslctx import svrctxinit
        import hacks, ticket
        self.conf = conf
        self.agent = agent
        self.tfactory = ticket.ticketfactory(conf.key)
        ctx = svrctxinit(conf.cert, conf.key, conf.cacert, "agentsslsvr")
        SSL.ThreadingSSLServer.allow_reuse_address = 1
        try:
            method = SSL.ThreadingSSLServer.__init__
            args = [ self, ("", conf.xmlsslport), agentsslhandler, ctx ]
            hacks.retryapply(method, args, 10, 1)
        except:
            raise "Could not bind to TCP port %d" % conf.xmlsslport
        
    def newtickets(self, principle, subject, args):
        import policy, ticket
        ticketsdata = []
        try:
            self.agent.rlock.acquire()
            allips = self.agent.gmetadthr.getips()
            slice = args[0]["slice"].strip()
            ntickets = args[0]["ntickets"]
            leaselen = args[0]["leaselen"]
            ips = args[0]["ips"]
            pol = policy.policy(allips)
            self.newtickets_checkargs(slice, ntickets, leaselen, self.agent, pol, ips)
            for ip in pol.getips(ntickets, ips):
                t = self.tfactory.createticket(principle, ip, slice, leaselen)
                self.agent.dbthr.addticket(t.data, subject)
                ticketsdata.append(t.data)
        finally:
            self.agent.rlock.release()
        return ticketsdata

    def newtickets_checkargs(self, slice, ntickets, leaselen, agent, policy, ips):
        import slicename, sys
        ticketips = policy.getips(ntickets, ips)
        maxlease = self.agent.conf.maxlease
        if slicename.islogin(slice):
            raise "Login %s already exists" % slice
        if slicename.isstaticslice(slice):
            raise "Static slice %s already exists" % slice
        if not slicename.isvalidlogin(slice):
            raise "Slice name %s is a bad name" % slice
#        if agent.dbthr.hastickets(slice):
#            raise "Outstanding tickets for slice %s" % slice
        if agent.gmetadthr.leaseexists(slice):
            raise "Outstanding leases for slice %s" % slice
        if agent.gmetadthr.sliceexists(slice):
            raise "Outstanding slivers for slice %s" % slice
        if ntickets <= 0:
            raise "Requested negative #tickets: %d." % ntickets
        if ntickets > len(ticketips):
            raise "Wanted %d tickets. %d available." % (ntickets, len(ticketips))
        if leaselen <= 0:
            raise "Requested negative lease length: %d" % leaselen
        if leaselen > maxlease:
            raise "Requested lease length %d, max is %d" % (leaselen, maxlease)

    def _deletetickets(self, ticketsdata):
        try:
            self.agent.rlock.acquire()
            for ticketdata in ticketsdata:
                self.agent.dbthr.removeticket(ticketdata)
        finally:
            self.agent.rlock.release()

class agentsslsvrthr(threading.Thread):
    def __init__(self, agent):
        threading.Thread.__init__(self)
        self.server = agentsslsvr(agent.conf, agent)

    def run(self):
        self.server.serve_forever()

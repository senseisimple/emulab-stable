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

DESCRIPTION: Agent database thread that maintains a mapping between
ticket data and slice names, keeps track of outstanding tickets for
slices, and processes ticket expiration events. Ticket data is stored
on disk in a database file for crash recovery.

AUTHOR: Brent Chun (bnc@intel-research.net)

$Id: agentdbthr.py,v 1.1 2003-08-19 17:17:18 aclement Exp $

"""
import calendar
import threading
import time
import event
import ticket

class agentdbthr(threading.Thread):
    def __init__(self, agent):
        import anydbm, os
        threading.Thread.__init__(self)
        self.waitlock = threading.Lock()
        self.waitcv = threading.Condition(self.waitlock)
        self.agent = agent
        self.db = anydbm.open(agent.conf.ticketdb, "c", 0644)
        self.slices = {}
        self.events = event.eventqueue(0)

    def postinit(self):
        for ticketdata in self.db.keys():
            t = ticket.ticket(ticketdata)
            if t.slice in self.slices:
                self.slices[t.slice] += 1
            else:
                self.slices[t.slice] = 1
            expiration = calendar.timegm(time.strptime(t.validto_time,
                                                       "%Y-%m-%d %H:%M:%S"))
            sslsvr = self.agent.sslsvrthr.server
            self.events.put(event.tickexpevent(expiration, ticketdata, sslsvr))
            self.agent.logfile.log("Adding a ticket expiration for slice %s at %s"
                                   % (t.slice, expiration))

    def run(self):
        import sys
        while 1:
            self.agent.rlock.acquire()
            now = calendar.timegm(time.gmtime())
            self.expiretickets(now)
            if self.events.qsize() != 0:
                waittime = self.events.top().time - now
            else:
                waittime = sys.maxint
            self.agent.rlock.release()
            self.waitlock.acquire()
            assert waittime >= 0
            self.waitcv.wait(waittime)
            self.waitlock.release()

    def wakeup(self):
        self.waitlock.acquire()
        self.waitcv.notify()
        self.waitlock.release()
        
    def addticket(self, ticketdata, subject):
        self.agent.rlock.acquire()
        t = ticket.ticket(ticketdata)
        self.db[ticketdata] = t.slice
        self.db.sync()
        if t.slice in self.slices:
            self.slices[t.slice] += 1
        else:
            self.slices[t.slice] = 1
        expiration = calendar.timegm(time.strptime(t.validto_time, "%Y-%m-%d %H:%M:%S"))
        sslsvr = self.agent.sslsvrthr.server
        self.events.put(event.tickexpevent(expiration, ticketdata, sslsvr))
        self.agent.rlock.release()
        self.wakeup()
        self.agent.logfile.log("Added ticket for slice %s for \"%s\"" % (t.slice, subject))

    def removeticket(self, ticketdata):
        self.agent.rlock.acquire()
        t = ticket.ticket(ticketdata)
        del self.db[ticketdata]
        self.db.sync()
        assert t.slice in self.slices
        if self.slices[t.slice] > 1:
            self.slices[t.slice] -= 1
        else:
            del self.slices[t.slice]
        self.agent.rlock.release()
        self.agent.logfile.log("Removed ticket for slice %s" % t.slice)

    def expiretickets(self, now):
        self.agent.rlock.acquire()
        while self.events.qsize() != 0:
            e = self.events.top()
            if e.time <= now:
                e.process()
                self.events.get()
            else:
                break
        self.agent.rlock.release()

    def gettickets(self):
        self.agent.rlock.acquire()
        ticketsdata = self.db.keys()
        self.agent.rlock.release()
        return ticketsdata

    def getslices(self):
        self.agent.rlock.acquire()
        slices = self.slices.keys()
        self.agent.rlock.release()
        return slices

    def hastickets(self, slice):
        return slice in self.slices

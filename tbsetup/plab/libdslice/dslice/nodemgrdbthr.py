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

DESCRIPTION: Node manager database thread that maintains a mapping
between lease data and slice names, a mapping between slice names and
lease data, a mapping of lease data to lease expiration events, and
processes lease expiration events. Lease data is stored on disk in a
database file for crash recovery.

AUTHOR: Brent Chun (bnc@intel-research.net)

$Id: nodemgrdbthr.py,v 1.1 2003-08-19 17:17:21 aclement Exp $

"""
import calendar
import threading
import time
import event
import lease

class nodemgrdbthr(threading.Thread):
    def __init__(self, nodemgr):
        import anydbm, os
        threading.Thread.__init__(self)
        self.waitlock = threading.Lock()
        self.waitcv = threading.Condition(self.waitlock)
        self.nodemgr = nodemgr
        self.lfactory = lease.leasefactory(nodemgr.conf.key)
        self.leasedb = anydbm.open(nodemgr.conf.leasedb, "c", 0644)
        self.sliverdb = anydbm.open(nodemgr.conf.sliverdb, "c", 0644)
        self.events = {}
        self.eventqueue = event.eventqueue(0)

    def postinit(self):
        for slice in self.leasedb.keys():
            leasedata = self.leasedb[slice]
            l = lease.lease(leasedata)
            expiration = calendar.timegm(time.strptime(l.end_time, "%Y-%m-%d %H:%M:%S"))
            sslsvr = self.nodemgr.sslsvrthr.server
            e = event.leaseexpevent(expiration, leasedata, sslsvr)
            self.events[leasedata] = e
            self.eventqueue.put(e)
            self.nodemgr.logfile.log("Adding a lease expiration for slice %s at %s"
                                     % (l.slice, expiration))
        
    def run(self):
        import sys
        while 1:
            self.nodemgr.rlock.acquire()
            now = calendar.timegm(time.gmtime())
            self.expireleases(now)
            if self.eventqueue.qsize() != 0:
                waittime = self.eventqueue.top().time - now
            else:
                waittime = sys.maxint
            self.nodemgr.rlock.release()
            self.waitlock.acquire()
            assert (waittime >= 0)
            self.waitcv.wait(waittime)
            self.waitlock.release()

    def wakeup(self):
        self.waitlock.acquire()
        self.waitcv.notify()
        self.waitlock.release()
        
    def addlease(self, leasedata, subject):
        self.nodemgr.rlock.acquire()
        l = lease.lease(leasedata)
        self.leasedb[l.slice] = leasedata
        self.leasedb.sync()
        expiration = calendar.timegm(time.strptime(l.end_time, "%Y-%m-%d %H:%M:%S"))
        sslsvr = self.nodemgr.sslsvrthr.server
        e = event.leaseexpevent(expiration, leasedata, sslsvr)
        self.events[leasedata] = e
        self.eventqueue.put(e)
        self.nodemgr.rlock.release()
        self.wakeup()
        self.nodemgr.logfile.log("Added lease (%d seconds) for slice %s for \"%s\"" %
                                 (l.leaselen, l.slice, subject))

    def removelease(self, leasedata):
        self.nodemgr.rlock.acquire()
        l = lease.lease(leasedata)
        if not l.slice in self.leasedb.keys():
            self.nodemgr.rlock.release()
            return
        del self.leasedb[l.slice]
        self.leasedb.sync()
        del self.events[leasedata]
        self.nodemgr.rlock.release()
        self.nodemgr.logfile.log("Removed lease (%d seconds) for slice %s" %
                                 (l.leaselen, l.slice))

    def renewlease(self, slice, subject):
        self.nodemgr.rlock.acquire()
        oldlease = self.lookuplease(slice)
        assert oldlease.data in self.events
        self.eventqueue.delete(self.events[oldlease.data])
        self.removelease(oldlease.data)
        now = time.gmtime()
        start = calendar.timegm(now)
        end = start + oldlease.leaselen
        start_time = time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime(start))
        end_time = time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime(end))
        newlease = self.lfactory.createlease(oldlease.principle, oldlease.ip,
                                             oldlease.slice, start_time, end_time)
        self.addlease(newlease.data, subject)
        self.renewsliver(slice, newlease.data, subject)
        self.nodemgr.rlock.release()
        self.nodemgr.logfile.log("Renewed lease (%d seconds) for slice %s for \"%s\"" %
                                 (newlease.leaselen, newlease.slice, subject))
        return newlease.data

    def expireleases(self, now):
        self.nodemgr.rlock.acquire()
        while self.eventqueue.qsize() != 0:
            e = self.eventqueue.top()
            if e.time <= now:
                e.process()
                self.eventqueue.get()
            else:
                break
        self.nodemgr.rlock.release()

    def addsliver(self, slice, leasedata, subject):
        import pwd
        self.nodemgr.rlock.acquire()
        assert not slice in self.sliverdb.keys()
        self.sliverdb[slice] = leasedata
        self.sliverdb.sync()
        self.nodemgr.rlock.release()
        passwdent = pwd.getpwnam(slice)
        uid, gid = passwdent[2], passwdent[3]
        msg = "Added sliver for slice %s (uid/gid = %d/%d) for \"%s\"" % \
              (slice, uid, gid, subject)
        self.nodemgr.logfile.log(msg)

    def removesliver(self, slice):
        self.nodemgr.rlock.acquire()
        assert slice in self.sliverdb.keys()
        del self.sliverdb[slice]
        self.sliverdb.sync()
        self.nodemgr.rlock.release()

    def renewsliver(self, slice, leasedata, subject):
        import pwd
        self.nodemgr.rlock.acquire()
        self.sliverdb[slice] = leasedata
        self.sliverdb.sync()
        self.nodemgr.rlock.release()
        passwdent = pwd.getpwnam(slice)
        uid, gid = passwdent[2], passwdent[3]
        msg = "Renewed sliver for slice %s (uid/gid = %d/%d) for \"%s\"" % \
              (slice, uid, gid, subject)
        self.nodemgr.logfile.log(msg)

    def lookuplease(self, slice):
        self.nodemgr.rlock.acquire()
        if slice in self.leasedb.keys():
            l = lease.lease(self.leasedb[slice])
        else:
            l = None
        self.nodemgr.rlock.release()
        return l

    def lookupsliver(self, slice):
        self.nodemgr.rlock.acquire()
        if slice in self.sliverdb.keys():
            l = lease.lease(self.sliverdb[slice])
        else:
            l = None
        self.nodemgr.rlock.release()
        return l

    def getleases(self):
        self.nodemgr.rlock.acquire()
        leases = []
        for slice in self.leasedb.keys():
            leases.append(self.leasedb[slice])
        self.nodemgr.rlock.release()
        return leases

    def getslivers(self):
        self.nodemgr.rlock.acquire()
        slivers = self.sliverdb.keys()
        self.nodemgr.rlock.release()
        return slivers

    def getprinciple(self, slice):
        self.nodemgr.rlock.acquire()
        l = self.lookuplease(slice)
        if l:
            principle = l.principle
        else:
            principle = None
        self.nodemgr.rlock.release()
        return principle    

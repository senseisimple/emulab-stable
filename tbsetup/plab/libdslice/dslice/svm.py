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

DESCRIPTION: Simple implementation of a service manager for slices.
Handles slice creation, deletion, and renewal. Also handles adding and
removing SSH keys on slivers comprising a slice and supports several
commands for probing agent and node manager state.

AUTHOR: Brent Chun (bnc@intel-research.net)

$Id: svm.py,v 1.2 2003-09-13 00:23:03 ricci Exp $

"""
import agent, agentproxy, broker, calendar, digest, fileutil, lease
import nodemgr, nodemgrproxy, os, re, shutil, sys, threading, ticket
import time, xmlconf

class svmthread(threading.Thread):
    def __init__(self, ip, method, inargs, outargs, flags, sem):
        threading.Thread.__init__(self)
        self.ip = ip
        self.method = method
        self.inargs = inargs
        self.outargs = outargs
        self.flags = flags
	self.sem = sem

    def run(self):
        from tcppinger import tcppinger
        try:
            if self.flags["timeout"]:
                pinger = tcppinger(self.flags["timeout"])
                if not pinger.pingable(self.ip, nodemgr.HTTPPORT):
                    raise "Could not connect to %s" % self.ip
            outarg = apply(self.method, self.inargs[self.ip])
            self.outargs[self.ip] = outarg
        except:
            print "Error on %s: %s" % (self.ip, sys.exc_info())
        else:
            print "Success on %s" % self.ip
	self.sem.release()
        
class svm:
    def __init__(self, clientconf, brokerconf):
        self.clientconf = clientconf
        self.brokerconf = brokerconf
        self.maxwait = 300  # Somewhat arbitrary
        self.slicesdir = "%s" % clientconf.slicesdir
        if not os.path.exists(self.slicesdir):
            os.makedirs(self.slicesdir)
            
    def setdirs(self, slice):
        self.slicedir = "%s/%s" % (self.slicesdir, slice)
        self.ticketsdir = "%s/%s/tickets" % (self.slicesdir, slice)
        self.leasesdir = "%s/%s/leases" % (self.slicesdir, slice)
        self.vmsdir = "%s/%s/vms" % (self.slicesdir, slice)
        self.sshkeysdir = "%s/%s/sshkeys" % (self.slicesdir, slice)
        self.slicekeypairdir = "%s/%s/slicekeypair" % (self.slicesdir, slice)
        for dir in [ self.slicedir, self.ticketsdir, self.leasesdir, 
                     self.vmsdir, self.sshkeysdir, self.slicekeypairdir ]:
            if not os.path.exists(dir):
                os.makedirs(dir)

    def sliceconfinit(self, xmlfile):
        self.sliceconf = xmlconf.sliceconf(xmlfile)
        self.setdirs(self.sliceconf.name)

    def nodemgrproxy(self, ip):
        return nodemgrproxy.nodemgrproxy(ip, nodemgr.PORT, nodemgr.SSLPORT,
                                         self.clientconf.key, self.clientconf.cert,
                                         self.clientconf.cacert)

    def docommand(self, cmd, args, flags):
        if cmd == "createslice":
            if len(args) != 1: self.printusage(cmd)
            return self.createslice(args, flags)
        elif cmd == "newtickets":
            if len(args) != 1: self.printusage(cmd)
            return self.newtickets(args, flags)
        elif cmd == "newleases":
            if len(args) < 2: self.printusage(cmd)
            return self.newleases(args, flags)
        elif cmd == "newvms":
            if len(args) < 2: self.printusage(cmd)
            return self.newvms(args, flags)
        elif cmd == "deleteslice":
            if len(args) != 1: self.printusage(cmd)
            return self.deleteslice(args, flags)
        elif cmd == "renewslice":
            if len(args) != 1: self.printusage(cmd)
            return self.renewslice(args, flags)
        elif cmd == "addkey":
            if len(args) != 2: self.printusage(cmd)
            return self.addkey(args, flags)
        elif cmd == "delkey":
            if len(args) != 2: self.printusage(cmd)
            return self.delkey(args, flags)
        elif cmd == "nukekeys":
            if len(args) != 1: self.printusage(cmd)
            return self.nukekeys(args, flags)            
        elif cmd == "nodelist":
            if len(args) != 1: self.printusage(cmd)
            return self.nodelist(args, flags)
        elif cmd == "expirations":
            if len(args) != 1: self.printusage(cmd)
            return self.expirations(args, flags)
        elif cmd == "agentconfig":
            if len(args) != 1: self.printusage(cmd)
            return self.agentconfig(args, flags)            
        elif cmd == "nodemgrconfig":
            if len(args) != 1: self.printusage(cmd)
            return self.nodemgrconfig(args, flags)            
        elif cmd == "getads":
            if len(args) != 1: self.printusage(cmd)
            return self.getads(args, flags)
        elif cmd == "gettickets":
            if len(args) != 1: self.printusage(cmd)
            return self.gettickets(args, flags)
        elif cmd == "getslices":
            if len(args) != 1: self.printusage(cmd)
            return self.getslices(args, flags)
        elif cmd == "getleases":
            if len(args) != 1: self.printusage(cmd)
            return self.getleases(args, flags)
        elif cmd == "getslivers":
            if len(args) != 1: self.printusage(cmd)
            return self.getslivers(args, flags)
        elif cmd == "getprinciple":
            if len(args) != 2: self.printusage(cmd)
            return self.getprinciple(args, flags)
        elif cmd == "clean":
            if len(args) != 0: self.printusage(cmd)
            return self.clean()
        else:
            raise "Unsupported command: %s" % cmd

    def checkreachable(self, host, port, timeout):
        """Check if host:port reachable over TCP in timeout seconds"""
        from tcppinger import tcppinger
        import sys
        assert timeout
        pinger = tcppinger(timeout)
        if not pinger.pingable(host, port):
            print "Error on %s: could not connect to port %d within %d secs" % \
                  (host, port, timeout)
            sys.exit(1)

    def createslice(self, args, flags):
        """
        Create slice specified by sliceconf using broker specified by
        brokerconf and using client configuration clientconf.
        """
        self.newtickets(args, flags)
        self.setdirs(self.sliceconf.name)
        ips = []
        inargs = {}
        outargs = {}
        privatekeyfile = "%s/identity" % self.slicekeypairdir
        publickeyfile = "%s/identity.pub" % self.slicekeypairdir
        privatekey = open(privatekeyfile).read()
        publickey = open(publickeyfile).read()
        for ticketfile in os.listdir(self.ticketsdir):
            f = open("%s/%s" % (self.ticketsdir, ticketfile))
            ticketdata = f.read()
            f.close()
            t = ticket.ticket(ticketdata)
            ips.append(t.ip)
            inargs[t.ip] = (ticketdata, privatekey, publickey)
        rval = self.doparallel(ips, "newleasevm", inargs, outargs, flags)
        self.deletetickets(self.ticketsdir, outargs.keys())
        self.saveleases(outargs.values())
        self.renameleases(self.leasesdir, self.vmsdir, outargs.keys())
        return rval
        
    def newtickets(self, args, flags):
        """Obtain new tickets from broker to create a slice."""
        self.sliceconfinit(args[0])
        if flags["timeout"]:
            agent = self.brokerconf.agents[0]  # Not the greatest
            host, port = agent["host"], agent["port"]
            self.checkreachable(host, port, flags["timeout"])
        brok = broker.broker(self.clientconf, self.brokerconf, self.sliceconf)
        ticketsdata = brok.newtickets()
        self.savetickets(ticketsdata)
        self.newslicekeypair()
        return 0
        
    def newleases(self, args, flags):
        """Redeem tickets for leases on nodes."""
        slice = args[0]
        self.setdirs(slice)
        ticketfiles = args[1:]
        ips = []
        inargs = {}
        outargs = {}
        for ticketfile in ticketfiles:
            ticketdata = open(ticketfile).read()
            t = ticket.ticket(ticketdata)
            ips.append(t.ip)
            inargs[t.ip] = (ticketdata,)
        rval = self.doparallel(ips, "newlease", inargs, outargs, flags)
        self.deletetickets(self.ticketsdir, outargs.keys())
        self.saveleases(outargs.values())
        return rval

    def newvms(self, args, flags):
        """Create set of distributed VMs using leases on nodes."""
        slice = args[0]
        self.setdirs(slice)
        leasefiles = args[1:]
        ips = []
        inargs = {}
        outargs = {}
        privatekeyfile = "%s/identity" % self.slicekeypairdir
        publickeyfile = "%s/identity.pub" % self.slicekeypairdir        
        privatekey = open(privatekeyfile).read()
        publickey = open(publickeyfile).read()
        for leasefile in leasefiles:
            leasedata = open(leasefile).read()
            l = lease.lease(leasedata)
            ips.append(l.ip)
            inargs[l.ip] = (leasedata, privatekey, publickey)
        rval = self.doparallel(ips, "newvm", inargs, outargs, flags)
        self.renameleases(self.leasesdir, self.vmsdir, outargs.keys())
        return rval
        
    def deleteslice(self, args, flags):
        """Delete all leases and slivers associated with slice."""
        slice = args[0]
        self.setdirs(slice)
        if len(os.listdir(self.leasesdir)) == 0 and \
           len(os.listdir(self.vmsdir)) == 0:
            sys.stderr.write("No leases/slivers for slice %s\n" % slice)
            return
        rval = 0
        for dir in [ self.leasesdir, self.vmsdir ]:
            inargs = {}
            outargs = {}
            ips = os.listdir(dir)
            for ip in ips:
                inargs[ip] = (slice,)
            rval |= self.doparallel(ips, "deletelease", inargs, outargs, flags)
            self.deleteleases(dir, outargs.keys())
        self.clean(slice)
        return rval

    def renewslice(self, args, flags):
        """Renew all slivers associated with slice."""
        slice = args[0]
        self.setdirs(slice)
        if len(os.listdir(self.vmsdir)) == 0:
            sys.stderr.write("No slivers for slice %s\n" % slice)
            return
        inargs = {}
        outargs = {}
        ips = os.listdir(self.vmsdir)
        for ip in ips:
            inargs[ip] = (slice,)
        rval = self.doparallel(ips, "renewlease", inargs, outargs, flags)
        self.saveleases(outargs.values())
        self.renameleases(self.leasesdir, self.vmsdir, outargs.keys())
        return rval 

    def addkey(self, args, flags):
        """Add SSH public key to all slivers associated with slice."""
        slice = args[0]
        self.setdirs(slice)
        if not os.path.exists(self.vmsdir):
            sys.stderr.write("No slivers for slice %s\n" % slice)
            return
        f = open(args[1])
        key = f.read().strip()
        f.close()
        self.savesshkey(args[1])
        inargs = {}
        outargs = {}
        ips = os.listdir(self.vmsdir)
        for ip in ips:
            inargs[ip] = (slice, key)
        return self.doparallel(ips, "addkey", inargs, outargs, flags)

    def delkey(self, args, flags):
        """Delete SSH public key on all slivers associated with slice."""
        slice = args[0]
        self.setdirs(slice)
        if not os.path.exists(self.vmsdir):
            sys.stderr.write("No slivers for slice %s\n" % slice)
            return
        f = open(args[1])
        key = f.read().strip()
        f.close()
        inargs = {}
        outargs = {}
        ips = os.listdir(self.vmsdir)
        for ip in ips:
            inargs[ip] = (slice, key)
        rval = self.doparallel(ips, "delkey", inargs, outargs, flags)
        if rval == 0:
            self.deletesshkey(args[1])
        return rval

    def nukekeys(self, args, flags):
        """Delete all SSH public keys for slivers associated with slice."""
        slice = args[0]
        self.setdirs(slice)
        if not os.path.exists(self.vmsdir):
            sys.stderr.write("No slivers for slice %s\n" % slice)
            return
        inargs = {}
        outargs = {}
        ips = os.listdir(self.vmsdir)
        for ip in ips:
            inargs[ip] = (slice,)
        rval = self.doparallel(ips, "nukekeys", inargs, outargs, flags)
        if rval == 0:
            for key in os.listdir(self.sshkeysdir):
                os.unlink("%s/%s" % (self.sshkeysdir, key))
        return rval

    def nodelist(self, args, flags):
        """Print list of IPs for nodes in slice."""
        slice = args[0]
        self.setdirs(slice)
        if not os.path.exists(self.vmsdir):
            sys.stderr.write("No slivers for slice %s\n" % slice)
            return
        ips = os.listdir(self.vmsdir)
        for ip in ips:
            print ip
        return 0

    def expirations(self, args, flags):
        """Print expiration times for slivers in slice."""
        slice = args[0]
        self.setdirs(slice)
        if not os.path.exists(self.vmsdir):
            sys.stderr.write("No slivers for slice %s\n" % slice)
            return
        ips = os.listdir(self.vmsdir)
        for ip in ips:
            leasefile = "%s/%s" % (self.vmsdir, ip)
            l = lease.lease(open(leasefile).read())
            print "Sliver for slice %s expires %s UTC on %s" % (l.slice, l.end_time, ip)
        return 0

    def doparallel(self, ips, methodname, inargs, outargs, flags):
        """
        Parallel XML-RPC to nodes. inargs and outargs are dictionaries
        of input/output arguments indexed by node IP addresses.  Wait
        at most self.maxwait seconds for all threads to complete.
        """
        sem = threading.Semaphore(flags["parallelism"])
        threads = []        
        for ip in ips:
            proxy = self.nodemgrproxy(ip)
            method = getattr(proxy, methodname)
            sem.acquire()
            thread = svmthread(ip, method, inargs, outargs, flags, sem)
            thread.start()
            threads.append(thread)
        wait = self.maxwait
        start = time.time()
        for thread in threads:
            thread.join(wait)
            now = time.time()
            diff = now - start
            if diff > self.maxwait:
                break
            else:
                wait = self.maxwait - diff
        if len(inargs) == len(outargs):
            return 0
        else:
            return 1

    def agentconfig(self, args, flags):
        ip = args[0]
        config = agentproxy.agentproxy(ip, agent.PORT).getconfig()
        for attr in config:
            print attr + ":" + " " + str(config[attr])
        return 0

    def nodemgrconfig(self, args, flags):
        ip = args[0]
        config = nodemgrproxy.nodemgrproxy(ip, nodemgr.PORT).getconfig()
        for attr in config:
            print attr + ":" + " " + str(config[attr])
        return 0

    def getads(self, args, flags):
        ip = args[0]
        ads = agentproxy.agentproxy(ip, agent.PORT).getads()
        for ad in ads:
            print ad
        return 0

    def gettickets(self, args, flags):
        ip = args[0]
        principle = digest.sha1file(self.clientconf.pubkey)
        ticketsdata = agentproxy.agentproxy(ip, agent.PORT).gettickets()
        for ticketdata in ticketsdata:
            t = ticket.ticket(ticketdata)
            if re.match(principle, t.principle):
                print ticketdata
        return 0

    def getleases(self, args, flags):
        ip = args[0]
        principle = digest.sha1file(self.clientconf.pubkey)
        leasesdata = nodemgrproxy.nodemgrproxy(ip, nodemgr.PORT).getleases()
        for leasedata in leasesdata:
            l = lease.lease(leasedata)
            if re.match(principle, l.principle):
                print leasedata
        return 0

    def getslices(self, args, flags):
        ip = args[0]
        slices = agentproxy.agentproxy(ip, agent.PORT).getslices()
        for slice in slices:
            print slice
        return 0

    def getslivers(self, args, flags):
        ip = args[0]
        slivers = nodemgrproxy.nodemgrproxy(ip, nodemgr.PORT).getslivers()
        for sliver in slivers:
            print sliver
        return 0

    def getprinciple(self, args, flags):
        ip = args[0]
        slice = args[1]
        print nodemgrproxy.nodemgrproxy(ip, nodemgr.PORT).getprinciple(slice)
        return 0

    def clean(self, slice=None):
        """Remove expired tickets, leases, slices based on local UTC time"""
        now = time.gmtime()
        if os.path.exists(self.slicesdir):
            if slice:
                slices = [ slice ]
            else:
                slices = os.listdir(self.slicesdir)
            for slice in slices:
                self.setdirs(slice)
                self.cleantickets(self.ticketsdir, now)
                self.cleanleases(self.leasesdir, now)
                self.cleanleases(self.vmsdir, now)
                if len(os.listdir(self.ticketsdir)) == 0 and \
                   len(os.listdir(self.leasesdir)) == 0 and \
                   len(os.listdir(self.vmsdir)) == 0:
                    shutil.rmtree("%s/%s" % (self.slicesdir, slice))
        return 0

    def cleantickets(self, dir, now):
        expiredticketips = []
        for ip in os.listdir(dir):
            f = open("%s/%s" % (dir, ip))
            ticketdata = f.read()
            f.close()
            l = ticket.ticket(ticketdata)
            if calendar.timegm(now) > calendar.timegm(l.end):
                expiredticketips.append(ip)
        self.deletetickets(dir, expiredticketips)

    def cleanleases(self, dir, now):
        expiredleaseips = []
        for ip in os.listdir(dir):
            f = open("%s/%s" % (dir, ip))
            leasedata = f.read()
            f.close()
            l = lease.lease(leasedata)
            if calendar.timegm(now) > calendar.timegm(l.end):
                expiredleaseips.append(ip)
        self.deleteleases(dir, expiredleaseips)

    def savetickets(self, ticketsdata):
        for ticketdata in ticketsdata:
            t = ticket.ticket(ticketdata)
            f = open("%s/%s" % (self.ticketsdir, t.ip), "w")
            f.write(ticketdata)
            f.close()

    def saveleases(self, leasesdata):
        for leasedata in leasesdata:
            l = lease.lease(leasedata)
            f = open("%s/%s" % (self.leasesdir, l.ip), "w")
            f.write(leasedata)
            f.close()

    def savesshkey(self, sshkey):
        sha1 = digest.sha1file(sshkey, strip=1)
        shutil.copy(sshkey, "%s/%s" % (self.sshkeysdir, sha1))

    def deletetickets(self, ticketsdir, ips):
        for ip in ips:
            file = ticketsdir + "/" + ip
            os.unlink(file)

    def deleteleases(self, leasesdir, ips):
        for ip in ips:
            file = leasesdir + "/" + ip
            os.unlink(file)

    def deletesshkey(self, sshkey):
        sha1 = digest.sha1file(sshkey, strip=1)
        key = "%s/%s" % (self.sshkeysdir, sha1)
        if os.path.exists(key):
            os.unlink(key)

    def renameleases(self, leasesdir, vmsdir, ips):
        for ip in ips:
            oldfile = leasesdir + "/" + ip
            newfile = vmsdir + "/" + ip
            fileutil.rename(oldfile, newfile) 

    def newslicekeypair(self):
        privatekey = "%s/identity" % self.slicekeypairdir
        os.system("ssh-keygen -t rsa -f %s -q -P ''" % privatekey)
        os.chmod(privatekey, 0600)

    def printusage(self, cmd):
        if cmd == "createslice":
            print "Usage: svm createslice slice.xml"
        elif cmd == "newtickets":
            print "Usage: svm newtickets slice.xml"
        elif cmd == "newleases":
            print "Usage: svm newleases slice ticket1.xml ticket2.xml .."
        elif cmd == "newvms":
            print "Usage: svm newvms slice lease1.xml lease2.xml .."
        elif cmd == "deleteslice":
            print "Usage: svm deleteslice slice"
        elif cmd == "renewslice":
            print "Usage: svm renewslice slice"
        elif cmd == "addkey":
            print "Usage: svm addkey slice sshkey"
        elif cmd == "delkey":
            print "Usage: svm delkey slice sshkey"
        elif cmd == "nukekeys":
            print "Usage: svm nukekeys slice"
        elif cmd == "nodelist":
            print "Usage: svm nodelist slice"
        elif cmd == "expirations":
            print "Usage: svm expirations slice"
        elif cmd == "agentconfig":
            print "Usage: svm agentconfig agenthost"
        elif cmd == "nodemgrconfig":
            print "Usage: svm nodemgrconfig nodemgrhost"
        elif cmd == "getads":
            print "Usage: svm getads agenthost"
        elif cmd == "gettickets":
            print "Usage: svm gettickets agenthost"
        elif cmd == "getslices":
            print "Usage: svm getslices agenthost"
        elif cmd == "getleases":
            print "Usage: svm getleases nodemgrhost"
        elif cmd == "getslivers":
            print "Usage: svm getslivers nodemgrhost"
        elif cmd == "getprinciple":
            print "Usage: svm getprinciple nodemgrhost slice"
        elif cmd == "clean":
            print "Usage: svm clean"
        sys.exit(2)

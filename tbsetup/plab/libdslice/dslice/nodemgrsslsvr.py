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
require authentication and encryption over SSL.

AUTHOR: Brent Chun (bnc@intel-research.net)

$Id: nodemgrsslsvr.py,v 1.1 2003-08-19 17:17:22 aclement Exp $

"""
import os
import re
import threading
from xmlrpcserver import RequestHandler
from M2Crypto import SSL
import sslhandler

def slicepubkey(slice):
    import pwd
    passwd = pwd.getpwnam(slice)
    sshdir = "%s/.ssh" % passwd[5]
    sshpubkey = "%s/identity.pub" % sshdir
    f = open(sshpubkey)
    pubkey = f.read()
    f.close()
    return pubkey.strip()

def slurpkeys(slice):
    keysfile = os.path.expanduser("~%s/.ssh/authorized_keys" % slice)
    if not os.path.exists(keysfile):
        return []
    f = open(keysfile)
    keys = []
    for key in f.readlines():
        if not re.match("$\W+$", key):
            keys.append(key.strip())
    f.close()
    return keys

def writekeys(slice, keys):
    import fileutil, tempfile
    keysfile = os.path.expanduser("~%s/.ssh/authorized_keys" % slice)
    temp = tempfile.mktemp()
    f = open(temp, "w")
    for k in keys:
        f.write(k + "\n")
    f.close()
    fileutil.rename(temp, keysfile)

def valid_sig(xml, sig, pubkey):
    import base64, digest
    from M2Crypto import RSA
    digest1 = digest.sha1(xml, None)
    rsa = RSA.load_pub_key(pubkey)
    digest2 = rsa.public_decrypt(base64.decodestring(sig), RSA.pkcs1_padding)
    return (digest1 == digest2)
    
class nodemgrsslhandler(sslhandler.sslhandler):
    def call(self, method, args):
        principle = self.getpeerkeysha1()
        subject = self.getpeersubject()
        if method == "newlease":
            result = self.server.newlease(principle, subject, args)
        elif method == "newvm":
            result = self.server.newvm(principle, subject, args)
        elif method == "newleasevm":
            result = self.server.newleasevm(principle, subject, args)
        elif method == "deletelease":
            result = self.server.deletelease(principle, args)
        elif method == "renewlease":
            result = self.server.renewlease(principle, subject, args)
        elif method == "addkey":
            result = self.server.addkey(principle, args)
        elif method == "delkey":
            result = self.server.delkey(principle, args)
        elif method == "nukekeys":
            result = self.server.nukekeys(principle, args)
        else:
            raise NotImplementedError, method + " not implemented"
        return result 

class nodemgrsslsvr(SSL.ThreadingSSLServer):
    def __init__(self, conf, nodemgr):
        from sslctx import svrctxinit
        import hacks, lease
        self.conf = conf
        self.dbthr = nodemgr.dbthr
        self.ip = nodemgr.ip
        self.nodemgr = nodemgr
        self.timesyncslack = 60 * 5
        self.lfactory = lease.leasefactory(conf.key)
        ctx = svrctxinit(conf.cert, conf.key, conf.cacert, "nodemgrsslsvr")
        SSL.ThreadingSSLServer.allow_reuse_address = 1
        try:
            method = SSL.ThreadingSSLServer.__init__
            args = [ self, ("", conf.xmlsslport), nodemgrsslhandler, ctx ]
            hacks.retryapply(method, args, 10, 1)
        except:
            raise "Could not bind to TCP port %d" % conf.xmlsslport

    def newlease(self, principle, subject, args):
        import lease, ticket
        leasedata = None
        try:
            self.nodemgr.rlock.acquire()
            t = ticket.ticket(args[0]["ticketdata"])
            self.newlease_checkargs(t, principle)
            l = self.lfactory.createlease(principle, t.ip, t.slice, t.start_time,
                                          t.end_time)
            self.nodemgr.dbthr.addlease(l.data, subject)
            leasedata = l.data
        finally:
            self.nodemgr.rlock.release()
        return leasedata

    def newlease_checkargs(self, t, principle):
        import calendar, resolve, slicename, socket, ticket, time, sys
        now = time.gmtime()
        nowsec = calendar.timegm(now)
        validfromsec = calendar.timegm(t.validfrom)
        validtosec = calendar.timegm(t.validto)
        nowstr = time.asctime(now)
        validfromstr = time.asctime(t.validfrom)
        validtostr = time.asctime(t.validto)
        if not valid_sig(t.xml, t.sig, self.conf.agentpubkey):
            raise "Ticket not signed by trusted agent"
        if t.principle != principle:
            raise "Principle does not own ticket"
        if t.ip != self.ip:
            raise "Ticket is for %s. This node is %s." % (t.ip, self.ip)
        if slicename.islogin(t.slice):
            raise "Login %s already exists" % t.slice
        if slicename.isstaticslice(t.slice):
            raise "Static sliver %s already exists" % t.slice
        if not slicename.isvalidlogin(t.slice):
            raise "Slice name %s is a bad name" % t.slice
        if self.nodemgr.dbthr.lookuplease(t.slice):
            raise "Lease for slice %s already exists" % t.slice
        if nowsec < (validfromsec - self.timesyncslack):
            raise "Ticket valid at %s. Currently %s" % (validfromstr, nowstr)
        if nowsec > (validtosec + self.timesyncslack):
            raise "Ticket expired at %s. Currently %s" % (validtostr, nowstr)

    def newvm(self, principle, subject, args):
        import lease
        try:
            self.nodemgr.rlock.acquire()
            l = lease.lease(args[0]["leasedata"])
            privatekey = args[0]["privatekey"]
            publickey = args[0]["publickey"]
            self.newvm_checkargs(principle, l)
            self.nodemgr.createsliver(l.slice, privatekey, publickey)
            self.nodemgr.dbthr.addsliver(l.slice, l.data, subject)
        finally:
            self.nodemgr.rlock.release()
        return 1

    def newvm_checkargs(self, principle, l):
        import calendar, time
        now = time.gmtime()
        nowsec = calendar.timegm(now)
        startsec = calendar.timegm(l.start)
        endsec = calendar.timegm(l.end)
        nowstr = time.asctime(now)
        startstr = time.asctime(l.start)
        endstr = time.asctime(l.end)
        lease = self.nodemgr.dbthr.lookuplease(l.slice)
        if not valid_sig(l.xml, l.sig, self.conf.pubkey):
            raise "Lease not signed by node manager %s" % self.ip
        if l.principle != principle:
            raise "Principle does not own lease"
        if l.ip != self.ip:
            raise "Lease is for %s. This node is %s." % (l.ip, self.ip)
        if self.nodemgr.dbthr.lookupsliver(l.slice):
            raise "Sliver for slice %s already bound to lease" % l.slice
        if nowsec < (startsec - self.timesyncslack):
            raise "Lease valid at %s. Currently %s" % (startstr, nowstr)
        if nowsec > (endsec + self.timesyncslack):
            raise "Lease expired at %s. Currently %s" % (endstr, nowstr)
        if not lease:
            raise "Lease for slice %s has expired" % l.slice

    def newleasevm(self, principle, subject, args):
        import lease, ticket, sys
        leasedata = None
        try:
            self.nodemgr.rlock.acquire()
            t = ticket.ticket(args[0]["ticketdata"])
            privatekey = args[0]["privatekey"]
            publickey = args[0]["publickey"]
            self.newleasevm_checkargs(t, principle)
            l = self.lfactory.createlease(principle, t.ip, t.slice, t.start_time,
                                          t.end_time)
            self.nodemgr.dbthr.addlease(l.data, subject)
            self.nodemgr.createsliver(l.slice, privatekey, publickey)
            self.nodemgr.dbthr.addsliver(l.slice, l.data, subject)
            leasedata = l.data
        finally:
            self.nodemgr.rlock.release()
        return leasedata

    def newleasevm_checkargs(self, t, principle):
        self.newlease_checkargs(t, principle)
        
    def deletelease(self, principle, args):
        """Delete lease (and sliver, if one is bound to it)."""
        try:
            self.nodemgr.rlock.acquire()
            slice = args[0]["slice"]
            self.deletelease_checkargs(principle, slice, self.nodemgr)
            lease = self.nodemgr.dbthr.lookuplease(slice)
            self._deleteleases([lease.data]) 
        finally:
            self.nodemgr.rlock.release()
        return 1

    def deletelease_checkargs(self, principle, slice, nodemgr):
        lease = nodemgr.dbthr.lookuplease(slice)
        if not lease:
            raise "Sliver %s does not exist" % slice
        if principle != lease.principle:
            raise "User does not own sliver %s" % slice

    def renewlease(self, principle, subject, args):
        import string
        leasedata = None
        try:
            self.nodemgr.rlock.acquire()
            slice = args[0]["slice"]
            self.renewlease_checkargs(principle, slice, self.nodemgr)
            leasedata = self.nodemgr.dbthr.renewlease(slice, subject)
        finally:
            self.nodemgr.rlock.release()
        return leasedata

    def renewlease_checkargs(self, principle, slice, nodemgr):
        lease = nodemgr.dbthr.lookuplease(slice)
        if not lease:
            raise "Sliver %s does not exist" % slice
        if principle != lease.principle:
            raise "User does not own sliver %s" % slice

    def addkey(self, principle, args):
        try:
            self.nodemgr.rlock.acquire()
            slice = args[0]["slice"]
            key = args[0]["key"]
            key = key.strip()
            self.addkey_checkargs(principle, slice, key, self.nodemgr)
            keysdir = os.path.expanduser("~%s/.ssh" % slice)
            if not os.path.exists(keysdir):
                os.makedirs(keysdir)
            keys = slurpkeys(slice)
            if key not in keys:
                keys.append(key)
                writekeys(slice, keys)
        finally:
            self.nodemgr.rlock.release()
        return 1

    def addkey_checkargs(self, principle, slice, key, nodemgr):
        lease = nodemgr.dbthr.lookuplease(slice)
        if not lease:
            raise "Sliver %s does not exist" % slice
        if principle != lease.principle:
            raise "User does not own sliver %s" % slice

    def delkey(self, principle, args):
        try:
            self.nodemgr.rlock.acquire()
            slice = args[0]["slice"]
            key = args[0]["key"]
            key = key.strip()
            self.delkey_checkargs(principle, slice, key, self.nodemgr)
            keys = slurpkeys(slice)
            if key in keys:
                keys.remove(key)
                writekeys(slice, keys)
        finally:
            self.nodemgr.rlock.release()
        return 1

    def delkey_checkargs(self, principle, slice, key, nodemgr):
        lease = nodemgr.dbthr.lookuplease(slice)
        if not lease:
            raise "Sliver %s does not exist" % slice
        if principle != lease.principle:
            raise "User does not own sliver %s" % slice

    def nukekeys(self, principle, args):
        """Nuke all SSH public keys, except slice public key"""
        try:
            self.nodemgr.rlock.acquire()
            slice = args[0]["slice"]
            self.nukekeys_checkargs(principle, slice, self.nodemgr)
            key = slicepubkey(slice)
            writekeys(slice, [ key ])
        finally:
            self.nodemgr.rlock.release()
        return 1

    def nukekeys_checkargs(self, principle, slice, nodemgr):
        lease = nodemgr.dbthr.lookuplease(slice)
        if not lease:
            raise "Sliver %s does not exist" % slice
        if principle != lease.principle:
            raise "User does not own sliver %s" % slice

    def _deleteleases(self, leasesdata):
        import lease
        try:
            self.nodemgr.rlock.acquire()
            for leasedata in leasesdata:
                l = lease.lease(leasedata)
                if self.nodemgr.dbthr.lookupsliver(l.slice):
                    self.nodemgr.deletesliver(l.slice)
                    self.dbthr.removesliver(l.slice)
                self.dbthr.removelease(leasedata)
        finally:
            self.nodemgr.rlock.release()
            
class nodemgrsslsvrthr(threading.Thread):
    def __init__(self, nodemgr):
        threading.Thread.__init__(self)
        self.server = nodemgrsslsvr(nodemgr.conf, nodemgr)

    def run(self):
        self.server.serve_forever()

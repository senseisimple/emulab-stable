#!/usr/bin/env python2.2
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

DESCRIPTION: Simple implementation of a PlanetLab node manager.
Handles creating, deleting, and renewal of leases (and associated
slivers, i.e., vservers) for resources associated with slivers. Also
handles adding and removing of SSH public keys for remote sliver
access. Requests are authenticated using SSL. New lease requests must
include a non-expired ticket signed by a trusted agent.

AUTHOR: Brent Chun (bnc@intel-research.net)

$Id: nodemgr.py,v 1.1 2003-08-19 17:17:20 aclement Exp $

"""
import gmetric
import os
import sys
import threading
import time
import HTMLgen

PORT = 800
SSLPORT = 801
HTTPPORT = 802

def filegetpwnam(file, name):
    import re, string
    f = open(file)
    lines = f.readlines()
    f.close()
    for line in lines:
        if re.search("^%s:" % name, line):
            fields = string.split(line.strip(), ":")
            return fields
    return None

class nodemgrhtml:
    def __init__(self, nodemgr):
        self.nodemgr = nodemgr
        self.title = "PlanetLab node manager on %s (%s)" % (self.nodemgr.host,
                                                            self.nodemgr.ip)
        self.doc = HTMLgen.SimpleDocument()
        self.setbodyattrs()
        self.doc.append(HTMLgen.Heading(2, self.title))
        self.doc.append(HTMLgen.HR())
        self.doc.append("Generated: %s UTC" % time.asctime(time.gmtime()), HTMLgen.P())
        self.setconfig()
        self.setleases()
        self.setslivers()
        self.settrailer()
        
    def setbodyattrs(self):
        self.doc.title = self.title
        self.doc.bgcolor = "#cccccc"
        self.doc.textcolor = "#000000"
        self.doc.linkcolor = "#0000ff"
        self.doc.vlinkcolor = "#0000ff"
        self.doc.alinkcolor = "#ff0000"

    def setconfig(self):
        conf = self.nodemgr.conf
        self.doc.append(HTMLgen.Heading(3, "Configuration"), HTMLgen.P())
        table = HTMLgen.TableLite(border = "0", cellpadding = "0", cellspacing = "0")
        tr = HTMLgen.TR(HTMLgen.TD("XML-RPC port: "), HTMLgen.TD(conf.xmlport))
        table.append(tr)
        tr = HTMLgen.TR(HTMLgen.TD("XML-RPC SSL port: "), HTMLgen.TD(conf.xmlsslport))
        table.append(tr)
        tr = HTMLgen.TR(HTMLgen.TD("HTTP port: "), HTMLgen.TD(conf.port))
        table.append(tr)
        self.doc.append(table, HTMLgen.P())
        self.doc.append("Trusted agent RSA public key", HTMLgen.P())
        f = open(conf.agentpubkey)
        self.doc.append(HTMLgen.PRE(f.read()), HTMLgen.P())
        f.close()
        self.doc.append("Lease requests accepted by principles with tickets ",
                        "signed by trusted agent.", HTMLgen.P())

    def setleases(self):
        import lease
        self.doc.append(HTMLgen.Heading(3, "Leases"))
        self.doc.append(HTMLgen.P())
        leasesdata = self.nodemgr.dbthr.getleases()
        if len(leasesdata) == 0:
            self.doc.append("No leases.")
            return
        table = HTMLgen.TableLite(border = "1", cellpadding = "2", cellspacing = "2")
        tr = HTMLgen.TR(HTMLgen.TD("Slice"), HTMLgen.TD("IP"), HTMLgen.TD("StartTime"),
                        HTMLgen.TD("EndTime"))
        table.append(tr)
        for leasedata in leasesdata:
            t = lease.lease(leasedata)
            tr = HTMLgen.TR(HTMLgen.TD(t.slice), HTMLgen.TD(t.ip),
                            HTMLgen.TD(t.start_time + " UTC"),
                            HTMLgen.TD(t.end_time + " UTC"))
            table.append(tr)
        self.doc.append(table, HTMLgen.P())

    def setslivers(self):
        import lease
        self.doc.append(HTMLgen.Heading(3, "Slivers"))
        self.doc.append(HTMLgen.P())
        slivers = self.nodemgr.dbthr.getslivers()
        if len(slivers) == 0:
            self.doc.append("No slivers.")
            return
        data = ""
        for sliver in slivers:
            data = data + sliver + "\n"
        self.doc.append(HTMLgen.PRE(data), HTMLgen.P())            

    def settrailer(self):
        self.doc.append(HTMLgen.HR())
        self.doc.append("Questions/comments?  Send email to %s." %
                        HTMLgen.HREF(url = "mailto:bnc@intel-research.net",
                                     text = "bnc@intel-research.net"))
        self.doc.append(HTMLgen.P())
                
    def __str__(self):
        import StringIO
        buf = StringIO.StringIO()
        buf.write(self.doc)
        return buf.getvalue()
    
class nodemgr(threading.Thread):
    def __init__(self, conffile):
        import nodemgrdbthr, nodemgrsvr, nodemgrsslsvr, nodemgrhttpsvr
        import socket, xmlconf, logfile, resolve
        threading.Thread.__init__(self)
        self.rlock = threading.RLock()
        self.conf = xmlconf.nodemgrconf(conffile)
        self.logfile = logfile.logfile(self.conf.logfile)
        self.host = socket.getfqdn(socket.gethostname())
        self.ip = resolve.getlocalip()
        self.dbthr = nodemgrdbthr.nodemgrdbthr(self)
        self.svrthr = nodemgrsvr.nodemgrsvrthr(self)
        self.sslsvrthr = nodemgrsslsvr.nodemgrsslsvrthr(self)
        self.httpsvrthr = nodemgrhttpsvr.nodemgrhttpsvrthr(self)
        
    def run(self):
        import lease
        self.dbthr.postinit()
        for thr in [ self.svrthr, self.sslsvrthr, self.dbthr, self.httpsvrthr ]:
            thr.start()
        heartbeat = 0
        gm = gmetric.gmetric()
        gaddr = self.conf.gangliaaddr
        gport = self.conf.gangliaport
        gheartbeat = self.conf.gangliaheartbeat
        nodemgr_tag = self.conf.nodemgr_tag
        leases_tag = self.conf.leases_tag
        slivers_tag = self.conf.slivers_tag
        while 1:
            now = time.asctime(time.gmtime())
            leases = ""
            for leasedata in self.dbthr.getleases():
                l = lease.lease(leasedata)
                leases = leases + " " + l.slice
            leases = leases.strip()
            slivers = ""
            for s in self.dbthr.getslivers():
                slivers = slivers + " " + s
            slivers = slivers.strip()
            gm.sendsimple(gaddr, gport, nodemgr_tag, `heartbeat`, gheartbeat, gheartbeat)
            if leases != "": 
                gm.send(gaddr, gport, leases_tag, leases, gheartbeat, gheartbeat, self.ip)
            if slivers != "": 
                gm.send(gaddr, gport, slivers_tag, slivers, gheartbeat, gheartbeat, self.ip)
            time.sleep(gheartbeat)
            heartbeat += 1

    def createsliver(self, slice, privatekey, publickey):
        # NOTE: uncomment os.system to do for real
        print "/usr/sbin/vuserdel %s" % slice
        print "/usr/sbin/vadduser %s" % slice
        os.system("/usr/sbin/vuserdel %s" % slice)
        os.system("/usr/sbin/vadduser %s" % slice)
        self.configsliverssh(slice, privatekey, publickey)

    def deletesliver(self, slice):
        # NOTE: uncomment os.system to do for real
        print "/usr/sbin/vuserdel %s" % slice
        os.system("/usr/sbin/vuserdel %s" % slice)

    def configsliverssh(self, slice, privatekey, publickey):
        import pwd, shutil
        passwd = pwd.getpwnam(slice)
        uid, gid, homedir = passwd[2], passwd[3], passwd[5]
        vpasswd = filegetpwnam("/vservers/%s/etc/passwd" % slice, slice)
        vuid, vgid = int(vpasswd[2]), int(vpasswd[3])
        sshdir = "%s/.ssh" % homedir
        vserversshdir = "/vservers/%s/home/%s/.ssh" % (slice, slice)
        sshpubkey = "%s/identity.pub" % sshdir
        keysfile = "%s/authorized_keys" % sshdir
        sshkey = "%s/identity" % vserversshdir
        config = "%s/config" % vserversshdir
        if not os.path.exists(sshdir):
            os.makedirs(sshdir, 0700)
        if not os.path.exists(vserversshdir):
            os.makedirs(vserversshdir, 0700)
        f = open(sshpubkey, "w")
        f.write(publickey.strip())
        f.close()
        shutil.copyfile(sshpubkey, keysfile)
        f = open(sshkey, "w")
        f.write(privatekey)
        f.close()
        f = open(config, "w")
        f.write("StrictHostKeyChecking no\n")
        f.close()
        os.chmod(sshkey, 0600)
        for path in [ sshpubkey, keysfile, config ]:
            os.chmod(path, 0644) 
        os.chmod(config, 0644)
        for path in [ sshdir, sshpubkey, keysfile ]:
            os.chown(path, uid, gid)
        for path in [ vserversshdir, sshkey, config ]:
            os.chown(path, vuid, vgid)

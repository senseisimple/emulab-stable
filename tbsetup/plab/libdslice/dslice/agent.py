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

DESCRIPTION: Simple implementation of a PlanetLab agent as described
in PDN-02-005.  Probes a resource broker for information about nodes
as reported by resource monitors running locally on those nodes. In
this implementation, the resource broker in a Ganglia gmetad daemon
while the resource monitors are Ganglia gmond daemons. The agent
handles two types of requests: ticket advertisement requests and new
ticket requests when creating or renewing leases for a slice.

AUTHOR: Brent Chun (bnc@intel-research.net)

$Id: agent.py,v 1.1 2003-08-19 17:17:18 aclement Exp $

"""
import os
import sys
import threading
import time
import HTMLgen

PORT = 700
SSLPORT = 701
HTTPPORT = 702

AGENT_OK = 0
AGENTERR_PERM = 1

class agenthtml:
    def __init__(self, agent):
        self.agent = agent
        self.title = "PlanetLab agent on %s (%s)" % (self.agent.host, self.agent.ip)
        self.doc = HTMLgen.SimpleDocument()
        self.setbodyattrs()
        self.doc.append(HTMLgen.Heading(2, self.title), HTMLgen.P())
        self.doc.append("Generated: %s UTC" % time.asctime(time.gmtime()), HTMLgen.P())
        self.setconfig()
        self.settickets()
        self.setleases()
        self.setslices()
        self.setnodemgrs()
        self.settrailer()
        
    def setbodyattrs(self):
        self.doc.title = self.title
        self.doc.bgcolor = "#cccccc"
        self.doc.textcolor = "#000000"
        self.doc.linkcolor = "#0000ff"
        self.doc.vlinkcolor = "#0000ff"
        self.doc.alinkcolor = "#ff0000"

    def setconfig(self):
        conf = self.agent.conf
        self.doc.append(HTMLgen.Heading(3, "Configuration"), HTMLgen.P())
        table = HTMLgen.TableLite(border = "0", cellpadding = "0", cellspacing = "0")
        tr = HTMLgen.TR(HTMLgen.TD("XML-RPC port: "), HTMLgen.TD(conf.xmlport))
        table.append(tr)
        tr = HTMLgen.TR(HTMLgen.TD("XML-RPC SSL port: "), HTMLgen.TD(conf.xmlsslport))
        table.append(tr)
        tr = HTMLgen.TR(HTMLgen.TD("HTTP port: "), HTMLgen.TD(conf.port))
        table.append(tr)
        self.doc.append(table, HTMLgen.P())
        self.doc.append("Polling Ganglia gmetad at %s:%d every %s seconds." %
                        (conf.gmetad_host, conf.gmetad_port, conf.gmetad_pollint))
        self.doc.append(HTMLgen.P())
        self.doc.append("Trusted CA X509 certificate", HTMLgen.P())
        f = open(conf.cacert)
        self.doc.append(HTMLgen.PRE(f.read()), HTMLgen.P())
        f.close()
        self.doc.append("Ticket requests accepted by principles with X509 ",
                        "certificates signed by trusted CA.")
        self.doc.append(HTMLgen.P())

    def settickets(self):
        import ticket
        self.doc.append(HTMLgen.Heading(3, "Tickets"), HTMLgen.P())
        ticketsdata = self.agent.dbthr.gettickets()
        if len(ticketsdata) == 0:
            self.doc.append("No tickets outstanding.")
            return
        self.doc.append("Tickets must be redeemed in (ValidFrom, ValidTo) interval")
        self.doc.append(HTMLgen.P())
        table = HTMLgen.TableLite(border = "1", cellpadding = "2", cellspacing = "2")
        tr = HTMLgen.TR(HTMLgen.TD("Slice"), HTMLgen.TD("IP"),
                        HTMLgen.TD("ValidFrom (UTC)"), HTMLgen.TD("ValidTo (UTC)"), 
                        HTMLgen.TD("StartTime (UTC)"), HTMLgen.TD("EndTime (UTC)"))
        table.append(tr)
        for ticketdata in ticketsdata:
            t = ticket.ticket(ticketdata)
            tr = HTMLgen.TR(HTMLgen.TD(t.slice), HTMLgen.TD(t.ip),
                            HTMLgen.TD(t.validfrom_time), HTMLgen.TD(t.validto_time),
                            HTMLgen.TD(t.start_time), HTMLgen.TD(t.end_time))
            table.append(tr)
        self.doc.append(table, HTMLgen.P())

    def setleases(self):
        self.doc.append(HTMLgen.Heading(3, "Leases"), HTMLgen.P())
        leasestoips = self.agent.gmetadthr.getleasestoips()
        if len(leasestoips) == 0:
            self.doc.append("No active leases on PlanetLab.")
            return
        data = ""
        for lease in leasestoips.keys():
            data = data + "<b>%s</b>:" % lease
            for ip in leasestoips[lease]:
                data = data + ip + " "
            data = data + "\n"
        self.doc.append(HTMLgen.PRE(data), HTMLgen.P())

    def setslices(self):
        self.doc.append(HTMLgen.Heading(3, "Slices"), HTMLgen.P())
        slicestoips = self.agent.gmetadthr.getslicestoips()        
        if len(slicestoips) == 0:
            self.doc.append("No active slices on PlanetLab.")
            return
        data = ""
        for slice in slicestoips.keys():
            data = data + "<b>%s</b>:" % slice
            for ip in slicestoips[slice]:
                data = data + ip + " "
            data = data + "\n"
        self.doc.append(HTMLgen.PRE(data), HTMLgen.P())

    def setnodemgrs(self):
        self.doc.append(HTMLgen.Heading(3, "Nodes"), HTMLgen.P())
        ips = self.agent.gmetadthr.getips()
        if len(ips) == 0:
            self.doc.append("No nodes running a node manager.")
            return
        data = ""
        for ip in ips:
            data = data + ip + "\n"
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

class agent(threading.Thread):
    def __init__(self, conffile):
        import gmetadthr, agentsvr, agentsslsvr, agenthttpsvr, agentdbthr
        import socket, xmlconf, logfile, resolve
        threading.Thread.__init__(self)
        self.rlock = threading.RLock()        
        self.conf = xmlconf.agentconf(conffile)
        self.logfile = logfile.logfile(self.conf.logfile)
        self.host = socket.getfqdn(socket.gethostname())
        self.ip = resolve.getlocalip()
        self.dbthr = agentdbthr.agentdbthr(self)
        self.gmetadthr = gmetadthr.gmetadthr(self)
        self.svrthr = agentsvr.agentsvrthr(self)
        self.sslsvrthr = agentsslsvr.agentsslsvrthr(self)
        self.httpsvrthr = agenthttpsvr.agenthttpsvrthr(self)
        self.leasestoips = {}
        self.slicestoips = {}
        self.logfile.log("Initializing")
        
    def run(self):
        self.dbthr.postinit()
        for thr in [ self.dbthr, self.gmetadthr, self.svrthr,
                     self.sslsvrthr, self.httpsvrthr ]:
            thr.start()
        self.logfile.log("Running")
        while 1:
            time.sleep(sys.maxint)

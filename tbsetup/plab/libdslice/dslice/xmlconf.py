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

DESCRIPTION: Classes that convert XML configuration files into object
representations of their contents which we can easily access through
attributes, as opposed to walking DOM trees.

AUTHOR: Brent Chun (bnc@intel-research.net)

$Id: xmlconf.py,v 1.1 2003-08-19 17:17:24 aclement Exp $

"""
import string
import xml.dom.minidom

def gettext(nodelist):
    """Return string for text in nodelist (encode() for Unicode conversion)"""
    text = ""
    for node in nodelist:
        if node.nodeType == node.TEXT_NODE:
            text = text + node.data
    return text.strip().encode()

class agentconf:
    def __init__(self, pathname):
        self.parse(pathname)

    def parse(self, pathname):
        doc = xml.dom.minidom.parse(pathname)
        e = doc.getElementsByTagName("xmlport")[0]
        self.xmlport = int(gettext(e.childNodes))
        e = doc.getElementsByTagName("xmlsslport")[0]
        self.xmlsslport = int(gettext(e.childNodes))
        e = doc.getElementsByTagName("port")[0]
        self.port = int(gettext(e.childNodes))
        e = doc.getElementsByTagName("key")[0]
        self.key = gettext(e.childNodes)
        e = doc.getElementsByTagName("cert")[0]
        self.cert = gettext(e.childNodes)
        e = doc.getElementsByTagName("cacert")[0]
        self.cacert = gettext(e.childNodes)
        e = doc.getElementsByTagName("ticketdb")[0]
        self.ticketdb = gettext(e.childNodes)
        e = doc.getElementsByTagName("gmetad_host")[0]
        self.gmetad_host = gettext(e.childNodes)
        e = doc.getElementsByTagName("gmetad_port")[0]
        self.gmetad_port = int(gettext(e.childNodes))
        e = doc.getElementsByTagName("gmetad_pollint")[0]
        self.gmetad_pollint = int(gettext(e.childNodes))
        e = doc.getElementsByTagName("nodemgr_tag")[0]
        self.nodemgr_tag = gettext(e.childNodes)
        e = doc.getElementsByTagName("leases_tag")[0]
        self.leases_tag = gettext(e.childNodes)
        e = doc.getElementsByTagName("slivers_tag")[0]
        self.slivers_tag = gettext(e.childNodes)
        e = doc.getElementsByTagName("logfile")[0]
        self.logfile = gettext(e.childNodes)
        e = doc.getElementsByTagName("maxlease")[0]
        self.maxlease = int(gettext(e.childNodes))

class nodemgrconf:
    def __init__(self, pathname):
        self.parse(pathname)

    def parse(self, pathname):
        doc = xml.dom.minidom.parse(pathname)
        e = doc.getElementsByTagName("xmlport")[0]
        self.xmlport = int(gettext(e.childNodes))
        e = doc.getElementsByTagName("xmlsslport")[0]
        self.xmlsslport = int(gettext(e.childNodes))
        e = doc.getElementsByTagName("port")[0]
        self.port = int(gettext(e.childNodes))
        e = doc.getElementsByTagName("key")[0]
        self.key = gettext(e.childNodes)
        e = doc.getElementsByTagName("pubkey")[0]
        self.pubkey = gettext(e.childNodes)
        e = doc.getElementsByTagName("cert")[0]
        self.cert = gettext(e.childNodes)
        e = doc.getElementsByTagName("agentpubkey")[0]
        self.agentpubkey = gettext(e.childNodes)
        e = doc.getElementsByTagName("cacert")[0]
        self.cacert = gettext(e.childNodes)
        e = doc.getElementsByTagName("leasedb")[0]
        self.leasedb = gettext(e.childNodes)
        e = doc.getElementsByTagName("sliverdb")[0]
        self.sliverdb = gettext(e.childNodes)
        e = doc.getElementsByTagName("gangliaaddr")[0]
        self.gangliaaddr = gettext(e.childNodes)
        e = doc.getElementsByTagName("gangliaport")[0]
        self.gangliaport = int(gettext(e.childNodes))
        e = doc.getElementsByTagName("gangliaheartbeat")[0]
        self.gangliaheartbeat = int(gettext(e.childNodes))
        e = doc.getElementsByTagName("nodemgr_tag")[0]
        self.nodemgr_tag = gettext(e.childNodes)
        e = doc.getElementsByTagName("leases_tag")[0]
        self.leases_tag = gettext(e.childNodes)
        e = doc.getElementsByTagName("slivers_tag")[0]
        self.slivers_tag = gettext(e.childNodes)
        e = doc.getElementsByTagName("logfile")[0]
        self.logfile = gettext(e.childNodes)

class clientconf:
    def __init__(self, pathname):
        self.parse(pathname)

    def parse(self, pathname):
        doc = xml.dom.minidom.parse(pathname)
        e = doc.getElementsByTagName("key")[0]
        self.key = gettext(e.childNodes)
        e = doc.getElementsByTagName("pubkey")[0]
        self.pubkey = gettext(e.childNodes)
        e = doc.getElementsByTagName("cert")[0]
        self.cert = gettext(e.childNodes)
        e = doc.getElementsByTagName("cacert")[0]
        self.cacert = gettext(e.childNodes)
        e = doc.getElementsByTagName("slicesdir")[0]
        self.slicesdir = gettext(e.childNodes)

class brokerconf:
    def __init__(self, pathname):
        self.parse(pathname)

    def parse(self, pathname):
        doc = xml.dom.minidom.parse(pathname)
        self.agents = []
        for a in doc.getElementsByTagName("agent"):
            agent = {}
            e = a.getElementsByTagName("host")[0]
            agent["host"] = gettext(e.childNodes) 
            e = a.getElementsByTagName("xmlport")[0]
            agent["xmlport"] = int(gettext(e.childNodes))
            e = a.getElementsByTagName("xmlsslport")[0]
            agent["xmlsslport"] = int(gettext(e.childNodes))
            e = a.getElementsByTagName("port")[0]
            agent["port"] = int(gettext(e.childNodes))
            self.agents.append(agent)
        if len(self.agents) == 0:
            raise "Broker configuration needs at least one agent"

class sliceconf:
    def __init__(self, pathname):
        self.parse(pathname)

    def parse(self, pathname):
        import string, sys
        doc = xml.dom.minidom.parse(pathname)
        e = doc.getElementsByTagName("name")[0]
        self.name = gettext(e.childNodes)
        e = doc.getElementsByTagName("numnodes")[0]
        self.numnodes = int(gettext(e.childNodes))
        e = doc.getElementsByTagName("leaselen")[0]
        self.leaselen = int(gettext(e.childNodes))
        e = doc.getElementsByTagName("ips")
        if e:
            self.ips = string.split(gettext(e[0].childNodes))
        else:
            self.ips = "0.0.0.0" # Any (XML-RPC can't handle None)

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

DESCRIPTION: Send and receive Ganglia metrics with no limitations on
size or content (e.g., XML data, binary, etc.).

AUTHOR: Brent Chun (bnc@intel-research.net)

$Id: gmetric.py,v 1.1 2003-08-19 17:17:00 aclement Exp $

"""
import base64
import os
import re
import zlib

def gettext(nodelist):
    text = ""
    for node in nodelist:
        if node.nodeType == node.TEXT_NODE:
            text = text + node.data
    return text

class gmetricxml:
    def __init__(self, host, port):
        """Read XML from either a gmetad or gmond"""
        import socket, xml.dom.minidom
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((host, port))
        f = s.makefile("r")
        self.xmldata = f.read()
        f.close()
        s.close()
        doc = xml.dom.minidom.parseString(self.xmldata)
        self.metrics = doc.getElementsByTagName("METRIC")

    def getips(self, name):
        """Return IPs for hosts that have data (name format: name.ip.n)"""
        ips = []
        for metric in self.metrics:
            nameattr = metric.getAttribute("NAME")
            m = re.match("^%s\.(\d+\.\d+\.\d+\.\d+)\.n$" % name, nameattr)
            if m:
                ips.append(m.group(1))
        return ips

    def getn(self, ip, name):
        """Return number of fragments for metric name on specific node"""
        for metric in self.metrics:
            nameattr = metric.getAttribute("NAME")
            m = re.match("^%s\.%s\.n$" % (name, ip), nameattr)
            if m:
                return int(metric.getAttribute("VAL"))
        return 0

    def getfragments(self, ip, name):
        """Return n fragments for metric name on specific node"""
        nfrags = self.getn(ip, name)
        fragsread = 0
        fragments = range(0, nfrags)
        for metric in self.metrics:
            nameattr = metric.getAttribute("NAME")
            m = re.match("^%s\.%s\.(\d+)$" % (name, ip), nameattr)
            if m:
                i = int(m.group(1))
                if (i < nfrags):
                    fragments[i] = metric.getAttribute("VAL")
                    fragsread += 1
        if (fragsread == nfrags):
            return fragments
        else:
            return []

class gmetric:
    def __init__(self):
        self.gmetric = "/usr/bin/gmetric"
        self.maxfrag = 1000

    def send(self, mcastip, mcastport, name, value, tmax, dmax, localip):
        """Send multicast data to gmonds. Perform fragmentation if necessary"""
        import resolve
        fragments = self.fragmentdata(value)
        fragname = "%s.%s.n" % (name, localip)
        cmd = "%s -n%s -v%d -t%s -x%d -d%d -c%s -p%d" % (self.gmetric, fragname, len(fragments), "string", tmax, dmax, mcastip, mcastport)
        os.system(cmd)
        for i in range(0, len(fragments)):
            fragname = "%s.%s.%d" % (name, localip, i)
            cmd = "%s -n%s -v%s -t%s -x%d -d%d -c%s -p%d" % (self.gmetric, fragname, fragments[i], "string", tmax, dmax, mcastip, mcastport)
            os.system(cmd)

    def sendsimple(self, mcastip, mcastport, name, value, tmax, dmax):
        """Send multicast data to gmonds. Data must fit in single IP datagram."""
        cmd = "%s -n%s -v\"%s\" -t%s -x%d -d%d -c%s -p%d" % (self.gmetric, name, value, "string", tmax, dmax, mcastip, mcastport)
        os.system(cmd)

    def recv(self, host, port, name, ip):
        """
        Receive metric data from gmond/gmetad.  Perform reassembly if necessary
        Data is returned for single IP.
        """
        xmldata = gmetricxml(host, port)
        data = ""
        fragments = xmldata.getfragments(ip, name)
        if (fragments):
            data += self.reassembledata(fragments)
        return data
    
    def recvall(self, host, port, name):
        """
        Receive metric data from gmond/gmetad.  Perform reassembly if necessary
        Data is returned for all IPs as a dictionary (IP address -> data).
        """
        xmldata = gmetricxml(host, port)
        data = {}
        for ip in xmldata.getips(name):
            fragments = xmldata.getfragments(ip, name)
            if (fragments):
                data[ip] = self.reassembledata(fragments)
        return data
                
    def fragmentdata(self, data):
        import re
        bdata = base64.encodestring(zlib.compress(data, zlib.Z_BEST_COMPRESSION))
        n = len(bdata) / self.maxfrag
        if (len(bdata) % self.maxfrag != 0):
            n += 1
        fragments = []
        for i in range(0, n):
            start = i * self.maxfrag
            end = (i + 1) * self.maxfrag
            fragbdata = bdata[start:end]
            fragbdata = re.sub("\n", "", fragbdata)
            fragments.append(fragbdata)
        return fragments

    def reassembledata(self, fragments):
        bdata = ""
        for i in range(len(fragments)):
            bdata += fragments[i]
        return zlib.decompress(base64.decodestring(bdata))

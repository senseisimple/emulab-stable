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

DESCRIPTION: Thread that periodically polls a resource broker (e.g.,
Ganglia gmetad) for a set of nodes.  In this implementation, the only
nodes considered are ones that are running node managers (which
publish their existence in Ganglia through their local gmond).

AUTHOR: Brent Chun (bnc@intel-research.net)

$Id: gmetadthr.py,v 1.2 2003-09-13 00:23:03 ricci Exp $

"""
import threading
import time
import agent
import gmon.ganglia

class gmetadthr(threading.Thread):
    def __init__(self, agent):
        threading.Thread.__init__(self)
        self.host = agent.conf.gmetad_host
        self.port = agent.conf.gmetad_port
        self.pollint = agent.conf.gmetad_pollint
        self.lock = threading.Lock()
        self.nodemgr_tag = agent.conf.nodemgr_tag
        self.leases_tag = agent.conf.leases_tag
        self.slivers_tag = agent.conf.slivers_tag
        
    def reset(self):
        self.ips = []
        self.ipstoleases = {}
        self.leasestoips = {}
        self.ipstoslices = {}
        self.slicestoips = {}
        
    def run(self):
        self.ips = []
        self.ipstoleases = {}
        self.leasestoips = {}
        self.ipstoslices = {}
        self.slicestoips = {}
        while 1:
            try:
                self.ganglia = gmon.ganglia.Ganglia(self.host, self.port)
                self.ganglia.refresh()
                self.updateips()
                self.updateleases()
                self.updateslices()
            except:
		self.reset() # Ganglia XML SAX badness
            time.sleep(self.pollint)

    def updateips(self):
        """Extract all nodes that are publishing the nodemgr metric"""
        ips = []
        cnames = self.ganglia.getClustersNames()
        for cname in cnames:
            cluster = self.ganglia.getCluster(cname)
            hosts = cluster.getHosts()
            for h in hosts:
                if h.getMetricValue(self.nodemgr_tag) != "unknown":
                    ips.append(h.getIP())
        self.lock.acquire()
        self.ips = ips
        self.lock.release()

    def updateleases(self):
        """Extract names of active leases on PlanetLab"""
        import gmetric, string
        leases = {}
        metric = gmetric.gmetric()
        ipstoleases = metric.recvall(self.host, self.port, self.leases_tag)
        leasestoips = {}
        for ip in ipstoleases.keys():
            ipstoleases[ip] = string.split(ipstoleases[ip])
            for lease in ipstoleases[ip]:
                if lease not in leasestoips:
                    leasestoips[lease] = [ ip ]
                else:
                    leasestoips[lease].append(ip)
        self.lock.acquire()
        self.ipstoleases = ipstoleases
        self.leasestoips = leasestoips
        self.lock.release()

    def updateslices(self):
        """Extract names of active slices (allocated VMs) on PlanetLab"""
        import gmetric, string
        slices = {}
        metric = gmetric.gmetric()
        ipstoslices = metric.recvall(self.host, self.port, self.slivers_tag)
        slicestoips = {}
        for ip in ipstoslices.keys():
            ipstoslices[ip] = string.split(ipstoslices[ip])
            for slice in ipstoslices[ip]:
                if slice not in slicestoips:
                    slicestoips[slice] = [ ip ]
                else:
                    slicestoips[slice].append(ip)
        self.lock.acquire()
        self.ipstoslices = ipstoslices
        self.slicestoips = slicestoips
        self.lock.release()

    def getips(self):
        self.lock.acquire()
        ips = self.ips
        self.lock.release()
        return ips

    def getleasestoips(self):
        self.lock.acquire()
        leasestoips = self.leasestoips
        self.lock.release()
        return leasestoips

    def getslicestoips(self):
        self.lock.acquire()
        slicestoips = self.slicestoips
        self.lock.release()
        return slicestoips

    def leaseexists(self, lease):
        return lease in self.leasestoips

    def sliceexists(self, slice):
        return slice in self.slicestoips

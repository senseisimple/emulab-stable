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

DESCRIPTION: Lease factory and lease classes.  The lease class is used
to create lease objects which we can manipulate.  Actual lease data
(i.e., what's sent back to clients, stored in databases, etc.) is an
XML file and a base64-encoded RSA signature of the SHA1 hash of the
XML file. This data is stored in RFC 822 format for each processing
and readability. When we need to manipulate leases, we simply pass the
lease data into the constructor for the lease class and get back a
lease object.

AUTHOR: Brent Chun (bnc@intel-research.net)

$Id: lease.py,v 1.1 2003-08-19 17:17:20 aclement Exp $

"""
import re
import xml.dom.minidom
import StringIO

class lease:
    """Convert lease data (sig + XML) into lease object"""
    def __init__(self, data):
        self.data = data
        self.parse()
        self.parsexml()

    def parse(self):
        import rfc822
        buf = StringIO.StringIO(self.data)
        m = rfc822.Message(buf)
        self.sig = m.getheader("sig-rsa-sha1-base64")
        self.xml = buf.read()

    def parsexml(self):
        import calendar, time
        doc = xml.dom.minidom.parseString(self.xml)
        e = doc.getElementsByTagName("principle_sha1")[0]
        self.principle = self.gettext(e.childNodes)
        e = doc.getElementsByTagName("ip")[0]
        self.ip = self.gettext(e.childNodes)
        e = doc.getElementsByTagName("slice")[0]
        self.slice = self.gettext(e.childNodes)
        e = doc.getElementsByTagName("start_time")[0]
        self.start_time = self.gettext(e.childNodes)
        e = doc.getElementsByTagName("end_time")[0]
        self.end_time = self.gettext(e.childNodes)
        self.start = time.strptime(self.start_time, "%Y-%m-%d %H:%M:%S")
        self.end = time.strptime(self.end_time, "%Y-%m-%d %H:%M:%S")
        self.leaselen = calendar.timegm(self.end) - calendar.timegm(self.start)

    def gettext(self, nodelist):
        """Return string for text in nodelist (encode() for Unicode conversion)"""
        text = ""
        for node in nodelist:
            if node.nodeType == node.TEXT_NODE:
                text = text + node.data
        return text.strip().encode()

class leasefactory:
    """Factory for creating new lease signed by an RSA private key"""
    def __init__(self, privkey):
        self.privkey = privkey

    def createlease(self, principle, ip, slice, start_time, end_time):
        import calendar, time
        start = time.strptime(start_time, "%Y-%m-%d %H:%M:%S")
        end = time.strptime(end_time, "%Y-%m-%d %H:%M:%S")
        leaselen = calendar.timegm(end) - calendar.timegm(start)
        xml = self.create_xml(principle, ip, slice, start_time, end_time)
        sig = self.create_sig(xml)
        buf = StringIO.StringIO()
        buf.write("sig-rsa-sha1-base64: %s\n\n%s" % (sig, xml))
        return lease(buf.getvalue())

    def create_xml(self, principle, ip, slice, start_time, end_time):
        from xml.dom import EMPTY_NAMESPACE
        impl = xml.dom.minidom.getDOMImplementation()
        doctype = impl.createDocumentType("lease", None, None)
        doc = impl.createDocument(EMPTY_NAMESPACE, "lease", doctype)
        e = doc.createElement("principle_sha1")
        e.appendChild(doc.createTextNode(principle))
        doc.documentElement.appendChild(e)
        e = doc.createElement("ip")
        e.appendChild(doc.createTextNode(ip))
        doc.documentElement.appendChild(e)
        e = doc.createElement("slice")
        e.appendChild(doc.createTextNode(slice))
        doc.documentElement.appendChild(e)
        e = doc.createElement("start_time")
        e.appendChild(doc.createTextNode(start_time))
        doc.documentElement.appendChild(e)
        e = doc.createElement("end_time")
        e.appendChild(doc.createTextNode(end_time))
        doc.documentElement.appendChild(e)
        return doc.toprettyxml("    ")

    def create_sig(self, data):
        import base64, digest
        from M2Crypto import RSA
        sha1 = digest.sha1(data, None)
        rsa = RSA.load_key(self.privkey)
        sig = rsa.private_encrypt(sha1, RSA.pkcs1_padding)
        sig = base64.encodestring(sig)
        sig = re.sub("\n", "", sig)
        return sig

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

DESCRIPTION: Functions to check the validity of a slice name.
Standard Linux logins and static, legacy slices are disallowed as are
slice names that aren't valid Linux usernames.

AUTHOR: Brent Chun (bnc@intel-research.net)

$Id: slicename.py,v 1.1 2003-08-19 17:17:23 aclement Exp $

"""
import re
import string

staticlogins = { "root":1, "bin":1, "daemon":1, "adm":1, "lp":1, "sync":1, "shutdown":1,
                 "halt":1, "mail":1, "news":1, "uucp":1, "operator":1, "games":1,
                 "gopher":1, "ftp":1, "nobody":1, "vcsa":1, "mailnull":1, "rpm":1,
                 "rpc":1, "rpcuser":1, "nfsnobody":1, "nscd":1, "ident":1, "radvd":1,
                 "apache":1, "squid":1, "named":1, "sshd":1, "ntp":1 }

staticslices = { "arizona":1, "bologna":1, "caltech":1, "cambridge":1, "canterbury":1,
                 "cmu":1, "columbia":1, "copenhagen":1, "cornell":1, "duke":1,
                 "gt":1, "harvard":1, "ib":1, "idsl":1, "irb":1, "irp":1,
                 "irs":1, "isi":1, "kansas":1, "kentucky":1, "lancaster":1,
                 "lbl":1, "michigan":1, "mit":1, "nyu":1, "princeton":1, "rice":1,
                 "stanford":1, "sydney":1, "tennessee":1, "ubc":1, "ucb":1, "ucla":1,
                 "ucsb":1, "ucsd":1, "umass":1, "upenn":1, "uppsala":1, "utah":1,
                 "utexas":1, "uw":1, "washu":1, "wisconsin":1 }

def islogin(name):
    return name in staticlogins

def isstaticslice(name):
    m = re.search("^([a-z]+)([0-9]+)$", name)
    if m:
        base = m.group(1)
        num = int(m.group(2))
        if base in staticslices and (num >= 1 and num <= 10):
            return 1
        else:
            return None
    else:
        return None

def isvalidlogin(name):
    if (name == "vserver-reference"):
        return None
    return re.search("^[a-zA-Z]", name) and re.search("^[\w\-]+$", name)

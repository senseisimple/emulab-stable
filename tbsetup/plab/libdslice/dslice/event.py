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

DESCRIPTION: Events and an event queue.  Used to time out old tickets
and leases.

AUTHOR: Brent Chun (bnc@intel-research.net)

$Id: event.py,v 1.1 2003-08-19 17:17:20 aclement Exp $

"""
import Queue
import event

class event:
    def __init__(self, time):
        self.time = time

    def process(self):
        pass

class tickexpevent(event):
    def __init__(self, time, ticketdata, sslsvr):
        event.__init__(self, time)
        self.ticketdata = ticketdata
        self.sslsvr = sslsvr

    def process(self):
        self.sslsvr._deletetickets([self.ticketdata]) 

class leaseexpevent(event):
    def __init__(self, time, leasedata, sslsvr):
        event.__init__(self, time)
        self.leasedata = leasedata
        self.sslsvr = sslsvr

    def process(self):
        self.sslsvr._deleteleases([self.leasedata])

class eventqueue(Queue.Queue):
    def put(self, event):
        Queue.Queue.put(self, (event.time, event))

    def put_nowait(self, event):
        Queue.Queue.put_nowait(self, (event.time, event))

    def get(self):
        time, event = Queue.Queue.get(self)
        return event

    def get_nowait(self):
        time, event = Queue.Queue.get_nowait(self)
        return event

    def delete(self, event):
        self.esema.acquire()
        self.mutex.acquire()
        was_full = self._full()
        self.queue.remove((event.time, event))
        if was_full:
            self.fsema.release()
        if not self._empty():
            self.esema.release()
        self.mutex.release()

    def top(self, block=1):
        if block:
            self.esema.acquire()
        elif not self.esema.acquire(0):
            raise Empty
        self.mutex.acquire()
        was_full = self._full()
        time, event = self._top()
        if was_full:
            self.fsema.release()
        if not self._empty():
            self.esema.release()
        self.mutex.release()
        return event

    def _put(self, event):
        import bisect
        bisect.insort_right(self.queue, event)

    def _top(self):
        item = self.queue[0]
        return item

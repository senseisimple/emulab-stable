# -*- python -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

import sys
import time
from libtestbed import logger

DEF_PREFIX = ""

log = sys.stdout
msgprefix = DEF_PREFIX
disabled = False

#
# Setup the timer advice function:
# Tell it to log to syslog, or setup a message prefix.
#
def initTimeAdvice(logname=None, prefix=DEF_PREFIX):
    global log, msgprefix
    if logname:
        log = logger(logname)
        pass
    msgprefix = prefix
    pass

# Turn off timer logging (on by default).
def disableTimeAdvice():
    global disabled
    disabled = True
    pass

# Turn on timer logging
def enableTimeAdvice():
    global disabled
    disabled = False
    pass

#
# Timing advice (AOP) function
#
# Log the nodeid (if applicable), function being called, and it's duration
# to the log file.
#
def timeAdvice(self, *args, **kwargs):
    global log, msgprefix
    start = time.time()
    ret = self.__proceed(*args, **kwargs)
    end = time.time()
    if disabled:
        return ret
    stack_entry = self.__proceed_stack[-1]
    nodestr = ""
    if hasattr(self, "nodeid"):
        nodestr = "nodeid: %s, " % self.nodeid
        pass
    msg = msgprefix + "%sfunction: %s, duration: %s" % \
          (nodestr, stack_entry.name, end-start)
    log.write(msg + "\n")
    log.flush()
    return ret

"""

DESCRIPTION: Daemon process functions. 

$Id: daemon.py,v 1.1 2003-08-19 17:17:19 aclement Exp $

"""
def daemonize():
    import os, sys
    pid = os.fork()
    if pid > 0:
        sys.exit(0)
    os.setsid()
    os.close(0)
    os.close(1)
    os.close(2)
    sys.stdin = open("/dev/null")
    sys.stdout = open("/dev/null", "w")
    sys.stderr = open("/dev/null", "w")
    pid = os.fork()
    if pid > 0:
        sys.exit(0)

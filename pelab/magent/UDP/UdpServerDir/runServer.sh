#!/bin/sh

# The argument below should be the interface on which this emulab node
# is connected to the other node ( running the client ) - not the 
# control connection interface.

# This can be looked up by running ifconfig and the interface with 10.*.*.* address
# is the correct one.

# NOTE: A single invocation of file script/program only works for one UdpClient session.
# The server program has to be restarted for every session.

# The server runs in an infinite while loop - it can be terminated after the session
# is determined to be done at the client - kill using Ctrl-C.

sudo ./UdpServer eth1 3492

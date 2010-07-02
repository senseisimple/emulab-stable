#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

# Usage: sh run-monitor.sh <experiment-name> <ip-address>

sudo /usr/sbin/tcpdump -tt -n -i eth1 "!(dst host $2) && dst net 10 && tcp" | tee tcpdump.txt | python monitor.py ip-mapping.txt $1 $2

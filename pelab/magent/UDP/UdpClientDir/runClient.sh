#!/bin/sh

#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#


# Command line arguments:
# First argument - Interface on which we are connected to the server node ( not the control conn ).
# Second - Address/hostname of the node running the server program.
# Third - Number of UDP packets to send to the server.
# Fourth - Size of the data part of the UDP packets.
# Fifth - The rate at which the packets should be sent ( bits per sec )
# This rate will also include the UDP, IP & ethernet headers along with the packet size.
# Sixth - MHz of CPU clock frequency

# The client runs in an infinite while loop - so when no more data is being printed on
# screen, it is safe to kill it( Ctrl-C) and look at the results.

# NOTE: The UdpServer needs to be restarted before running the client for a second time.

sudo ./UdpClient eth0 node1 10000 1470 500000 601

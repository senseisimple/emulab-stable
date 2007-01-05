#!/bin/sh

# Command line arguments:
# First argument - Interface on which we are connected to the server node ( not the control conn ).
# Second - Local IP address on the above interface.
# Third - Address/hostname of the node running the server program.
# Four - Number of UDP packets to send to the server.
# Five - Size of the data part of the UDP packets.
# Six - The rate at which the packets should be sent ( bits per sec )
# This rate will also include the UDP, IP & ethernet headers along with the packet size.

# The client runs in an infinite while loop - so when no more data is being printed on
# screen, it is safe to kill it( Ctrl-C) and look at the results.

# NOTE: The UdpServer needs to be restarted before running the client for a second time.

sudo ./UdpClient eth1 10.1.1.2 udpnode2 200000 958 500000

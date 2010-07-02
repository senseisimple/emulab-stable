#!/usr/local/bin/python
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#



import sys
import socket
import time

hostName = sys.argv[1]
serverPort = 5001

serverSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

serverSocket.bind((socket.gethostname(), serverPort))

while True:
    clientData, clientAddr = serverSocket.recvfrom(1024)
    print str(clientAddr[0]) + " " + str(clientAddr[1]) + " " + clientData
    currentTime = int(time.time()*1000)
    replyMessage = clientData + " " + str(currentTime - int(clientData.split()[1]))
    serverSocket.sendto(replyMessage, clientAddr)

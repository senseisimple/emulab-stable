#!/usr/local/bin/python
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#



import sys
import time
import socket
import select
import fcntl, os

hostNameFile = sys.argv[1]
timeout = 5 # seconds
probeRate = 10 # Hz
probeDuration = 15 # seconds
portNum = 5001

outputDirectory = sys.argv[2]
localHostName = sys.argv[3]

# Create the output directory.
os.mkdir(outputDirectory)

# Read the input file having all the planetlab node IDs.

# Get the hostname of this host.
#localHostName = socket.gethostname()

inputFileHandle = open(hostNameFile, 'r')
hostList = []

for hostLine in inputFileHandle.readlines():
#{
    hostLine = hostLine.strip()

    if hostLine != localHostName:
    #{
        hostList.append(hostLine)
    #}
#}

inputFileHandle.close()


numHosts = len(hostList)
targetSleepTime = float ((1000.0/float(probeRate)) ) - 0.001

clientSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
fcntl.fcntl(clientSocket, fcntl.F_SETFL, os.O_NONBLOCK)

for i in range(0, numHosts-1):
#{

    firstHostAddr = (hostList[i] + ".flex-plab2.tbres.emulab.net", portNum) 
    delaySequenceArray = []

    delaySequenceArray.append(list())
    delaySequenceArray.append(list())

    packetTimeMaps = []

    packetTimeMaps.append(dict())
    packetTimeMaps.append(dict())

    packetCounter = 0

    for j in range(i+1, numHosts):
    #{
        secondHostAddr = (hostList[j] + ".flex-plab2.tbres.emulab.net", portNum) 

        startTime = int(time.time())
        lastSentTime = startTime

        endProbesFlag = 0
        readTimeoutFlag = 0

        # For each combination(pair), send a train of UDP packets.
        while (( lastSentTime - startTime) < probeDuration) or \
              not(readTimeoutFlag):
        #{

        # Stop waiting for probe replies after a timeout - calculated from the
        # time the last probe was sent out.
            if endProbesFlag and ( (time.time() - lastSentTime) > timeout):
                readTimeoutFlag = 1

        # Stop sending probes after the given probe duration.
            if not(endProbesFlag) and (lastSentTime - startTime) > probeDuration:
                endProbesFlag = 1

            if endProbesFlag:
                time.sleep(timeout/10.0)

            if not(endProbesFlag):
                readSet, writeSet, exceptionSet = select.select([clientSocket], [clientSocket], [], 0.0)
            else:
                readSet, writeSet, exceptionSet = select.select([clientSocket], [], [], 0.0)

            if not(readTimeoutFlag):
            #{
                if clientSocket in readSet:
                #{
                    serverReply = "a"
                    while serverReply:
                    #{
                        try:
                        #{
                            serverReply, serverAddr = clientSocket.recvfrom(1024)
                            print hostList[i] + " " + hostList[j] + " " + serverReply
                            hostIndex, origTimestamp, oneWayDelay = serverReply.split()
                            hostIndex = int(hostIndex)
                            oneWayDelay = int(oneWayDelay)
                            delaySequenceArray[hostIndex][packetTimeMaps[hostIndex][origTimestamp]] = oneWayDelay
                        #}
                        except socket.error:
                        #{
                            if endProbesFlag:
                            #{
                                time.sleep(timeout/10.0)
                            #}
                            break
                        #}
                    #}
                #}
            #}

            if not(endProbesFlag):
            #{
                if clientSocket in writeSet:
                #{
                    # Send the probe packets.
                    sendTime = int(time.time())
                    messageString = "0 "
                    messageString = messageString + str(sendTime)
                    clientSocket.sendto(messageString, firstHostAddr)
                    packetTimeMaps[0][str(sendTime)] = packetCounter
                    delaySequenceArray[0].append(-9999)

                    sendTime = int(time.time())
                    messageString = "1 "
                    messageString = messageString + str(sendTime)
                    clientSocket.sendto(messageString, secondHostAddr)
                    packetTimeMaps[1][str(sendTime)] = packetCounter
                    delaySequenceArray[1].append(-9999)

                    # Sleep for 99 msec for a 10Hz target probing rate.
                    lastSentTime = time.time()
                    time.sleep(targetSleepTime) 

                    packetCounter += 1
                #}
                else:
                #{
                    if not(time.time() - lastSentTime > targetSleepTime):
                        time.sleep( float( ( targetSleepTime - (time.time() - lastSentTime) )) )
                #}
            #}
        #}

        # If we lost some replies/packets, linearly interpolate their delay values.
        delaySeqLen = len(delaySequenceArray[0])
        firstSeenIndex = -1
        lastSeenIndex = -1

        for k in range(0, delaySeqLen):
        #{
            if delaySequenceArray[0][k] != -9999 and delaySequenceArray[1][k] != -9999:
                if firstSeenIndex == -1:
                    firstSeenIndex = k
                lastSeenIndex = k
        #}

        if lastSeenIndex != -1:
        #{
            for k in range(firstSeenIndex, lastSeenIndex+1):
            #{
                if delaySequenceArray[0][k] == -9999:
                    delaySequenceArray[0][k] = int( ( delaySequenceArray[0][k-1] + delaySequenceArray[0][k+1]) /2 )
                if delaySequenceArray[1][k] == -9999:
                    delaySequenceArray[1][k] = int( ( delaySequenceArray[1][k-1] + delaySequenceArray[1][k+1]) /2 )
            #}
        #}

        dirPath = outputDirectory + "/" + hostList[i]
        if not(os.path.isdir(dirPath)):
            os.mkdir(dirPath)

        dirPath = dirPath + "/" + hostList[j]
        os.mkdir(dirPath)

        outputFileHandle = open(dirPath + "/" + "delay.log", 'w')
        if lastSeenIndex != -1:
        #{
            for k in range(firstSeenIndex, lastSeenIndex+1):
            #{
                outputFileHandle.write(str(delaySequenceArray[0][k]) + " " + str(delaySequenceArray[1][k]) + "\n")
            #}
        #}
        outputFileHandle.close()
        if lastSeenIndex == -1:
            print "ERROR: No samples were seen for hosts " + hostList[i] + " " + hostList[j]
    #}

#}

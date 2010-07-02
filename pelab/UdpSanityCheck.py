#!/usr/local/bin/python
#
# EMULAB-COPYRIGHT
# Copyright (c) 2007 University of Utah and the Flux Group.
# All rights reserved.
#

#########################################################
# This script requires two command line arguments
# Argument 1 - Pointer to the 
# /proj/.../exp/project_name/ directory
#
# (The elab-*, planet-* directories should
# have tcpdump log files named SanityCheck.log)
#
# Argument 2 - Output file name
#########################################################

#########################################################
# Sanity check for Flexlab UDP runs
#
# Calculates the correlation of the traffic
# on Planetlab nodes versus the traffic on the
# corresponding Emulab nodes.
#
# Requires that Tcpdump files be saved on elab-*
# and planet-* nodes during the Flexlab run.
#
# Run this script after collecting the logs
# with stop-experiment and point to the 
# directory having the collected logs.
# 
# Cross Correlation reference: Paul Bourke
# http://local.wasp.uwa.edu.au/~pbourke/other/correlate
#
#########################################################

import sys
import re
import os
import array
import math
from posix import *

minAvgSamples = 100
avgTime = 2000000
maxAvgSamples = 500

tmpFile = "/tmp/flexlab-tmp"

if len(sys.argv) < 3:
#{
    print "\nUsage: python UdpSanityCheck.py proj_exp_expname_directory output_filename\n"
    sys.exit()
#}

# Take the directory to be processed from the command line arguments
logDir = sys.argv[1]
OutputFilename = sys.argv[2]

OutputFileHandle = file(OutputFilename, "w")

###############################################################
###############################################################

def CalcAvgThroughput(fileName, outputFile, minSamples, avgTimeScale, maxSamples):
#{
    initTime = 0
    lastPacketTime = 0
    arrayIndex = 0
    packetAvgBytes = 0
    packetAvgTime = 0
    numPackets = 0
    numLines = 0
    packetSizeArray = [0] * maxAvgSamples
    packetTimeArray = [0] * maxAvgSamples

    tmpFileHandle = file(tmpFile,"w")
    outputFileHandle = file(outputFile,"w")

    command = "tcpdump -r " + fileName + " 2> /dev/null "
    inputFile = popen(command, "r")
    regExp = re.compile('^(\d\d):(\d\d):(\d\d)\.(\d{6})\sIP\s([\w\d\.\-]*?)\.([\d\w\-]*)\s>\s([\w\d\.\-]*?)\.([\d\w\-]*):\sUDP,\slength\s(\d*)')

    for lineRead in inputFile:
    #{
        match = regExp.match(lineRead)


        if match:
        #{
            time = ((int(match.group(1)))*3600 + (int(match.group(2)))*60 + int(match.group(3)))*1000000 + int(match.group(4))

            if initTime == 0:
            #{
                initTime = time
                lastPacketTime = time
            #}
            else:
            #{
                packetSizeArray[arrayIndex] = int(match.group(9))
                packetTimeArray[arrayIndex] = time - lastPacketTime

                packetAvgBytes = packetAvgBytes + int(match.group(9))
                packetAvgTime = packetAvgTime + packetTimeArray[arrayIndex]

                arrayIndex = (arrayIndex + 1) % maxSamples

                if numPackets < maxSamples:
                #{
                    numPackets += 1
                #}

                if numPackets >= minSamples:
                #{

                    if numPackets < maxSamples:
                    #{
                        sampleCount = numPackets
                    #}
                    else:
                    #{
                        sampleCount = maxSamples
                    #}

                    timePeriod = 0
                    packetSizeSum = 0
                    index = 0
                    iter = 0
                    throughput = 0

                    while iter < maxSamples and timePeriod < avgTimeScale :
                    #{
                        index = (arrayIndex - 1 - iter + maxSamples)%maxSamples
                        timePeriod += packetTimeArray[index]
                        packetSizeSum += packetSizeArray[index]
                        iter += 1
                    #}

                    if timePeriod != 0 :
                    #{
                        throughput = 8000000*packetSizeSum/(timePeriod*1024.0)
                        tmpFileHandle.write(str(time-initTime)+" "+str(throughput)+"\n")
                        numLines += 1
                    #}

                #}
            #}

            lastPacketTime = time
        #}

    #}

    outputFileHandle.write(str(numLines) + "\n")
    outputFileHandle.close()

    tmpFileHandle.close()
    inputFile.close()

    os.system("cat " + tmpFile + " >> " + outputFile)
    os.remove(tmpFile)
#}

###############################################################
###############################################################
def PreProcess(inputFile, outputFile):
#{

    i = 0
    j = 0
    rowCount = 0
    lastTimeVal = 0
    currentTimeVal = 0
    index = 0
    slope = 0.0
    intercept = 0.0
    indexChanged = 1
    intermValue = 0
    numRows = 0
    numLines = 0

    inputFileHandle = file(inputFile,"r")
    outputFileHandle = file(outputFile,"w")
    tmpFileHandle = file(tmpFile,"w")

    numRows = int( (inputFileHandle.readline()).split()[0] )
    timeArray = [0] * (numRows + 1)
    valueArray = [0.0] * (numRows + 1)

    i = 1
    splitString = []

    for lineRead in inputFileHandle:
    #{
        splitString = lineRead.split()
        timeArray[i] = int(splitString[0])
        valueArray[i] = float(splitString[1])
        i += 1
    #}

    inputFileHandle.close()

    rowCount = numRows
    lastTimeVal = timeArray[rowCount-1]

    index = 0
    while 1:
    #{
        if index > rowCount - 1:
            break

        # Recalculate the slope and intercept
        # for the new index.
        if indexChanged:
        #{
            slope = (valueArray[index+1] - valueArray[index]) \
                    /float(timeArray[index+1] - timeArray[index])
            intercept = valueArray[index] - slope*timeArray[index]
            indexChanged = 0
        #}

        if currentTimeVal == timeArray[index]:
        #{
            tmpFileHandle.write(str(currentTimeVal) + " " + \
                    str(valueArray[index]) + "\n")
            currentTimeVal += 10000
            numLines += 1
        #}
        elif currentTimeVal == timeArray[index+1]:
        #{
            tmpFileHandle.write(str(currentTimeVal) + " " + \
                    str(valueArray[index+1]) + "\n")
            currentTimeVal += 10000
            index += 1
            indexChanged = 1
            numLines += 1
        #}
        elif currentTimeVal < timeArray[index+1]:
        #{
            intermValue = slope*timeArray[index] + intercept
            tmpFileHandle.write(str(currentTimeVal) + " " + \
                    str(intermValue) + "\n")
            currentTimeVal += 10000
            numLines += 1
        #}
        elif currentTimeVal > timeArray[index+1]:
        #{
            index += 1
            indexChanged = 1
        #}
    #}

    outputFileHandle.write(str(numLines) + "\n")
    outputFileHandle.close()
    tmpFileHandle.close()
    os.system("cat " + tmpFile + " >> " + outputFile)
    os.remove(tmpFile)

#}
##############################################################
##############################################################
def CalcCorrelation(InFile1, InFile2):
#{

    i = 0; j = 0
    mx = 0.0;my = 0.0; sx = 0.0; sy = 0.0; sxy = 0.0; denom = 0.0; r = 0.0
    n = 0
    maxdelay = 1000; delay = 0
    count1 = 0 ; count2 = 0
    rows1 = 0; rows2 = 0

    InFileHandle1 = file(InFile1, "r")
    InFileHandle2 = file(InFile2, "r")

    rows1 = int( (InFileHandle1.readline()).split()[0] )
    rows2 = int( (InFileHandle2.readline()).split()[0] )

    if rows1 < rows2:
    #{
        n = rows1
    #}
    else:
    #{
        n = rows2
    #}

    x = [0.0] * n
    y = [0.0] * n

    count1 = 0
    for lineRead in InFileHandle1:
    #{
        if count1 >= n:
            break
        x[count1] = float( lineRead.split()[1] )
        count1 += 1
    #}

    InFileHandle1.close()

    count2 = 0

    for lineRead in InFileHandle2:
    #{
        if count2 >= n:
            break
        y[count2] = float( lineRead.split()[1] )
        count2 += 1
    #}

    InFileHandle2.close()


    # Calculate the mean of the two series x[], y[] 
    mx = 0
    my = 0   

    for i in range(0, n):
    #{
        mx += x[i]
        my += y[i]
    #}

    mx /= n
    my /= n

    # Calculate the denominator
    sx = 0
    sy = 0

    for i in range(0,n): 
    #{
        sx += (x[i] - mx) * (x[i] - mx)
        sy += (y[i] - my) * (y[i] - my)
    #}

    denom = math.sqrt(sx*sy)

    maxCorr = -1.0
    minCorr = 1.0
    maxDelay = 0
    minDelay = 0

    # Calculate the correlation series 
    for delay in range(-maxdelay,maxdelay):
    #{
        sxy = 0

        for i in range(0,n): 
        #{
            j = i + delay

            if  j < 0 or j >= n:
                continue
            else:
                sxy += (x[i] - mx) * (y[j] - my)
        #}
        r = sxy / denom
#        OutputFileHandle.write(str(delay) + " " +str(r) + "\n")

        if r > maxCorr:
        #{
            maxCorr = r
            maxDelay = delay
        #}
        if r < minCorr:
        #{
            minCorr = r
            minDelay = delay
        #}
    #}
    OutputFileHandle.write("Max correlation = " + str(maxCorr) + " at Delay = " + str(maxDelay*10) + " millisec \n")
    OutputFileHandle.write("Min correlation = " + str(minCorr) + " at Delay = " + str(minDelay*10) + " millisec \n\n")

#}
###############################################################
###############################################################
def processFiles(fileNameElab, fileNamePlab):
#{
    tmpFileElab1 = "/tmp/tmpFileElab1"
    tmpFileElab2 = "/tmp/tmpFileElab2"
    tmpFilePlab1 = "/tmp/tmpFilePlab1"
    tmpFilePlab2 = "/tmp/tmpFilePlab2"
    CalcAvgThroughput(fileNameElab, tmpFileElab1, minAvgSamples, avgTime, maxAvgSamples)
    CalcAvgThroughput(fileNamePlab, tmpFilePlab1, minAvgSamples, avgTime, maxAvgSamples)

    PreProcess(tmpFileElab1, tmpFileElab2)
    PreProcess(tmpFilePlab1, tmpFilePlab2)

    OutputFileHandle.write("File1 = " + fileNameElab + " \nFile2 = " + fileNamePlab + "\n")
    CalcCorrelation(tmpFilePlab2, tmpFileElab2)

    os.remove(tmpFileElab1)
    os.remove(tmpFileElab2)
    os.remove(tmpFilePlab1)
    os.remove(tmpFilePlab2)

#}
###############################################################
###############################################################
def Main():
#{


    for i in range(1,500):
    #{
        dirNameElab = logDir + "/logs/elab-" + str(i)
        dirNamePlab = logDir + "/logs/planet-" + str(i)
        if os.path.exists(dirNameElab) and os.path.exists(dirNamePlab) :
        #{

            fileNameElab = dirNameElab + "/local/logs/" + "SanityCheck.log"
            fileNamePlab = dirNamePlab + "/local/logs/" + "SanityCheck.log"

            if not(os.path.exists(fileNameElab) and os.path.exists(fileNamePlab)) :
            #{

                print "Warning: tcpdump log files required for Sanity" \
                    + "checking are missing from '" + dirNameElab + \
                    "' or '" + dirNamePlab + "'"
            #}
            else:
            #{

                processFiles(fileNameElab, fileNamePlab)
            #}
        #}
        else:
            break
    #}

    OutputFileHandle.close()
#}
###############################################################
###############################################################

Main()

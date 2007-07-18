#!/usr/local/bin/python
#
# EMULAB-COPYRIGHT
# Copyright (c) 2007 University of Utah and the Flux Group.
# All rights reserved.
#

#########################################################
# Input:
# This script requires one command line arguments
# Argument 1 - Pointer to the 
# /proj/.../exp/project_name/ directory
#
# (The elab-*, planet-* directories should
# have tcpdump log files named SanityCheck.log)
#
# Output:
# The report will be a file named SanityCheckReport.txt
# in the "given_directory/logs"
# Details for each pair of machines - the exact
# cross correlation value, Percentage difference between
# Flexlab & Planetlab transfer rates etc. can be found
# in "given_directory/logs/SanityCheckDetails.txt"
#
#########################################################

#########################################################
# Sanity check for Flexlab TCP & UDP runs
#
# Calculates the correlation of the traffic
# on Planetlab nodes versus the traffic on the
# corresponding Flexlab (Emulab) nodes.
#
# Requires that Tcpdump files be saved on elab-*
# and planet-* nodes during the Flexlab run.
# This can be done by passing "-t" option
# to start-experiment and stop-experiment scripts
#
# This script can also be executed in stand-alone mode 
# after collecting the logs
# with stop-experiment  - Just point it to the 
# directory containing the logs.
# 
# Cross Correlation references: 
# -----------------------------
# FFT method  - http://en.wikipedia.org/wiki/Cross-correlation
# Paul Bourke - http://local.wasp.uwa.edu.au/~pbourke/other/correlate
#
#########################################################

import sys
import re
import os
import array
import math
from posix import *
from optparse import OptionParser

global minAvgSamples
global avgTime
global maxAvgSamples 

# Take the directory to be processed from the command line arguments
global logDir
global OutputFileHandle 


tmpFile = "/tmp/flexlab-tmp"

###############################################################
###############################################################

def CalcBucketAvgThroughput(fileName, outputFile, bucketSize, transportFlag):
#{
    initTime = 0
    lastPacketTime = 0
    arrayIndex = 0
    packetAvgBytes = 0
    packetAvgTime = 0
    numPackets = 0
    numLines = 0
    lastBucketThreshold = 0
    timePeriod = 0

    maxAvgSamples = 100000

    packetSizeArray = [0] * maxAvgSamples
    packetTimeArray = [0] * maxAvgSamples


    tmpFileHandle = file(tmpFile,"w")
    outputFileHandle = file(outputFile,"w")

    command = "tcpdump -r " + fileName + " 2> /dev/null "
    inputFile = popen(command, "r")

    if transportFlag == 0 : # TCP 
        regExp = re.compile('^(\d\d):(\d\d):(\d\d)\.(\d{6})\sIP\s([\w\d\.\-]*?)\.([\d\w\-]*)\s>\s([\w\d\.\-]*?)\.([\d\w\-]*):\s[\w\d\s\.\:]*?\(([\d]*)\)[\w\d\s]*')
    elif transportFlag == 1: # UDP
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

                timePeriod += packetTimeArray[arrayIndex]

                if timePeriod >= bucketSize:
                #{
                    iter = 0
                    packetSizeSum = 0

                    while iter <= arrayIndex :
                    #{
                        packetSizeSum += packetSizeArray[iter]
                        iter += 1
                    #}
                    throughput = 8000000*packetSizeSum/(timePeriod*1024.0)

                    numLinesToWrite = timePeriod / 10000;

                    iter = 0
                    tmpTimeCounter = lastBucketThreshold * 10000 + 10000

                    while iter <  numLinesToWrite:
                        tmpFileHandle.write(str(tmpTimeCounter)+" "+str(throughput)+"\n")
                        iter += 1
                        numLines += 1
                        tmpTimeCounter = tmpTimeCounter + 10000

                    lastBucketThreshold = lastBucketThreshold + timePeriod / 10000

                    arrayIndex = 0
                    timePeriod = 0
                #}
                else:
                    arrayIndex = (arrayIndex + 1) % maxAvgSamples

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

    del packetSizeArray
    del packetTimeArray

    # Return the avg. transfer rate - in kbps
    return (8000000*packetAvgBytes/(packetAvgTime*1024.0))

#}
###############################################################
###############################################################

def CalcMovingAvgThroughput(fileName, outputFile, minSamples, avgTimeScale, maxSamples, transportFlag):
#{
    initTime = 0
    lastPacketTime = 0
    arrayIndex = 0
    packetAvgBytes = 0
    packetAvgTime = 0
    numLines = 0
    packetSizeArray = [0] * maxAvgSamples
    packetTimeArray = [0] * maxAvgSamples

    tmpFileHandle = file(tmpFile,"w")
    outputFileHandle = file(outputFile,"w")

    command = "tcpdump -r " + fileName + " 2> /dev/null "
    inputFile = popen(command, "r")

    if transportFlag == 0 : # TCP 
        regExp = re.compile('^(\d\d):(\d\d):(\d\d)\.(\d{6})\sIP\s([\w\d\.\-]*?)\.([\d\w\-]*)\s>\s([\w\d\.\-]*?)\.([\d\w\-]*):\s[\w\d\s\.\:]*?\(([\d]*)\)[\w\d\s]*')
    elif transportFlag == 1: # UDP
        regExp = re.compile('^(\d\d):(\d\d):(\d\d)\.(\d{6})\sIP\s([\w\d\.\-]*?)\.([\d\w\-]*)\s>\s([\w\d\.\-]*?)\.([\d\w\-]*):\sUDP,\slength\s(\d*)')


    runningByteTotal = 0
    runningTimeTotal = 0
    runningPacketTotal = 0
    lastIndex = 0

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
                currentPacketSize = int(match.group(9))
                currentPacketTime = time - lastPacketTime

                runningByteTotal += currentPacketSize
                runningTimeTotal += currentPacketTime
                runningPacketTotal += 1

                packetAvgBytes = packetAvgBytes + int(match.group(9))
                packetAvgTime = packetAvgTime + packetTimeArray[arrayIndex]

                if runningPacketTotal >= minSamples:
                #{
                    if (runningPacketTotal < maxSamples) and (runningTimeTotal < avgTimeScale):
                    #{
                        throughput = 8000000*runningByteTotal/(runningTimeTotal*1024.0)
                    #}
                    elif runningPacketTotal < maxSamples:
                    #{
                        while (runningPacketTotal >= minSamples) and (runningTimeTotal > avgTimeScale):
                        #{
                            runningTimeTotal -= packetTimeArray[lastIndex]
                            runningByteTotal -= packetSizeArray[lastIndex]
                            lastIndex = (lastIndex + 1)%maxSamples
                            runningPacketTotal -= 1
                        #}
                        throughput = 8000000*runningByteTotal/(runningTimeTotal*1024.0)
                    #}
                    elif runningTimeTotal < avgTimeScale:
                    #{
                        runningByteTotal -= packetSizeArray[lastIndex]
                        runningTimeTotal -= packetTimeArray[lastIndex]
                        runningPacketTotal -= 1
                        lastIndex = (lastIndex + 1)%maxSamples
                        throughput = 8000000*runningByteTotal/(runningTimeTotal*1024.0)
                    #}
                    else:
                    #{
                        while (runningPacketTotal >= minSamples) and (runningPacketTotal > maxSamples) and (runningTimeTotal > avgTimeScale):
                        #{
                            runningTimeTotal -= packetTimeArray[lastIndex]
                            runningByteTotal -= packetSizeArray[lastIndex]
                            lastIndex = (lastIndex + 1)%maxSamples
                            runningPacketTotal -= 1
                        #}
                        throughput = 8000000*runningByteTotal/(runningTimeTotal*1024.0)
                    #}

                    if runningTimeTotal != 0 :
                    #{
                        tmpFileHandle.write(str(time-initTime)+" "+str(throughput)+"\n")
                        numLines += 1
                    #}
                #}

                packetSizeArray[arrayIndex] = currentPacketSize
                packetTimeArray[arrayIndex] = currentPacketTime
                arrayIndex = (arrayIndex + 1) % maxSamples
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

    del packetSizeArray
    del packetTimeArray

    # Return the avg. transfer rate - in kbps
    if packetAvgTime == 0:
        return 0

    return (8000000*packetAvgBytes/(packetAvgTime*1024.0))
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

# Skip the bandwidth comparison for 5 seconds, while Flexlab converges
# to the correct available bandwidth and delay estimates.
    initialThreshold = 500

    inputFileHandle = file(inputFile,"r")
    outputFileHandle = file(outputFile,"w")

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
            if numLines > initialThreshold:
                outputFileHandle.write(str(currentTimeVal) + " " + \
                        str(valueArray[index]) + "\n")
            currentTimeVal += 10000
            numLines += 1
        #}
        elif currentTimeVal == timeArray[index+1]:
        #{
            if numLines > initialThreshold:
                outputFileHandle.write(str(currentTimeVal) + " " + \
                        str(valueArray[index+1]) + "\n")
            currentTimeVal += 10000
            index += 1
            indexChanged = 1
            numLines += 1
        #}
        elif currentTimeVal < timeArray[index+1]:
        #{
            intermValue = slope*timeArray[index] + intercept
            if numLines > initialThreshold:
                outputFileHandle.write(str(currentTimeVal) + " " + \
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

    outputFileHandle.close()

    del timeArray
    del valueArray
#}
##############################################################
##############################################################
def CalcCorrelation(InFile1, InFile2, transportFlag, plabAvg, elabAvg):
#{

    delayVal = 0
    maxCorr = -2.0
    lineCount1 = 0
    lineCount2 = 0
    numLines = 0

    pipeObj = popen("wc -l " + InFile1, "r")
    lineCount1 = int(pipeObj.readline().split()[0])
    pipeObj.close()

    pipeObj = popen("wc -l " + InFile2, "r")
    lineCount2 = int(pipeObj.readline().split()[0])
    pipeObj.close()

    if lineCount1 < lineCount2:
        numLines = lineCount1
    else:
        numLines = lineCount2

    if numLines != 0:
    #{
        os.system("sh CorrelationR.sh " + InFile1 + " " + InFile2 + " " + "/tmp/Correlation.log" )

        InFileHandle1 = file("/tmp/Correlation.log", "r")

        for lineRead in InFileHandle1:
        #{
            shiftVal = int( lineRead.split()[0] )
            corrVal = float( lineRead.split()[1] )

            if corrVal > maxCorr:
                maxCorr = corrVal
                delayVal = shiftVal
        #}

        InFileHandle1.close()
    #}
    else:
    #{
        delayVal = 0
        maxCorr = 0.0
    #}

    if transportFlag == 0:
        transport = "TCP"
    elif transportFlag == 1:
        transport = "UDP"

    diffTransferRate = elabAvg - plabAvg

    if elabAvg != 0:
        diffPercentage = 100.0*(float(diffTransferRate))/(float(elabAvg))
    else:
        diffPercentage = 0.0

    OutputFileHandle.write("Max_Correlation=" + str(maxCorr) + ":Delay=" + str(delayVal*10) + "ms:Transport=" + transport + ":N=" + str(numLines) + ":KbpsPercentDiff=" + str(diffPercentage) + "\n")

#}

###############################################################
###############################################################
def processFiles(fileNameElab, fileNamePlab, averagingMethod, transportFlag):
#{
    tmpFileElab1 = "/tmp/tmpFileElab1"
    tmpFileElab2 = "/tmp/tmpFileElab2"
    tmpFilePlab1 = "/tmp/tmpFilePlab1"
    tmpFilePlab2 = "/tmp/tmpFilePlab2"


    if averagingMethod == 0: # Moving average
    #{
        elabAvg = CalcMovingAvgThroughput(fileNameElab, tmpFileElab1, minAvgSamples, avgTime, maxAvgSamples, transportFlag)
        sys.stdout.write("#")
        sys.stdout.flush()
        plabAvg = CalcMovingAvgThroughput(fileNamePlab, tmpFilePlab1, minAvgSamples, avgTime, maxAvgSamples, transportFlag)
        sys.stdout.write("#")
        sys.stdout.flush()

        PreProcess(tmpFileElab1, tmpFileElab2)
        sys.stdout.write("#")
        sys.stdout.flush()
        PreProcess(tmpFilePlab1, tmpFilePlab2)
        sys.stdout.write("#")
        sys.stdout.flush()
        CalcCorrelation(tmpFilePlab2, tmpFileElab2, transportFlag, plabAvg, elabAvg)
        sys.stdout.write("#")
        sys.stdout.flush()

        os.remove(tmpFileElab1)
        os.remove(tmpFileElab2)
        os.remove(tmpFilePlab1)
        os.remove(tmpFilePlab2)
    #}
    elif averagingMethod == 1: # Bucket average
    #{
        elabAvg = CalcBucketAvgThroughput(fileNameElab, tmpFileElab1, avgTime, transportFlag)
        sys.stdout.write("#")
        sys.stdout.flush()
        plabAvg = CalcBucketAvgThroughput(fileNamePlab, tmpFilePlab1, avgTime, transportFlag)
        sys.stdout.write("#")
        sys.stdout.flush()
        CalcCorrelationFFT(tmpFilePlab1, tmpFileElab1, transportFlag, plabAvg, elabAvg)
        sys.stdout.write("#")
        sys.stdout.flush()

        os.remove(tmpFileElab1)
        os.remove(tmpFilePlab1)
    #}

#}
###############################################################
###############################################################
# This is the current set of crieteria for declaring a run
# to be valid.
#
# For all pairs of nodes in the Flexlab experiment:
# a) Cross-correlation > 0.5
# b) Offset used for this value of cross-correlation <= 0
# c) Offset > -8 seconds
# d) Difference between Flexlab & Planetlab transfer rates
#    is less than 40%
# e) SanityCheck Tcpdump files exist for all pairs of nodes.
#
def PrintReport(correlationDataFile, numNodes, numNodesProcessed, reportFileName):
#{
    correlationFileHandle = file(correlationDataFile, "r")
    reportFileHandle = file (reportFileName, "w")

    correlationRegex = re.compile('Max_Correlation=([\d\.]*?):Delay=([\-\d]*?)ms:Transport=([\w]*?):N=([\d]*):KbpsPercentDiff=([\-\d\.]*?)$')

    errorFlag = 0

    for i in range(1, numNodesProcessed+1):
    #{

        # Read the two lines with the path to the SanityCheck files.
        flexLabFile = correlationFileHandle.readline().strip()
        pLabFile = correlationFileHandle.readline().strip()

        for j in range(0,2):
        #{
            correlationLine = correlationFileHandle.readline()

            matchObj = correlationRegex.match(correlationLine)

            if  ( ( int(matchObj.group(4)) ) and  \
                    ( ( float(matchObj.group(1)) < 0.5 ) or ( int(matchObj.group(2)) > 0 ) or ( int(matchObj.group(2)) < -8000 )  or ( float(matchObj.group(5)) > 40.0 or float(matchObj.group(5)) < -40.0 ) ) ) :
            #{
                print "\n***** ERROR: This Flexlab run is possibly tainted. \nDetected " + matchObj.group(3) + " cross-correlation mismatch for files '" + flexLabFile + "' and '" + pLabFile + "'"
                reportFileHandle.write( "***** ERROR: This Flexlab run is possibly tainted. \nDetected " + matchObj.group(3) + " cross-correlation mismatch for files '" + flexLabFile + "' and '" + pLabFile + "'\n")
                errorFlag = 1

            #}
# Check the delay offset by comparing it against the sum
# of Emulab-to-Planetlab RTT + Planetlab internode RTT.
# Not implemented yet.
            if  ( ( int(matchObj.group(4)) ) and ( float (matchObj.group(1)) > 0.0 ) and ( int(matchObj.group(2)) == 0 ) ) :
                
            #{
                print "\n***** ERROR: This Flexlab run is possibly tainted. \nDetected " + matchObj.group(3) + " cross-correlation mismatch for files '" + flexLabFile + "' and '" + pLabFile + "'"
                reportFileHandle.write( "***** ERROR: This Flexlab run is possibly tainted. \nDetected " + matchObj.group(3) + " cross-correlation mismatch for files '" + flexLabFile + "' and '" + pLabFile + "'\n")
                errorFlag = 1
            #}

        #}

    #}

    if numNodesProcessed != numNodes :
    #{
        print "\n***** ERROR: " + str(numNodes - numNodesProcessed) + " files required for cross-correlation sanity check are missing.\nThis Flexlab run is possibly tainted."
        reportFileHandle.write("***** ERROR: " + str(numNodes - numNodesProcessed) + " files required for cross-correlation sanity check are missing.\nThis Flexlab run is possibly tainted.\n")

        errorFlag = 1
    #}

    if errorFlag == 0:
    #{
        print "\n##### Cross-correlation checks successful. This Flexlab run is Good!"
        reportFileHandle.write("##### Cross-correlation checks successful. This Flexlab run is Good!\n")
    #}

    reportFileHandle.close()
    correlationFileHandle.close()

#}

###############################################################
###############################################################
def Main():
#{
    global minAvgSamples 
    global avgTime 
    global maxAvgSamples
    global logDir 
    global OutputFileHandle 

    # Parse the command line options
    usage = "usage: %prog [options] Input_Directory_Path"

    cmdParser = OptionParser(usage=usage)

    cmdParser.add_option("-t","--tcp",action="store_true",dest="tcp",default=False,help="Check TCP packet flows for correlation")
            
    cmdParser.add_option("-u", "--udp", action="store_true", dest="udp",\
            default=False, help="Check UDP packet flows for correlation")

    cmdParser.add_option("-b", "--bucket", action="store_true", dest="avgMethod", \
            default=False, help="Use Bucket average instead of Moving average")

    cmdParser.add_option("-a", "--avgTime", action="store", dest="avgTime", \
            type="int",default=2000000, help="Time period for Moving/Bucket Average")

    cmdParser.add_option("-m", "--minSamples", action="store", dest="minAvgSamples", \
            type="int",default=100, help="Min Samples for Moving Average")

    cmdParser.add_option("-x", "--maxSamples", action="store", dest="maxAvgSamples", \
            type="int",default=500, help="Max Samples for Moving Average")

    (cmdOptions, args) = cmdParser.parse_args()

    if len(args) != 1:
        cmdParser.print_help()
        sys.exit()

    logDir = args[0]
    OutputFileName = logDir + "/logs/SanityCheckDetails.txt"
    reportFileName = logDir + "/logs/SanityCheckReport.txt"

    if not(os.path.exists(logDir + "/logs")) :
    #{
        print "##### Error: Path given as input is invalid. Provide the path upto(not including) logs/ directory"
        sys.exit()
    #}

    OutputFileHandle = file(OutputFileName, "w")

    if cmdOptions.avgMethod:
        avgMethod = 1
    else:
        avgMethod = 0

    if cmdOptions.tcp and cmdOptions.udp:
        transportFlag = 2
        iterator = 0
    elif cmdOptions.tcp:
        transportFlag = 1
        iterator = 0
    elif cmdOptions.udp:
        transportFlag = 2
        iterator = 1
    else:
        transportFlag = 2
        iterator = 0


    minAvgSamples = cmdOptions.minAvgSamples
    avgTime = cmdOptions.avgTime
    maxAvgSamples = cmdOptions.maxAvgSamples

    numNodes = 0
    numNodesProcessed = 0

    for i in range(1,100):
    #{
        dirNameElab = logDir + "/logs/elab-" + str(i)
        dirNamePlab = logDir + "/logs/planet-" + str(i)
        if os.path.exists(dirNameElab) and os.path.exists(dirNamePlab) :
        #{
            numNodes = numNodes + 1

            fileNameElab = dirNameElab + "/local/logs/" + "SanityCheck.log"
            fileNamePlab = dirNamePlab + "/local/logs/" + "SanityCheck.log"

            if (os.path.exists(fileNameElab) and os.path.exists(fileNamePlab)) :
            #{
                numNodesProcessed = numNodesProcessed + 1

                if (os.stat(fileNameElab).st_size > 0 and os.stat(fileNamePlab).st_size > 0):
                #{

                    OutputFileHandle.write("Flexlab_File = " + fileNameElab + " \nPlab_File = " + fileNamePlab + "\n")

                    if transportFlag == 2 and iterator == 1:
                        OutputFileHandle.write("Max_Correlation=0.000000" + ":Delay=0" + "ms:Transport=TCP" + ":N=0:KbpsPercentDiff=0.000000" + "\n")

                    for j in range(iterator, transportFlag):
                        processFiles(fileNameElab, fileNamePlab, avgMethod, j)

                    if transportFlag == 1 and iterator == 0:
                        OutputFileHandle.write("Max_Correlation=0.000000" + ":Delay=0" + "ms:Transport=UDP" + ":N=0:KbpsPercentDiff=0.000000" + "\n")

                #}
                else:
                #{
                    OutputFileHandle.write("Flexlab_File = " + fileNameElab + " \nPlab_File = " + fileNamePlab + "\n")
                    OutputFileHandle.write("Max_Correlation=0.000000" + ":Delay=0" + "ms:Transport=TCP" + ":N=0:KbpsPercentDiff=0.000000" + "\n")
                    OutputFileHandle.write("Max_Correlation=0.000000" + ":Delay=0" + "ms:Transport=UDP" + ":N=0:KbpsPercentDiff=0.000000" + "\n")
                #}
            #}
        #}
        else:
            break
    #}

    OutputFileHandle.close()

    PrintReport(OutputFileName, numNodes, numNodesProcessed, reportFileName)

#}
###############################################################
###############################################################

Main()

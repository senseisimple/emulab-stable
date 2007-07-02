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
# corresponding Emulab nodes.
#
# Requires that Tcpdump files be saved on elab-*
# and planet-* nodes during the Flexlab run.
# This can be done by passing "-t" option
# to start-experiment and stop-experiment scripts
#
# This script can also be done in stand-alone mode 
# after collecting the logs
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
from numpy import *
from numpy.fft import *
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
    numPackets = 0
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

    del packetSizeArray
    del packetTimeArray

    # Return the avg. transfer rate - in kbps
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

    del timeArray
    del valueArray
#}
##############################################################
##############################################################
def FindPowerOf2(n):
#{
    mask = 1
    bitPosition = 0
    multipleOf2 = 0

    for i in range(1, 32):
    #{
        if n & mask == mask:
            bitPosition = i
            multipleOf2 += 1

        mask = mask*2
    #}

    if multipleOf2 == 1:
        return n
    else:
        return int(math.pow(2,bitPosition)) 

#}
##############################################################
##############################################################
def CalcCorrelationFFT(InFile1, InFile2, transportFlag, plabAvg, elabAvg):
#{

    i = 0; j = 0
    mx = 0.0;my = 0.0; sx = 0.0; sy = 0.0; sxy = 0.0; denom = 0.0
    n = 0
    delay = 0
    count1 = 0 ; count2 = 0
    rows1 = 0; rows2 = 0
    maxCorr = 0

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

    arraySize = FindPowerOf2(n)

    x = [0.0] * arraySize
    y = [0.0] * arraySize

    xFFT = empty(arraySize, complex)
    yFFT = empty(arraySize, complex)

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

    # Calculate the delay interval with maximum
    # correlation using FFT
    xFFT = fft(x)
    yFFT = fft(y)

    inverseCorrArray = empty(arraySize,complex)
    corrArray = empty(arraySize,complex)

    for i in range(0, n):
    #{
        inverseCorrArray[i] = xFFT[i].conjugate() * yFFT[i]
    #}

    corrArray = ifft(inverseCorrArray)

    maxCorr = -99999999
    maxIndex = 0

    # Find out the optimal shift in -10 sec: 10 sec range
    for i in range(-1000, 1000):
    #{
        if corrArray[i].real > maxCorr:
            maxCorr = corrArray[i].real
            maxIndex = i
    #}

    delay = maxIndex

    # Calculate the mean of the two series x[], y[] 
    mx = 0
    my = 0   

    # Both the files are empty - just return.
    if n != 0:
    #{

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

        # Calculate the correlation coefficient for the 
        # above determinted shift
        sxy = 0

        for i in range(0,n): 
        #{
            j = i + delay

            if  j < 0 or j >= n:
                continue
            else:
                sxy += (x[i] - mx) * (y[j] - my)
        #}
        maxCorr = sxy / denom

    #}
    else:
    #{
        delay = 0
        maxCorr = 0
    #}

    if transportFlag == 0:
        transport = "TCP"
    elif transportFlag == 1:
        transport = "UDP"

    diffTransferRate = elabAvg - plabAvg

    diffPercentage = 100.0*(float(diffTransferRate))/(float(elabAvg))

    OutputFileHandle.write("Max_Correlation=" + str(maxCorr) + ":Delay=" + str(delay*10) + "ms:Transport=" + transport + ":N=" + str(n) + ":KbpsPercentDiff=" + str(diffPercentage) + "\n")

    del x
    del y
    del xFFT
    del yFFT
    del corrArray
    del inverseCorrArray
#}
##############################################################
##############################################################
def CalcCorrelation(InFile1, InFile2, transportFlag, plabAvg, elabAvg):
#{

    i = 0; j = 0
    mx = 0.0;my = 0.0; sx = 0.0; sy = 0.0; sxy = 0.0; denom = 0.0; r = 0.0
    n = 0
    maxdelay = 1000; delay = 0
    count1 = 0 ; count2 = 0
    rows1 = 0; rows2 = 0
    maxCorr = 0

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

    # Both the files are empty - just return.
    if n != 0:
    #{

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

    # Subtract the mean of each series from individual values
    # This saves us a lot of time in the nested for loop below.
        for i in range(0,n):
        #{
            x[i] = x[i] - mx
            y[i] = y[i] - my
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
            sys.stdout.write("$")
            sys.stdout.flush()

            for i in range(0,n): 
            #{
                j = i + delay

                if  j < 0 or j >= n:
                    continue
                else:
                    #sxy += (x[i] - mx) * (y[j] - my)
                    sxy += (x[i]) * (y[j])
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
    #}
    else:
    #{
        maxDelay = 0
        maxCorr = 0
    #}

    if transportFlag == 0:
        transport = "TCP"
    elif transportFlag == 1:
        transport = "UDP"

    diffTransferRate = elabAvg - plabAvg

    diffPercentage = 100.0*(float(diffTransferRate))/(float(elabAvg))

    OutputFileHandle.write("Max_Correlation=" + str(maxCorr) + ":Delay=" + str(maxDelay*10) + "ms:Transport=" + transport + ":N=" + str(n) + ":KbpsPercentDiff=" + str(diffPercentage) + "\n")

    del x
    del y


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
        CalcCorrelationFFT(tmpFilePlab2, tmpFileElab2, transportFlag, plabAvg, elabAvg)
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
# b) Offset used for this value of cross-correlation >= 0
# c) Offset < 8 seconds
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
                    ( ( float(matchObj.group(1)) < 0.5 ) or ( int(matchObj.group(2)) < 0 ) or ( int(matchObj.group(2)) > 8000 )  or ( float(matchObj.group(5)) > 40.0 or float(matchObj.group(5)) < -40.0 ) ) ) :
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

    if not(os.path.exists(OutputFileName) and os.path.exists(reportFileName)) :
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

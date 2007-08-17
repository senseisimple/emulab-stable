#!/usr/local/bin/python
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#



import sys,os

delayFileHandle = open(sys.argv[1], 'r')

pathOneDelayList = []
pathTwoDelayList = []

for delayLine in delayFileHandle.readlines():
#{
    pathOneDelay, pathTwoDelay = delayLine.split()

    pathOneDelay = int(pathOneDelay)
    pathTwoDelay = int(pathTwoDelay)

    pathOneDelayList.append(pathOneDelay)
    pathTwoDelayList.append(pathTwoDelay)
#}

delayFileHandle.close()
delayListLen = len(pathOneDelayList)

# Check whether there are a minimum of number
# of delay measurements.
if delayListLen < 25:
#{
    # Indicate an error.
    print "CORRELATION=-2.0"
    sys.exit()
#}

powerOf2 = 1

while True:
#{
    if delayListLen > powerOf2:
    #{
        powerOf2 *= 2
    #}
    else:
    #{
        break
    #}
#}

# Pad the delay sequences with zeros at the end - 
# to make the sequences of length power of '2'.
for i in range(0, powerOf2 - delayListLen):
#{
    pathOneDelayList.append(0)
    pathTwoDelayList.append(0)
#}

tmpWaveletFile = "/tmp/bw-wavelet.tmp"
tmpCorrFile = "/tmp/bw-corr.tmp"

tmpFileHandle = open(tmpWaveletFile, 'w')

for i in range(0, powerOf2):
#{
    tmpFileHandle.write(str(pathOneDelayList[i]) + " " + str(pathTwoDelayList[i]) + "\n")
#}

tmpFileHandle.close()

os.system("source WaveletR.sh " + tmpWaveletFile + " " + tmpCorrFile)


tmpFileHandle = open(tmpCorrFile, 'r')

corrLine = tmpFileHandle.readline()

tmpFileHandle.close()

print "CORRELATION=" + str(float(corrLine.split()[1]))


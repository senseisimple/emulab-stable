#!/usr/local/bin/python

import sys,os

delayFileHandle = open(sys.argv[1], 'r')

pathOneDelayList = []
pathTwoDelayList = []

for delayLine in delayFileHandle.readlines():
#{
    pathOneDelay, pathTwoDelay, ignore1, ignore2 = delayLine.split()

    pathOneDelay = int(pathOneDelay)
    pathTwoDelay = int(pathTwoDelay)

    pathOneDelayList.append(pathOneDelay)
    pathTwoDelayList.append(pathTwoDelay)
#}

delayFileHandle.close()
delayListLen = len(pathOneDelayList)

sys.stderr.write(sys.argv[1] + " LINES=" + str(delayListLen) + "\n")

# Check whether there are a minimum of number
# of delay measurements.
if delayListLen < 25:
#{
    # Indicate an error.
    print "CORRELATION=-2.0"
    sys.exit()
#}

# Execute Min Sik Kim's program to perform wavelet-denoising and
# find the cross-correlation on the processed delay sequences.
os.system("./MinSikKim/MinSikKimProgram " + sys.argv[1]);


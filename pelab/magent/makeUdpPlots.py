import sys
import re
import math

# Read stats from the log output by UdpClient.
inFile = open(sys.argv[1], 'r')

bandwidthFile = "AvailBandwidth.log"
throughputFile = "Throughput.log"
avgThroughputFile = "AvgThroughput.log"
lossFile = "Loss.log"
minDelayFile = "MinDelay.log"
maxDelayFile = "MaxDelay.log"
sendDeviationFile = "SendDeviation.log"
queueDelayFile = "QueueDelay.log"
messageFile = "Messages.log"

# And print out the bandwidth, packet loss, minimum delay and
# queue size into seperate files, so that they can be plotted
# with GnuPlot.
outFileBandwidth = open(bandwidthFile, 'w')
outFileThroughput = open(throughputFile, 'w')
outFileAvgThroughput = open(avgThroughputFile, 'w')
outFileLoss = open(lossFile, 'w')
outFileMinD = open(minDelayFile, 'w')
outFileMaxD = open(maxDelayFile, 'w')
outFileSendDeviation = open(sendDeviationFile, 'w')
outFileQueueD = open(queueDelayFile, 'w')
outFileMessages = open(messageFile, 'w')

regExp = re.compile('^\w*?\s*?\d*?\.\d*?:\s(\w*?):(\w*?)\=(\d*)\,(\w*?)\=([\d\.]*)')

availBandwidth = 0
bandWidthBps = 0
throughputBps = 0

# Array indices, 0 - throughput, 1 - loss, 2 - minimum delay, 3 - queue size
# 4 - deviation in send times, 5 - actual queuing delay per packet
# 6 - average throughput
initTimeArray = [0,0,0,0,0,0,0]
timeDiffArray = [0,0,0,0,0,0,0]
initFlagsArray = [0,0,0,0,0,0,0]

for line in inFile:
	match = regExp.match(line)

	if match:
		if match.group(1) == 'TPUT':
			if initFlagsArray[0] == 0:
				initTimeArray[0] = int(match.group(3))
				timeDiffArray[0] = 0
				initFlagsArray[0] = 1
			else:
				timeDiffArray[0] = int(match.group(3)) - initTimeArray[0]

			if match.group(4) == 'AUTHORITATIVE':
				availBandwidth = float(match.group(5))
			elif match.group(4) == 'TENTATIVE':
				if float(match.group(5)) > availBandwidth:
					availBandwidth = float(match.group(5))

			bandWidthBps = 1024*availBandwidth / ( 8 )
			throughputBps = 1024* ( float(match.group(5) ) ) / ( 8 )
			
			outFileBandwidth.write(str(timeDiffArray[0]) + "  " + str(bandWidthBps) + "\n" )
			outFileThroughput.write(str(timeDiffArray[0]) + "  " + str(throughputBps) + "\n" )

		elif match.group(1) == 'LOSS':
			if initFlagsArray[1] == 0:
				initTimeArray[1] = int(match.group(3))
				timeDiffArray[1] = 0
				initFlagsArray[1] = 1
			else:
				timeDiffArray[1] = int(match.group(3)) - initTimeArray[1]

			outFileLoss.write(str(timeDiffArray[1]) + "  " + match.group(5) + "\n" )

		elif match.group(1) == 'MIND':
			if initFlagsArray[2] == 0:
				initTimeArray[2] = int(match.group(3))
				timeDiffArray[2] = 0
				initFlagsArray[2] = 1
			else:
				timeDiffArray[2] = int(match.group(3)) - initTimeArray[2]

			outFileMinD.write(str(timeDiffArray[2]) + "  " + match.group(5) + "\n" )

		elif match.group(1) == 'MAXD':
			if initFlagsArray[3] == 0:
				initTimeArray[3] = int(match.group(3))
				timeDiffArray[3] = 0
				initFlagsArray[3] = 1
			else:
				timeDiffArray[3] = int(match.group(3)) - initTimeArray[3]

			outFileMaxD.write(str(timeDiffArray[3]) + "  " + match.group(5) + "\n" )
		elif match.group(1) == 'SendDeviation':
			if initFlagsArray[4] == 0:
				initTimeArray[4] = int(match.group(3))
				timeDiffArray[4] = 0
				initFlagsArray[4] = 1
			else:
				timeDiffArray[4] = int(match.group(3)) - initTimeArray[4]

			outFileSendDeviation.write(str(timeDiffArray[4]) + "  " + match.group(5) + "\n" )
		elif match.group(1) == 'ACTUAL_MAXD':
			if initFlagsArray[5] == 0:
				initTimeArray[5] = int(match.group(3))
				timeDiffArray[5] = 0
				initFlagsArray[5] = 1
			else:
				timeDiffArray[5] = int(match.group(3)) - initTimeArray[5]

			outFileQueueD.write(str(timeDiffArray[5]) + "  " + match.group(5) + "\n" )
		elif match.group(1) == 'AVGTPUT':
			if initFlagsArray[6] == 0:
				initTimeArray[6] = int(match.group(3))
				timeDiffArray[6] = 0
				initFlagsArray[6] = 1
			else:
				timeDiffArray[6] = int(match.group(3)) - initTimeArray[6]

			if match.group(4) == 'AUTHORITATIVE':
				availBandwidth = float(match.group(5))
			elif match.group(4) == 'TENTATIVE':
				if float(match.group(5)) > availBandwidth:
					availBandwidth = float(match.group(5))

			bandWidthBps = 1024*availBandwidth / ( 8 )
			
			outFileAvgThroughput.write(str(timeDiffArray[6]) + "  " + str(bandWidthBps) + "\n" )

	else:
		outFileMessages.write(line)




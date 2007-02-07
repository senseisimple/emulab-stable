#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

import sys
import re

inFile = open(sys.argv[1], 'r')

outFile = open(sys.argv[2], 'w')

regExp = re.compile('^(\w*?)\=(\d*)\,(\w*?)\=(\d*)')

count = 0
bandWidth = 0
initTime = 0
timeDiff = 0;
lastTime = 0;
currentTime = 0;
line = ""

for line in inFile:
	match = regExp.match(line)

	if count == 0:
		initTime = int(match.group(2))
		timeDiff = 0
		count = count + 1
		lastTime = initTime
	else:
		currentTime = int(match.group(2))
		timeDiff = currentTime - initTime
		bandWidth = int(match.group(4))*1000000 / ( ( currentTime - lastTime ))
		lastTime = currentTime

	outFile.write(str(timeDiff) + "  " + str(bandWidth) + "\n" )


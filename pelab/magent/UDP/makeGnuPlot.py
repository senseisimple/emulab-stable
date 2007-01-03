import sys
import re

inFile = open(sys.argv[1], 'r')

outFile = open(sys.argv[2], 'w')

regExp = re.compile('^(\w*?)\=(\d*)\,(\w*?)\=([\d\.]*)')

count = 0
availBandwidth = 0
initTime = 0
timeDiff = 0;

for line in inFile:
	match = regExp.match(line)

	if count == 0:
		initTime = match.group(2)
		timeDiff = 0
		count = count + 1
	else:
		timeDiff = int(match.group(2)) - int(initTime)

	if match.group(3) == 'AUTHORITATIVE':
		availBandwidth = float(match.group(4))
	elif match.group(3) == 'TENTATIVE':
		if float(match.group(4)) > availBandwidth:
			availBandwidth = float(match.group(4))
		
	outFile.write(str(timeDiff) + "  " + str(availBandwidth) + "\n" )



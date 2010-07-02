#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#



InputFile="\"$1\""
OutputFile="\"$2\""

/usr/bin/R --no-save  > /dev/null 2>&1 << EOF

library(rwt)

delays <- read.table($InputFile, header=0)

xval <- delays[,1]
yval <- delays[,2]

h <- daubcqf(6)\$h.1

xResult.dwt <- denoise.dwt(xval, h)
yResult.dwt <- denoise.dwt(yval, h)

correctedX <- xResult.dwt\$xd
correctedY <- yResult.dwt\$xd

xvec <- as.vector(correctedX)
yvec <- as.vector(correctedY)

corrObj <- ccf(xvec, yvec, lag.max=0, type="correlation", plot=FALSE)
dataFrame <- data.frame(corrObj[[4]], corrObj[[1]])
write.table(dataFrame,file=$OutputFile,quote=FALSE,row.names=FALSE, col.names=FALSE)

q(runLast=FALSE)

EOF



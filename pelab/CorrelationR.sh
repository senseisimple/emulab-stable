#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2007 University of Utah and the Flux Group.
# All rights reserved.
#

#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#


InputFile1="\"$1\""
InputFile2="\"$2\""
OutputFile="\"$3\""

/proj/tbres/R/bin/R --slave  << EOF

xv <- read.table($InputFile1, header=0)
yv <- read.table($InputFile2, header=0)

xval <- xv[,2]
yval <- yv[,2]

corrObj <- ccf(xval, yval, lag.max=1000, type="correlation", plot=FALSE)
dataFrame <- data.frame(corrObj[[4]], corrObj[[1]])
write.table(dataFrame,file=$OutputFile,quote=FALSE,row.names=FALSE, col.names=FALSE)

q(runLast=FALSE)

EOF



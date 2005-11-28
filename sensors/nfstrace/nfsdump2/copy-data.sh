#!/bin/csh -f
#
# $Id: copy-data.sh,v 1.1 2005-11-28 15:44:00 stack Exp $
#
# Automates the archival process.

set dataDir	=	/u1/ellard/data
set archiveDir	=	/home/lair/ellard/Work/SOS/EECS-Traces/lair62

cd $dataDir

foreach f ( *.gz )
	if (! -f "$archiveDir/$f" ) then
		cp "$f" "$archiveDir/$f"
		cmp "$f" "$archiveDir/$f"
	endif
end

exit 0

#!/bin/csh -f

if ( -e time.log)  then
    set xxx=0
    while ( -e time.log$xxx) 
	set xxx=`expr $xxx + 1`
    end
    echo "Moving Old time.log to time.log$xxx"
    mv time.log time.log$xxx
    unset xxx
endif

#time to wait between tests in milliseconds
if ( $1 > 0 ) then 
set delay=$1
else if ($1 < 0) then
set delay=0
else
set delay=5000
endif
echo "Wait between tests is set by the command line parameter,"
echo "an integer number of milliseconds, or -1 for no wait."
echo "Using wait time of $delay milliseconds."
echo "CAUTION: Too little wait will cause failures. Recommended Range"
echo "         is from 1 second to 5 seconds. Wait time does NOT"
echo "         affect the time each command takes to execute."
echo ""
echo ""
echo "Longest Delay: Change last port"
echo ""
echo ""
pause.tcl $delay
/usr/bin/time -a -o time.log setport.exp Alpha:TestBox 24 -b
pause.tcl $delay
/usr/bin/time -a -o time.log setport.exp Alpha:TestBox 24 -f -a
pause.tcl $delay
/usr/bin/time -a -o time.log setport.exp Alpha:TestBox 24 -d Full -s 100
pause.tcl $delay
/usr/bin/time -a -o time.log setport.exp Alpha:TestBox 24 -d Half -s 10
pause.tcl $delay
/usr/bin/time -a -o time.log setport.exp Alpha:TestBox 24 -a
echo ""
echo ""
echo "Total: 5 tests" 
echo ""
echo ""
echo ""
echo "Shortest Delay: Change first port"
echo ""
echo ""
pause.tcl $delay
/usr/bin/time -a -o time.log setport.exp Beta:TestBox 1 -b
pause.tcl $delay
/usr/bin/time -a -o time.log setport.exp Beta:TestBox 1 -f -a
pause.tcl $delay
/usr/bin/time -a -o time.log setport.exp Beta:TestBox 1 -d Full -s 100
pause.tcl $delay
/usr/bin/time -a -o time.log setport.exp Beta:TestBox 1 -d Half -s 10
pause.tcl $delay
/usr/bin/time -a -o time.log setport.exp Beta:TestBox 1 -a
echo ""
echo ""
echo "Total: 5 tests" 
echo ""
echo "Results in time.log"
echo ""
echo "Starting logger..."
echo ""
exec logger time.log


#!/bin/csh -f

set xxx=0
while ( -e time$xxx.log) 
    set xxx=`expr $xxx + 1`
end
echo "Using file time$xxx.log"
unset xxx

echo "Pinging..."
ping -c 10 -q 155.99.214.170 >! pinged
grep -i received pinged >! ping2
cat ping2
echo "Continue?"
set i= 5
while ($i > 0)
    echo -n "$i  "
    pause.tcl 1000
    set i = `expr $i - 1`
end
rm ping2
rm pinged
echo "0"
echo "Starting..."
echo ""
echo ""
#time to wait between tests in milliseconds
if ( $1 > 0 ) then 
set delay=$1
else if ($1 < 0) then
set delay=0
else
set delay=2500
endif
echo "Wait between tests is set by the command line parameter,"
echo "an integer number of milliseconds, or -1 for no wait."
echo "Using wait time of $delay milliseconds."
echo "CAUTION: Too little wait will cause failures. Recommended Range"
echo "         is from 1 second to 5 seconds. Wait time does NOT"
echo "         affect the time each command takes to execute."
echo ""
echo ""
echo "This will test ports 1-24, 5 tests, 10 times per port."
echo "That will be 1200 tests. They take on the average 10 seconds"
echo "including the wait time. This will take about 3 to 3.5 hours."
echo ""
echo ""
foreach port ( 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 )
    echo ""
    echo ""
    echo "Testing Port $port..." 
    echo ""
    echo ""
    foreach i ( 1 2 3 4 5 6 7 8 9 10 )
echo ""
echo "Round $i of 10" 
echo ""
pause.tcl $delay
/usr/bin/time -a -o time.log setport.exp Alpha:TestBox $port -b
pause.tcl $delay
/usr/bin/time -a -o time.log setport.exp Alpha:TestBox $port -f -a
pause.tcl $delay
/usr/bin/time -a -o time.log setport.exp Alpha:TestBox $port -d Full -s 100
pause.tcl $delay
/usr/bin/time -a -o time.log setport.exp Alpha:TestBox $port -d Half -s 10
pause.tcl $delay
/usr/bin/time -a -o time.log setport.exp Alpha:TestBox $port -a
    end
end
echo ""
echo ""
echo "24 ports x 5 tests each x 10 reps = 1200 times logged"
echo "Results in time.log"
echo ""
echo "Starting logger..."
echo ""
exec logger time.log


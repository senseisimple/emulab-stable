#!/bin/csh -f

#set i=0
#while ( $i < 150 )
#    pause.tcl 60000
#Wait 1 minute
#    set i = `expr $i + 1`
#end
#unset i
#wait 2 1/2 hrs before continuing

if ($1>0) then
    set tests = $1
else
    set tests = 5
endif

set n=0
while ( $n < $tests )
    delaysp.csh
    set n = `expr $n + 1` 
end

unset n

Class event

#print
# output format is: time operation args

event instproc print {file} {

puts -nonewline $file "[$self set time] [$self set op] "

# we interpret args based on the operation.
# not too many supported at the moment...
switch [$self set op] {
	link_up	-
	link_down {puts $file [$self set link]}
}
}


#setlink

event instproc setlink {l} { 
$self set link $l
}


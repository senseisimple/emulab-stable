source labels.tcl
source avery_.5x1.75.lbl

set wirefont "Courier 14 bold"
set pcfont "Courier 24 bold"

# 40 pcs
set numpcs 0
for {set i 1} {$i <= $numpcs} {incr i} {
    LABEL::label "pc$i" -font $pcfont
}

# 160 sharks
set numshs 0
for {set i 1} {$i <= $numshs} {incr i} {
    LABEL::label "sh$i" -font $pcfont
}

# 4 switches
#foreach a {alpha beta gamma delta} {
#    LABEL::label "$a" -font $pcfont
#}

# Yellow      N   PC Node to Switch
# Orange      D   Dnard to Switch (old)
# Orange      X   Crossover Switch to Switch (new)
# White       S   Serial
# Green       P   Power
# Red         C   Control
# Grey        R   Random patch-panel to switch, etc.
set cables {
{N 10  0}
{S 10  0}
{P 10  0}
{C 10  0}
{R 10  0}
}

set cableindex 10001
foreach cable $cables {
    set type [lindex $cable 0]
    set size [lindex $cable 1]
    set num [lindex $cable 2]
    for {set i 1} {$i <= $num} {incr i} {
	LABEL::label [format "%06s-%03s-%s" $cableindex $size $type] \
	    -font $wirefont
	LABEL::label [format "%06s-%03s-%s" $cableindex $size $type] \
	    -font $wirefont
	puts "$cableindex $size $type"
	incr cableindex
    }
}

LABEL::print "tb"
exit
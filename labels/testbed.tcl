source labels.tcl
source avery_.5x1.75.lbl

set wirefont "Courier 14 bold"
set pcfont "Courier 24 bold"

# 40 pcs
for {set i 1} {$i <= 40} {incr i} {
    LABEL::label "pc$i" -font $pcfont
}

# 160 sharks
for {set i 1} {$i <= 160} {incr i} {
    LABEL::label "sh$i" -font $pcfont
}

# 4 switches
foreach a {alpha beta gamma delta} {
    LABEL::label "$a" -font $pcfont
}

# Yellow      N   PC Node to Switch
# Orange      D   Dnard to Switch
# White       S   Serial
# Green       P   Power
# Red         C   Control
set cables {
{D 16 3}
{D 18 5}
{D 20 7}
{D 22 7}
{D 24 5}
{D 26 3}
{D 30 10}
{N 16 17}
{N 18 35}
{N 20 50}
{N 22 50}
{N 24 35}
{N 26 17}
{P 18 4}
{P 20 4}
{P 22 4}
{P 24 4}
{C 16 5}
{C 18 10}
{C 20 13}
{C 22 13}
{C 24 10}
{C 26 5}
{S 10 5}
{S 14 5}
{S 16 12}
{S 18 23}
{S 20 32}
{S 22 32}
{S 24 23}
{S 26 12}
}

set cableindex 1
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
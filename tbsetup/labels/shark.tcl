source labels.tcl
source avery_.5x1.75.lbl

set wirefont "Courier 14 bold"

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
	set lables($cableindex) [format "%06s-%03s-%s" $cableindex $size $type]
	incr cableindex
    }
}

# Override some labels
set lables(39) "000039-028-D"
set lables(40) "000040-028-D"
for {set i 34} {$i <= 38} {incr i} {
    set lables($i) [format "%06s-026-D" $i]
}
for {set i 31} {$i <= 33} {incr i} {
    set lables($i) [format "%06s-024-D" $i]
}

set newlabels {3 7 8 14 15 21 22 26 27 31 33 28 34 36 37 38 39 40}

foreach label $newlabels {
    LABEL::label $lables($label) -font $wirefont
    LABEL::label $lables($label) -font $wirefont
}

LABEL::print "dnard"

exit
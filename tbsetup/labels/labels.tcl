namespace eval LABEL {

# configure [-<option> <value>]+
#   <option> = 
#    startx
#    starty
#    deltax
#    deltay
#    columns
#    rows
#    font
#    color
#    currentlabel
# label <text> [<options>]
# print <fileprefix>
# clear
# widget
# 
# Note: Currently limited to one page of labels at a time.
# Note: All coordinates are in inches

variable options 
set options(startx) .5
set options(starty) .5
set options(deltax) 3
set options(deltay) .5
set options(columns) 3
set options(rows) 20
set options(font) "Courier 12"
set options(color) "black"
set options(currentlabel) 0
variable labels {}

proc lpop {listv} {
    upvar $listv V
    set ret [lindex $V 0]
    set V [lrange $V 1 end]
    return $ret
}

proc configure {args} {
    variable options
    
    while {$args != {}} {
	set op [lpop args]
	if {[string index $op 0] != "-"} {
	    error "Malformed option: $op"
	}
	set op [string range $op 1 end]
	if {! [info exists options($op)]} {
	    error "No such option: -$op"
	}
	if {$args == {}} {
	    error "No value given for option: -$op"
	}
	set options($op) [lpop args]
    }
}

proc label {text args} {
    variable labels 
    lappend labels [list $text $args]
}

proc print {file} {
    variable options
    variable labels

    set curpage 1
    set numlabels [llength $labels]
    set maxi [expr $numlabels + $options(currentlabel)]
    set labelsperpage [expr $options(columns) * $options(rows)]
    for {set i $options(currentlabel)} {$i < $maxi} {incr i} {
	set label [lindex $labels $i]
	set pagei [expr $i % $labelsperpage]
	set col [expr $pagei / $options(rows)]
	set row [expr $pagei % $options(rows)]
	set x [expr $options(startx) + $options(deltax) * $col]
	set y [expr $options(starty) + $options(deltay) * $row]
	set text [lindex $label 0]
	set ops [lindex $label 1]
	eval ".label_canvas create text ${x}i ${y}i -text \"$text\" -anchor center -fill $options(color) -font \"$options(font)\" $ops"
	
	if {[expr $pagei + 1] == $labelsperpage} {
	    .label_canvas postscript -file ${file}${curpage}.ps \
		-height 11i -width 8.5i \
		-x 0 -y 0
	    incr curpage
	    clear
	}
    }
    .label_canvas postscript -file ${file}${curpage}.ps \
	-height 11i -width 8.5i \
	-x 0 -y 0
}

proc clear {} {
    eval ".label_canvas delete [.label_canvas find all]"
}

proc widget {} {return .label_canvas}

canvas .label_canvas -bg white -height 11i -width 8.5i

}
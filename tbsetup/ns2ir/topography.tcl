# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005 University of Utah and the Flux Group.
# All rights reserved.
#

Class Topography -superclass NSObject

namespace eval GLOBALS {
    set new_classes(Topography) {}
}

Topography instproc init {} {
    global ::GLOBALS::last_class

    $self set area_name {}
    $self set width {}
    $self set height {}

    set ::GLOBALS::last_class $self
}

Topography instproc rename {old new} {
}

## Topography instproc load_flatgrid {width height} {
##    if {$width < 0} {
##	perror "\[load_flatgrid] negative width - $width"
##	return
##    }
##    if {$height < 0} {
##	perror "\[load_flatgrid] negative height - $height"
##	return
##    }
##
##    $self set width $width
##    $self set height $height
## }

Topography instproc load_area {area} {
    if {! [info exists ::TBCOMPAT::areas($area)]} {
	perror "\[load_area] Unknown area $area."
	return
    }

    $self set area_name $area
    # XXX Load the width/height of the floor in here.
    $self set width 10.0
    $self set height 10.0
}

Topography instproc initialized {} {
    $self instvar width
    $self instvar height

    if {$width != {} && $height != {}} {
	return 1
    } else {
	return 0
    }
}

Topography instproc checkdest {obj x y} {
    var_import ::TBCOMPAT::obstacles
    var_import ::TBCOMPAT::cameras
    $self instvar area_name
    $self instvar width
    $self instvar height

    if {$x < 0 || $x >= $width} {
	perror "$x is out of bounds for node \"$obj\""
	return 0
    }
    if {$y < 0 || $y >= $height} {
	perror "$y is out of bounds for node \"$obj\""
	return 0
    }
    
    if {$area_name != {}} {
	set oblist [array get obstacles *,$area_name,description]
	foreach {key value} $oblist {
	    set id [lindex [split $key ,] 0]
	    
	    if {($x >= [expr $obstacles($id,$area_name,x1) - 0.25]) &&
	        ($x <= [expr $obstacles($id,$area_name,x2) + 0.25]) &&
	        ($y >= [expr $obstacles($id,$area_name,y1) - 0.25]) &&
	        ($y <= [expr $obstacles($id,$area_name,y2) + 0.25])} {
		    perror "Destination $x,$y puts $obj in obstacle $value."
		    return 0
	    }
	}
	
	set camlist [array get cameras *,$area_name,x]
	set in_cam ""
	foreach {key value} $camlist {
	    set id [lindex [split $key ,] 0]

	    if {($x >= $cameras($id,$area_name,x)) &&
	        ($x < [expr $cameras($id,$area_name,x) + \
		       $cameras($id,$area_name,width)]) &&
	        ($y >= $cameras($id,$area_name,y)) &&
	        ($y < [expr $cameras($id,$area_name,y) + \
		       $cameras($id,$area_name,height)])} {
		set in_cam $id
	    }
	}

	if {$in_cam == ""} {
	    perror "Destination $x,$y is out of view of the tracking cameras";
	    return 0
	}
    }
    return 1
}

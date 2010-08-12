# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2010 University of Utah and the Flux Group.
# All rights reserved.
#
######################################################################
# path.tcl
#
# A path is comprised of a set of links.
######################################################################

Class Path -superclass NSObject

namespace eval GLOBALS {
    set new_classes(Path) {}
}

Path instproc init {s links} {
    # This is a list of the links
    $self set mylinklist $links

    # The simulator
    $self set sim $s
}

Path instproc rename {old new} {
    $self instvar sim
    $self instvar mylinklist

    $sim rename_path $old $new
}

Path instproc updatedb {DB} {
    $self instvar mylinklist
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid
    $self instvar sim
    
    set idx 0
    set layer 0

    foreach link $mylinklist {
	set layer  [$link set layer]
	set fields [list "pathname" "segmentname" "segmentindex" "layer"]
	set values [list $self $link $idx $layer]

	$sim spitxml_data "virt_paths" $fields $values	

	incr idx
    }
}

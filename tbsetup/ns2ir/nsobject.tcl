# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

######################################################################
# nsobject.tcl
#
# This defines the class NSObject which most other classes derive from.
# Base defines the unknown procedure which deals with unknown 
# method calls.
######################################################################
Class NSObject

# unknown 
# This is invoked whenever any method is called on the object that is
# not defined.  We display a warning message and otherwise ignore it.
NSObject instproc unknown {m args} {
    punsup "[$self info class] $m"
}

# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#

######################################################################
# nse.null.tcl
#
# This defines the NullClass.  The NullClass is used for all classes
# created that we don't know about.  The NullClass will accept any
# method invocation with any arguments.  It will display an
# unsupported message in all such cases.
######################################################################

Class NullClass

NullClass instproc init {mtype} {
    $self set type $mtype
    $self set objname $self
}

NullClass instproc unknown {m args} {
    $self instvar type
    punsup "$type $m"
}

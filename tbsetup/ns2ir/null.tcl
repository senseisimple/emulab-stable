######################################################################
# null.tcl
#
# This defines the NullClass.  The NullClass is used for all classes
# created that we don't know about.  The NullClass will accept any
# method invocation with any arguments.  It will display an
# unsupported message in all such cases.
######################################################################

Class NullClass

NullClass instproc init {mytype} {
    $self set type $mytype
}

NullClass instproc unknown {m args} {
    var_import ::GLOBALS::verbose
    $self instvar type
    if {$verbose} {
	puts stderr "Unsupported: $type $m"
    }
}
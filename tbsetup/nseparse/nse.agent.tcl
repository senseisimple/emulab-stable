# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#

######################################################################
# nse.agent.tcl
#
#
######################################################################

Class Agent

# Agent
Agent instproc init {args} {
    var_import GLOBALS::last_class
    var_import GLOBALS::last_cmd

    $self set nseconfig {}
    $self set classname "Agent"
    $self set objname $self
    $self set node {}
    $self set application {}
    $self set destination {}
    $self set ip {}
    $self set port {}

    $self instvar classname

    if { $last_cmd == {} } {
	if { $args == {} } {
	    $self set createcmd "\[new $classname]"
	} else {
	    $self set createcmd "\[new $classname $args]"
	}
    } else {
	$self set createcmd $last_cmd
    }

    real_set last_class $self
}
Agent instproc set_node {node} {
    $self set node $node
}
Agent instproc get_node {} {
    $self instvar node
    return $node
}
Agent instproc set_application {application} { 
    $self set application $application
}

Agent instproc connect {dst} {
    $self instvar destination
    $self instvar nseconfig
    $self instvar objname
    
    real_set destination $dst
    real_set sim [lindex [Simulator info instances] 0]
    append nseconfig "\$[$sim set objname] ip-connect \$$objname [$dst set ip] [$dst set port]\n"
}

Agent instproc unknown {m args} {

    $self instvar nseconfig
    $self instvar objname

    append nseconfig "\$$objname $m $args\n"
}

Class Application

# Application
Application instproc init {args} {
    var_import GLOBALS::last_class
    var_import GLOBALS::last_cmd

    $self set nseconfig {}
    $self set classname "Application"
    $self set objname $self

    $self instvar classname

    if { $last_cmd == {} } {
	if { $args == {} } {
	    $self set createcmd "\[new $classname]"
	} else {
	    $self set createcmd "\[new $classname $args]"
	}
    } else {
	$self set createcmd $last_cmd
    }

    real_set last_class $self
}

Application instproc attach-agent {agent} {
    $self set agent $agent
    $agent set_application $self
    $self instvar objname
    $self instvar createcmd
    $self instvar nseconfig

    append nseconfig "\$$objname attach-agent \$[$agent set objname]\n"
}

Application instproc get_node {} {
    $self instvar agent
    return [$agent get_node]
}

Application instproc unknown {m args} {

    $self instvar nseconfig
    $self instvar objname

    append nseconfig "\$$objname $m $args\n"
}

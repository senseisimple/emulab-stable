#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

#
# kill processes associated with wanetmon on ops
#

use strict;

sub kill_pid($){
    my ($pid) = @_;
    `kill $pid`;
    my $successresult = `ps -a -o pid | grep $pid`;

    if( $successresult ne "" ){
        print "failresult = $successresult\n";
        return 0;
    }else{
        return 1;
    }
}


my ($pid, $command);

###########################################
($pid, $command) = split(" ",
    `ps -a -o pid,command | grep "perl automanagerclient.pl" | grep -v "grep"`,
                            2);
chomp $command;

print "stopping: ($pid,$command)\n";
if( kill_pid($pid) ){
    print "success!\n";
}else{
    print "FAILURE of killing pid=$pid\n";
}

###########################################


($pid, $command) = split(" ",
    `ps -a -o pid,command | grep "perl manager.pl" | grep -v "grep"`,
                            2);
chomp $command;

print "stopping: ($pid,$command)\n";
if( kill_pid($pid) ){
    print "success!\n";
}else{
    print "FAILURE of killing pid=$pid\n";
}
###########################################


($pid, $command) = split(" ",
    `ps -a -o pid,command | grep "perl opsrecv.pl" | grep -v "grep"`,
                            2);
chomp $command;

print "stopping: ($pid,$command)\n";
if( kill_pid($pid) ){
    print "success!\n";
}else{
    print "FAILURE of killing pid=$pid\n";
}
###########################################


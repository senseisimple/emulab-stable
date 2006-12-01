#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

package Cmdqueue;
use strict;

sub new{
    my $class = shift;
    my $self = {
	CMDS => []
    };
    bless( $self, $class );
    return $self;
}

=pod
overview:
 adds a command to the queue, where/when appropriate
properties:
 - If a cmd is "forever", it replaces any previous commands by the given
   managerid.
 - If a cmd is "temp", it is added to the queue. Additional operations
   will take place as follows:
    + If there exists a duplicate command in the queue, the last one received
      will be used (dup cmds only differ by received timestamp).
    + If the new command has a period of 0, all cmds in the queue with a
      matching managerid will be deleted.
 - Resulting queue is sorted by ascending test periods.
=cut
sub add{
    my $self = shift;
    my $cmd = shift;
    bless $cmd, "Cmd";

    $self->cleanQueue();
    
    # special case if new command is "forever"
    if( $cmd->duration() == 0 ){
	#search for a matching managerID and "forever" status
	my $replaced = 0;
	for( my $i=0; $i<scalar(@{$self->{CMDS}}); $i++ ){
	    if( $self->{CMDS}->[$i]->duration() == 0 &&
		($self->{CMDS}->[$i]->managerid() eq $cmd->managerid()) )
	    {
		#replace existing forever
		$self->{CMDS}->[$i] = $cmd;
#		print "REPLACED\n";
		$replaced = 1;
	    }
	}
	if(!$replaced){
	    #add
	    push @{$self->{CMDS}}, $cmd;
	}
    }else{
	# not forever... just add to queue

	#delete duplicates
	for( my $i=0; $i<scalar(@{$self->{CMDS}}); $i++ ){
	    if( $self->{CMDS}->[$i]->eqCmd($cmd) ){
		splice( @{$self->{CMDS}}, $i, 1 );
		$i--; #since we just removed this position and need
		#to check the element moved into it's place
	    }
	}
	push @{$self->{CMDS}}, $cmd;
    }

    # if new cmd has per=0, REMOVE ALL TESTS with its manid
    if( $cmd->period() == 0 ){
	$self->rmCmds($cmd->managerid());
    }

    #sort queue
    $self->sortQueue();
}

sub getQueueInfo{
    my $self = shift;
    my $info = "-----------\n";
    foreach my $cmd (@{$self->{CMDS}}){
	$info .= $cmd->getCmdInfo()."---------\n";
    }
    return $info;
}


#
# remove expired commands
#
sub cleanQueue{
    my $self = shift;
    for( my $i=0; $i<scalar(@{$self->{CMDS}}); $i++ ){
	if( defined $self->{CMDS}->[$i]->timeleft() &&
	    $self->{CMDS}->[$i]->timeleft() < 0 )
	{
#	    print "q len = ".scalar(@{$self->{CMDS}})."\n";
	    #cmd expired, so remove from queue
	    splice( @{$self->{CMDS}}, $i, 1 );
	    $i--; #since we just removed this position and need
	          #to check the element moved into it's place
	}
    }
    $self->sortQueue();
}

sub head{
    my $self = shift;
    return $self->{CMDS}->[0];
}

#
# remove all commands with a given managerID
#   If no managerID, remove ALL commands
# returns: number of deleted elements
sub rmCmds{
    my $self = shift;
    my $managerID = shift;
#    print "CMD: manid=$managerID\n";
    my $numDel = 0;
    if( !defined $managerID ){
	# remove all commands
	$numDel = scalar(@{$self->{CMDS}});
	$self->{CMDS} = [];
    }else{
	for( my $i=0; $i < scalar(@{$self->{CMDS}}); $i++ ){
	    if( $self->{CMDS}->[$i]->managerid() eq $managerID ){
		#delete cmd if it has a matching managerid
		splice( @{$self->{CMDS}}, $i, 1 );
		$i--; #since we just removed this position and need
   		      #to check the element moved into it's place
		$numDel++;
	    }
	}
    }
    
    #resort
    $self->sortQueue();
    return $numDel;
}

sub sortQueue{
    my $self = shift;
    #TODO: do something faster....
    my $l = scalar(@{$self->{CMDS}});
    for( my $i=0; $i<$l-1; $i++ ){
	for( my $j=$i+1; $j<$l; $j++ ){
	    if( $self->{CMDS}->[$j]->period()
		< 
		$self->{CMDS}->[$i]->period() )
	    {
		my $tmpcmd = $self->{CMDS}->[$j];
		$self->{CMDS}->[$j] = $self->{CMDS}->[$i];
		$self->{CMDS}->[$i] = $tmpcmd;
	    }
	}
    }
}



1;  #DON'T REMOVE!






#########################################################################
package Cmd;
use strict;


sub new{
    my $class = shift;
    my ($manid, $per, $dur) = @_;
    my $self = {
	MANAGERID => $manid,
	PERIOD    => scalar($per),
	DURATION  => scalar($dur),
	TIME_RECVD => time()
    };
    bless( $self, $class );
    return $self;
}

sub getCmdInfo{
    my $self = shift;
    my $info = "";
    foreach my $key (keys %{$self}){
	$info .= "$key = ".$self->{$key}."\n";
    }
    return $info;
}

sub managerid{
    my $self = shift;
    return $self->{MANAGERID};
}
sub period{
    my $self = shift;
    return $self->{PERIOD};
}
sub duration{
    my $self = shift;
    return $self->{DURATION};
}

# if given command has no set duration, return undef
sub timeleft{
    my $self = shift;
    if( !defined $self->{DURATION} || $self->{DURATION} == 0 ){
	return undef;
    }else{
	return ($self->{TIME_RECVD} + $self->{DURATION}) - time();
    }
}

sub eqCmd{
    my $self = shift;
    my $cmd = shift;
    if( $self->{PERIOD} == $cmd->period() &&
	$self->{MANAGERID} eq $cmd->managerid() &&
	$self->{DURATION} == $cmd->duration() )
    {
	return 1;
    }else{
	return 0;
    }
}
1;

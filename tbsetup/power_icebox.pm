#! /usr/bin/perl -w

#
# EMULAB-COPYRIGHT
# Copyright (c) 2011 University of Utah and the Flux Group.
# All rights reserved.
#

#
# module for controlling power and reset via Linux NetworX's ICEBox
#
# supports new(ip), power(on|off|cyc[le]), status
#

package power_icebox;

use POSIX;
use IO::Pty;

use strict;
$ENV{'PATH'} = '/bin:/usr/bin:/usr/local/bin'; # Required when using system() or backticks `` in combination with the perl -T taint checks

sub spawn_subprocess {
    my(@cmd) = @_;
    my($pid, $pty, $tty, $tty_fd);

    $pty = new IO::Pty() or die $!;

    $pid = fork();
    if (not defined $pid) {
        die "fork() failed: $!\n";
    } elsif ($pid == 0) {
        # detach from controlling terminal
        POSIX::setsid or die "setsid failed: $!";

        $tty = $pty->slave;
        $pty->make_slave_controlling_terminal();
        $tty_fd = $tty->fileno;
	close $pty;
  
	open STDIN, "<&$tty_fd" or die $!;
	open STDOUT, ">&$tty_fd" or die $!;
	open STDERR, ">&STDOUT" or die $!;
	close $tty;

        exec @cmd or die "exec($cmd[0]) failed: $!\n";
    }

    return $pty;
}

sub _icebox_exec ($$) {
    my ($self, $host, $cmd) = @_;
    my $user = "admin";
    my $password = "icebox";
    my $prompt = '/# /';

    # OK, start ssh child process
    my $pty = spawn_subprocess("ssh",
                  "-o", "RSAAuthentication=no",
                  "-o", "PubkeyAuthentication=no",
                  "-o", "PasswordAuthentication=yes",
                  "-l", $user, $host);

    my $ssh = new Net::Telnet (-fhopen => $pty,
                               -prompt => $prompt,
                               -telnetmode => 0,
                               -cmd_remove_mode => 1,
                               -output_record_separator => "\r");

    # Log in to the icebox
    $ssh->waitfor(-match => '/password: ?$/i',
                  -errmode => "return")
        or die "failed to connect to host: ", $ssh->lastline;
    $ssh->print($password);
    $ssh->waitfor(-match => $ssh->prompt,
                  -errmode => "return")
        or die "login failed: ", $ssh->lastline;

    # Send the command to the icebox and get any output
    my @output = $ssh->cmd($cmd);
    return \@output;
}

sub new($$;$) {

    require Net::Telnet; # Saves us from parsing the ssh output by hand

    # The next two lines are some voodoo taken from perltoot(1)
    my $proto = shift;
    my $class = ref($proto) || $proto;

    my $devicename = shift;
    my $debug = shift;

    if (!defined($debug)) {
        $debug = 0;
    }

    if ($debug) {
        print "power_icebox module initializing... debug level $debug\n";
    }

    my $self = {};

    $self->{DEBUG} = $debug;

    $self->{DEVICENAME} = $devicename;

    bless($self,$class);
    return $self;
}

sub power {
    my ($self, $op, @ports) = @_;

    my $errors = 0;
    my $output;

    if    ($op eq "on")  { $op = "power on";    }
    elsif ($op eq "off") { $op = "power off";   }
    elsif ($op =~ /cyc/) { $op = "reset"; }
    
    $output = $self->_icebox_exec($self->{DEVICENAME}, $op . ' ' . join(',', @ports));

    if (not defined $output) {
	    print STDERR $self->{DEVICENAME}, ": could not execute power command \"$op\" for ports @ports\n";
	    $errors++;
    } elsif ($$output[-1] !~ /^\s*OK$/) {
	    print STDERR $self->{DEVICENAME}, ": power command \"$op\" failed with error @$output\n";
	    $errors++;
    }

    return $errors;
}

sub status {
    my $self = shift;
    my $statusp = shift; # pointer to an associative (hashed) array (i.o.w. passed by reference)
    my %status;          # local associative array which we'll pass back through $statusp

    my $errors = 0;
    my $output;

    # Get power status (i.e. whether system is on/off)
    $output = $self->_icebox_exec($self->{DEVICENAME}, "power status all");
    if (not defined $output) {
        $errors++;
        print STDERR $self->{DEVICENAME}, ": could not get power status from device\n";
    }
    else {
        for my $line (@$output) {
            next unless $line =~ /^port\s+(\d+):\s+(.*)$/;
	    $status{"outlet$1"} = ucfirst $2;
	    print("Power status for outlet $1 is: $2\n") if $self->{DEBUG};
	}
    }

    if ($statusp) { %$statusp = %status; } # update passed-by-reference array
    return $errors;
}

1;

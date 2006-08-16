#!/usr/bin/perl -w

#
# EMULAB-COPYRIGHT
# Copyright (c) 2005 University of Utah and the Flux Group.
# All rights reserved.
#

#
# libtblog-simple: Logging library for testbed
#
# This version ...
#

package libtblog_simple;

sub import {
    'libtblog'->export_to_level (1, @_);
}

package libtblog;
use Exporter;

@ISA = "Exporter";
@EXPORT = qw (tblog tberror tberr tbwarn tbwarning tbnotice tbinfo tbdebug 
	      tbdie
	      tblog_session
	      TBLOG_EMERG TBLOG_ALERT TBLOG_CRIT TBLOG_ERR 
	      TBLOG_WARNING TBLOG_NOTICE TBLOG_INFO TBLOG_DEBUG
	      SEV_DEBUG SEV_NOTICE SEV_WARNING SEV_SECONDARY
	      SEV_ERROR SEV_ADDITIONAL SEV_IMMEDIATE);

# After package decl.
# DO NOT USE "use English" in this module
use File::Basename;
use IO::Handle;
use Text::Wrap;
use Carp;

use strict;

use vars qw($SCRIPTNAME 
	    $EMERG $ALRET $CRIT $ERR $WARNING $NOTICE $INFO $DEBUG
	    %PRIORITY_MAP_TO_STR %PRIORITY_MAP_TO_NUM);

#
# Duplicate STDOUT and STDERR to SOUT and SERR respectfully, since
# tblog_capture() will redirect the real STDOUT and STDERR
#

open SOUT ,">&=STDOUT"; # Must be "&=" not "&" to avoid creating a
                        # new low level file descriper as this
                        # interacts strangly with the fork in swapexp.
autoflush SOUT 1;
open SERR ,">&=STDERR"; # Ditto
autoflush SERR 1;
						       
#
#
#

$SCRIPTNAME = basename($0);

#
# Make constants for the error level, the sub, prefixed with TBLOG_,
# are exported, the non-prefixed variables are used internally
#

sub TBLOG_EMERG   {000} $EMERG   = 000;
sub TBLOG_ALERT   {100} $ALRET   = 100;
sub TBLOG_CRIT    {200} $CRIT    = 200;
sub TBLOG_ERR     {300} $ERR     = 300;
sub TBLOG_WARNING {400} $WARNING = 400;
sub TBLOG_NOTICE  {500} $NOTICE  = 500;
sub TBLOG_INFO    {600} $INFO    = 600;
sub TBLOG_DEBUG   {700} $DEBUG   = 700;

%PRIORITY_MAP_TO_STR = (000 => 'EMERG',
	                100 => 'ALRET',
               	        200 => 'CRIT',
           	        300 => 'ERR',
			400 => 'WARNING',
			500 => 'NOTICE',
			600 => 'INFO',
			700 => 'DEBUG');
while (my ($n,$v) = each %PRIORITY_MAP_TO_STR) {
  $PRIORITY_MAP_TO_NUM{uc $v} = $n;
  $PRIORITY_MAP_TO_NUM{lc $v} = $n;
}

#
# tbreport related constants
#

use constant SEV_DEBUG      =>   0;
use constant SEV_NOTICE     => 100;
use constant SEV_WARNING    => 300;
use constant SEV_SECONDARY  => 400;
use constant SEV_ERROR      => 500; # Threshold
use constant SEV_ADDITIONAL => 600;
use constant SEV_IMMEDIATE  => 900;

#
# Utility functions
#

sub if_defined ($$) {
    return $_[0] if defined $_[0];
    return $_[1] if defined $_[1];
    return '';
}

sub oneof ($@) {
    my ($to_find) = shift;
    my @res = grep {$to_find eq $_} @_;
    return @res > 0;
}

sub scriptname() {
  if_defined($SCRIPTNAME, $ENV{TBLOG_SCRIPTNAME});
}

#
#
#

sub tblog_session() {
    return $ENV{TBLOG_SESSION};
}

#
# Dummy dblog, does nothing in this module
# Once the real "libtblog.pm" is used than this will be replaced
# with the real function which logs to the database
#
sub dblog_dummy( $$@ ) {
  return 1;
}
*dblog = \&dblog_dummy;

#
# tblog(priority, mesg, ...)
# tblog(priority, {parm=>value,...}, mesg, ...)
#
#   The main log function.  Logs a message to the database and print
#   the message to STDERR with an approate prefix depending on the
#   severity of the error.  If more than one string is given for the
#   message than they will concatenated.  If the env. var. TBLOG_OFF
#   is set to a true value than nothing will be written to the
#   database, but the message will still be written to STDOUT.  
#   Useful parms: sublevel, cause, type
#
sub tblog( $@ ) {
    my ($priority) = shift;
    my $parms = {};
    $parms = shift if ref $_[0] eq 'HASH';
    my $mesg = join('',@_);

    if (exists $PRIORITY_MAP_TO_STR{$priority}) {
      # $priority already a valid priority number
    } elsif (exists $PRIORITY_MAP_TO_NUM{$priority}) {
      # $priority a priority string, convert to num
      $priority = $PRIORITY_MAP_TO_NUM{$priority}
    } else {
      croak "Unknown priority \"$priority\" in call to tblog";
    }

    my $res = dblog($priority, $parms, $mesg) unless $mesg =~ /^\s+$/;
    
    print SERR format_message(scriptname(), $priority, $mesg);

    return $res;
}


# Useful alias functions

sub tberror( @ ) {&tblog($ERR, @_)}
sub tberr( @ ) {&tblog($ERR, @_)}
sub tbwarn( @ ) {&tblog($WARNING, @_)}
sub tbwarning( @ ) {&tblog($WARNING, @_)}
sub tbnotice( @ ) {&tblog($NOTICE, @_)}
sub tbinfo( @ ) {&tblog($INFO, @_)}
sub tbdebug( @ ) {&tblog($DEBUG, @_)}

#
# Log the message to the database as an error and than die.  An
# optional set of paramaters may be specified as the first paramater.
# Not exactly like die as the message bust be specified.
#
sub tbdie( @ ) {
    my $parms = {};
    $parms = shift if ref $_[0] eq 'HASH';
    my $mesg = join('',@_);

    if ($^S) {
	# Handle case when we are in an eval block special:
	#   1) downgrade error to warning as it may not be fatal
	#   2) Call tblog (not just dblog) to print the error since 
        #      the message may never actually appear. 
	#   3) Don't stop capturing as we are not trully dying
	#   4) Don't format message as it may be modified latter
	tblog($WARNING, $parms, $mesg);
	$mesg .= "\n" unless $mesg =~ /\n$/;
	die $mesg;
    } else {
	dblog($ERR, $parms, $mesg);
	tblog_stop_capture() if exists $INC{'libtblog.pm'};
	die format_message(scriptname(), $ERR, $mesg);
    }
}

#
# Format the message based on $priority
#
sub format_message ( $$$ ) {

    my ($scriptname, $priority, $mesg) = @_;

    $mesg =~ s/\s+$//;

    my $header;

    if ($mesg =~ /\s*\*\*\*/) {
        # do nothing
    } elsif ($priority <= $ERR ) {
        $header = "ERROR: $scriptname";
    } elsif ($priority == $WARNING) {
        $header = "WARNING: $scriptname";
    } elsif ($priority == $NOTICE) {
        $header = "$scriptname";
    }
    my $text;

    my @mesg = split /\n/, $mesg;
    if (@mesg == 1) {
        $mesg[0] =~ s/^\s+//;
        $mesg = $mesg[0];
    }
    if ($header) {
        my $line = "*** $header: $mesg[0]";
        if (@mesg > 1 || length($line) > $Text::Wrap::columns) {
            $line = "*** $header:\n";
            if (@mesg == 1) { # NOTE: $mesg[0] eq $mesg
                $mesg =~ s/^\s+//;
                $line .= wrap('***   ','***   ', $mesg, "\n");
            } else {
                foreach (@mesg) {
                    s/\s+$//;
                    $line .= "***   $_\n";
                }
            }
            return $line;
        } else {
            return "$line\n";
        }
    } else {
        if (@mesg == 1) {
            return wrap ('', '    ', $mesg, "\n");
        } else {
            return "$mesg\n";
        }
    }
}




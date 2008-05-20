#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2008 University of Utah and the Flux Group.
# All rights reserved.
#
# Perl code to access an XMLRPC server using http. Derived from the
# Emulab library (pretty sure Dave wrote the http code in that file,
# and I'm just stealing it).
#
package GeniResponse;
use strict;
use Exporter;
use vars qw(@ISA @EXPORT);
@ISA    = "Exporter";
@EXPORT = qw (GENIRESPONSE_SUCCESS GENIRESPONSE_BADARGS GENIRESPONSE_ERROR
	      GENIRESPONSE_FORBIDDEN GENIRESPONSE_BADVERSION
	      GENIRESPONSE_SERVERERROR
	      GENIRESPONSE_TOOBIG GENIRESPONSE_REFUSED
	      GENIRESPONSE_TIMEDOUT GENIRESPONSE_DBERROR
	      GENIRESPONSE_RPCERROR GENIRESPONSE_UNAVAILABLE
	      GENIRESPONSE_SEARCHFAILED);

use overload ('""' => 'Stringify');

#
# GENI XMLRPC defs. Also see ../lib/Protogeni.pm.in if you change this.
#
sub GENIRESPONSE_SUCCESS()        { 0; }
sub GENIRESPONSE_BADARGS()        { 1; }
sub GENIRESPONSE_ERROR()          { 2; }
sub GENIRESPONSE_FORBIDDEN()      { 3; }
sub GENIRESPONSE_BADVERSION()     { 4; }
sub GENIRESPONSE_SERVERERROR()    { 5; }
sub GENIRESPONSE_TOOBIG()         { 6; }
sub GENIRESPONSE_REFUSED()        { 7; }
sub GENIRESPONSE_TIMEDOUT()       { 8; }
sub GENIRESPONSE_DBERROR()        { 9; }
sub GENIRESPONSE_RPCERROR()       {10; }
sub GENIRESPONSE_UNAVAILABLE()    {11; }
sub GENIRESPONSE_SEARCHFAILED()   {12; }

#
# This is the (python-style) "structure" we want to return.
#
# class Response:
#    def __init__(self, code, value=0, output=""):
#        self.code     = code            # A RESPONSE code
#        self.value    = value           # A return value; any valid XML type.
#        self.output   = output          # Pithy output to print
#        return
#
sub new($$;$$)
{
    my ($class, $code, $value, $output) = @_;

    $output = ""
	if (!defined($output));
    $value = 0
	if (!defined($value));

    my $self = {"code"   => $code,
		"value"  => $value,
		"output" => $output};
    bless($self, $class);
    return $self;
}

sub Create($$;$$)
{
    my ($class, $code, $value, $output) = @_;

    $output = ""
	if (!defined($output));
    $value = 0
	if (!defined($value));

    my $self = {"code"   => $code,
		"value"  => $value,
		"output" => $output};
    return $self;
}

# accessors
sub field($$)           { return ($_[0]->{$_[1]}); }
sub code($)		{ return field($_[0], "code"); }
sub value($)		{ return field($_[0], "value"); }
sub output($)		{ return field($_[0], "output"); }

#
# Stringify for output.
#
sub Stringify($)
{
    my ($self) = @_;
    
    my $code  = $self->code();
    my $value = $self->value();

    return "[GeniResponse: code:$code, value:$value]";
}

sub MalformedArgsResponse($)
{
    return GeniResponse->Create(GENIRESPONSE_BADARGS,
				undef, "Malformed arguments to method");
}

sub BadArgsResponse(;$)
{
    my ($msg) = @_;

    $msg = "Bad arguments to method"
	if (!defined($msg));
    
    return GeniResponse->Create(GENIRESPONSE_BADARGS, undef, $msg);
}

# _Always_ make sure that this 1 is at the end of the file...
1;

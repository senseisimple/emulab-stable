#!/usr/bin/perl
use Modern::Perl;

package TestBed::XMLRPC::Client;
use Mouse;
use RPC::XML::Client;
use TBConfig;
use Data::Dumper;
use Carp;
use Tools;

my $loglevel = "INFO";
$loglevel = "DEBUG" if $TBConfig::DEBUG_XML_CLIENT;
my $logger = init_tbts_logger("XMLRPCClient", undef, "INFO", "SCREEN");

BEGIN {
  use TBConfig;
  $ENV{HTTPS_CERT_FILE} = glob($TBConfig::SSL_CLIENT_CERT);
  $ENV{HTTPS_KEY_FILE}  = glob($TBConfig::SSL_CLIENT_KEY);
}


has 'client' => ( isa => 'RPC::XML::Client', is => 'rw', default => sub { 
  my $c = RPC::XML::Client->new($TBConfig::XMLRPC_SERVER, 'timeout' => (10*60));
  $c->{'__useragent'}->timeout(60*10);
  $c; } );

our $AUTOLOAD;
sub AUTOLOAD {
  my $self = shift;
  my $type = ref($self) or croak "$self is not an object";
  $self->xmlrpc_req_value(($self->xmlrpcfunc($AUTOLOAD))[0], $self->args(@_));
}

sub args { 
  my $self = shift;
  +{ @_ };
}

sub xmlrpcfunc {
  $_[1] =~ m/.*:([^:]*)::([^:]*)/;
  my ($package, $funcname) = (lc($1), $2);
  $funcname =~ s/\_.*//;
  ("$package.$funcname", $package, $funcname);
}

sub pkgfunclist { 
  my $caller = ((caller(3))[3]);
  (shift->xmlrpcfunc($caller));
}

sub pkgfunc { (shift->pkgfunclist())[0]; }
sub pkg     { (shift->pkgfunclist())[1]; }
sub func    { (shift->pkgfunclist())[2]; }

sub single_request {
  my ($self, $command, @args) = @_;
  $logger->debug(toperl($command, @args));
  $logger->debug("Sent");
  my $resp = $self->client->send_request($command, $TBConfig::XMLRPC_VERSION, @args); 
  $logger->debug("Received");
  $logger->debug( sub { Dumper($resp); } );
  $resp;
}

sub xmlrpc_req_value  { single_request(@_)->value-> {'value'}; }
sub xmlrpc_req_output { single_request(@_)->value-> {'output'}; }
sub xmlrpc_req_code   { single_request(@_)->value-> {'code'}; }
sub augment { 
  my $self = shift;
  $self->xmlrpc_req_value($self->pkgfunc(), $self->args(@_));
}
sub augment_output { 
  my $self = shift;
  $self->xmlrpc_req_output($self->pkgfunc(), $self->args(@_));
}
sub augment_code { 
  my $self = shift;
  $self->xmlrpc_req_code($self->pkgfunc(), $self->args(@_));
}
sub augment_func { 
  my $self = shift;
  $self->xmlrpc_req_value($self->pkg() . "." . shift, $self->args(@_));
}
sub augment_func_output { 
  my $self = shift;
  $self->xmlrpc_req_output($self->pkg() . "." . shift, $self->args(@_));
}
sub augment_func_code { 
  my $self = shift;
  $self->xmlrpc_req_code($self->pkg() . "." . shift, $self->args(@_));
}

1;

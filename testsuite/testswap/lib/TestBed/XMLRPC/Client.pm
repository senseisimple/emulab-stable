#!/usr/bin/perl
use Modern::Perl;

package TestBed::XMLRPC::Client;
use Mouse;
use RPC::XML::Client;
use TBConfig;
use Data::Dumper;
use Carp;
use Tools;

BEGIN {
  use TBConfig;
  $ENV{HTTPS_CERT_FILE} = glob($TBConfig::SSL_CLIENT_CERT);
  $ENV{HTTPS_KEY_FILE}  = glob($TBConfig::SSL_CLIENT_KEY);
}

our $DEBUG = $TBConfig::DEBUG_XML_CLIENT;

has 'client' => ( isa => 'RPC::XML::Client', is => 'rw', default => sub { 
  my $c = RPC::XML::Client->new($TBConfig::XMLRPC_SERVER, 'timeout' => (4*60));
  $c->{'__useragent'}->timeout(60*4);
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

sub augment { 
  my $self = shift;
  $self->xmlrpc_req_value($self->pkgfunc(), $self->args(@_));
}

sub augment_output { 
  my $self = shift;
  $self->xmlrpc_req_output($self->pkgfunc(), $self->args(@_));
}

sub augment_func { 
  my $self = shift;
  $self->xmlrpc_req_value($self->pkg() . "." . shift, $self->args(@_));
}

sub xmlrpcfunc {
  $_[1] =~ m/.*:([^:]*)::([^:]*)/;
  my ($package, $func) = ($1, $2);
  $func =~ s/\_.*//;
  (lc($package) . "." . $func, lc($package), $func);
}

sub pkgfunclist { 
  my $c = ((caller(3))[3]);
  #say $c;
  (shift->xmlrpcfunc($c));
}

sub pkgfunc { (shift->pkgfunclist())[0]; }
sub pkg { (shift->pkgfunclist())[1]; }
sub func { (shift->pkgfunclist())[2]; }

sub single_request {
  my ($self, $command, @args) = @_;
  if ($DEBUG) {
    sayperl($command, @args);
    sayts("Sent");
  }
  my $resp = $self->client->send_request($command, $TBConfig::XMLRPC_VERSION, @args); 
  if ($DEBUG) {
    sayts("Received");
    say Dumper($resp);
  }
  $resp;
}

sub xmlrpc_req_value  { single_request(@_)->value-> {'value'}; }
sub xmlrpc_req_output { single_request(@_)->value-> {'output'}; }
sub xmlrpc_req_code   { single_request(@_)->value-> {'code'}; }

1;

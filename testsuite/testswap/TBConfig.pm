use Modern::Perl;

package TBConfig;
use Sys::Hostname;
=pod
require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw($XMLRPC_SERVER, $CLIENT_CERT, $CLIENT_KEY);
=cut

sub istan {
  hostname() =~ /tan/;
}

our $XMLRPC_SERVER   = "https://boss.emulab.net:3069/usr/testbed";
our $XMLRPC_VERSION  = "0.1";
our $SSL_CLIENT_CERT = "~/.ssl/emulab.cert";
our $SSL_CLIENT_KEY  = "~/.ssl/emulabkeyout.pem";
our $DEBUG_XML_CLIENT = istan();

sub KEVINS_MOCK_SETUP {
  our $XMLRPC_SERVER   = "https://localhost:3069/home/tewk/srcs/tbxmlrpctests";
  our $SSL_CLIENT_CERT = "mock/client/cl-cert.pem";
  our $SSL_CLIENT_KEY  = "mock/client/cl-key.pem";
  our $DBI_CONNECT_STR = "DBI:mysql:tbdb";
}

1;

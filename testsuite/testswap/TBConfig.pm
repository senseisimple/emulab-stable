package TBConfig;

use SemiModern::Perl;
use Crypt::X509;
use Tools qw(slurp);
use Data::Dumper;
use MIME::Base64;

our $XMLRPC_SERVER    = "https://boss.emulab.net:3069/usr/testbed";
our $OPS_SERVER       = "users.emulab.net";
our $XMLRPC_VERSION   = "0.1";
our $SSL_CLIENT_CERT  = glob("~/.ssl/emulab.cert");
our $SSL_CLIENT_KEY   = glob("~/.ssl/emulabkeyout.pem");
our $EMULAB_USER      = get_emulab_user();
our $DEFAULT_PID      = 'tbres';
our $DEFAULT_GID      = '';
our $DEBUG_XML_CLIENT = $ENV{TBTS_DEBUG} || 0;

sub get_emulab_user {
  my $cert = slurp($SSL_CLIENT_CERT);
  $cert =~ s/^-----BEGIN CERTIFICATE-----//;
  $cert =~ s/-----END CERTIFICATE-----//;
  $cert = decode_base64($cert);
  $cert = Crypt::X509->new(cert => $cert);
  my $user = $cert->subject_ou;
  $user =~ /.*\.(.*)/;
  $1
}

1;

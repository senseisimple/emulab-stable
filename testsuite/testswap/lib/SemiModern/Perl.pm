our $VERSION = '1.00';

use 5.008_000;

use strict;
use warnings;
package SemiModern::Perl;
use IO::Handle;
use Scalar::Util 'openhandle';
use Carp;

sub say {
    my $currfh = select();
    my $handle;
    {
        no strict 'refs';
        $handle = openhandle($_[0]) ? shift : \*$currfh;
        use strict 'refs';
    }
    @_ = $_ unless @_;
    my $warning;
    local $SIG{__WARN__} = sub { $warning = join q{}, @_ };
    my $res = print {$handle} @_, "\n";
    return $res if $res;
    $warning =~ s/[ ]at[ ].*//xms;
    croak $warning;
}

if (1 || $] < 5.010) {
  *IO::Handle::say = \&say if ! defined &IO::Handle::say;
}

sub import {
  warnings->import();
  strict->import();
  if (1 || $] < 5.010) {
    no strict 'refs';
    *{caller() . '::say'} = \&say;
    use strict 'refs';
  }
}

1;

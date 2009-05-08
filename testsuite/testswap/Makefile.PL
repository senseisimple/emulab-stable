#!/usr/bin/perl
use inc::Module::Install;

# Define metadata
name            'tbts';
perl_version    '5.008';
author          'Kevin Tew <tewk@flux.utah.edu>';
version         '0.01';

requires      'Mouse'               => '0.22';
requires      'RPC::XML::Client'    => '1.24';
requires      'RPC::XML'            => '1.41';
requires      'Test::More'          => '0.86';
requires      'Time::Local'         => '1.1901';
requires      'TAP::Harness'        => '3.16';
requires      'Crypt::X509'         => '0.32';
requires      'Log::Log4perl'       => '1.20';
requires      'Data::UUID'          => '1.149';
requires      'Net::Ping'           => '2.31';
requires      'Crypt::SSLeay'       => '0.57';
requires      'Test::Class'         => '0.31';
requires      'Test::Exception'     => '';
requires      'Term::ReadKey'       => '';

=pod
#too much effort to install for now
#required for fast SSH and Crypt::DH
requires      'bignum'              => '0.22';
requires      'Math::BigRat'        => '0.22';
requires      'Math::BigInt::GMP'   => '1.24';

requires      'Net::SSH::Perl'      => '1.34';
requires      'Net::SFTP'           => '0.10';
=cut

auto_install();

WriteAll;

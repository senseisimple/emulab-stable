#!/usr/bin/perl
use inc::Module::Install;

# Define metadata
name            'tbts';
all_from        'lib/TestBed/XMLRPC/Client.pm';
perl_version    '5.008';
author          'Kevin Tew <tewk@flux.utah.edu>';
version         '0.01';

# Specific dependencies
requires      'bignum'              => '0.23';
requires      'Math::BigRat'        => '0.22';
requires      'Crypt::SSLeay'       => '';
requires      'Test::Exception'     => '';
requires      'Sub::Uplevel'        => '';
requires      'Mouse'               => '';
requires      'RPC::XML::Client'    => '1.24';
requires      'RPC::XML'            => '1.41';
requires      'Sys::Hostname'       => '1.11';
requires      'Test::More'          => '0.86';
requires      'Time::Local'         => '1.1901';
requires      'TAP::Harness'        => '3.16';
requires      'Math::BigInt::GMP'   => '';
requires      'Math::BigInt::Pari'  => '';
requires      'Net::SSH::Perl'      => '1.34';
requires      'Net::SFTP'           => '0.10';
requires      'Crypt::X509'         => '0.32';
requires      'Log::Log4perl'       => '1.20';
requires      'Log::Dispatch::File' => '1.22';
requires      'Data::UUID'          => '';
requires      'Net::Ping'           => '';

test_requires  'Test::More'  => '0.42';
no_index       'directory'   => 'demos';

auto_install();

WriteAll;

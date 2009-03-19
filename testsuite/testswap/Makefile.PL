#!/usr/bin/perl
use Modern::Perl;
use inc::Module::Install;

# Define metadata
name            'tbts';
all_from        'lib/TestBed/XMLRPC/Client.pm';
perl_version    '5.010';
author          'Kevin Tew <tewk@flux.utah.edu>';
version         '0.01';

# Specific dependencies
requires       'Crypt::SSLeay'       => '';
requires       'File::Spec'          => '0.80';
requires       'Carp'                => '1.08';
requires       'Modern::Perl'        => '1.03';
requires       'Mouse'               => '0.19';
requires       'RPC::XML::Client'    => '1.24';
requires       'RPC::XML'            => '1.41';
requires       'Sys::Hostname'       => '1.11';
requires       'Test::More'          => '0.86';
requires       'Time::Local'         => '1.1901';
requires       'TAP::Harness'        => '3.16';
requires       'Math::BigInt::GMP'   => '';
requires       'Math::BigInt::Pari'  => '';
requires       'Net::SSH::Perl'      => '1.34';
requires       'Net::SFTP'           => '0.10';
requires       'Crypt::X509'         => '0.32';
requires       'Log::Log4perl'       => '1.20';
requires       'Log::Dispatch::File' => '1.22';
requires       'Data::UUID'          => '';

test_requires  'Test::More'  => '0.42';
no_index       'directory'   => 'demos';

auto_install();

WriteAll;

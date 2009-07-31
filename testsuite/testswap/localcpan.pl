#!/usr/bin/perl
use strict;
use warnings;
use CPAN;

sub prep_local_cpan {
  open(FH, "|cpan");
  print FH "no\n";
  print FH "quit\n";
  close(FH);

  mkdir '~/lib/perl5';
  mkdir '~/share/man/man1';
  mkdir '~/share/man/man3';

  CPAN::Config->load;
  $CPAN::Config->{'makepl_arg'} = q[PREFIX=~/ SITELIBEXP=~/lib/perl5 LIB=~/lib/perl5 INSTALLMAN1DIR=~/share/man/man1 INSTALLMAN3DIR=~/share/man/man3 INSTALLSITEMAN1DIR=~/share/man/man1 INSTALLSITEMAN3DIR=~/share/man/man3];
  $CPAN::Config->{'prerequisites_policy'} = q[follow];
  $CPAN::Config->{'urllist'} = [q[ftp://cpan.cs.utah.edu/pub/CPAN/]];
  CPAN::Config->commit;
}

sub automate_module_install {
  CPAN::Shell->install('Module::Install');
  system("perl Makefile.PL --defaultdeps");
  system("make");
}

sub install_deps_from_cpan {
  my @deps = qw(
      Mouse
      RPC::XML::Client
      RPC::XML
      Test::More
      Time::Local
      TAP::Harness
      Crypt::X509
      Log::Log4perl
      Data::UUID
      IPC::Run3
      Crypt::SSLeay
      Test::Exception
      Term::ReadKey
      );
#Test::Class
#Crypt::SSLeay # required for SSL
#Data::UUID requires user input
#Net::Ping #tests fail, default installed version 2.31 is good enough
  CPAN::Shell->install($_) for(@deps);
}

sub automate_ssh_install {
  my @ssh_math_deps = qw(
      bignum
      Math::BigRat
      Math::BigInt::GMP
      );
  
  my @sshdeps = qw(
      Digest::BubbleBabble
      Crypt::RSA
      Convert::PEM
      Data::Buffer
      );

  my @ssh_modules = qw(
      Net::SSH::Perl
      Net::SFTP
      );

  CPAN::Shell->install($_) for(@ssh_math_deps);
  CPAN::Shell->install($_) for(@sshdeps);
  CPAN::Shell->install($_) for(@ssh_modules);
}

sub main {
  if (!(grep {$_ eq '--override' } @ARGV) and -e glob("~/.cpan")) {
    die "NOT installing local CPAN ~/.cpan exists, specify --override to ignore check";
  }

  print "WARNING installing local CPAN to '~/lib/perl5' -- type yes <ENTER> to continue\n";
  my $response = <STDIN>;
  chomp $response;
  if ($response ne "yes") {
    die "$response does not match yes";
  }

  prep_local_cpan;
  $ENV{PERL5LIB} = glob('~/lib/perl5');

  if ($ARGV[0] && $ARGV[0] eq 'MI') {
    automate_module_install;  #too complicated for fluxers on FreeBSD
    automate_ssh_install;     #too complicated for fluxers on FreeBSD
  }
  else {
    install_deps_from_cpan;
  }
}

main;

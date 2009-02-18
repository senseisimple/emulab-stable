# pull in ExtUtils/AutoInstall.pm from 'inc'
use lib 'inc';
use ExtUtils::AutoInstall (
    -core              => [            # mandatory modules
    'Carp'             => '',
    'Modern::Perl'     => '',
    'Mouse'            => '',
    'RPC::XML::Client' => '',
    'RPC::XML'         => '',
    'Sys::Hostname'    => '',
    'Test::More'       => '',
    'Time::Local'      => '',
    'TAP::Harness'     => '',
    ],
);
=pod
WriteMakefile(
    AUTHOR          => 'Emula Perl  <perl@emulab.net>',
    ABSTRACT        => 'Perl Testsuite for Emulab',
    NAME            => 'Testbed::Testsuite',
    VERSION_FROM    => 'Testbed::Testsuite::Experiment.pm',
    DISTNAME        => 'Emulab-Testbed',
);
=cut

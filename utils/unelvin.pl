#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2007 University of Utah and the Flux Group.
# All rights reserved.
#

use English;

#
# Remove all traces of elvin from a client
#
my @BSDPKGS = (
    'elvind-4.0.3',
    'libelvin-4.0.3_2'
);

my @BSDFILES = (
    '/usr/local/etc/rc.d/elvind.sh',
    '/usr/local/bin/elvin-config',
    '/usr/local/etc/elvind*',
    '/usr/local/include/elvin',
    '/usr/local/info/elvin*',
    '/usr/local/lib/libvin4*',
    '/usr/local/lib/nls/msg/elvin*',
    '/usr/local/man/man*/elvin*',
    '/usr/local/libexec/elvind',
    '/var/log/elvind.log'
);

my @LINUXFILES = (
    '/etc/rc.d/*/*elvin',
    '/usr/local/bin/elvin-config',
    '/usr/local/etc/elvind*',
    '/usr/local/include/elvin',
    '/usr/local/info/elvin*',
    '/usr/local/lib/libvin4*',
    '/usr/local/lib/nls/msg/elvin*',
    '/usr/local/man/man*/elvin*',
    '/usr/local/sbin/elvind'
);

if ($UID ne 0) {
    print STDERR "You will want to be doing this as root ya know!\n";
    exit(1);
}

my $isbsd = 0;
if (-e "/usr/sbin/pkg_delete") {
    $isbsd = 1;
}

print "Bye, bye elvin...\n";
if ($isbsd) {
    # disable elvind logging in syslog.conf
    system("sed -i '' -e '/elvind/d' /etc/syslog.conf");

    # remove any packages?
    foreach my $pkg (@BSDPKGS) {
	if (!system("pkg_info -e $pkg")) {
	    print "removing $pkg package...\n";
	    system("pkg_delete -f $pkg");
	}
    }

    # remove known files
    my @list = `ls -d @BSDFILES 2>/dev/null`;
    print "removing: @list\n";
    chomp(@list);
    my $lstr = join(' ', @list);
    system("rm -rf $lstr");
    @list = `ls -d @BSDFILES 2>/dev/null`;
    print "what's left: @list\n";
} else {
    # remove any rpms?

    my @list = `ls -d @LINUXFILES 2>/dev/null`;
    print "removing: @list\n";
    chomp(@list);
    my $lstr = join(' ', @list);
    system("rm -rf $lstr");
    @list = `ls -d @LINUXFILES 2>/dev/null`;
    print "what's left: @list\n";
}

print "Elvin has left the building (you KNEW that was coming!)\n";
exit 0;

#!/usr/bin/perl -w

#
# EMULAB-COPYRIGHT
# Copyright (c) 2011 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Hack to figure out a PHP style timezone string based on /etc/localtime
# on FreeBSD.
#
# Compares /etc/localtime to all choices in /usr/share/zoneinfo/zone.tab
# and all the traditional "aliases" (e.g., MST7MDT) in /usr/share/zoneinfo.
#
# Comparison is done by calling "sum" on all files and comparing the checksum.
# If anything goes wrong, default to the center of the Emulab Universe
# "America/Denver" (aka, MST7MDT).
#
# Note that this script WILL fail if you localtime file is not an up-to-date
# copy of the appropriate file in /usr/share/zoneinfo.
#

# MST: the center of the universe
my $defaulttz = "America/Denver";
my $tzstring = "";

# XXX unfortunately I don't know all these map to
my %aliases = (
    "CET"     => "",
    "CST6CDT" => "America/Chicago",
    "EET"     => "",
    "EST"     => "",
    "EST5EDT" => "America/New_York",
    "GMT"     => "Europe/London",
    "HST"     => "",
    "MET"     => "",
    "MST"     => "America/Phoenix",
    "MST7MDT" => "America/Denver",
    "PST8PDT" => "America/Los_Angeles",
    "WET"     => ""
);

sub sumfile($);

# no localtime file, use default
if (! -e "/etc/localtime") {
    print STDERR "WARNING: you have no timezone set, default to $defaulttz\n";
    print "$defaulttz\n";
    exit(0);
}

# symlink, we are done
if (-l "/etc/localtime") {
    if (readlink("/etc/localtime") =~ /share\/zoneinfo\/(\S+)$/) {
	$tzstring = $1;
    } else {
	print STDERR "WARNING: cannot make sense of /etc/localtime symlink, default to $defaulttz\n";
	$tzstring = $defaulttz;
    }
    print "$tzstring\n";
    exit(0);
}

# sum our TZ file
my $tzsum = sumfile("/etc/localtime");

# cannot open zone.tab, use default
if (!open(FD, "</usr/share/zoneinfo/zone.tab")) {
    print STDERR "WARNING: no zoneinfo table, default to $defaulttz\n";
    print "$defaulttz\n";
    exit(0);
}

# compare against others til we find it
while (<FD>) {
    next if (/^#/);
    my @cols = split(/\t/);
    my $tzf = "/usr/share/zoneinfo/" . $cols[2];
    chomp($tzf);
    next if (! -r "$tzf");

    my $sum = sumfile($tzf);
    if (defined($sum) && $sum eq $tzsum) {
	$tzstring = $cols[2];
	last;
    }
}
close(FD);

# try aliases
if (!$tzstring) {
    foreach my $al (keys %aliases) {
	if (-e "/usr/share/zoneinfo/$al") {
	    my $sum = sumfile("/usr/share/zoneinfo/$al");
	    if (defined($sum) && $sum eq $tzsum) {
		$tzstring = $aliases{$al};
		last;
	    }
	}
    }
}

if (!$tzstring) {
    print STDERR "WARNING: cannot find matching zoneinfo file, default to $defaulttz\n";
    $tzstring = $defaulttz;
}
print "$tzstring\n";
exit(0);

sub sumfile($)
{
    my $file = shift;
    my $sum = `sum $file 2>/dev/null`;
    chomp $sum;
    if ($sum =~ /^(\d+)/) {
	return $1;
    }

    return undef;
}

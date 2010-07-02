#! /usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2008 University of Utah and the Flux Group.
# All rights reserved.
#

use XML::Parser;
use Data::Dumper;
use IO::Zlib;

use strict;

my %packages;
my %provides;
my %files;

my $current_data;
my $current_arch;
my $current_location;
my $current_package;

my $in_provides = 0;
my $in_requires = 0;

sub usage
{
	my $progname = (split(/\//, $0))[-1];

	print STDERR "Usage: $progname media_path package1 [package2 ...]\n";
}

sub is_same_version
{
	my ($a, $b) = @_;
	my $rc = 0;

	if (($$a{'version'} eq $$b{'version'}) &&
	    ($$a{'release'} eq $$b{'release'}) &&
	    ($$a{'epoch'} eq $$b{'epoch'})) {
		$rc = 1;
    	}

	return $rc;
}

sub merge_package_entries
{
	my ($a, $b) = @_;
	for (keys %{$$b{'location'}}) {
		$$a{'location'}{$_} =
		       $$b{'location'}{$_};
	}
}

sub hash_provides
{
	my ($package) = @_;

	# add to hash of 'who provides what'
	for my $symbol (keys %{$$package{'provides'}}) {
		my $ptr = $provides{$symbol};
		if (ref $ptr eq 'ARRAY') {
			push @$ptr, $package;
		}
		elsif (defined $ptr) {
			$provides{$symbol} = [ $ptr, $package ];
		}
		else {
			$provides{$symbol} = $package;
		}
	}
}

sub hash_files
{
	my ($package) = @_;

	# add to hash of 'who provides this file'
	for my $file (@{$$package{'files'}}) {
		my $ptr = $files{$file};
		if (ref $ptr eq 'ARRAY') {
			push @$ptr, $package;
		}
		elsif (defined $ptr) {
			$files{$file} = [ $ptr, $package ];
		}
		else {
			$files{$file} = $package;
		}
	}

}

sub start_tag
{
	my ($parser, $element, %attrs) = @_;

	if ($element eq 'package') {
		$current_package = {};
	}
	elsif ($element eq 'version') {
		$$current_package{'version'} = $attrs{'ver'};
		$$current_package{'release'} = $attrs{'rel'};
		$$current_package{'epoch'}   = $attrs{'epoch'};
	}
	elsif ($element eq 'location') {
		$current_location   = $attrs{'href'};
	}
	elsif ($element eq 'rpm:provides') { $in_provides = 1; }
	elsif ($element eq 'rpm:requires') { $in_requires = 1; }
	elsif ($element eq 'rpm:entry') {
		my $name = $attrs{'name'};

		# rpmlib dependencies only pertain to the version of rpm used
		# to install the package.  We don't care about these, so ignore
		# them.
		return if ($name =~ /^rpmlib\(/);

		my $entry = { 'name' => $name };

		$$entry{'flags'} = $attrs{'flags'} if $attrs{'flags'};
		$$entry{'version'} = $attrs{'ver'} if $attrs{'ver'};
		$$entry{'release'} = $attrs{'rel'} if $attrs{'rel'};
		$$entry{'epoch'} = $attrs{'epoch'} if $attrs{'epoch'};
		#$$entry{'pre'} = $attrs{'pre'} if $attrs{'pre'};

		if ($in_provides) {
			my $provides = $$current_package{'provides'};
			if (not defined $provides) {
				$$current_package{'provides'} = {};
			}
			$$current_package{'provides'}{$name} = $entry;
		}
		elsif ($in_requires) {
			my $requires = $$current_package{'requires'};
			if (not defined $requires) {
				$$current_package{'requires'} = {};
			}
			$$current_package{'requires'}{$name} = $entry;
		}
		else {
			#print STDERR "WARNING: entry tag not in provides or requires tag\n";
		}
	}
}

sub end_tag
{
	my ($parser, $element) = @_;

	$current_data =~ s/^\s*(\S.*\S)\s*$/$1/;

	if ($element eq 'package') {
		my $name = $$current_package{'name'};
		my $ptr = $packages{$name};
		my $is_duplicate = 0;

		$$current_package{'location'} = { $current_arch =>
		                                  $current_location };
		if (ref $ptr eq 'HASH') {
			if (is_same_version($ptr, $current_package)) {
				merge_package_entries($ptr, $current_package);
				$is_duplicate = 1;
			}
			else {
				$packages{$name} = [ $ptr,
				                     $current_package ];
			}
		}
		elsif (ref $ptr eq 'ARRAY') {
			for (@$ptr) {
				if (is_same_version($ptr, $current_package)) {
					merge_package_entries($ptr,
					                      $current_package);
					$is_duplicate = 1;
				}
			}
			push @$ptr, $current_package if (!$is_duplicate)

		}
		else {
			$packages{$name} = $current_package;
		}

		if (!$is_duplicate) {
			hash_provides($current_package);
			hash_files($current_package);
		}

		$current_package = undef;
		$current_location = undef;
		$current_arch = undef;
	}
	elsif ($element eq 'name') {
		$$current_package{'name'} = $current_data;
	}
	elsif ($element eq 'arch') {
		$current_arch = $current_data;
	}
	elsif ($element eq 'rpm:provides') { $in_provides = 0; }
	elsif ($element eq 'rpm:requires') { $in_requires = 0; }
	elsif ($element eq 'file') {
		my $file_list = $$current_package{'files'};
		if (not defined $file_list) {
			$$current_package{'files'} = [];
			$file_list = $$current_package{'files'};
		}
		push @$file_list, $current_data;
	}

	$current_data = undef;
}

sub tag_data
{
	my ($parser, $data) = @_;

	$current_data .= $data;
}

my $base_dir = $ARGV[0];

if (! $base_dir) {
	usage();
	exit 1;
}

my $parser = new XML::Parser(Handlers => {Start => \&start_tag,
                                          End => \&end_tag,
                                          Char => \&tag_data});

my $handle = new IO::Zlib;
$handle->open("$base_dir/repodata/primary.xml.gz", "rb") || 
	die "Couldn't open primay.xml.gz: $!\n";
$parser->parse($handle);

$handle->close();

my %pkgdeps;
my @pkgqueue = @ARGV;
while (@pkgqueue) {
	my $package = shift @pkgqueue;

	next if (not exists $packages{$package});
	next if (exists $pkgdeps{$package});

	$pkgdeps{$package} = 1;

	for my $req (keys %{$packages{$package}{'requires'}}) {
		my $req_info = $packages{$package}{'required'}{$req};
		my $provider;
		if ($req =~ /^\//) {
			if (exists $files{$req}) {
				$provider = $files{$req};
			}
		}
		elsif (exists $packages{$req}) {
			$provider = $packages{$req};
		}
		elsif (exists $provides{$req}) {
			$provider = $provides{$req};
		}
		else {
			print STDERR "WARNING: could not resolve dependency \"$req\" for package $package\n";
			next;
		}

		# XXX HACK If multiple packages (and package versions) work,
		# assume the first one is what we want. No package or symbol
		# version checking is done.  This is very, very wrong, but
		# right now I'm too lazy to do it correctly. Also, no conflict
		# checking is done currently.  Also bad, but I don't want to
		# deal with it right now.  It really shouldn't be an issue
		# anyway, as this is only meant to bootstrap a system in lieu
		# of the Fedora installer.  None of the core packages have
		# conflicts or relevant version dependencies.
		if (ref $provider eq 'ARRAY') {
			print STDERR "WARNING: multiple candidates can satisfy \"$req\" for package $package\n";
			$provider = @$provider[0];
		}

		push @pkgqueue, $$provider{'name'} if ($provider);
	}
}

#print "$_\n" for (sort keys %pkgdeps);
for my $package (sort keys %pkgdeps) {
	my $locations = $packages{$package}{'location'};
	for my $arch qw/i686 i586 i386 noarch/ {
		if (exists $$locations{$arch}) {
			print $base_dir . "/". $$locations{$arch} . "\n";
			last;
		}
	}
}

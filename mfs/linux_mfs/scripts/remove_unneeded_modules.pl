#! /usr/bin/perl

use File::Temp;
use File::Path qw(mkpath rmtree);
use File::Copy;

use strict;

use constant TAR => 'tar';

my $PROGNAME = (split /\/+/, $0)[-1];

#my $basedir = "$ENV{HOME}/testbed/mfs/linux_mfs/target/lib/modules";
#my $version = '2.6.27.8';
#my $outdir = "$ENV{HOME}/tmp/modstuff_test";

my $basedir;
my $version;
my $outdir;

my %seen;
my %deps;
my %files;
my %aliases;
my %symbols;
my @order;

my @firmware;

sub parse_deps
{
	my ($module_dir) = @_;
	my $depfile = "$module_dir/modules.dep";
	open DEPS, $depfile ||
		die "Couldn't open modules.dep: $!\n";

	while (<DEPS>) {
		chomp;
		my ($filename, $deplist) = /^(\S+):\s*(.*)$/;
		my $module = $filename;
		$module =~ s/.*\/([^\/]+)\.ko$/$1/;

		#my @d = map { s/.*\/([^\/]+)\.ko$/$1/; y/_/-/; $_ }
		my @d = map { s/.*\/([^\/]+)\.ko$/$1/; $_ }
				split(/\s+/, $deplist);

		# Normalize underscores to dashes
		#$module =~ y/_/-/;

		$deps{$module} = \@d if (@d);
		$files{$module} = $filename;
	}
	close DEPS;
}

sub parse_symbols
{
	my ($module_dir) = @_;
	my $symfile = "$module_dir/modules.symbols";
	open SYMS, $symfile ||
		die "Couldn't open modules.symbols: $!\n";

	while (<SYMS>) {
		chomp;
		next if (/^#/ || /^\s*$/);
		split /\s+/;
		my $sym = $_[1];
		my $module = $_[2];
		$sym =~ s/^symbol://;
		if (not exists $symbols{$module}) {
			$symbols{$module} = [];
		}
		push @{$symbols{$module}}, $sym;
	}
	close SYMS;
}

sub parse_aliases
{
	my ($module_dir) = @_;
	my $aliasfile = "$module_dir/modules.alias";
	open SYMS, $aliasfile ||
		die "Couldn't open modules.alias: $!\n";

	while (<SYMS>) {
		chomp;
		next if (/^#/ || /^\s*$/);
		split /\s+/;
		my $alias = $_[1];
		my $module = $_[2];
		if (not exists $aliases{$module}) {
			$aliases{$module} = [];
		}
		push @{$aliases{$module}}, $alias;
	}
	close SYMS;
}

sub parse_order
{
	my ($module_dir) = @_;
	my $orderfile = "$module_dir/modules.order";
	open ORDER, $orderfile ||
		die "Couldn't open modules.order: $!\n";

	while (<ORDER>) {
		chomp;
		/.*\/([^\/]+)\.ko$/;
		if (exists $seen{$1}) {
			push @order, $_;
		}
	}
	close ORDER;
}

sub parse_firmware
{
	my ($module_file) = @_;
	my @files;
	open STRINGS, "strings \"$module_file\"|" or
		die "Couldn't read $module_file: $!\n";
	while (<STRINGS>) {
		chomp;
		next unless (/^firmware=(.*)$/);
		push @files, $1;
	}

	return @files;
}

#my @modules = @ARGV;
my @modules = qw/unix sunrpc af_packet ext3 ext2 ufs/;
if (@ARGV != 3) {
	print STDERR "Usage: $PROGNAME modules_tarball rootdir module_list\n";
	exit 1;
}
my ($tarball, $outdir, $modlist) = @ARGV;
if (! -e $tarball) {
	print STDERR "$PROGNAME: $tarball: $!\n";
	exit 1;
} elsif (! -d $outdir) {
	print STDERR "$PROGNAME: $outdir: $!\n";
	exit 1;
} elsif (! -e $modlist) {
	print STDERR "$PROGNAME: $modlist: $!\n";
	exit 1;
}

my $tmpdir = mkdtemp("$0_XXXX");
system(TAR . " -xzf $tarball -C $tmpdir");
if ($? != 0) {
	rmtree($tmpdir);
	exit 1;
}

$version = glob "$tmpdir/lib/modules/*";
$version = (split /\/+/, $version)[-1];
$basedir = "$tmpdir/lib";
my $module_basedir = "$basedir/modules";
my $firmware_basedir = "$basedir/firmware";

open MODLIST, $modlist || die "Couldn't read $modlist: $!\n";
while (<MODLIST>) {
	chomp;
	next if (/^#/ || /^\s+$/);
	push @modules, $_;
}
close MODLIST;

my @list = @modules;

parse_deps("$module_basedir/$version");
parse_symbols("$module_basedir/$version");
parse_aliases("$module_basedir/$version");

while (@list) {
	my $module = shift @list;
	#$module =~ y/_/-/;

	next if (exists $seen{$module});
	next if (not exists $files{$module});

	if (exists $deps{$module}) {
		push @list, @{$deps{$module}};
	}

	$seen{$module} = 1;
}
parse_order("$module_basedir/$version");

my $module_outdir = $outdir . '/lib/modules/' . $version;
my $firmware_outdir = $outdir . '/lib/firmware';
rmtree($module_outdir);
rmtree($firmware_outdir);
mkpath($module_outdir);
mkpath($firmware_outdir);

open DEPS, ">$module_outdir/modules.dep" || die "Can't write to modules.dep: $!\n";
#print "#modules.dep\n\n";
for my $module (keys %seen) {
	my @tmp = map { $files{$_} } @{$deps{$module}};
	printf DEPS "%s: %s\n", $files{$module}, join(' ', @tmp);
}
close DEPS;

open SYMS, ">$module_outdir/modules.symbols" || die "Can't write to modules.symbols: $!\n";
#print "#modules.symbols\n\n";
for my $module (keys %seen) {
	for my $sym (@{$symbols{$module}}) {
		printf SYMS "alias symbol:%s %s\n", $sym, $module;
	}
}
close SYMS;

open ALIAS, ">$module_outdir/modules.alias" || die "Can't write to modules.alias: $!\n";
#print "#modules.aliases\n\n";
for my $module (keys %seen) {
	for my $alias (@{$aliases{$module}}) {
		printf ALIAS "alias %s %s\n", $alias, $module;
	}
}
close ALIAS;

open ORDER, ">$module_outdir/modules.order" || die "Can't write to modules.order: $!\n";
print ORDER "$_\n" for (@order);
close ORDER;

for my $module (keys %seen) {
	my $file = $files{$module};
	my $path = $file;
	$path =~ s/\/[^\/]+$//;
	mkpath($module_outdir . '/' . $path);
	copy("$module_basedir/$version/$file", "$module_outdir/$file");
	push @firmware, parse_firmware("$module_basedir/$version/$file");
}

for my $fw (@firmware) {
	my $path = $fw;
	$path =~ s/\/[^\/]+$//;
	mkpath($firmware_outdir . '/' . $path);
	if (! -f "$firmware_basedir/$fw") {
		print STDERR "WARNING: $firmware_basedir/$fw does not exist\n";
		next;
	}
	copy("$firmware_basedir/$fw", "$firmware_outdir/$fw");
}
rmtree($tmpdir);

#! /usr/bin/perl

my $VOL_ID = '/lib/udev/vol_id';
my $BLKID = '/sbin/blkid';
my $FDISK = '/sbin/fdisk';
my $MOUNT = '/bin/mount';
my $UMOUNT = '/bin/umount';
my $RM = '/bin/rm';
my $CP = '/bin/cp';
my $CPIO = 'cpio';
my $GZIP = 'gzip';
my $MKSWAP = '/sbin/mkswap';
my $UUIDGEN = 'uuidgen';
my $LOSETUP = 'losetup';
my $TUNE2FS = 'tune2fs';

use constant GZHDR1 => 0x1f8b0800;
use constant GZHDR2 => 0x1f8b0808;
use constant LARGEST_PAGE_SIZE => 0x4000;
use constant UUID_OFFSET => 1036;

# Load up the paths. Done like this in case init code is needed.
BEGIN
{
    if (! -e "/etc/emulab/paths.pm") {
	die("Yikes! Could not require /etc/emulab/paths.pm!\n");
    }
    require "/etc/emulab/paths.pm";
    import emulabpaths;
}

use strict;

sub get_uuid
{
	my ($device) = @_;
	my $uuid;

	if (-x $VOL_ID) {
		open CMD, $VOL_ID . " --uuid $device |" || die
		     "Couldn't run vol_id: $!\n";

		$uuid = <CMD>;
		chomp $uuid;

		close CMD;
	}
	elsif (-x $BLKID) {
		open CMD, "$BLKID|" || die
			"Couldn't run blkid: $!\n";

		while (<CMD>) {
			next unless m#^$device:\s+#;
			chomp;
			next unless s/.*\s+UUID="([a-f0-9-]+)".*/\1/;
			$uuid = $_;
		}

		close CMD;
	}
	else {
		print STDERR "Can't find program to get FS UUID\n";
	}

	return $uuid;
}

sub get_label
{
	my ($device) = @_;
	my $label;

	if (-x $VOL_ID) {
		open CMD, $VOL_ID . " --label-raw $device|" || die
		     "Couldn't run vol_id: $!\n";

		$label = <CMD>;
		chomp $label;

		close CMD;
	}
	elsif (-x $BLKID) {
		open CMD, "$BLKID|" || die
			"Couldn't run blkid: $!\n";

		while (<CMD>) {
			next unless m#^$device:\s+#;
			chomp;
			next unless s/.*\s+LABEL="([^"]*)".*/\1/;
			$label = $_;
		}

		close CMD;
	}
	else {
		print STDERR "Can't find program to get FS label\n";
	}

	return $label;
}

sub set_random_rootfs_uuid
{
	my ($root) = @_;
	
	system("$TUNE2FS -U random $root");
}

sub kernel_version_compare
{
	my ($v1, $v2) = @_;

	my $v1_code = sprintf "%x%02x%02x", (split /\./, $v1)[0 .. 2];
	my $v2_code = sprintf "%x%02x%02x", (split /\./, $v2)[0 .. 2];

	if ($v1_code < $v2_code) {
		return -1;
	}
	elsif ($v1_code == $v2_code) {
		return 0;
	}
	else {
		return 1;
	}
}

sub find_swap_partitions
{
	my ($device) = @_;
	my @swap_devices;

	open CMD, $FDISK . " -l $device|" ||
	     die "Couldn't run fdisk: $!\n";

	while (<CMD>) {
		next unless m#^$device#;
		split /\s+/;
		if ($_[4] eq '82') {
			push @swap_devices, $_[0];
		}
	}

	close CMD;

	return @swap_devices;
}

sub set_swap_uuid
{
	my ($device, $uuid) = @_;
	my $swap_header;
	
	if (!open DEVICE, "+<$device") {
		print STDERR "Couldn't open $device: $!\n";      
		return 1;
	}
	
	my $id;
	for (my $i = 0x1000; $i <= LARGEST_PAGE_SIZE; $i <<= 1) {
		seek DEVICE, 0, 0;
		read DEVICE, $swap_header, $i;
		$id = unpack('Z10', substr($swap_header, $i - 10));
		last if ($id eq 'SWAPSPACE2');
	}
	                              
	if ($id ne 'SWAPSPACE2') {
		print STDERR "Device $device does not contain a new-style swap header\n";
		close DEVICE;
		return 1;
	}
	
	my $new_uuid = pack('H8H4H4H4H12', split(/-/, $uuid));
	substr $swap_header, UUID_OFFSET, 16, $new_uuid;
	
	seek DEVICE, 0, 0;
	print DEVICE $swap_header;
	
	close DEVICE;
}

sub fix_swap_partitions
{
	my ($imageroot, $root, $old_root) = @_;
	my @swapdevs;

	return undef unless (-x $MKSWAP);

	my ($root_disk) = ($root =~ m#^(/dev/[a-z]+)#);
	my ($old_root_disk) = ($old_root =~ m#^(/dev/[a-z]+)#);
	my @swap_partitions = find_swap_partitions($root_disk);
	my ($l, $u) = binary_supports_blkid("$imageroot/sbin/swapon");

	for my $part (@swap_partitions) {
		my $swapdev = $part;
		next if system("$MKSWAP $part");
		my $uuid = get_uuid($part);
		if ($u) {
			if ($uuid) {
				$swapdev = "UUID=$uuid";
			}
			else {
				# BusyBox's mkswap doesn't support UUIDs, so
				# we'll need to generate it and write it out.
				$uuid = `$UUIDGEN`;
				chomp $uuid;
				set_swap_uuid($swapdev, $uuid);
				$swapdev="UUID=$uuid";
			}
		}
		elsif ($old_root) {
			$swapdev =~ s#^$root_disk#$old_root_disk#;
		}

		push @swapdevs, $swapdev;
	}

	return undef unless (@swapdevs);

	my @buffer;
	if (!open FSTAB, "+<$imageroot/etc/fstab") {
		print STDERR "Failed to open fstab: $!\n";
		return undef;
	}

	@buffer = grep {!/^[^#].*\bswap\b.*$/} <FSTAB>;
	for (@swapdevs) {
		push @buffer, "$_\tnone\tswap\tsw\t0 0\n";
	}
	seek FSTAB, 0, 0;
	print FSTAB @buffer;
	close FSTAB;
}

sub file_replace_root_device
{
	my ($imageroot, $file, $old_root, $new_root) = @_;
	my @buffer;

	open FILE, "+<$imageroot/$file" ||
	     die "Couldn't open $imageroot/$file: $!\n";

	while (<FILE>) {
		s#$old_root#$new_root#g;
		push @buffer, $_;
	}

	seek FILE, 0, 0;

	print FILE @buffer;

	close FILE;

	return 1;
}

sub rewrite_lilo_config
{
	my ($imageroot, $old_root, $new_root) = @_;
	my @buffer;

	open FILE, "+<$imageroot/etc/lilo.conf" ||
	     die "Couldn't open $imageroot/etc/lilo.conf: $!\n";

	while (<FILE>) {
		chomp;
		next if /^\s*root\s*=\s*/;
		s/^\s*boot\s*=\s*.*$/boot = $new_root/g;
		if (/append\s*=\s*/i) {
			s/\s*root=[^"'\s]*//g;
			s/(["'])\s*$/ root=$new_root$1/;
		}
		push @buffer, $_;
	}

	seek FILE, 0, 0;

	print FILE "$_\n" for (@buffer);

	close FILE;

	return 1;
}

sub set_runlilo_flag
{
	my ($imageroot, $default_entry, $new_root) = @_;
	my $cmdline;

	open FILE, ">$imageroot/var/emulab/boot/runlilo" ||
	     die "Couldn't write to runlilo file: $!\n";
	
	$cmdline = "$default_entry root=$new_root";

	print FILE "$cmdline\n";

	close FILE;

	return $cmdline;
}

sub find_default_grub_entry
{
	my ($imageroot, $conf) = @_;
	my @buffer;
	my $default = 0;
	my $current_entry = -1;
	my ($kernel, $cmdline, $initrd);

	open FILE, "$imageroot/$conf" || die "Couldn't read grub config: $!\n";
	while (<FILE>) {
		if (/^default\s+(\d+)$/) {
			$default = $1;
			next;
		}
		elsif (/^\s*(\S+)\s+(.*)$/) {
			if ($1 eq 'title') {
				$current_entry++;
			}

			next if ($current_entry < $default);
			last if ($current_entry > $default);

			if ($1 eq 'kernel') {
				$_ = $2;
				($kernel, $cmdline) = /^(\S+)\s+(.*)$/;
			}
			elsif ($1 eq 'initrd') {
				$initrd = $2;
			}
		}
	}
	close FILE;

	return ($kernel, $cmdline, $initrd);
}

sub find_default_grub2_entry
{
	my ($imageroot, $conf) = @_;
	my @buffer;
	my $default = 0;
	my $current_entry = -1;
	my ($kernel, $cmdline, $initrd);

	open FILE, "$imageroot/$conf" || die "Couldn't read grub config: $!\n";
	while (<FILE>) {
		if (/^set\s+default\s*=\s*["']?(\d+)["']?$/) {
			$default = $1;
			next;
		}
		elsif (/^\s*(\S+)\s+(.*)$/) {
			if ($1 eq 'menuentry') {
				$current_entry++;
			}

			next if ($current_entry < $default);
			last if ($current_entry > $default);

			if ($1 eq 'linux') {
				$_ = $2;
				($kernel, $cmdline) = /^(\S+)\s+(.*)$/;
			}
			elsif ($1 eq 'initrd') {
				$initrd = $2;
			}
		}
	}
	close FILE;

	return ($kernel, $cmdline, $initrd);
}

sub set_grub_root_device
{
	my ($imageroot, $grub_config, $root) = @_;
	my ($root_disk, $root_part) =
	   ($root =~ m#/dev/(.+)(\d+)#);
	my $grub_disk;

	if (-f $BOOTDIR . "/edd_map") {
		open FILE, $BOOTDIR . "/edd_map";
		while (<FILE>) {
			chomp;
			split /=/;
			if ($_[0] eq $root_disk) {
				$grub_disk = hex($_[1]) - 0x80;
				print "Found GRUB root device using EDD\n";
				last;
			}
		}
		close FILE;
	}

	if (not defined $grub_disk) {
		$grub_disk = $root;
		$grub_disk =~ s/^\/dev\/[hs]d(.).*$/$1/;
		$grub_disk =~ y/[a-h]/[0-7]/;
		print "Found GRUB root device by guessing\n";
	}

	printf "GRUB root device is (hd%d,%d)\n", $grub_disk, $root_part - 1;
	$root_part--;

	open FILE, "+<$imageroot/$grub_config" or
	     die "Couldn't open GRUB config: $!\n";
	my @buffer;
	while (<FILE>) {
		s/^(\s*)root \([^)]*\)/$1root (hd$grub_disk,$root_part)/;
		push @buffer, $_;
	}
	seek FILE, 0, 0;
	print FILE @buffer;
	close FILE;
}

sub set_grub2_root_device
{
	my ($imageroot, $grub_config, $root) = @_;
	my ($root_disk, $root_part) =
	   ($root =~ m#/dev/(.+)(\d+)#);
	my $grub_disk;

	if (-f $BOOTDIR . "/edd_map") {
		open FILE, $BOOTDIR . "/edd_map";
		while (<FILE>) {
			chomp;
			split /=/;
			if ($_[0] eq $root_disk) {
				$grub_disk = hex($_[1]) - 0x80;
				print "Found GRUB root device using EDD\n";
				last;
			}
		}
		close FILE;
	}

	if (not defined $grub_disk) {
		$grub_disk = $root;
		$grub_disk =~ s/^\/dev\/[hs]d(.).*$/$1/;
		$grub_disk =~ y/[a-h]/[0-7]/;
		print "Found GRUB root device by guessing\n";
	}

	printf "GRUB root device is (hd%d,%d)\n", $grub_disk, $root_part;

	open FILE, "+<$imageroot/$grub_config" or
	     die "Couldn't open GRUB config: $!\n";
	my @buffer;
	while (<FILE>) {
		s/^(\s*set\s+root\s*=\s*["']?\(?)[^)]*(\)?["']?)/$1hd$grub_disk,$root_part$2/;
		push @buffer, $_;
	}
	seek FILE, 0, 0;
	print FILE @buffer;
	close FILE;
}

sub find_default_lilo_entry
{
	my ($imageroot, $conf) = @_;
	my %images;
	my %globals;
	my @image_list;
	my ($kernel, $cmdline, $initrd);
	my $default;

	open FILE, "$imageroot/$conf" || die "Couldn't read lilo config: $!\n";
	while (<FILE>) {
		chomp;

		my $command;
		my $args;
		s/\s*#.*$//; # Remove comments
		s/\s+$//; # Remove trailing whitespace
		next if (/^$/); # Skip blank lines
		if (/^\s*image\s*=\s*(.*)\s*$/i) {
			push @image_list, $1;
			$images{$1} = {'image' => $1};
		}
		elsif (/^\s*(?:label|alias)\s*=\s*(.*)\s*$/i) {
			$images{$1} = $images{$image_list[-1]};
		}
		elsif (/^\s*(\S+)\s*=\s*(.*)\s*$/) {
			($command, $args) = (lc $1, $2);
		}
		elsif (/^\s*(?:read-only|ro)$/) {
			($command, $args) = ('read-only', 'ro');
		}
		elsif (/^\s*(?:read-write|rw)$/) {
			($command, $args) = ('read-only', 'rw');
		}
		else {
			($command, $args) = ($_, 1);
		}

		if (@image_list) {
			${$images{$image_list[-1]}}{$command} = $args;
		}
		else {
			$globals{$command} = $args;
		}
	}
	close FILE;

	if (exists $globals{'default'}) {
		$default = $globals{'default'};
	}
	else {
		$default = $image_list[0];
	}

	my $root = $images{$default}{'root'};
	$initrd = $images{$default}{'initrd'};
	$cmdline = $images{$default}{'append'};
	$kernel = $images{$default}{'image'};
	my $ro = $images{$default}{'read-only'};
	if (not defined $root) {
		$root = $globals{'root'};
	}
	if (not defined $initrd) {
		$initrd = $globals{'initrd'};
	}
	if (not defined $cmdline) {
		$cmdline = $globals{'append'};
	}
	if (not defined $ro) {
		$ro = $globals{'read-only'};
	}

	$cmdline =~ s/^"(.*)"$/$1/;
	if ($root && $cmdline !~ /\broot=\S+\b/) {
		$cmdline .= " root=$root";
	}
	if ($ro && $cmdline !~ /\br[ow]\b/) {
		$cmdline .= " $ro";
	}

	return ($default, $kernel, $cmdline, $initrd);
}

sub guess_bootloader
{
	my ($device) = @_;
	my $buffer;
	my $bootloader;

	open DEVICE, $device || die "Couldn't open $device: $!\n";
	read DEVICE, $buffer, 512;
	close DEVICE;

	if ($buffer =~ /GRUB/) {
		$bootloader = 'grub';
	}
	elsif ($buffer =~ /LILO/) {
		$bootloader = 'lilo';
	}

	return $bootloader;
}

sub udev_supports_label
{
	my ($imageroot) = @_;
	my ($handles_label, $handles_uuid) = (0, 0);

	if (! -d "$imageroot/etc/udev/rules.d") {
		return (0, 0);
	}

	my @files = glob "$imageroot/etc/udev/rules.d/*";
	if (@files) {
		@files = glob "$imageroot/lib/udev/rules.d/*";
	}
	for my $file (@files) {
		open FILE, $file or next;
		my @buffer = <FILE>;
		close FILE;
		if (grep(/[^#]*ID_FS_UUID/, @buffer)) {
			$handles_uuid = 1;
		}

		if (grep(/[^#]*ID_FS_LABEL/, @buffer)) {
			$handles_label = 1;
		}
	}

	return ($handles_label, $handles_uuid);
}

sub binary_supports_blkid
{
	my ($file) = @_;
	my $handles_label = 0;
	my $handles_uuid = 0;

	if (-x $file) {
		local $/;
		my $buffer;
		open FILE, $file or
		     die "Couldn't grep $file: $!\n";
		$buffer = <FILE>;
		close FILE;

		if ($buffer =~ /libblkid\.so/) {
			$handles_label = 1;
			$handles_uuid = 1;
		}
		else{ 
			if ($buffer =~ /LABEL/) {
				$handles_label = 1;
			}
			if ($buffer =~ /UUID/) {
				$handles_uuid = 1;
			}
		}
	}

	return ($handles_label, $handles_uuid);
}

sub get_fstab_root
{
	my ($imageroot) = @_;
	my $root;

	open FSTAB, "$imageroot/etc/fstab" ||
	     die "Couldn't open fstab: $!\n";

	while (<FSTAB>) {
		split /\s+/;
		if ($_[1] eq '/') {
			$root = $_[0];
		}
	}

	close FSTAB;

	return $root;
}

sub check_kernel
{
	my ($kernel) = @_;
	my $offset = 0;;
	my $buffer;
	my $rc;
	my $kernel_file = "/tmp/kernel.$$";
	my $kernel_has_ide = 0;
	my $version_string;

	open KERNEL, $kernel or die "Couldn't open kernel: $!\n";
	read KERNEL, $buffer, 4;
	while ($rc = read KERNEL, $buffer, 1, length($buffer)) {
		my ($value) = unpack 'N', $buffer;
		if ($value == GZHDR1 || $value == GZHDR2) {
			last;
		}
		$buffer = substr $buffer, 1;
	}
	if ($rc == 0) {
		return undef;
	}

	open GZIP, "|$GZIP -dc > $kernel_file 2> /dev/null";
	print GZIP $buffer;
	while (read KERNEL, $buffer, 4096) {
		print GZIP $buffer;
	}
	close KERNEL;
	close GZIP;


	open KERNEL, $kernel_file or die "Couldn't open raw kernel: $!\n";
	while (<KERNEL>) {
		if (/ide[-_]disk/) {
			$kernel_has_ide = 1;
		}
		if (/Linux version (\S+)/) {
			$version_string = $1;
		}
	}
	close KERNEL;

	unlink "$kernel_file";

	return ($version_string, $kernel_has_ide);
}

sub check_initrd
{
	my ($initrd) = @_;
	my $decompressed_initrd = "/tmp/initrd.$$";
	my $initrd_dir = "/tmp/initrd.dir.$$";
	my $handles_label = 0;
	my $handles_uuid = 0;

	return undef if (! -f $initrd);

	mkdir $initrd_dir;
	`$GZIP -dc < "$initrd" > "$decompressed_initrd" 2> /dev/null`;
	if ($? >> 8) {
		`$CP "$initrd" "$decompressed_initrd"`;
		if ($? & 0xff) {
			return undef;
		}
	}

	`$MOUNT -o ro,loop -t ext3 "$decompressed_initrd" "$initrd_dir" 2> /dev/null`;
	if ($? >> 8) {
		`$MOUNT -o ro,loop -t ext2 "$decompressed_initrd" "$initrd_dir" 2> /dev/null`;
		if ($? >> 8) {
			`cd "$initrd_dir" && $CPIO -idu < "$decompressed_initrd" > /dev/null 2>/dev/null`;
			if ($? >> 8) {
				return undef;
			}
		}
	}

	for ('/bin/busybox', '/bin/nash', '/bin/mount') {
		next unless (-f "$initrd_dir/$_");
		($handles_label, $handles_uuid) =
		    binary_supports_blkid("$initrd_dir/$_");
		last;
	}

	if (!$handles_label && !$handles_uuid) {
		($handles_label, $handles_uuid) =
		    udev_supports_label($initrd_dir);
	}

	`$UMOUNT "$initrd_dir" > /dev/null 2> /dev/null`;
	`$RM -rf "$initrd_dir" "$decompressed_initrd"`;
	
	my @loopdevs;
	open LOSETUP, "$LOSETUP|";
	while (<LOSETUP>) {
		chomp;
		split /:/;
		push @loopdevs, $_[0];
	}
	
	for my $dev (@loopdevs) {
		`$LOSETUP -d $dev`;
	}
		

	return ($handles_label, $handles_uuid);
}

sub mount_image
{
	my ($root, $imageroot) = @_;
	my $fstype;

	for my $type (qw/ext3 ext2/) {
		`mount -t $type $root $imageroot`;
		if (!($? >> 8)) {
			$fstype = $type;
			last;
		}
	}

	return $fstype;
}

sub update_random_seed
{
	my ($imageroot) = @_;
	my $seed;
	my $size;
	my $rc = 1;

	if (! -f "$imageroot/var/lib/random-seed") {
		return undef;
	}

	print "Updating /var/lib/random-seed\n";
	open URANDOM, '/dev/urandom';
	$size = 512;
	while ($rc && $size > 0) {
		my $rc = read URANDOM, $seed, $size, length($seed);
		$size -= $rc;
	}
	close URANDOM;

	open SEED, ">$imageroot/var/lib/random-seed";
	print SEED $seed;
	close SEED;
}

sub hardwire_boss_node
{
	my ($imageroot) = @_;
	my $bossnode;

	if (-r $ETCDIR . "/bossnode") {
		local $/;
		open BOSSNODE, $ETCDIR . "/bossnode";
		$bossnode = <BOSSNODE>;
		close BOSSNODE;

		chomp $bossnode;
	}
	else {
		return undef;
	}

	if (!-d "$imageroot/etc/emulab") {
		return undef;
	}

	print "Hardwiring boss to $bossnode\n";
	if (!open BOSSNODE, "$imageroot/etc/emulab/bossnode") {
		print STDERR "Failed to create /etc/emulab/bossnode\n";
		return undef;
	}

	print BOSSNODE "$bossnode\n";
	close BOSSNODE;

	return 1;
}

sub main
{
	my ($root, $old_root) = @_;
	my $imageroot = '/mnt';
	my $new_fstab_root;
	my $new_bootloader_root;
	my $old_bootloader_root;
	my $grub_config;
	my ($kernel, $cmdline, $initrd);
	my $lilo_default;
	my $lilo_commandline = 0;

	my $old_uuid = get_uuid($root);
	set_random_rootfs_uuid($root);
	my $fstype = mount_image($root, $imageroot);
	my $uuid = get_uuid($root);
	my $label = get_label($root);
	my $bootloader = guess_bootloader($root);
	my $old_fstab_root = get_fstab_root($imageroot);

	# HACK: there's no simple way to distinguish grub2 from grub
	# by the boot sector.
	if ($bootloader eq 'grub' && -f "$imageroot/boot/grub/grub.cfg") {
		$bootloader = 'grub2';
	}

	if ($bootloader eq 'lilo') {
		($lilo_default, $kernel, $cmdline, $initrd) =
		    find_default_lilo_entry($imageroot, "/etc/lilo.conf");
	}
	elsif ($bootloader eq 'grub') {
		for (qw#/boot/grub/grub.conf /etc/grub.conf /boot/grub/menu.lst#) {
			if (-f "$imageroot/$_") {
				$grub_config = $_;
				last;
			}
		}

		($kernel, $cmdline, $initrd) =
		    find_default_grub_entry($imageroot, $grub_config);

	}
	elsif ($bootloader eq 'grub2') {
		for (qw#/boot/grub/grub.cfg#) {
			if (-f "$imageroot/$_") {
				$grub_config = $_;
				last;
			}
		}

		($kernel, $cmdline, $initrd) =
		    find_default_grub2_entry($imageroot, $grub_config);

	}
	else {
		print STDERR "Couldn't guess bootloader\n";
		return 1;
	}

	for my $token (split /\s+/, $cmdline) {
		next unless ($token =~ /^root=/);
		$token =~ s/^root=//;
		$old_bootloader_root = $token;
		last;
	}

	my ($initrd_does_label, $initrd_does_uuid) = 
	    check_initrd("$imageroot/$initrd");
	my ($mount_does_label, $mount_does_uuid) = 
	    binary_supports_blkid("$imageroot/bin/mount");

	my ($kernel_version, $kernel_has_ide) = 
	    check_kernel("$imageroot/$kernel");

	print "Root FS UUID: $uuid\n";
	print "Root FS LABEL: $label\n";
	print "Installed bootloader: $bootloader\n";
	print "fstab root: $old_fstab_root\n";
	print "kernel: $kernel\n";
	print "kernel version: $kernel_version\n";
	print "kernel has IDE support: $kernel_has_ide\n";
	print "cmdline: $cmdline\n";
	print "initrd: $initrd\n";
	print "initrd blkid: $initrd_does_label $initrd_does_uuid\n";
	print "mount blkid: $mount_does_label $mount_does_uuid\n";

	if ($uuid && $mount_does_uuid) {
		$new_fstab_root = "UUID=$uuid";
	}
	elsif ($label && $mount_does_label) {
		$new_fstab_root = "LABEL=$label";
	}
	elsif (kernel_version_compare($kernel_version, '2.6.21') < 0 ||
	       $kernel_has_ide) {
		$new_fstab_root = $old_root;
	}
	else {
		$new_fstab_root = $root;
	}

	if ($uuid && $initrd_does_uuid) {
		$new_bootloader_root = "UUID=$uuid";
	}
	elsif ($label && $initrd_does_label) {
		$new_bootloader_root = "LABEL=$label";
	}
	elsif (kernel_version_compare($kernel_version, '2.6.21') < 0 ||
	       $kernel_has_ide) {
		$new_bootloader_root = $old_root;
	}
	else {
		$new_bootloader_root = $root;
	}

	print "Rewriting /etc/fstab to use '$new_fstab_root' as root device\n";
	file_replace_string($imageroot, '/etc/fstab', $old_fstab_root,
	                         $new_fstab_root);

	print "Rewriting $bootloader config to use '$new_bootloader_root' as root device\n";
	if ($bootloader eq 'lilo') {
		rewrite_lilo_config($imageroot, $old_bootloader_root, $new_bootloader_root);
		$lilo_commandline = set_runlilo_flag($imageroot, $lilo_default, $new_bootloader_root);
	}
	elsif ($bootloader eq 'grub') {
		file_replace_string($imageroot, $grub_config, $old_bootloader_root,
		                         $new_bootloader_root);
		set_grub_root_device($imageroot, $grub_config, $root);
	}
	elsif ($bootloader eq 'grub2') {
		file_replace_string($imageroot, $grub_config, $old_bootloader_root,
		                         $new_bootloader_root);
		file_replace_string($imageroot, $grub_config, $old_uuid, $uuid);
		set_grub2_root_device($imageroot, $grub_config, $root);
	}

	fix_swap_partitions($imageroot, $root,
		$kernel_has_ide ? $old_root : undef );

	update_random_seed($imageroot);
	hardwire_boss_node($imageroot);

	# Run any postconfig scripts
	if (-x $BINDIR . '/osconfig') {
		print "Checking for dynamic client-side updates to slice...\n";
		system($BINDIR . "/osconfig -m $imageroot -M \"-t $fstype\" -f $fstype " .
		       "-D $root -s Linux postload");
	}

	`umount $imageroot`;

	if ($lilo_commandline) {
		$root =~ m/^(.*)(\d+)$/;
		my $disk = $1;
		my $part = $2;

		system("$BINDIR/groklilo -c \"$lilo_commandline\" $part $disk");
		if ($?) {
			print STDERR "Failed to set LILO command line\n";
			return 1;
		}
	}

	return 0;
}

main(@ARGV);

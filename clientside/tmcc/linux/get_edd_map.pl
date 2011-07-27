#! /usr/bin/perl

use constant EDD_PREFIX => '/sys/firmware/edd';
use strict;

my %mbr_sig_map;
my %disk_size;
my %edd_data;
my %blockdev_found;

sub read_file
{
	my ($filename) = @_;
	my $output;


	open FILE, $filename || return undef;
	while (<FILE>) {
		$output .= $_;
	}
	close FILE;
	chomp $output;

	return $output;
}

sub read_mbr
{
	my ($device) = @_;
	my $output;

	open FILE, $device or die "Couldn't open $device: $!\n";
	read FILE, $output, 512;
	close FILE;

	return $output;
}

sub read_disk_mbr_signatures
{
	for my $disk (glob "/sys/block/[hs]d[a-z]") {
		my $size;
		open SIZE, "$disk/size";
		($size) = <SIZE>;
		close SIZE;
		chomp $size;
		$disk =~ s/^.*\///;
		$disk_size{$disk} = $size;
		my $sig = substr read_mbr("/dev/$disk"), 440, 4;
		$mbr_sig_map{$disk} = unpack 'V', $sig;
	}
}

sub read_edd_data
{
	for my $dir (glob EDD_PREFIX . "/*") {
		my ($bus, $busid, $channel);
		my ($interface, $device, $lun);
		my $junk;
		my $disk_hash = {};

		my $data = read_file("$dir/host_bus");
		if (defined $data) {
			my ($bus, $busid, $junk1, $channel) = split(/\s+/, $data);
			$$disk_hash{'bus'} = $bus;
			$$disk_hash{'busid'} = $busid;
			$$disk_hash{'channel'} = $channel;
		}

		my $data = read_file("$dir/interface");
		if (defined $data) {
			my ($interface, $junk, $device, $junk, $lun) = split /\s+/, $data;
			$lun = 0 unless defined $lun;

			$$disk_hash{'interface'} = $interface;
			$$disk_hash{'device'} = $device;
			$$disk_hash{'lun'} = $lun;
		}

		if (-l "$dir/pci_dev") {
			$$disk_hash{'pci_dev'} = "$dir/pci_dev";
		}

		my $sectors = read_file("$dir/sectors");
		if (defined $sectors) {
			$$disk_hash{'sectors'} = $sectors;
		}

		my $sig = read_file("$dir/mbr_signature");
		if (defined $sig) {
			$$disk_hash{'mbr_signature'} = hex $sig;
		}

		$dir =~ s/.*int13_dev//;
		$edd_data{$dir} = $disk_hash;
	}
}

sub resolve_edd_pcidev
{
	my ($bios_device) = @_;

	my $pci_dev_file = ${$edd_data{$bios_device}}{'pci_dev'};
	my $channel = ${$edd_data{$bios_device}}{'channel'};
	my $device = ${$edd_data{$bios_device}}{'device'};
	my $lun = ${$edd_data{$bios_device}}{'lun'};

	return undef unless ($pci_dev_file);

	my @host_adapters = sort glob "$pci_dev_file/host*";
	if (@host_adapters < $channel) {
		return undef;
	}

	$host_adapters[$channel] =~ /\d+$/;
	my $host_num = $&;

	my $path;
	my $dev_path = $host_adapters[$channel] .
	               "/target$host_num:0:$device";
	my $lun_path = "$dev_path/$host_num:0:$device:$lun";

	if (! -e $lun_path) {
		$path = "$dev_path/$host_num:0:$device:0";
	}
	else {
		$path = $lun_path;
	}

	my $blockdev;
	if ( -d "$path/block") {
		($blockdev) = glob "$path/block/*";
	}
	elsif ($blockdev =  glob "$path/block:*") {
		$blockdev =~ s/.*block://;
	}
	else {
		return undef;
	}

	$blockdev =~ s/.*\///;
	${$edd_data{$bios_device}}{'blockdev'} = $blockdev;
	$blockdev_found{$blockdev} = 1;
}

sub resolve_edd_mbrsig
{
	my ($bios_device) = @_;

	my $mbr_signature = ${$edd_data{$bios_device}}{'mbr_signature'};
	my $blockdev = ${$edd_data{$bios_device}}{'blockdev'};
	my $sectors = ${$edd_data{$bios_device}}{'sectors'};

	return if defined $blockdev;
	return undef unless defined ($mbr_signature);

	my $last_match;
	for my $blockdev (keys %disk_size) {
		next if ($blockdev_found{$blockdev});
		next if ($sectors != $disk_size{$blockdev});
		if ($mbr_signature == $mbr_sig_map{$blockdev}) {
			if (not defined $last_match) {
				$last_match = $blockdev;
			}
			else {
				return undef;
			}
		}
	}

	if ($last_match) {
		${$edd_data{$bios_device}}{'blockdev'} = $last_match;
	}
}

sub main
{
	`modprobe edd`;
	read_edd_data();
	read_disk_mbr_signatures();


	for my $device (keys %edd_data) {
		resolve_edd_pcidev($device);
	}

	for my $device (keys %edd_data) {
		resolve_edd_mbrsig($device);
	}

	for my $device (keys %edd_data) {
		my $blockdev = ${$edd_data{$device}}{'blockdev'};

		if ($blockdev) {
			print "$blockdev=$device\n";
		}
	}
}

main(@ARGV);

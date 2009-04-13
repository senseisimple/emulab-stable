#!/usr/bin/perl -wT
##
## EMULAB-COPYRIGHT
## Copyright (c) 2008-2009 University of Utah and the Flux Group.
## All rights reserved.
##
## Implements the libvnode API for Xen support in Emulab.
##
package libvnode_xen;
use File::Path;
use Data::Dumper;
require "/etc/emulab/paths.pm"; import emulabpaths;
use libvnode;

# Doesn't seem like we need this stuff
use Exporter;
@ISA    = "Exporter";
@EXPORT = qw( init setDebug rootPreConfig
              rootPreConfigNetwork rootPostConfig
	      vnodeCreate vnodeDestroy vnodeState
	      vnodeBoot vnodeHalt vnodeReboot vnodeKill
	      vnodePreConfig vnodePreConfigControlNetwork
              vnodePreConfigExpNetwork vnodeConfigResources
              vnodeConfigDevices vnodePostConfig
	    );

%ops = ( 'init' => \&init,
         'setDebug' => \&setDebug,
         'rootPreConfig' => \&rootPreConfig,
         'rootPreConfigNetwork' => \&rootPreConfigNetwork,
         'rootPostConfig' => \&rootPostConfig,
         'vnodeCreate' => \&vnodeCreate,
         'vnodeDestroy' => \&vnodeDestroy,
         'vnodeState' => \&vnodeState,
         'vnodeBoot' => \&vnodeBoot,
         'vnodeHalt' => \&vnodeHalt,
         'vnodeReboot' => \&vnodeReboot,
         'vnodeKill' => \&vnodeKill,
         'vnodePreConfig' => \&vnodePreConfig,
         'vnodePreConfigControlNetwork' => \&vnodePreConfigControlNetwork,
         'vnodePreConfigExpNetwork' => \&vnodePreConfigExpNetwork,
         'vnodeConfigResources' => \&vnodeConfigResources,
         'vnodeConfigDevices' => \&vnodeConfigDevices,
         'vnodePostConfig' => \&vnodePostConfig,
       );

sub init($){
    return 0;
}

sub setDebug($){
    return 0;
}

sub rootPreConfig($){
    # xend start
    # route add default gw ...
    # fdisk /dev/sda4, set partition 4 to 83 - linux
    # mkfs -t ext3 /dev/sda4
    # mount /dev/sda4 /mnt/xen
    return 0;
}

sub rootPreConfigNetwork($$$){
    my ($interfaces, $d1, $d2) = @_;
    return 0;
}

sub rootPostConfig($){
    return 0;
}

my $vnodes = 0;
sub vnodeCreate($){
    $vnodes += 1;
    return $vnodes;
}

sub createDisk($){
    my ($name) = @_;
    my $disk = $name;
    my $disk_path = "/mnt/xen/disk";
    my $root_path = "/mnt/xen/root";
    my $root = "/dev/sda2";
    my $dummy;
    print "Created " . mkpath(["/mnt/xen", $disk_path, $root_path]) . "\n";
    # dd ...
    print "Create disk with dd\n";
    system("dd if=/dev/zero of=$disk seek=4000 count=1 bs=640k");
    system("echo y | mkfs -t ext3 $disk");
    system("e2label $disk /");
    print "Mount root\n";
    system("mount $root $root_path");
    # mount -o loop ...
    system("mount -o loop $disk $disk_path");
    # cp -a ...
    mkpath([map{"$disk_path/$_"} qw(proc sys home tmp)]);
    system("cp -a $root_path/{root,dev,var,etc,usr,bin,sbin,lib} $disk_path");
    # mkdir /mnt/guest/{proc,sys,home,tmp}
    # mkpath(["$disk_path/proc", "$disk_path/sys", "$disk_path/home", "$disk_path/tmp"]);
    system("umount $root_path");
    system("umount $disk_path");
}

sub diskName($){
    my ($id) = @_;
    return "/mnt/xen/disk-$id.img";
}

sub configFile($){
    my ($id) = @_;
    return "/var/xen/configs/config-$id";
}

sub xenConfig($){
    my ($data) = @_;
    my $name = $data->{'name'};
    my $disk = $data->{'disk'};
    my $mac = $data->{'mac'};    
    return <<EOF

kernel = '/boot/vmlinuz-2.6.18.8-xenU'
ramdisk = '/boot/initrd-2.6.18.8-xenU.img'
memory = 256
name = '$name'
vif = ['mac=$mac, bridge=test, script=emulab', 'bridge=experimental']
disk = ['file:$disk,hda1,w']
root = '/dev/hda1 ro'
extra = '5 xencons=tty'

EOF
}

sub createConfig($$){
    my ($file,$data) = @_;
    open FILE, ">", $file or die $!;
    print FILE xenConfig($data);
    close FILE;
}

sub vnodePreConfig($){
    my ($id,) = @_;
    return 0 if (-e diskName($id));
    createDisk(diskName($id));
    return 0;
}

my @dhcp;
sub addDhcp($){
    my ($data) = @_;
    push(@dhcp, $data);
}

# TODO: set up more than one host
sub createDhcp($){
    my ($all) = @_;
    my $words = <<EOF
ddns-update-style none;
option domain-name "emulab";
default-lease-time 604800;
max-lease-time 704800;
option domain-name-servers 155.98.32.70;
option routers 155.98.36.1;

subnet 172.18.54.0 netmask 255.255.255.0 {
  range 172.18.54.1 172.18.54.253;
}
EOF
;

    open FILE, ">", "/etc/dhcpd.conf" or die $!;
    print FILE $words;

    sub add($){
        my ($data) = @_;
        my $mac = $data->{'mac'};
        my $ip = $data->{'ip'};
        my $fqdn = $data->{'fqdn'};
        my $host = $1 if ($fqdn =~ m/(\w+)\..*/);
        my $domain = $1 if ($fqdn =~ m/\w+\.(.*)/);
        # option domain-name "$domain";
        my $words = <<EOF

host whocares {
  hardware ethernet $mac;
  fixed-address $ip;
  option host-name $host;
  option domain-name "emulab.net";
}

EOF
;

        print FILE $words;
        }

    foreach (@$all){
        add($_);
    }

    close FILE;
}

# convert 123456 into 12:34:56
sub fixupMac($){
    my ($x) = @_;
    $x =~ s/(\w\w)/$1:/g;
    chop($x);
    return $x;
}

sub vnodePreConfigControlNetwork($$$$$$$$$$){
    my ($id,$vmid,$ip,$mask,$mac,$gw,
            $vname,$longdomain,$shortdomain,$bossip) = @_;
    print "mac is $mac\n";
    print "Pre config the network for this garbage\n";  
    $stuff = {'id' => $id,
              'name' => $id,
              'ip' => $ip,
              'fqdn', => $longdomain,
              'disk' => diskName($id),
              'mac' => fixupMac($mac)};
    # createConfig($id, configFile($id), diskName($id));
    createConfig(configFile($id), $stuff);
    addDhcp($stuff);
    return 0;
}

sub vnodePreConfigExpNetwork($){
    return 0;
}

sub vnodeConfigResources($){
    return 0;
}

sub vnodeState(){
    my $err = 0;
    my $out = VNODE_STATUS_STOPPED();
    return ($err, $out);
}

sub vnodeConfigDevices($){
    return 0;
}

my $booted = 0;
sub vnodeBoot($){
    my ($id) = @_;
    if (!$booted){
        createDhcp(\@dhcp);
        $booted = 1;
    }
    my $config = configFile($id);
    if (! -e $config){
        print "Cannot find xen configuration file for virtual instance $id\n";
        return 1;
    }
    system("xm create $config");
    print "Created virtual machine $id\n";
    return 0;
}

sub vnodePostConfig($){
    return 0;
}

sub vnodeReboot($){
    my ($id) = @_;
    if ($id =~ m/(.*)/){
        $id = $1;
    }
    system("/usr/sbin/xm destroy $id");
    my $file = configFile($id);
    system("/usr/sbin/xm create $file");
}

1

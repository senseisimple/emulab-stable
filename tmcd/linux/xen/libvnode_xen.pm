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
use File::Copy;
use Data::Dumper;
require "/etc/emulab/paths.pm"; import emulabpaths;
use libvnode;

# Doesn't seem like we need this stuff
use Exporter;
@ISA    = "Exporter";
@EXPORT = qw( init setDebug rootPreConfig
              rootPreConfigNetwork rootPostConfig
	      vnodeCreate vnodeDestroy vnodeState
	      vnodeBoot vnodePreBoot vnodeHalt vnodeReboot vnodeKill
	      vnodePreConfig vnodePreConfigControlNetwork
              vnodePreConfigFinal
              vnodePreConfigExpNetwork vnodeConfigResources
              vnodeConfigDevices vnodePostConfig vnodeExec
	    );

%ops = ( 'init' => \&init,
         'setDebug' => \&setDebug,
         'rootPreConfig' => \&rootPreConfig,
         'rootPreConfig' => \&rootPreConfig,
         'rootPreConfigNetwork' => \&rootPreConfigNetwork,
         'rootPreBoot' => \&rootPreBoot,
         'rootPostConfig' => \&rootPostConfig,
         'vnodeCreate' => \&vnodeCreate,
         'vnodeDestroy' => \&vnodeDestroy,
         'vnodeState' => \&vnodeState,
         'vnodeBoot' => \&vnodeBoot,
         'vnodeHalt' => \&vnodeHalt,
         'vnodeReboot' => \&vnodeReboot,
         'vnodeExec' => \&vnodeExec,
         'vnodeKill' => \&vnodeKill,
         'vnodePreConfig' => \&vnodePreConfig,
         'vnodePreConfigFinal' => \&vnodePreConfigFinal,
         'vnodePreConfigControlNetwork' => \&vnodePreConfigControlNetwork,
         'vnodePreConfigExpNetwork' => \&vnodePreConfigExpNetwork,
         'vnodeConfigResources' => \&vnodeConfigResources,
         'vnodeConfigDevices' => \&vnodeConfigDevices,
         'vnodePostConfig' => \&vnodePostConfig,
       );

my %xen_interfaces;
my $need_root_config = 0;

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

    system("brctl addbr control-net");
    system("ifconfig control-net 172.18.54.254 up");
    system("route del -net 172.18.0.0 netmask 255.255.0.0");
    system("/sbin/sysctl -w net.ipv4.conf.eth0.proxy_arp=1");

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

sub replace_hack($){
    my ($q) = @_;
    if ($q =~ m/(.*)/){
        return $1;
    }
    return "";
}

sub disk_hacks($){
    my ($path) = @_;
    # erase cache from LABEL to devices
    my @files = <$path/etc/blkid/*>;
    unlink map{&replace_hack($_)} (grep{m/(.*blkid.*)/} @files);

    my $filetobecopied = "/var/xen/rc.ifconfig";
    my $newfile = "$path/usr/local/etc/emulab/rc/rc.ifconfig";
    copy($filetobecopied, $newfile) or die $!;
}

sub createDisk($){
    my ($name) = @_;
    my $disk = $name;
    my $disk_path = "/mnt/xen/disk";
    my $root_path = "/mnt/xen/root";
    my $root = "/dev/sda2";
    my $dummy;
    print "Created " . mkpath(["/mnt/xen", $disk_path, $root_path]) . "\n";
    print "Create disk with dd\n";
    mysystem("dd if=/dev/zero of=$disk seek=4000 count=1 bs=640k");
    mysystem("echo y | mkfs -t ext3 $disk");
    mysystem("e2label $disk /");
    print "Mount root\n";
    mysystem("mount $root $root_path");
    mysystem("mount -o loop $disk $disk_path");
    mkpath([map{"$disk_path/$_"} qw(proc sys home tmp)]);
    system("cp -a $root_path/{root,dev,var,etc,usr,bin,sbin,lib} $disk_path");

    # hacks to make things work!
    disk_hacks($disk_path);

    mysystem("umount $root_path");
    mysystem("umount $disk_path");
}

sub diskName($){
    my ($id) = @_;
    return "/mnt/xen/disk-$id.img";
}

sub configFile($){
    my ($id) = @_;
    return "/var/xen/configs/config-$id";
}

# make sure to remove the old bridge if it exists
sub bringUpExperimentalBridge($){
    my ($bridge) = @_;
    system("/usr/sbin/brctl addbr $bridge");
    system("/sbin/ifconfig $bridge up 0");
    system("/sbin/sysctl -w net.ipv4.conf.$bridge.proxy_arp=1");
}

# set up the xen configuration. should memory be configurable? currently its
# set at 256m
sub xenConfig($$){
    my ($data, $script_name) = @_;
    my $name = $data->{'control'}->{'name'};
    my $disk = $data->{'control'}->{'disk'};
    my $control_mac = $data->{'control'}->{'mac'};    
    my $links = "";
    # configure memory at some point?
    my $memory = 256;
    my $kernel = "/boot/vmlinuz-2.6.18.8-xenU";
    my $ramdisk = "/boot/initrd-2.6.18.8-xenU.img";
    my $addLink = sub($){
        my ($link) = @_;
        my $lan = $link->{'link'};
        my $xmac = fixupMac($link->{'mac'});
        my $stuff = "'bridge=exp-$lan, mac=$xmac', ";
        $links = $links . $stuff;
    };

    for (@{$data->{'links'}}){ &$addLink($_); };

    # this text is python code
    return <<EOF
# xen configuration script for $name
kernel = '$kernel'
ramdisk = '$ramdisk'
memory = $memory
name = '$name'
vif = ['mac=$control_mac, bridge=control-net, script=$script_name', $links]
disk = ['file:$disk,hda1,w']
root = '/dev/hda1 ro'
extra = '3 xencons=tty'

EOF
}

sub createConfig($$$){
    my ($file,$data,$script_name) = @_;
    print "Creating xen configuration file $file\n";
    open FILE, ">", $file or die $!;
    print FILE xenConfig($data, $script_name);
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

sub findMyIp(){
    my $CTRLIPFILE = "/var/emulab/boot/myip";
    my $ip = `cat $CTRLIPFILE`;
    chomp($ip);
    if ($ip =~ /^(\d+\.\d+\.\d+\.\d+)$/) {
        $ip = $1;
    } else {
        die "could not find valid ip in $CTRLIPFILE!";
    }
    return $ip;
}

sub vnodePreConfigFinal($$){
    my ($id,$vmid) = @_;
    my $config = configFile($id);
    createConfig($config, $xen_interfaces{$id}, $xen_interfaces{$id}->{'script'});
    $need_root_config = 1;
    return 0;
}

sub createDhcp($){
    my ($all) = @_;
    my $my_ip = findMyIp();
    # compute this
    my $dns = "155.98.32.70";
    my $words = <<EOF
ddns-update-style none;
option domain-name "emulab";
default-lease-time 604800;
max-lease-time 704800;
option domain-name-servers $dns;
option routers $my_ip;

subnet 172.18.54.0 netmask 255.255.255.0 {
  range 172.18.54.1 172.18.54.253;
}
EOF
;

    open FILE, ">", "/etc/dhcpd.conf" or die $!;
    print FILE $words;

    my $add = sub($){
        my ($data) = @_;
        my $mac = $data->{'mac'};
        my $ip = $data->{'ip'};
        my $fqdn = $data->{'fqdn'};
        my $host = $1 if ($fqdn =~ m/(\w+)\..*/);
        my $domain = $1 if ($fqdn =~ m/\w+\.(.*)/);
        my $xip = $ip;
        $xip =~ s/\.//g;
        # option domain-name "$domain";
        my $words = <<EOF

host whocares$xip {
  hardware ethernet $mac;
  fixed-address $ip;
  option host-name $host;
  option domain-name "emulab.net";
}

EOF
;

        print FILE $words;
    };

    foreach (@$all){
        &$add($_);
    }

    close FILE;
}

sub rootPreBoot(){
    if ($need_root_config){
        createDhcp(\@dhcp);
        mysystem("/etc/init.d/dhcpd restart");
        createBridges(findBridges(\%xen_interfaces));
    }
    return 0;
}

# convert 123456 into 12:34:56
sub fixupMac($){
    my ($x) = @_;
    $x =~ s/(\w\w)/$1:/g;
    chop($x);
    return $x;
}

# local tmcc ports, one tmcc proxy is run for each vnode (well two, one for
# tcp and one for udp)
my $_nextPort = 7777;
sub nextPort(){
    $_nextPort = $_nextPort + 1;
    return $_nextPort;
}

# write out the script that will be called when the control-net interface
# is instantiated by xen. this script goes in /etc/xen/scripts
sub createXenNetworkScript($$){
    my ($data,$file) = @_;

    my $stuff = sub(){
        # TODO: compute host ip
        my $host_ip = findMyIp();
        my $id = $data->{'id'};
        my $ip = $data->{'ip'};
        my $local_tmcd_port = nextPort();
        my $vars = <<EOF
host_ip=$host_ip
tmcd_port=7777
evproxy_port=16505
slothd_port=8509
local_tmcd_port=$local_tmcd_port
vhost_id='$id'
boss_emulab_net=155.98.32.70
fs_emulab_net=155.98.33.74
ops_emulab_net=ops.emulab.net
vif_ip=$ip
EOF
;

        my $rest = <<'EOF'
# xen's configuration for a vif
sh /etc/xen/scripts/vif-bridge $*

script_name=$(basename $0)
cleanup="/tmp/cleanup-$script_name"

do_offline(){
    sh $cleanup
    rm -f $cleanup
}

do_online(){
    echo "# " $(date) > $cleanup
    echo "# Cleanup script for $vif in vhost $vhost_id" > $cleanup
    # prevent dhcp requests from reaching eth0
    /sbin/iptables -A OUTPUT -j DROP -o $vif -m pkttype --pkt-type broadcast -m physdev --physdev-out $vif
    echo "/sbin/iptables -D OUTPUT -j DROP -o $vif -m pkttype --pkt-type broadcast -m physdev --physdev-out $vif" >> $cleanup

    # reroute tmcd calls to the proxy on the physical host
    /sbin/iptables -t nat -A PREROUTING -j DNAT -p tcp --dport $tmcd_port -d $boss_emulab_net -s $vif_ip --to-destination $host_ip:$local_tmcd_port
    /sbin/iptables -t nat -A PREROUTING -j DNAT -p udp --dport $tmcd_port -d $boss_emulab_net -s $vif_ip --to-destination $host_ip:$local_tmcd_port

    echo "/sbin/iptables -t nat -D PREROUTING -j DNAT -p tcp --dport $tmcd_port -d $boss_emulab_net -s $vif_ip --to-destination $host_ip:$local_tmcd_port" >> $cleanup
    echo "/sbin/iptables -t nat -D PREROUTING -j DNAT -p udp --dport $tmcd_port -d $boss_emulab_net -s $vif_ip --to-destination $host_ip:$local_tmcd_port" >> $cleanup

    # start a tmcc proxy for udp
    /usr/local/etc/emulab/tmcc-proxy -u -n $vhost_id -X $local_tmcd_port -s boss.emulab.net -p $tmcd_port &
    echo "kill -2 $!" >> $cleanup
    # and for tcp
    /usr/local/etc/emulab/tmcc-proxy -n $vhost_id -X $local_tmcd_port -s boss.emulab.net -p $tmcd_port &
    echo "kill -2 $!" >> $cleanup

    # slothd
    /sbin/iptables -t nat -A POSTROUTING -j SNAT -p udp --dport $slothd_port --to-source $host_ip -s 172.18.54.0/255.255.255.0 --destination $boss_emulab_net -o peth0
    echo "/sbin/iptables -t nat -D POSTROUTING -j SNAT -p udp --dport $slothd_port --to-source $host_ip -s 172.18.54.0/255.255.255.0 --destination $boss_emulab_net -o peth0" >> $cleanup

    # route mount points and evproxy server
    # todo: only forward ports the mount server needs (use rpcinfo on fs.emulab.net)
    /sbin/iptables -t nat -A POSTROUTING -j SNAT --to-source $host_ip -s 172.18.54.0/255.255.255.0 --destination $fs_emulab_net -o peth0
    echo "/sbin/iptables -t nat -D POSTROUTING -j SNAT --to-source $host_ip -s 172.18.54.0/255.255.255.0 --destination $fs_emulab_net -o peth0" >> $cleanup

    # reroute evproxy packets
    /sbin/iptables -t nat -D PREROUTING -j DNAT -p tcp --dport $evproxy_port -d ops.emulab.net -s $vif_ip --to-destination $host_ip:$evproxy_port
    echo "/sbin/iptables -t nat -D PREROUTING -j DNAT -p tcp --dport $evproxy_port -d ops.emulab.net -s $vif_ip --to-destination $host_ip:$evproxy_port" >> $cleanup
}

case "$1" in
    'online')
        do_online
    ;;
    'offline')
        do_offline
    ;;
esac

EOF
;

        return <<EOF
#!/bin/sh
$vars
$rest
EOF
;
    };

    open FILE, ">", "/etc/xen/scripts/$file" or die $!;
    print FILE &$stuff();
    close FILE;
    chmod 0555, "/etc/xen/scripts/$file";
}

sub vnodePreConfigControlNetwork($$$$$$$$$$){
    my ($id,$vmid,$ip,$mask,$mac,$gw,
            $vname,$longdomain,$shortdomain,$bossip) = @_;
    my $script_name = "emulab$mac";
    # print "mac is $mac\n";
    # print "id is $id\n";
    $stuff = {'id' => $id,
              'name' => $id,
              'ip' => $ip,
              'fqdn', => $longdomain,
              'disk' => diskName($id),
              'mac' => fixupMac($mac)};
    $xen_interfaces{$id} = {'control' => $stuff,
                            'script' => $script_name,
                            'links' => []};
    # createConfig(configFile($id), $stuff, $script_name);
    system("route add $ip dev control-net");
    createXenNetworkScript($stuff, $script_name);
    addDhcp($stuff);
    return 0;
}

sub vnodePreConfigExpNetwork($){
    my ($id, $vmid, $ifconfigs, $ldconfigs) = @_;
    my $links = $xen_interfaces{$id}->{'links'};

    # print "experimental configs " . Dumper($ifconfigs) . "\n";
    foreach my $interface (@$ifconfigs){
        # print "interface " . Dumper($interface) . "\n";
        my $link = {'mac' => $interface->{'MAC'},
                    'link' => $interface->{'LAN'},};
        push @$links, $link;
    }
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

sub createBridges($){
    my ($bridges) = @_;
    for (@$bridges){
        bringUpExperimentalBridge($_);
    }
}

sub findBridges($){
    my ($nodes) = @_;
    my $bridges = [];

    my $uniq = sub(){
        my ($in) = @_;
        my %saw;
        return [grep(!$saw{$_}++, @$in)];
    };

    foreach my $key (keys(%$nodes)){
        for (@{$nodes->{$key}->{'links'}}){
            push @$bridges, "exp-" . $_->{'link'};
        }
    }

    return &$uniq($bridges);
}

sub vnodeBoot($){
    my ($id) = @_;
    my $config = configFile($id);

    if (! -e $config){
        print "Cannot find xen configuration file for virtual instance $id\n";
        return 1;
    }
    mysystem("xm create $config");
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
    mysystem("/usr/sbin/xm destroy $id");
    my $file = configFile($id);
    mysystem("/usr/sbin/xm create $file");
}

sub vnodeExec {
    my ($vnode_id,$vmid,$command) = @_;

    return -1;
}

1

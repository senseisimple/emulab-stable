#!/etc/testbed/nse

# consults tmcc hostnames database to translate hostname to IP
# returns ip address of name
proc getipaddr {name} {

    set hostnamelist [split [exec tmcc hostnames] "\n"]
    foreach hostname $hostnamelist {
	scan $hostname "NAME=%s IP=%s ALIASES=\'%s\'" hname ip aliases
	set aliaslist [split $aliases " "]
	foreach alias $aliaslist {
	    if { $alias == $name } {
		return $ip
	    }
	}
    }
    puts stderr "NSE: Could not find ipaddress for $name"
    return ""
}


# consults tmcc ifconfig and findif to find the interface name
# returns the interface name for ipaddr
proc getmac {ipaddr} {

    set ifconfiglist [split [exec tmcc ifconfig] "\n"]
    foreach ifconfig $ifconfiglist {
	scan $ifconfig "INTERFACE=%s INET=%s MASK=%s MAC=%s " iface inet mask mac
	if { $inet == $ipaddr } {
	    return [exec findif $mac]
	}
    }

    puts stderr "NSE: Could not find the interface name for $ipaddr"
    return ""
}

set ns [new Simulator]
$ns use-scheduler RealTime

set n0 [$ns node]
set n1 [$ns node]

# required hack for now or FullTcp timer cancels cause aborts
$ns duplex-link $n0 $n1 100Mb 0ms DropTail

# assume PATH properly set by wrapper script
set nseconfig [exec tmcc nseconfigs]

# If there is no nseconfig associated with this
# node, then we just give up
if { $nseconfig == {} } {
   exit 0
}

eval $nseconfig

# set sysctl tcp blackhole to 2
exec sysctl -w net.inet.tcp.blackhole=2

lappend tcpclasses "Agent/TCP/FullTcp"
foreach tcpsubclass [Agent/TCP/FullTcp info subclass] {
    lappend tcpclasses $tcpsubclass
}

set objnamelist ""

foreach tcpclass $tcpclasses {
    set tcpobjs [$tcpclass info instances]

    foreach tcpobj $tcpobjs {
	$ns attach-agent $n0 $tcpobj
	lappend objnamelist [$tcpobj set objname]
    }
}

foreach ftpobj [Application/FTP info instances] {
    lappend objnamelist [$ftpobj set objname]
}

foreach telnetobj [Application/Telnet info instances] {
    lappend objnamelist [$telnetobj set objname]
}

# for each entry in `tmcc trafgens` that has NSE as the generator
# configure that object to connect to a newly created
# TCPTap along with the required Live and RAW IP objects. Set the filter and interface
# after learning it from tmcc commands and other scripts in /etc/testbed

set trafgenlist [split [exec tmcc trafgens] "\n"]
set formatstr {TRAFGEN=%s MYNAME=%s MYPORT=%u PEERNAME=%s PEERPORT=%u PROTO=%s ROLE=%s GENERATOR=%s}
foreach trafgen $trafgenlist {

    variable i 0

    scan $trafgen $formatstr trafgen myname myport peername peerport proto role gen
    if { $gen != "NSE" || $proto != "tcp" } {
	continue
    }
    

    # if the variable in $trafgen exists, we create a bunch of taps
    # first do the tap part
    if { [info exists $trafgen] == 1 } {

	# convert myname and peername to corresponding ipaddresses
	# using the getipaddr helper subroutine
	set myipaddr [getipaddr $myname]
	set peeripaddr [getipaddr $peername]

	# find interface name with a helper subroutine
	set interface [getmac $myipaddr]

	# one TCPTap object per TCP class that we have instantiated
	set tcptap($i) [new Agent/TCPTap]
	$tcptap($i) nsipaddr $myipaddr
	$tcptap($i) nsport $myport
	$tcptap($i) extipaddr $peeripaddr
	$tcptap($i) extport $peerport

	# open the bpf, set the filter for capturing incoming packets towards
	# the current tcp object
	set bpf($i) [new Network/Pcap/Live]
	set dev($i) [$bpf($i) open readonly $interface]
	$bpf($i) filter "tcp and dst $myipaddr and dst port $myport and src $peeripaddr and src port $peerport"

	# open RAW IP interface for outgoing packets
	set ipnet($i) [new Network/IP]
	$ipnet($i) open writeonly

	# associate the 2 network objects in the TCPTap object
	$tcptap($i) network-incoming $bpf($i)
	$tcptap($i) network-outgoing $ipnet($i)

	# attach the TCPTap agent to node n1
	$ns attach-agent $n1 $tcptap($i)

	# connect this tap and the particular tcp agent
	set tcpagent($i) [expr \$$trafgen]
	$ns connect $tcpagent($i) $tcptap($i)

	incr i
    }

}

# get some params to configure the event system interface
set boss [lindex [split [exec tmcc bossinfo] " "] 0]
set pideidlist [split [exec hostname] "."]
set vnode [lindex $pideidlist 0]
set eid [lindex $pideidlist 1]
set pid [lindex $pideidlist 2]
set logpath "/proj/$pid/exp/$eid/logs/nse-$vnode.log"

# Configuring the Scheduler to monitor the event system
set evsink [new TbEventSink]
$evsink event-server "elvin://$boss"
$evsink objnamelist [join $objnamelist ","]
$evsink logfile $logpath
[$ns set scheduler_] tbevent-sink $evsink

$ns run

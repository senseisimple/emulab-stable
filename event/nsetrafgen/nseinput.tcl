#!/etc/testbed/nse

# consults tmcc hostnames database to translate hostname to IP
# returns ip address of name
proc getipaddr {name} {

    set resolver [new TbResolver]
    set ip [$resolver lookup $name]
    delete $resolver
    if { $ip != {} } {
	return $ip	
    }

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

    if { [catch {set ifconf [exec tmcc ifconfig]} errMsg] == 1 } {
	puts stderr "NSE: tmcc ifconfig: $errMsg"
	exit 1
    }
    set ifconfiglist [split $ifconf "\n"]
    foreach ifconfig $ifconfiglist {
	scan $ifconfig "INTERFACE=%s INET=%s MASK=%s MAC=%s " iface inet mask mac
	if { $inet == $ipaddr } {
	    return [exec findif $mac]
	}
    }

    puts stderr "NSE: Could not find the interface name for $ipaddr"
    return ""
}

# assume PATH properly set by wrapper script
if { [catch {set nseconfig [exec tmcc nseconfigs]} errMsg] == 1 } {
    puts stderr "NSE: tmcc nseconfigs: $errMsg"
    exit 2
}

# If there is no nseconfig associated with this
# node, then we just give up
if { $nseconfig == {} } {
   exit 0
}

set nsetrafgen_present 0
set simcode_present 0

# since we ran the original script through NSE (without running the simulation),
# we can ignore all sorts of errors
if { [catch {eval $nseconfig} errMsg] == 1 } {
    puts stderr "NSE: syntax error evaluating script: $errMsg"
}

# the name of the simulator instance variable might not
# always be 'ns', coming from the TB parser
set ns [Simulator instance]
# configuring NSE FullTcp traffic generators
set objnamelist ""
if { $nsetrafgen_present == 1 } {

    # The following nodes are present 
    set n0_FullTcp [$ns node]
    set n1_FullTcp [$ns node]

    # required hack for now or FullTcp timer cancels cause aborts
    # otherwise, we just need 1 node
    $ns duplex-link $n0_FullTcp $n1_FullTcp 100Mb 0ms DropTail

    # set sysctl tcp blackhole to 2
    exec sysctl -w net.inet.tcp.blackhole=2

    lappend tcpclasses "Agent/TCP/FullTcp"
    foreach tcpsubclass [Agent/TCP/FullTcp info subclass] {
	lappend tcpclasses $tcpsubclass
    }
    
    
    foreach tcpclass $tcpclasses {
	set tcpobjs [$tcpclass info instances]
	
	foreach tcpobj $tcpobjs {
	    
	    # objname is a instance variable that was put
	    # by TB parser on objects instantiated by it.
	    # we are concerned only with those. the rest
	    # of the FullTcp objects could be created as
	    # as a result of a combined simulation and
	    # emulation scenario
	    if { [$tcpobj set objname] != {} } {
		$ns attach-agent $n0_FullTcp $tcpobj
		lappend objnamelist [$tcpobj set objname]
	    }
	}
    }
    
    foreach ftpobj [Application/FTP info instances] {
	# objname is a instance variable that was put
	# by TB parser on objects instantiated by it.
	# we are concerned only with those. the rest
	# of the FullTcp objects could be created as
	# as a result of a combined simulation and
	# emulation scenario
	if { [$ftpobj set objname] != {} } {
	    lappend objnamelist [$ftpobj set objname]
	}
    }
    
    foreach telnetobj [Application/Telnet info instances] {
	# objname is a instance variable that was put
	# by TB parser on objects instantiated by it.
	# we are concerned only with those. the rest
	# of the FullTcp objects could be created as
	# as a result of a combined simulation and
	# emulation scenario
	if { [$telnetobj set objname] != {} } {
	    lappend objnamelist [$telnetobj set objname]
	}
    }
}

# for each entry in `tmcc trafgens` that has NSE as the generator
# configure that object to connect to a newly created
# TCPTap along with the required Live and RAW IP objects. Set the filter and interface
# after learning it from tmcc commands and other scripts in /etc/testbed

# we only need 1 RAW IP socket for introducing packets into the network
set ipnet [new Network/IP]
$ipnet open writeonly

# configuring NSE FullTcp traffic generators
if { [catch {set tmcctrafgens [exec tmcc trafgens]} errMsg] == 1 } {
    puts stderr "NSE: tmcc trafgens: $errMsg"
    exit 3
}
set trafgenlist [split $tmcctrafgens "\n"]
set formatstr {TRAFGEN=%s MYNAME=%s MYPORT=%u PEERNAME=%s PEERPORT=%u PROTO=%s ROLE=%s GENERATOR=%s}
variable i 0
foreach trafgen $trafgenlist {

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
	set bpf_tcp($i) [new Network/Pcap/Live]
	set dev_tcp($i) [$bpf_tcp($i) open readonly $interface]
	$bpf_tcp($i) filter "tcp and dst $myipaddr and dst port $myport and src $peeripaddr and src port $peerport"

	# associate the 2 network objects in the TCPTap object
	$tcptap($i) network-incoming $bpf_tcp($i)
	$tcptap($i) network-outgoing $ipnet

	# attach the TCPTap agent to node n1
	$ns attach-agent $n1 $tcptap($i)

	# connect this tap and the particular tcp agent
	set tcpagent($i) [expr \$$trafgen]
	$ns connect $tcpagent($i) $tcptap($i)

	incr i
    }

}

if { $simcode_present == 1 } {

    exec sysctl -w net.inet.ip.forwarding=0 net.inet.ip.fastforwarding=0
    
    # Now, we configure IPTaps for links between real and simulated nodes
    set nodelist [Node info instances]
    set i 0    
    
    foreach nodeinst [Node info instances] {
	if { [$nodeinst info vars nsenode_ipaddr] == {} } {
	    continue
	}
	set nodeinst_ipaddr [$nodeinst set nsenode_ipaddr]
	if { $nodeinst_ipaddr != {} } {
	    set iface [getmac $nodeinst_ipaddr]
	    
	    # one iptap per node that has real - simulated link
	    set iptap($i) [new Agent/IPTap]
	    
	    # open the bpf, set the filter for capturing incoming ip packets
	    # except for the current host itself
	    set bpf_ip($i) [new Network/Pcap/Live]
	    set dev_ip($i) [$bpf_ip($i) open readonly $iface]
	    $bpf_ip($i) filter "ip and not dst host $nodeinst_ipaddr"
	    
	    # associate the 2 network objects in the IPTap object
	    $iptap($i) network-outgoing $ipnet
	    $iptap($i) network-incoming $bpf_ip($i)
	    
	    $ns attach-agent $nodeinst $iptap($i)
	    
	    # We do not connect iptaps coz they figure out their
	    # destination node address dynamically using routing tables
	    
	    incr i
	}
    }
}

# get some params to configure the event system interface
#set boss [lindex [split [exec tmcc bossinfo] " "] 0]
#set pideidlist [split [exec hostname] "."]
#set vnode [lindex $pideidlist 0]
#set eid [lindex $pideidlist 1]
#set pid [lindex $pideidlist 2]
#set logpath "/proj/$pid/exp/$eid/logs/nse-$vnode.log"

#if { $objnamelist != {} } {
    # Configuring the Scheduler to monitor the event system
#    set evsink [new TbEventSink]
#    $evsink event-server "elvin://$boss"
#    $evsink objnamelist [join $objnamelist ","]
#    $evsink logfile $logpath
#    [$ns set scheduler_] tbevent-sink $evsink
#}

$ns run

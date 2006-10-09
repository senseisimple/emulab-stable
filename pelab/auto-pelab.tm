source tb_compat.tcl

set ns [new Simulator]
$ns rtproto Static

##########
# Beginning of user-settable parameters

#
# If set to 1, doesn't allocate any real Plab nodes (or fake ones either).
# Attributes for the elab-* machines are taken from DB data for nodes in
# the plabnodes list.
#
# This option requires that you have nodes listed in plabnodes and forces
# use of the dbmonitor.
#
$ns define-template-parameter NO_PLAB 0 \
{If non-zero, do not allocate any real PlanetLab nodes\
 rather initialize elab characteristics from nodes in the $plabnodes list. }

#
# This control how many _pairs_ of PCs there are. ie. 2 gets you 2 monitors,
# plus matching stubs
#
$ns define-template-parameter NUM_PCS 3 \
{The number of *pairs* of nodes in the experiment.\
 Example: '2' would allocate 2 Emulab nodes in the 'elabc' cloud,\
 along with either or both of 2 Plab nodes and 2 Emulab nodes\
 in the 'plabc' cloud. }

#
# If set to 1, we create a fake PlanetLab inside Emulab
#
$ns define-template-parameter FAKE_PLAB 0 \
{If non-zero, allocate a cloud of Emulab machines to emulate Plab nodes. }

#
# If set to 1, we grab real PlanetLab nodes. Both this and FAKE_PLAB can be
# set at the same time
#
$ns define-template-parameter REAL_PLAB 1 \
{If non-zero, use real PlanetLab nodes.\
 NOTE: can be combined with FAKE_PLAB. }

#
# Run FreeBSD 6.1 on elab_ nodes.  Forces use of dbmonitor.
#
$ns define-template-parameter DO_BSD 0 \
{If non-zero, run FreeBSD 6.1 instead of Fedora Core 2 on elab nodes.\
 NOTE: Forces use of dbmonitor. }

#
# If set to 1, we trace all links (real/fake plab, elab) at the delay nodes.
#
$ns define-template-parameter TRACE 1 \
{If non-zero, run Emulab link tracing on all links\
 (real and fake plab, elab). }

#
# If set to 1, we trace on all end nodes instead of (or in addition to)
# all delay nodes above.
#
# XXX this option won't do anything right now.  If there are delay nodes
# involved for local LAN/clouds it is ignored, and as long as tracing is
# enabled, plab nodes will get end_node tracing anyway.
#
$ns define-template-parameter TRACE_END_NODES 0 \
{If non-zero, perform link tracing on the end nodes instead of\
 (or in addition to) on the delay nodes.\
  NOTE: This option doesn't do anything right now. }

#
# If you want to get specific real planetlab nodes, ask for them here by site
# or by emulab node ID. 
# NOTES:
#   An empty list for either selection method turns it off
#   Do not use both site and node selection at the same time!
#   The list must be at least as long as NUM_PCS above, It can be longer, but
#     extra elements will be ignored
#   I recommend using the sites option, rather than the nodes option, since that
#     may get you less-loaded nodes and is more robust to crashed nodes
#   You can get a list of node IDs and sites at:
#     https://www.emulab.net/nodecontrol_list.php3?showtype=widearea
#   Site names are case-sensitive!
#

# Empty list
$ns define-template-parameter PLABSITES {} \
{List of specific PlanetLab sites to use for node selection.\
 Leave empty to let the system choose.\
 NOTE: do not use both plabnodes and PLABSITES at the same time.\
 NOTE: You can get a list of node IDs and sites at:\
     https://www.emulab.net/nodecontrol_list.php3?showtype=widearea\
 NOTE: site names are case-sensitive. }
# Example site list
#set PLABSITES {UCB Stanford Cornell Jerusalem Taiwan}

# Empty list
$ns define-template-parameter PLABNODES {} \
{List of specific PlanetLab nodes to use.\
 Leave empty to let the system choose.\
 NOTE: must specify at least NUM_PCS nodes in the list if NO_PLAB is non-zero.\
 NOTE: do not use both plabnodes and PLABSITES at the same time.\
 NOTE: You can get a list of node IDs and sites at:\
     https://www.emulab.net/nodecontrol_list.php3?showtype=widearea }
# Example node list
#set PLABNODES {plab518 plab541 plab628 plab736 plab360}

#
# Where to grab your tarball of pelab software from. To make this tarball:
#   Go to your testbed source tree
#   Run 'cvs up'!!!
#   Run 'gmake' in pelab/stub and pelab/libnetmon (on a DEVBOX node)
#   From the root of your source tree, run:
#       tar czvf /proj/tbres/my-pelab.tar.gz pelab
#   Of course, don't name it 'my-plab.tar.gz'! You can put this file in
#       a subdirectory if you want, but it must be somewhere in
#       /proj/tbres/
#   Put the path to your tarball in this variable
#
$ns define-template-parameter PELAB_TAR "/proj/tbres/tarfiles/pelab.tar.gz" \
{Tarball of pelab software.\
 NOTE: To make this tarball:\
   (1) Go to your testbed source tree,\
   (2) Run 'cvs up',\
   (3) Run 'gmake' in pelab/magent, pelab/stub and pelab/libnetmon\
       (on a DEVBOX node)\
 From the root of your source tree, run:\
   tar czvf /proj/tbres/my-pelab.tar.gz pelab\
 Of course, don't name it 'my-plab.tar.gz'. You can put this file in\
    a subdirectory if you want, but it must be somewhere in /proj/tbres/\
 Put the path to your tarball in this variable. }
#set PELAB_TAR "/proj/tbres/mike/pelab-bin.tar.gz"


#
# When using a fake plab, these are the parameters for the fake Internet
# 'cloud' connecting the plab nodes. For now, all nodes have the same
# parameters - hopefully this will change in a later version
#
$ns define-template-parameter CLOUD_DELAY "30ms" \
{Initial one-way delay for the fake plab ('plabc') cloud.\
 NOTE: only used if fake_plab is non-zero. }
$ns define-template-parameter CLOUD_BW "1.5Mbps" \
{Initial bandwidth for the fake plab ('plabc') cloud.\
 NOTE: only used if fake_plab is non-zero. }

#
# When using a fake plab, these are the parameters for the 'control'
# delay (ie. the latency for the elab nodes to reach the plab nodes
#
$ns define-template-parameter CONTROL_DELAY "0ms" \
{Delay for control network between fake Plab and elab nodes.\
 NOTE: only used if fake_plab is non-zero. }
$ns define-template-parameter CONTROL_BW "100Mbps" \
{Bandwidth for control network between fake Plab and elab nodes.\
 NOTE: only used if fake_plab is non-zero. }

#
# These are the initial conditions for the 'elabc' cloud, the Emulab side of
# a pelab experiment
#
$ns define-template-parameter ECLOUD_DELAY "0ms" \
{Initial one-way delay for the 'elabc' cloud. }
$ns define-template-parameter ECLOUD_BW "100Mbps" \
{Initial bandwidth for the 'elabc' cloud. }

#
# Hardare type to use for PCs inside of emulab
#
$ns define-template-parameter HWTYPE "pc" \
{Emulab node type to use for nodes allocated in both\
 'elabc' and 'plabc' clouds. }

#
# Server and client to use for automated testing. If set, will automatically
# be started by the 'start-experiment' script
#
if {$DO_BSD} {
    set sstr "/usr/local/etc/emulab/emulab-iperf -s "
    # NOTE: No client support for now, you'll have to run the client yourself
    set cstr "/usr/local/etc/emulab/emulab-iperf -t 60 -c "
} else {
    set sstr "/usr/bin/iperf -s "
    # NOTE: No client support for now, you'll have to run the client yourself
    set cstr "/usr/bin/iperf -t 60 -c "
}
$ns define-template-parameter SERVERPROG $sstr \
{Server program to run automatically on all elab nodes. }
$ns define-template-parameter CLIENTPROG $cstr \
{Client to run automatically on Plab nodes.\
 NOTE: not supported currently. }

#
# If non-zero, uses the new stub (magent) instead of the old one
#
$ns define-template-parameter USE_MAGENT 1 \
{If non-zero, uses the new stub (magent)\
 instead of the old one on plab nodes. }

#
# If non-zero, uses the DB-based "monitor" to control the cloud shaping
#
set val 0
if {$DO_BSD} {
    set val 1
}
$ns define-template-parameter USE_DBMONITOR $val \
{If non-zero, uses the DB-based "monitor"\
 (rather than the application monitor) to control the cloud shaping. }

#
# If dbmonitor is set, this is the interval in seconds at which to poll the DB
# and potentially update the shaping characteristics
#
$ns define-template-parameter DBMONITOR_INTERVAL 10 \
{If USE_DBMONITOR is non-zero, the interval in seconds at which to poll\
 the DB to check for path characteristic changes. }

#
# If non-zero, limits the number of slots in the queues for the fake PlanetLab
#
$ns define-template-parameter LIMIT_FAKE_QUEUE_SLOTS 0 \
{If non-zero, limits the size of the dummynet queues used\
 for the fake Plab cloud to the given value.\
 NOTE: default queue size is 50 packets. }

#
# Use this to set a unique port so that you don't collide with others on the
# same node. Only supported when USE_MAGENT is 1
#
$ns define-template-parameter STUB_PORT 3149 \
{TCP port number to use for the new stub (magent).\
 Change to avoid collisions with others on the same node.\
 NOTE: only used when USE_MAGENT is non-zero. }

#
# Enforce NO_PLAB requirements
#
if {$NO_PLAB} {
    set FAKE_PLAB 0
    set REAL_PLAB 0
    set USE_DBMONITOR 1
}

# End of user-settable options
##########

set pid ${GLOBALS::pid}
set eid ${GLOBALS::eid}

set delay_os FBSD54-DNODE

if {$DO_BSD} {
    set node_os FBSD61-STD
} else {
    # XXX -UPDATE for now.  Contains bug fixes (progagent command lines).
    set node_os PLAB-DEVBOX-UPDATE
    #set node_os PLAB-DEVBOX
}

tb-set-delay-os $delay_os

set stub_peer_port $STUB_PORT
set stub_command_port [expr $STUB_PORT + 1]

#
# Tarballs and RPMs we install on all nodes
#
set tarfiles "/local $PELAB_TAR"
set plab_rpms "/proj/tbres/auto-pelab/libpcap-0.8.3-3.i386.rpm /proj/tbres/auto-pelab/iperf-2.0.2-1.1.fc2.rf.i386.rpm"
if {$DO_BSD} {
    set elab_rpms ""
} else {
    set elab_rpms "/proj/tbres/auto-pelab/libpcap-0.8.3-3.i386.rpm /proj/tbres/auto-pelab/iperf-2.0.2-1.1.fc2.rf.i386.rpm"
}

if {$DO_BSD} {
    set elabshell "/bin/sh -T"
} else {
    set elabshell "/bin/sh"
}

if {$USE_MAGENT} {
    set stubcommand "/bin/sh /local/pelab/magent/auto-magent.sh --peerserverport=$stub_peer_port --monitorserverport=$stub_command_port"
} else {
    set stubcommand "/bin/sh /local/pelab/stub/auto-stub.sh"
}

# we don't run dbmonitor on the nodes anymore
if {0 && $USE_DBMONITOR} {
    set moncommand "$elabshell /local/pelab/dbmonitor/auto-dbmonitor.sh"
} else {
    set moncommand "$elabshell /local/pelab/monitor/auto-monitor.sh --stub-port=$stub_command_port"
}

#
# So far, nothing I tried can get the auto-* scripts to return 0
# when interrupted
#
set ecode 15

set elan_string ""
set plan_string ""
set inet_string ""

set stublist {}
set planetstublist {}
set plabstublist {}
set monitorlist {}
set planetservers {}
set serverlist {}
set clientlist {}
set tflist {}

#
# Create all of the nodes
#
set tfix 1
for {set i 1} {$i <= $NUM_PCS} {incr i} {

    if {$REAL_PLAB} {
        set planet($i) [$ns node]
        tb-set-hardware $planet($i) pcplab
        append inet_string "$planet(${i}) "
        set planetstub($i) [$planet($i) program-agent -expected-exit-code $ecode -command $stubcommand]
        lappend stublist $planetstub($i)
        lappend planetstublist $planetstub($i)

        tb-set-node-tarfiles $planet($i) $tarfiles
        tb-set-node-rpms $planet($i) $plab_rpms
        set tfupdate($tfix) [$planet($i) program-agent -command "sudo /usr/local/etc/emulab/update -it"]
	lappend tflist $tfupdate($tfix)
	incr tfix

        if {[llength $PLABSITES] > 0} {
            set why_doesnt_tcl_have_concat "*&"
            append why_doesnt_tcl_have_concat [lindex $PLABSITES [expr $i - 1]]
            $planet($i) add-desire $why_doesnt_tcl_have_concat 1.0
        } elseif {[llength $PLABNODES] > 0} {
            tb-fix-node $planet($i) [lindex $PLABNODES [expr $i - 1]]
        }
    }

    if {$FAKE_PLAB} {
        set plab($i) [$ns node]
        tb-set-node-os $plab($i) $node_os
        tb-set-hardware $plab($i) $HWTYPE
        append plan_string "$plab(${i}) "
        set plabstub($i) [$plab($i) program-agent -expected-exit-code $ecode -command $stubcommand]
        lappend stublist $plabstub($i)
        lappend plabstublist $plabstub($i)

        tb-set-node-tarfiles $plab($i) $tarfiles
        tb-set-node-rpms $plab($i) $plab_rpms
        set tfupdate($tfix) [$plab($i) program-agent -command "sudo /usr/local/etc/emulab/update -it"]
	lappend tflist $tfupdate($tfix)
	incr tfix
    }

    set elab($i) [$ns node]
    tb-set-node-os $elab($i) $node_os
    tb-set-hardware $elab($i) $HWTYPE
    append elan_string "$elab(${i}) "
    set monitor($i) [$elab($i) program-agent -expected-exit-code $ecode -command $moncommand]
    lappend monitorlist $monitor($i)

    set server($i) [$elab($i) program-agent -expected-exit-code $ecode -command $SERVERPROG]
    set client($i) [$elab($i) program-agent -expected-exit-code $ecode -command $CLIENTPROG]
    lappend serverlist $server($i)
    lappend clientlist $client($i)

    tb-set-node-tarfiles $elab($i) $tarfiles
    if {$elab_rpms != ""} {
        tb-set-node-rpms $elab($i) $elab_rpms
    }
    set tfupdate($tfix) [$elab($i) program-agent -command "sudo /usr/local/etc/emulab/update -it"]
    lappend tflist $tfupdate($tfix)
    incr tfix

    #
    # If the user has given a PLABNODES list, we set up $opt variables so that
    # even if they are not actually using PlanetLab nodes, the simple model can
    # still see the mapping between elab and plab nodes
    #
    if {[llength $PLABNODES] > 0} {
        set opt(pelab-elab-$i-mapping) [lindex $PLABNODES [expr $i - 1]]
    }
}

#
# Run the DB monitor on ops
#
if {$USE_DBMONITOR} {
    set dbmonitor [new Program $ns]
    $dbmonitor set node "ops"
    $dbmonitor set command "/usr/testbed/sbin/dbmonitor.pl -i $DBMONITOR_INTERVAL $pid $eid"
    set monitorlist $dbmonitor
}

#
# Set up groups to make it easy for us to start/stop program agents
#
if {$stublist != {}} {
    set stubgroup [$ns event-group $stublist]
    if {$REAL_PLAB} {
        set planetstubs [$ns event-group $planetstublist]
    }
    if {$FAKE_PLAB} {
        set plabstubs [$ns event-group $plabstublist]
    }
}
set monitorgroup [$ns event-group $monitorlist]

set allservers [$ns event-group $serverlist]
set allclients [$ns event-group $clientlist]

set tfhosts [$ns event-group $tflist]

set tracelist {}

#
# Real Internet cloud for real plab nodes
#
if {$REAL_PLAB} {
    set realinternet [$ns make-lan "$inet_string" 100Mbps 0ms]
    tb-set-lan-protocol $realinternet ipv4
    if {$TRACE} {
	$realinternet trace header "tcp and port $stub_peer_port"
	if {$TRACE_END_NODES} {
	    $realinternet trace_endnode 1
	}
	lappend tracelist $realinternet
    }
}

#
# Fake 'Internet' cloud for fake plab nodes
#
if {$FAKE_PLAB} {
    set plabc [$ns make-cloud "$plan_string" $CLOUD_BW $CLOUD_DELAY]
    tb-set-ip-lan $plab(1) $plabc 10.1.0.1
    if {$TRACE} {
	$plabc trace
	if {$TRACE_END_NODES} {
	    $plabc trace_endnode 1
	}
	lappend tracelist $plabc
    }
    if {$LIMIT_FAKE_QUEUE_SLOTS} {
        for {set i 1} {$i <= $NUM_PCS} {incr i} {
            set fakequeues($i) [[$ns lanlink $plabc $plab($i)] queue]
            $fakequeues($i) set limit_ $LIMIT_FAKE_QUEUE_SLOTS
        }
    }
}

#
# Lan which will be controlled by the monitor
#
set elabc [$ns make-cloud "$elan_string" $ECLOUD_BW $ECLOUD_DELAY]
tb-set-ip-lan $elab(1) $elabc 10.0.0.1
if {$TRACE} {
    $elabc trace
    if {$TRACE_END_NODES} {
	$elabc trace_endnode 1
    }
    lappend tracelist $elabc
}

#
# We don't want the sync server to end up out there on some plab node
#
tb-set-sync-server $elab(1)

#
# Set up a fake Internet link between the PlanetLab and Emulab sides
# when using fake plab nodes
#
if {$FAKE_PLAB} {
    set erouter [$ns node]
    set prouter [$ns node]

    set elabcontrol [$ns make-lan "$elan_string $erouter" 100Mbps 0ms]
    set plabcontrol [$ns make-lan "$plan_string $prouter" 100Mbps 0ms]

    set internet [$ns duplex-link $erouter $prouter $CONTROL_BW $CONTROL_DELAY DropTail]
    if {$TRACE} {
	$internet trace
	if {$TRACE_END_NODES} {
	    $internet trace_endnode 1
	}
	lappend tracelist $internet
    }

    tb-set-ip-lan $elab(1) $elabcontrol 192.168.0.1
    tb-set-ip-lan $plab(1) $plabcontrol 192.168.1.1

    tb-set-ip-link $erouter $internet 192.168.254.1
    tb-set-ip-link $prouter $internet 192.168.254.2
}

# for one-off ops commands
set opsagent [new Program $ns]
$opsagent set node "ops"
$opsagent set command "/bin/true"

#
# The set of traced links
# XXX still cannot use an event-group of link tracing events in a sequence
#
#set tracegroup [$ns event-group $tracelist]

#
# Build up the event sequences to start and stop an actual run.
#
if {$NO_PLAB || $REAL_PLAB} {
  set start [$ns event-sequence]
    $start append "$ns log \"Starting REAL plab experiment\""

    # stop stubs and monitors
    $start append "$ns log \"##### Stopping stubs and monitors...\""
    if {$REAL_PLAB} {
	$start append "$planetstubs stop"
    }
    $start append "$monitorgroup stop"

    # stop servers
    $start append "$ns log \"##### Stopping servers...\""
    $start append "$allservers stop"

    # stop link logging
    $start append "$ns log \"##### Roll link trace logs...\""
    foreach link $tracelist {
        $start append "$link trace snapshot"
        $start append "$link trace stop"
    }

    # clean out log files
    # XXX original script passed --root, cleanlogs does not--may be a problem.
    $start append "$ns log \"##### Cleaning logs...\""
    $start append "$ns cleanlogs"

    # reset shaping characteristics for all nodes
    $start append "$ns log \"##### Resetting links...\""
    $start append "$elabc clear"
    $start append "$elabc reset"

    if {$REAL_PLAB} {
	# distinguish between real/fake runs
	# XXX I'm thinkin...we can do better than this!
	$start append "$opsagent run -command \"cp /dev/null /proj/$pid/exp/$eid/tmp/real_plab\""

	# save off node list
	$start append "$ns log \"##### Creating node list...\""
	$start append "$opsagent run -command \"/usr/testbed/bin/node_list -m -e $pid,$eid > /proj/$pid/exp/$eid/tmp/node_list\""

	# initialize path characteristics
	$start append "$ns log \"##### Initialize emulation node path characteristics...\""
	$start append "$elabc create"
	$start append "$opsagent run -command \"/usr/testbed/bin/init-elabnodes.pl  -o /proj/$pid/exp/$eid/tmp/initial-conditions.txt $pid $eid\""
    } else {
	$start append "$elabc create"
    }

    # restart link tracing
    # XXX cleanlogs has unlinked the current trace log, so we have to
    # snapshot to bring it back into existence
    $start append "$ns log \"##### Starting link tracing...\""
    foreach link $tracelist {
        $start append "$link trace snapshot"
        $start append "$link trace start"
    }

    # restart servers
    $start append "$ns log \"##### Starting server...\""
    $start append "$allservers start"

    # restart stubs and monitors
    $start append "$ns log \"##### Starting stubs and monitors...\""
    if {$REAL_PLAB} {
	$start append "$planetstubs start"
    }
    $start append "$monitorgroup start"

    # gather up the data and inform the user
    $start append "$ns log \"##### Experiment run started!\""
    # XXX cannot do a report here as that will cause the logs to be
    #     deleted before the next loghole sync

  set stop [$ns event-sequence]
    $stop append "$ns log \"Stopping REAL plab experiment\""

    # stop stubs and monitors
    $stop append "$ns log \"##### Stopping stubs and monitors...\""
    if {$REAL_PLAB} {
	$stop append "$planetstubs stop"
    }
    $stop append "$monitorgroup stop"

    # stop servers
    $stop append "$ns log \"##### Stopping servers...\""
    $stop append "$allservers stop"

    # stop link logging and save logs
    $stop append "$ns log \"##### Stop link tracing...\""
    foreach link $tracelist {
        $stop append "$link trace snapshot"
        $stop append "$link trace stop"
    }

    # reset shaping characteristics for all nodes
    $stop append "$ns log \"##### Resetting links...\""
    $stop append "$elabc clear"
    $stop append "$elabc reset"

    # gather up the data and inform the user
    $stop append "$ns log \"##### Experiment run stopped!\""
    $stop append "$ns report"
}

#
# Build up the event sequences to start and stop an actual run on FAKE
# plab nodes.  The only difference here (besides the names) is that these
# sequences use plabstubs rather than planetstubs.
#
if {$FAKE_PLAB} {
  set start_fake [$ns event-sequence]
    $start_fake append "$ns log \"Starting FAKE plab experiment\""

    # stop stubs and monitors
    $start_fake append "$ns log \"##### Stopping stubs and monitors...\""
    $start_fake append "$plabstubs stop"
    $start_fake append "$monitorgroup stop"

    # stop servers
    $start_fake append "$ns log \"##### Stopping servers...\""
    $start_fake append "$allservers stop"

    # stop link logging
    $start_fake append "$ns log \"##### Roll link trace logs...\""
    foreach link $tracelist {
        $start_fake append "$link trace snapshot"
        $start_fake append "$link trace stop"
    }

    # clean out log files
    # XXX original script passed --root, cleanlogs does not--may be a problem.
    $start_fake append "$ns log \"##### Cleaning logs...\""
    $start_fake append "$ns cleanlogs"

    # reset shaping characteristics for all nodes
    $start_fake append "$ns log \"##### Resetting links...\""
    $start_fake append "$elabc clear"
    $start_fake append "$elabc reset"

    # distinguish between real/fake runs
    # XXX I'm thinkin...we can do better than this!
    $start_fake append "$opsagent run -command \"rm -f /proj/$pid/exp/$eid/tmp/real_plab\""

    # save off node list
    $start_fake append "$ns log \"##### Creating node list...\""
    $start_fake append "$opsagent run -command \"/usr/testbed/bin/node_list -m -e $pid,$eid > /proj/$pid/exp/$eid/tmp/node_list\""

    # restart link tracing
    # XXX cleanlogs has unlinked the current trace log, so we have to
    # snapshot to bring it back into existence
    $start_fake append "$ns log \"##### Starting link tracing...\""
    foreach link $tracelist {
        $start_fake append "$link trace snapshot"
        $start_fake append "$link trace start"
    }

    # restart servers
    $start_fake append "$ns log \"##### Starting server...\""
    $start_fake append "$allservers start"

    # restart stubs and monitors
    $start_fake append "$ns log \"##### Starting stubs and monitors...\""
    $start_fake append "$plabstubs start"
    $start_fake append "$monitorgroup start"

    # gather up the data and inform the user
    $start_fake append "$ns log \"##### Experiment run started!\""
    # XXX cannot do a report here as that will cause the logs to be
    #     deleted before the next loghole sync

  set stop_fake [$ns event-sequence]
    $stop_fake append "$ns log \"Stopping FAKE plab experiment\""

    # stop stubs and monitors
    $stop_fake append "$ns log \"##### Stopping stubs and monitors...\""
    $stop_fake append "$plabstubs stop"
    $stop_fake append "$monitorgroup stop"

    # stop servers
    $stop_fake append "$ns log \"##### Stopping servers...\""
    $stop_fake append "$allservers stop"

    # stop link logging and save logs
    $stop_fake append "$ns log \"##### Stop link tracing...\""
    foreach link $tracelist {
        $stop_fake append "$link trace snapshot"
        $stop_fake append "$link trace stop"
    }

    # reset shaping characteristics for all nodes
    $stop_fake append "$ns log \"##### Resetting links...\""
    $stop_fake append "$elabc clear"
    $stop_fake append "$elabc reset"

    # gather up the data and inform the user
    $stop_fake append "$ns log \"##### Experiment run stopped!\""
    $stop_fake append "$ns report"
}

#
# Schedule sequences for start/stop run.
#
# For starting a run, the event system is restarted so we just need
# to have an "at 0" event to trigger it.
#
# For stopping a run, we don't need anything special.  The default action
# of stopping all program agents, snapshotting link tracing, and collecting
# logfiles is sufficient.  The only other action we used to do was to reset
# link shaping on the elabc cloud with the "clear" and "reset" events.
# However, we do that on a start run, so it is not strictly necessary to
# do it on stop.
#
if {$NO_PLAB || $REAL_PLAB} {
    $ns at 0 "start run"
} elseif {$FAKE_PLAB} {
    $ns at 0 "start_fake run"
}

$ns run

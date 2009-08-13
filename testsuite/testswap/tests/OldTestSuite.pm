package OldTestSuite;

our $tests = {
          'cbr' => {
                     'info' => 'Test UDP and a TCP agent/CBR. Also throw in some events to start/stop
the trafgen.
',
                     'nsfile' => 'source tb_compat.tcl
set ns [new Simulator]

#Create four nodes
set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

# Turn on routing.
$ns rtproto Session

#Create links between the nodes
set link0 [$ns duplex-link $n0 $n2 100Mb 2ms DropTail]
set link1 [$ns duplex-link $n1 $n2 100Mb 2ms DropTail]
set link2 [$ns duplex-link $n3 $n2 100Mb 2ms DropTail]

#Create a UDP agent and attach it to node n0
set udp0 [new Agent/UDP]
#$udp0 set class_ 1
$ns attach-agent $n0 $udp0

# Create a CBR traffic source and attach it to udp0
set cbr0 [new Application/Traffic/CBR]
$cbr0 set packetSize_ 500
$cbr0 set interval_ 0.005
$cbr0 attach-agent $udp0

#Create a TCP agent and attach it to node n1
set tcp0 [new Agent/TCP]
$ns attach-agent $n1 $tcp0

# Create a CBR traffic source and attach it to tcp0
set cbr1 [new Application/Traffic/CBR]
$cbr1 set packetSize_ 500
$cbr1 set interval_ 0.005
$cbr1 attach-agent $tcp0

#Create a Null agent (a traffic sink) and attach it to node n3
set null0 [new Agent/Null]
$ns attach-agent $n3 $null0

#Create a TCPSink agent (a traffic sink) and attach it to node n3
set null1 [new Agent/TCPSink]
$ns attach-agent $n3 $null1

#Connect the traffic sources with the traffic sink
$ns connect $udp0 $null0  
$ns connect $tcp0 $null1

# Create a program object and attach it to nodeA
set prog0 [new Program $ns]
$prog0 set node $n3
$prog0 set command "/bin/ls -lt >& /tmp/foo"

set prog1 [new Program $ns]
$prog1 set node $n3
$prog1 set command "/bin/sleep 60 >& /tmp/bar"

$ns at 1.0  "$cbr0 start"
$ns at 2.0  "$link0 down"
$ns at 4.0  "$link0 up"
$ns at 5.0  "$cbr0 stop"

$ns at 10.0 "$cbr1 start"
$ns at 11.0 "$cbr1 stop"

$ns at 20.0 "$prog0 start"
$ns at 21.0 "$prog0 stop"
$ns at 22.0 "$prog1 start"
$ns at 23.0 "$prog1 stop"

$ns at 30.0  "$link1 down"
$ns at 35.0  "$link1 up"

#Run the simulation
$ns run
',
                     'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                   },
          'lan1' => {
                      'info' => 'Two undelayed lans, one of three and one of four.  node3 is shared by both 
LANs.
',
                      'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]
set node3 [$ns node]
set node4 [$ns node]

set lan3 [$ns make-lan "$node1 $node2 $node3" 100Mb 0ms]
set lan2 [$ns make-lan "$node4 $node3" 100Mb 0ms]

$ns run

',
                      'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                    },
          'mininodes' => {
                            'info' => 'Six nodes:

	node2 - node0 - node1
                 |
        node4  node3 - node4
           
node0 to node3 is delayed.

',
                            'nsfile' => 'source tb_compat.tcl
set ns [new Simulator]

set node0 [$ns node]
set node1 [$ns node]
set node2 [$ns node]
set node3 [$ns node]
set node4 [$ns node]

$ns duplex-link $node0 $node1 100Mb 0ms DropTail 
$ns duplex-link $node0 $node3 10Mb 100ms DropTail
$ns duplex-link $node2 $node4 100Mb 0ms DropTail
$ns duplex-link $node3 $node1 100Mb 0ms DropTail

$ns run

',
                            'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                          },
          'dnardshelf' => {
                            'info' => 'If sharks be known as dnards does the world still make sense?
',
                            'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set node0 [$ns node]
set node1 [$ns node]
set node2 [$ns node]
set node3 [$ns node]

set router [$ns node]

set sharklan [$ns make-lan "$node0 $node1 $node2 $node3 $router" 10Mb 0ms]

tb-set-hardware $node0 dnard
tb-set-hardware $node1 dnard
tb-set-hardware $node2 shark
tb-set-hardware $node3 shark

$ns run

',
                            'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                          },
          'singlenode' => {
                            'info' => 'One node.
',
                            'nsfile' => 'source tb_compat.tcl
set ns [new Simulator]
set node [$ns node]
$ns run

',
                            'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                          },
          'spinglass' => {
                           'info' => 'Spinglass test

Contact: rvr@cs.cornell.edu

',
                           'nsfile' => 'set ns [new Simulator]
source tb_compat.tcl
set router [$ns node]
tb-set-node-startup $router /proj/Spinglass/router.script
set node0 [$ns node]
tb-set-hardware $node0 shark
tb-set-node-startup $node0 {/proj/Spinglass/node.script emulab0}
set node1 [$ns node]
tb-set-hardware $node1 shark
tb-set-node-startup $node1 {/proj/Spinglass/node.script emulab0}
set node2 [$ns node]
tb-set-hardware $node2 shark
tb-set-node-startup $node2 {/proj/Spinglass/node.script emulab0}
set node3 [$ns node]
tb-set-hardware $node3 shark
tb-set-node-startup $node3 {/proj/Spinglass/node.script emulab0}
set node4 [$ns node]
tb-set-hardware $node4 shark
tb-set-node-startup $node4 {/proj/Spinglass/node.script emulab0}
set node5 [$ns node]
tb-set-hardware $node5 shark
tb-set-node-startup $node5 {/proj/Spinglass/node.script emulab0}
set node6 [$ns node]
tb-set-hardware $node6 shark
tb-set-node-startup $node6 {/proj/Spinglass/node.script emulab0}
set node7 [$ns node]
tb-set-hardware $node7 shark
tb-set-node-startup $node7 {/proj/Spinglass/node.script emulab0}
set node8 [$ns node]
tb-set-hardware $node8 shark
tb-set-node-startup $node8 {/proj/Spinglass/node.script emulab1}
set node9 [$ns node]
tb-set-hardware $node9 shark
tb-set-node-startup $node9 {/proj/Spinglass/node.script emulab1}
set node10 [$ns node]
tb-set-hardware $node10 shark
tb-set-node-startup $node10 {/proj/Spinglass/node.script emulab1}
set node11 [$ns node]
tb-set-hardware $node11 shark
tb-set-node-startup $node11 {/proj/Spinglass/node.script emulab1}
set node12 [$ns node]
tb-set-hardware $node12 shark
tb-set-node-startup $node12 {/proj/Spinglass/node.script emulab1}
set node13 [$ns node]
tb-set-hardware $node13 shark
tb-set-node-startup $node13 {/proj/Spinglass/node.script emulab1}
set node14 [$ns node]
tb-set-hardware $node14 shark
tb-set-node-startup $node14 {/proj/Spinglass/node.script emulab1}
set node15 [$ns node]
tb-set-hardware $node15 shark
tb-set-node-startup $node15 {/proj/Spinglass/node.script emulab1}
set lan0 [$ns make-lan "$router $node0 $node1 $node2 $node3 $node4
$node5 $node6 $node7" 10Mb 0ms]
set lan1 [$ns make-lan "$router $node8 $node9 $node10 $node11 $node12
$node13 $node14 $node15" 10Mb 0ms]
$ns run



',
                           'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                         },
          'trafgen' => {
                         'info' => 'Basic test of the traffic generation code.

Note: this file is designed to run with NS as well and includes some \'at\' 
statements that cause parser warnings.
',
                         'nsfile' => '# Simple 5 node setup: Client, server and router, with CBR cross traffic
#	   Server
#	     ^
#	     |
#	     V
# Send <-> Router <-> Consume
#	     ^
#	     |
#	     V
#	   Client
#
# To see something interesting, start up the expt, then do a flood ping
# from client to server (see /users/newbold/linktest). You can also log
# into the router and tcpdump to see the traffic going through.

source tb_compat.tcl

set ns [new Simulator]

set send [$ns node]
set consume [$ns node]

set router [$ns node]

set client [$ns node]
set server [$ns node]

set client-lan [$ns make-lan "$send $router $client" 100Mb 2ms]
set server-lan [$ns make-lan "$consume $server $router" 100Mb 2ms]

tb-set-node-startup $router /users/newbold/trafgen/fbsd-router-startup

tb-set-node-startup $client /users/newbold/trafgen/clientroutecmd
tb-set-node-startup $server /users/newbold/trafgen/serverroutecmd

########## Magic code ###############
#Create a UDP agent and attach it to node send
set udp0 [new Agent/UDP]
$ns attach-agent $send $udp0

# Create a CBR traffic source and attach it to udp0
set cbr0 [new Application/Traffic/CBR]
$cbr0 set packetSize_ 1024
$cbr0 set rate_ 100Mbps
$cbr0 attach-agent $udp0

set null0 [new Agent/Null] 
$ns attach-agent $consume $null0

$ns connect $udp0 $null0

$ns at 0.00001 "$cbr0 start"
$ns at 0.00500 "$cbr0 stop"
#####################################

$ns run


',
                         'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                       },
          'red' => {
                     'info' => 'Test RED links
',
                     'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set nodeA [$ns node]
set nodeB [$ns node]

#
# A queue in each direction. 
#
set link0  [$ns duplex-link $nodeA $nodeB 100Mb 0ms RED]
set queue0 [[$ns link $nodeA $nodeB] queue]
$queue0 set gentle_ 1
$queue0 set queue-in-bytes_ 0
$queue0 set limit_ 50
$queue0 set maxthresh_ 20
$queue0 set thresh_ 7
$queue0 set linterm_ 11
$queue0 set q_weight_ 0.004

set queue1 [[$ns link $nodeB $nodeA] queue]
$queue1 set gentle_ 0
$queue1 set queue-in-bytes_ 1
$queue1 set limit_ 60
$queue1 set maxthresh_ 18
$queue1 set thresh_ 9
$queue1 set linterm_ 13
$queue1 set q_weight_ 0.033

# Some events.
$ns at 1.0  "$queue0 set thresh_ 8"
$ns at 2.0  "$queue1 set thresh_ 10"
$ns at 3.0  "$queue0 set limit_ 55"
$ns at 4.0  "$queue1 set limit_ 65"
$ns at 30.0 "$link0 down"
$ns at 35.0 "$link0 up"

$ns run
',
                     'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                   },
          'delaylan1' => {
                           'info' => 'Two lans sharing a node.  One lan is undelayed the other delayed.
',
                           'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]
set node3 [$ns node]
set node4 [$ns node]

set lan3 [$ns make-lan "$node1 $node2 $node3" 100Mb 100ms]
set lan2 [$ns make-lan "$node4 $node3" 100Mb 50ms]

$ns run

',
                           'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                         },
          'complete5' => {
                           'info' => 'Complete undelayed graph of five nodes.

',
                           'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]
set node3 [$ns node]
set node4 [$ns node]
set node5 [$ns node]

$ns duplex-link $node1 $node2 100Mb 0ms DropTail
$ns duplex-link $node1 $node3 100Mb 0ms DropTail
$ns duplex-link $node1 $node4 100Mb 0ms DropTail
$ns duplex-link $node1 $node5 100Mb 0ms DropTail

$ns duplex-link $node2 $node3 100Mb 0ms DropTail
$ns duplex-link $node2 $node4 100Mb 0ms DropTail
$ns duplex-link $node2 $node5 100Mb 0ms DropTail

$ns duplex-link $node3 $node4 100Mb 0ms DropTail
$ns duplex-link $node3 $node5 100Mb 0ms DropTail

$ns duplex-link $node4 $node5 100Mb 0ms DropTail

$ns run

',
                           'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                         },
          'sharkshelf' => {
                            'info' => 'Four sharks connected to a PC.

',
                            'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set node0 [$ns node]
set node1 [$ns node]
set node2 [$ns node]
set node3 [$ns node]

set router [$ns node]

set sharklan [$ns make-lan "$node0 $node1 $node2 $node3 $router" 10Mb 0ms]

tb-set-hardware $node0 shark
tb-set-hardware $node1 shark
tb-set-hardware $node2 shark
tb-set-hardware $node3 shark

$ns run

',
                            'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                          },
          'simplex' => {
                         'info' => 'Basic with the simplex tb commands.
',
                         'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]
set node3 [$ns node]
set node4 [$ns node]

set lan1 [$ns make-lan "$node1 $node2 $node3" 100Mb 0ms]
set link1 [$ns duplex-link $node4 $node1 100Mb 50ms DropTail]
set link2 [$ns duplex-link $node4 $node3 10Mb 100ms DropTail]

tb-set-lan-simplex-params $lan1 $node1 0ms 100Mb 0 100ms 10Mb 0.2
tb-set-link-simplex-params $link1 $node4 300ms 20Mb 0.4

$ns run

',
                         'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                       },
          'negprerun' => {
                           'info' => 'This test should result in a failed tbprerun.
',
                           'nsfile' => 'foo bar
',
                           'test' => 'tb_prerun("tbprerun",255);
'
                         },
          'simplelink' => {
                            'info' => 'Two nodes connected by an undelayed link.

',
                            'nsfile' => '# Two nodes connected.
source tb_compat.tcl

set ns [new Simulator]

set node0 [$ns node]
set node1 [$ns node]

$ns duplex-link $node0 $node1 100Mb 0ms DropTail

$ns run
',
                            'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                          },
          'ping' => {
                      'info' => 'Very basic ping experiment.
',
                      'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

# Set up two nodes, one called ping, and one called echo
set ping [$ns node]
set echo [$ns node]

# Now we define the characteristics of the link between
# the nodes - We want a 2-way link between the ping and
# echo nodes, with a bandwith of 100MB, and a delay in
# each direction of 150 ms
$ns duplex-link $ping $echo 100Mb 150ms DropTail

tb-set-ip ping 192.168.101.1
tb-set-ip echo 192.168.101.2

# And away we go...
$ns run
',
                      'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                    },
          'nodes' => {
                       'info' => 'Six nodes:

	node2 - node0 - node1
                 |
        node4  node3 - node4
           
node0 to node3 is delayed.

',
                       'nsfile' => 'source tb_compat.tcl
set ns [new Simulator]
set node0 [$ns node]
set node1 [$ns node]
set node2 [$ns node]
set node3 [$ns node]
set node4 [$ns node]
set node5 [$ns node]

set link0 [$ns duplex-link $node0 $node1 100Mb 2ms DropTail]
set link1 [$ns duplex-link $node0 $node2 100Mb 2ms DropTail]
set link2 [$ns duplex-link $node0 $node3 10Mb 100ms DropTail]
set link3 [$ns duplex-link $node3 $node4 100Mb 2ms DropTail]
set link4 [$ns duplex-link $node3 $node5 100Mb 2ms DropTail]

# Turn on manual routing.
$ns rtproto Manual

# Set manual routes
$node0 add-route $node4 $node3
$node0 add-route $node5 $node3
$node1 add-route $node2 $node0
$node1 add-route $link2 $node0
$node1 add-route $node4 $node0
$node1 add-route $node5 $node0
$node2 add-route $node1 $node0
$node2 add-route [$ns link $node3 $node0] $node0
$node2 add-route $node4 $node0
$node3 add-route $node1 $node0
$node3 add-route $node2 $node0
$node4 add-route $link2 $node3
$node4 add-route $node1 $node3
$node4 add-route $node2 $node3
$node4 add-route $node5 $node3
$node5 add-route [$ns link $node0 $node3] $node3
$node5 add-route $node1 $node3
$node5 add-route $node2 $node3
$node5 add-route $node4 $node3

$ns run
',
                       'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                     },
          'vtypes' => {
                        'info' => 'Based on basic.  Simple 6 node topology with 3 vtypes, one soft, one hard, 
and one lightly weighted.
',
                        'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]
set node3 [$ns node]
set node4 [$ns node]
set node5 [$ns node]
set node6 [$ns node]

set lan1 [$ns make-lan "$node1 $node2 $node3" 100Mb 0ms]
set link1 [$ns duplex-link $node4 $node1 100Mb 50ms DropTail]
set link2 [$ns duplex-link $node4 $node3 10Mb 100ms DropTail]
set link3 [$ns duplex-link $node5 $node2 100Mb 0ms DropTail]
set link4 [$ns duplex-link $node6 $node5 100Mb 0ms DropTail]

tb-make-soft-vtype A {pc600 pc850}
tb-make-hard-vtype B {pc600 pc850}
tb-make-weighted-vtype C 0.1 {pc600 pc850}

tb-set-hardware $node1 B
tb-set-hardware $node2 B
tb-set-hardware $node3 A
tb-set-hardware $node4 A
tb-set-hardware $node5 C
tb-set-hardware $node6 C

$ns run

',
                        'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                      },
          'spinglass2' => {
                            'info' => 'A hefty shark/node test.

Contact: rvr@cs.cornell.edu
',
                            'nsfile' => 'set ns [new Simulator]
source tb_compat.tcl

set supernets {p q r}
set nets {a e i}

set nsupernets [llength $supernets]
set nnets [llength $nets]

# All the set X [set Y [$ns node]] is so that the node gets the name Y
# but we can refer to it as X for loops.
set j 0
set k 2
set routers {}
foreach supernet $supernets {
        set router [set router$supernet [$ns node]]
        lappend routers $router
        tb-set-node-startup $router "/proj/Spinglass/router.script $nsupernets
$j $nnets"
        foreach net $nets {
                set nodes {}
                for {set i 0} {$i <= 7} {incr i} {
                        set tmp [set $supernet$net$i [$ns node]]
                        tb-set-hardware $tmp shark
                        tb-set-node-startup $tmp  "/proj/Spinglass/host.script
emulab $tmp /$supernet$net/$tmp 192.168.$k.2"
                        lappend nodes $tmp
                }
                set lan [set lan$router$net [$ns make-lan "$router $nodes" 10Mb 0ms]] 
                tb-set-ip-lan $router $lan 192.168.$k.2
                set i 3
                foreach node $nodes {
                        tb-set-ip-lan $node $lan 192.168.$k.$i
                        incr i
                }
                incr k
        }
        incr j
}

set backbone [$ns make-lan "$routers" 100Mb 0ms]

set j 2
foreach router $routers {
        tb-set-ip-lan $router $backbone 192.168.1.$j
        incr j
}

$ns run
',
                            'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                          },
          'basic' => {
                       'info' => 'Just a basic initial test.

Topology:

          link1     link2
         +-----node4-----+
         |               |
	node1	node2	node3
	 |       |       |
   lan1---------------------

Delays:
lan1	100Mb/0ms
link1   100Mb/50ms
link2   100Mb/100ms

TB commands:
None

Warnings:
None

Errors:
None

',
                       'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]
set node3 [$ns node]
set node4 [$ns node]

set lan1 [$ns make-lan "$node1 $node2 $node3" 100Mb 0ms]
set link1 [$ns duplex-link $node4 $node1 100Mb 50ms DropTail]
set link2 [$ns duplex-link $node4 $node3 10Mb 100ms DropTail]

$ns run

',
                       'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                     },
          'fixed' => {
                       'info' => 'The basic initial test with fixing node1 to pc3 and node3 to pc6.

Topology:

          link1     link2
         +-----node4-----+
         |               |
	node1	node2	node3
	 |       |       |
   lan1---------------------

Delays:
lan1	100Mb/0ms
link1   100Mb/50ms
link2   100Mb/100ms

TB commands:
None

Warnings:
None

Errors:
None
',
                       'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]
set node3 [$ns node]
set node4 [$ns node]

set lan1 [$ns make-lan "$node1 $node2 $node3" 100Mb 0ms]
set link1 [$ns duplex-link $node4 $node1 100Mb 50ms DropTail]
set link2 [$ns duplex-link $node4 $node3 10Mb 100ms DropTail]

tb-fix-node $node1 pc3
tb-fix-node $node3 pc6

$ns run

',
                       'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                     },
          'setip' => {
                        'info' => 'Sets up a basic topology and then tries out all the tb-set-ip commands.

',
                        'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set node0 [$ns node]
set node1 [$ns node]
set node2 [$ns node]

set lan0 [$ns make-lan "$node0 $node1 $node2" 100Mb 0ms]

set node3 [$ns node]

set link0 [$ns duplex-link $node0 $node3 100Mb 0ms DropTail]

set node4 [$ns node]

$ns duplex-link $node4 $node0 100Mb 0ms DropTail

tb-set-ip $node4 1.0.4.1
tb-set-ip-interface $node0 $node4 1.0.0.1
tb-set-ip-link $node0 $link0 1.0.0.2
tb-set-ip-lan $node0 $lan0 1.0.0.3

$ns run

',
                        'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                      },
          'basicrsrv' => {
                            'info' => 'Just a basic initial test.

Topology:

          link1     link2
         +-----node4-----+
         |               |
	node1	node2	node3
	 |       |       |
   lan1---------------------

Delays:
lan1	100Mb/0ms
link1   100Mb/50ms
link2   100Mb/100ms

TB commands:
None

Warnings:
None

Errors:
None

',
                            'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]
set node3 [$ns node]
set node4 [$ns node]

set lan1 [$ns make-lan "$node1 $node2 $node3" 100Mb 0ms]
set link1 [$ns duplex-link $node4 $node1 100Mb 50ms DropTail]
set link2 [$ns duplex-link $node4 $node3 10Mb 100ms DropTail]

$ns run

',
                            'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
',
                            'dbstate' => 'replace into reserved (node_id,pid,eid,rsrv_time,vname,erole) values 
(\'pc1\',\'testbed\',\'unavailable\',0,\'pc1\',\'node\'),
(\'pc2\',\'testbed\',\'unavailable\',0,\'pc2\',\'node\'),
(\'pc3\',\'testbed\',\'unavailable\',0,\'pc3\',\'node\'),
(\'pc4\',\'testbed\',\'unavailable\',0,\'pc4\',\'node\'),
(\'pc5\',\'testbed\',\'unavailable\',0,\'pc5\',\'node\'),
(\'pc6\',\'testbed\',\'unavailable\',0,\'pc6\',\'node\'),
(\'pc7\',\'testbed\',\'unavailable\',0,\'pc7\',\'node\'),
(\'pc8\',\'testbed\',\'unavailable\',0,\'pc8\',\'node\'),
(\'pc9\',\'testbed\',\'unavailable\',0,\'pc9\',\'node\'),
(\'pc10\',\'testbed\',\'unavailable\',0,\'pc10\',\'node\');
'
                          },
          'wideareatypes' => {
                                'info' => 'Tests widearea nodes, using general types, such as Internet, Internet2, etc.
',
                                'nsfile' => 'set ns [new Simulator]
source tb_compat.tcl

set nodeA [$ns node]
set nodeB [$ns node]
set nodeC [$ns node]
set nodeD [$ns node]
set nodeE [$ns node]

tb-set-hardware $nodeA pcvroninet2
tb-set-hardware $nodeB pcvroninet
tb-set-hardware $nodeC pcvronintl
tb-set-hardware $nodeD pcvrondsl
tb-set-hardware $nodeE pcvwainet

$ns run

',
                                'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                              },
          'ixp' => {
                     'info' => 'A simple test using an IXP, to exercise the subnode capabilities.
',
                     'nsfile' => 'source tb_compat.tcl
set ns [new Simulator]

set myixp   [$ns node]
set ixphost [$ns node]

tb-set-hardware $myixp ixp-bv
tb-fix-node $myixp $ixphost

tb-set-node-os $ixphost RHL73-IXPHOST

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set link1 [$ns duplex-link $n0 $myixp 100Mb 0ms DropTail]
set link2 [$ns duplex-link $n1 $myixp 100Mb 0ms DropTail]
set link3 [$ns duplex-link $n2 $myixp 100Mb 0ms DropTail]
set link4 [$ns duplex-link $n3 $myixp 100Mb 0ms DropTail]

$ns rtproto Static
$ns run
',
                     'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                   },
          'multilink' => {
                           'info' => 'Two nodes connceted by undelayed links.

',
                           'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]

$ns duplex-link $node1 $node2 100Mb 0ms DropTail
$ns duplex-link $node1 $node2 100Mb 0ms DropTail
$ns duplex-link $node1 $node2 100Mb 0ms DropTail
$ns duplex-link $node1 $node2 100Mb 0ms DropTail

$ns run

',
                           'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                         },
          'minisetip' => {
                             'info' => 'Sets up a basic topology and then tries out all the tb-set-ip commands.

',
                             'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set node0 [$ns node]
set node1 [$ns node]
set node2 [$ns node]

set lan0 [$ns make-lan "$node0 $node1 $node2" 100Mb 0ms]

set node3 [$ns node]

set link0 [$ns duplex-link $node0 $node3 100Mb 0ms DropTail]

set node4 [$ns node]

$ns duplex-link $node4 $node2 100Mb 0ms DropTail

tb-set-ip $node4 1.0.4.1
tb-set-ip-interface $node2 $node4 1.0.0.1
tb-set-ip-link $node0 $link0 1.0.0.2
tb-set-ip-lan $node0 $lan0 1.0.0.3

$ns run

',
                             'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                           },
          'delaylink' => {
                           'info' => 'Two nodes connected by a delayed link.

',
                           'nsfile' => '# Two nodes connected over a delayed link.
source tb_compat.tcl

set ns [new Simulator]

set node0 [$ns node]
set node1 [$ns node]

$ns duplex-link $node0 $node1 100Mb 0ms DropTail

$ns run
',
                           'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                         },
          'db1' => {
                     'info' => 'This is the first test that checks the sanity of the database.
It only checks the virt_lans and virt_nodes tables.

',
                     'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]
set node3 [$ns node]
set node4 [$ns node]
set node5 [$ns node]
set node6 [$ns node]

set lan3 [$ns make-lan "$node1 $node2 $node3" 100Mb 100ms]
set lan2 [$ns make-lan "$node4 $node3" 100Mb 50ms]
set link1 [$ns duplex-link $node4 $node5 100Mb 0ms DropTail]
set link2 [$ns duplex-link $node4 $node5 100Mb 0ms DropTail]
tb-set-link-loss $link2 0.05
set link3 [$ns duplex-link $node6 $node5 100Mb 0ms DropTail]
set link4 [$ns duplex-link $node6 $node1 45Mb 1000ms DropTail]

$ns run



',
                     'test' => 'tb_prerun("tbprerun",0);
# Check DB state.
@result = (
["lan2","25.00","100000","0.000","node3:1"],
["lan2","25.00","100000","0.000","node4:0"],
["lan3","50.00","100000","0.000","node1:0"],
["lan3","50.00","100000","0.000","node2:0"],
["lan3","50.00","100000","0.000","node3:0"],
["link1","0.00","100000","0.000","node4:1"],
["link1","0.00","100000","0.000","node5:0"],
["link2","0.00","100000","0.025","node4:2"],
["link2","0.00","100000","0.025","node5:1"],
["link3","0.00","100000","0.000","node5:2"],
["link3","0.00","100000","0.000","node6:0"],
["link4","500.00","45000","0.000","node1:1"],
["link4","500.00","45000","0.000","node6:1"],
);

tb_compare("SELECT vname,delay,bandwidth,lossrate,member from virt_lans" .
    " where pid=\\"testbed\\" and eid=\\"test\\" " .
    "order by vname,member",
    \\@result);
@result = (
["0:10.1.5.2 1:10.1.4.3","node1","pc"],
["0:10.1.5.3","node2","pc"],
["0:10.1.5.4 1:10.1.3.3","node3","pc"],
["0:10.1.3.2 1:10.1.6.2 2:10.1.1.2","node4","pc"],
["0:10.1.6.3 1:10.1.1.3 2:10.1.2.3","node5","pc"],
["0:10.1.2.2 1:10.1.4.2","node6","pc"],
);
tb_compare("SELECT ips,vname,type from virt_nodes" .
    " where pid=\\"testbed\\" and eid=\\"test\\"",
    \\@result);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                   },
          'buddycache' => {
                            'info' => 'This is the buddycache experiment from Brandeis University.  It\'s a LAN of
7 nodes, one of which, or, has a delay.

Contact for the original experiment is:

Liuba Shrira
liuba@cs.brandeis.edu


',
                            'nsfile' => 'source tb_compat.tcl
set ns [new Simulator]

set fe1 [$ns node]
set fe2 [$ns node]
set fe3 [$ns node]
set fe4 [$ns node]
set fe5 [$ns node]
set proxy [$ns node]
set or [$ns node]

set lan0 [$ns make-lan "$fe1 $fe2 $fe3 $fe4 $fe5 $proxy $or" 100Mb 2ms]
tb-set-node-lan-delay $or $lan0 20ms

$ns run
',
                            'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                          },
          'trivial' => {
                         'info' => 'Trivial test.  Creates a simulator and runs it.  No nodes, no lans, 
nothing.
',
                         'nsfile' => '
source tb_compat.tcl
set ns [new Simulator]
$ns run

',
                         'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                       },
          'wideareamapped' => {
                                 'info' => 'A widearea test that asks for specific links between nodes, which must be
mapped with the WAN solver.
',
                                 'nsfile' => 'set ns [new Simulator]
source tb_compat.tcl

set nodeA [$ns node]
set nodeB [$ns node]
set nodeC [$ns node]

tb-set-hardware $nodeA pcvron
tb-set-hardware $nodeB pcvron
tb-set-hardware $nodeC pcvron

set link0 [$ns duplex-link $nodeA $nodeB 2Mb 50ms DropTail]
set link1 [$ns duplex-link $nodeA $nodeC 1Mb 75ms DropTail]
set link2 [$ns duplex-link $nodeB $nodeC 0.5Mb 100ms DropTail]

$ns run

',
                                 'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                               },
          'minitbcmd' => {
                            'info' => 'NOT A FULL TEST!

This is a test of all the tb-* commands.  It also checks does checks on 
virt_lans and virt_nodes to make sure they done the right thing.

',
                            'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set node0 [$ns node]
set node1 [$ns node]
set node2 [$ns node]

set lan0 [$ns make-lan "$node0 $node2 $node1" 100Mb 0ms]

tb-set-node-lan-delay $node0 $lan0 100ms
tb-set-node-lan-bandwidth $node0 $lan0 20Mb
tb-set-node-lan-loss $node0 $lan0 0.4
tb-set-node-lan-params $node1 $lan0 150ms 30Mb 0.0

set node3 [$ns node]

set link0 [$ns duplex-link $node2 $node3 100Mb 0ms DropTail]

tb-set-link-loss $link0 0.1

set node4 [$ns node]

$ns duplex-link $node4 $node1 100Mb 0ms DropTail
tb-set-link-loss $node4 $node1 0.025

tb-set-ip $node4 1.0.4.1
tb-set-ip-interface $node1 $node4 1.0.0.1
tb-set-ip-link $node2 $link0 1.0.0.2
tb-set-ip-lan $node1 $lan0 1.0.0.3

tb-set-hardware $node4 pc

tb-set-node-os $node4 MYOS
tb-set-node-cmdline $node4 "my command line"
tb-set-node-rpms $node4 "my node rpms"
tb-set-node-startup $node4 "my node startup"
tb-set-node-tarfiles $node4 dira tara dirb tarb dirc tarc
tb-set-node-deltas $node4 "deltas!"
tb-set-node-failure-action $node4 nonfatal

$ns run
',
                            'test' => 'tb_prerun("tbprerun",0);
@result = (
["l8","0.00","100000","0.000","node2:0"],
["l8","0.00","100000","0.000","node2:0"],
["link0","0.00","100000","0.051","node0:1"],
["link0","0.00","100000","0.051","node3:0"],
["lan0","100.00","20000","0.400","node0:0"],
["lan0","0.00","100000","0.000","node2:0"],
["lan0","150.00","30000","0.000","node1:0"],
);
tb_compare("select vname,delay,bandwidth,lossrate,member from virt_lans" .
	" where pid=\\"testbed\\" and eid=\\"test\\"",\\@result);
@result = (
["0:1.0.0.2","node0","pc"],
["0:1.0.0.3 1:1.0.0.1","node1","pc"],
["0:1.0.0.4 1:1.0.0.2","node2","pc"],
["0:1.0.0.3","node3","pc"],
["0:1.0.4.1","node4","pc"],
);
tb_compare("select ips,vname,type from virt_nodes" .
    " where pid=\\"testbed\\" and eid=\\"test\\"",\\@result);
@result = (
["0:1.0.4.1","MYOS","my command line","my node rpms","deltas!","my node startup","dira tara:dirb tarb:dirc tarc","nonfatal","pc"], );
tb_compare("select  ips,osid,cmd_line,rpms,deltas,startupcmd,tarfiles,failureaction,type" .
    " from virt_nodes where pid=\\"testbed\\" and eid=\\"test\\"" .
    " and vname=\\"node4\\"",\\@result);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
',
                            'dbstate' => 'replace into os_info (osid,description,version,pid) values ("MYOS","Test OS","0","testbed");
'
                          },
          'toomanylinks' => {
                              'info' => 'This test tries to make an experiment with more links than our nodes can 
support, and makes sure that it fails.

',
                              'nsfile' => '#generated by Netbuild
set ns [new Simulator]
source tb_compat.tcl

set node2 [$ns node]
set node3 [$ns node]
set node4 [$ns node]
set node5 [$ns node]
set node6 [$ns node]

set link1 [$ns duplex-link $node2 $node3 100Mb 0ms DropTail]
set link2 [$ns duplex-link $node2 $node4 100Mb 0ms DropTail]
set link3 [$ns duplex-link $node2 $node5 100Mb 0ms DropTail]
set link4 [$ns duplex-link $node2 $node6 100Mb 0ms DropTail]
set link5 [$ns duplex-link $node6 $node5 100Mb 0ms DropTail]
set link6 [$ns duplex-link $node6 $node4 100Mb 0ms DropTail]
set link7 [$ns duplex-link $node6 $node3 100Mb 0ms DropTail]


set lan0 [$ns make-lan "$node2 $node6 " 100Mb 0ms]

tb-set-node-lan-params $node2 $lan0 0ms 100Mb 0.0
tb-set-node-lan-params $node6 $lan0 0ms 100Mb 0.0


$ns rtproto Static
$ns run
#netbuild-generated ns file ends.






',
                              'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",255);
tb_run("tbend",0);
'
                            },
          'minimultilink' => {
                                'info' => 'Two nodes connceted by undelayed links.

',
                                'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]

$ns duplex-link $node1 $node2 100Mb 0ms DropTail
$ns duplex-link $node1 $node2 100Mb 0ms DropTail

$ns run

',
                                'test' => 'tb_prerun("tbprerun",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
'
                              },
          'toofast' => {
                         'info' => 'This test tries to make a link that is faster than the testbed supports, 
and makes sure that it fails.

',
                         'nsfile' => '#generated by Netbuild
set ns [new Simulator]
source tb_compat.tcl

set node0 [$ns node]
set node1 [$ns node]

set link0 [$ns duplex-link $node1 $node0 1150Mb 0ms DropTail]

$ns rtproto Static
$ns run
#netbuild-generated ns file ends.
',
                         'test' => 'tb_prerun("tbprerun",255);
'
                       },
          'tbcmd' => {
                       'info' => 'NOT A FULL TEST!

This is a test of all the tb-* commands.  It also checks does checks on 
virt_lans and virt_nodes to make sure they done the right thing.

',
                       'nsfile' => 'source tb_compat.tcl

set ns [new Simulator]

set node0 [$ns node]
set node1 [$ns node]
set node2 [$ns node]

set lan0 [$ns make-lan "$node0 $node1 $node2" 100Mb 0ms]

tb-set-lan-loss $lan0 0.2
tb-set-node-lan-delay $node0 $lan0 100ms
tb-set-node-lan-bandwidth $node0 $lan0 20Mb
tb-set-node-lan-loss $node0 $lan0 0.4
tb-set-node-lan-params $node1 $lan0 150ms 30Mb 0.0

set node3 [$ns node]

set link0 [$ns duplex-link $node0 $node3 100Mb 0ms DropTail]

tb-set-link-loss $link0 0.1

set node4 [$ns node]

$ns duplex-link $node4 $node0 100Mb 0ms DropTail
tb-set-link-loss $node4 $node0 0.025

tb-set-ip $node4 1.0.4.1
tb-set-ip-interface $node0 $node4 1.0.0.1
tb-set-ip-link $node0 $link0 1.0.0.2
tb-set-ip-lan $node0 $lan0 1.0.0.3

set s5 [$ns node]
set s6 [$ns node]
$ns make-lan "$s5 $s6 $node4" 100Mb 0ms

tb-set-hardware $s5 pc
tb-set-hardware $s6 pc
tb-set-hardware $node4 pc

tb-set-node-os $node4 MYOS
tb-set-node-cmdline $node4 "my command line"
tb-set-node-rpms $node4 /my /node /rpms
tb-set-node-startup $node4 "my node startup"
tb-set-node-tarfiles $node4 /dira /tara /dirb /tarb /dirc /tarc
tb-set-node-failure-action $node4 nonfatal

$ns run
',
                       'test' => 'tb_prerun("tbprerun",0);
@result = (
["lan0","100.00","20000","0.400","node0:0"],
["lan0","150.00","30000","0.000","node1:0"],
["lan0","0.00","100000","0.106","node2:0"],
["link0","0.00","100000","0.051","node0:1"],
["link0","0.00","100000","0.051","node3:0"],
["tblan-lan11","0.00","100000","0.000","node4:1"],
["tblan-lan11","0.00","100000","0.000","s5:0"],
["tblan-lan11","0.00","100000","0.000","s6:0"],
["tblink-l8","0.00","100000","0.013","node0:2"],
["tblink-l8","0.00","100000","0.013","node4:0"],
);
tb_compare("select vname,delay,bandwidth,lossrate,member from virt_lans" .
	" where pid=\\"testbed\\" and eid=\\"test\\" order by vname,member",
	\\@result);
@result = (
["0:1.0.0.3 1:1.0.0.2 2:1.0.0.1","node0","pc"],
["0:1.0.0.2","node1","pc"],
["0:1.0.0.4","node2","pc"],
["0:1.0.0.3","node3","pc"],
["0:1.0.4.1 1:10.1.1.4","node4","pc"],
["0:10.1.1.2","s5","pc"],
["0:10.1.1.3","s6","pc"],
);
tb_compare("select ips,vname,type from virt_nodes" .
    " where pid=\\"testbed\\" and eid=\\"test\\"",\\@result);
@result = (
["0:1.0.4.1 1:10.1.1.4","MYOS","my command line","/my;/node;/rpms","my node startup","/dira /tara;/dirb /tarb;/dirc /tarc","nonfatal","pc"], );
tb_compare("select ips,osname,cmd_line,rpms,startupcmd,tarfiles,failureaction,type" .
    " from virt_nodes where pid=\\"testbed\\" and eid=\\"test\\"" .
    " and vname=\\"node4\\"",\\@result);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbswap in",0);
tb_run("tbswap out",0);
tb_run("tbend",0);
',
                       'dbstate' => 'replace into os_info (osname,osid,description,version,pid) values ("MYOS","testbed-MYOS","Test OS","0","testbed");
'
                     }
        };
1;

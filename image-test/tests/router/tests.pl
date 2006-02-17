
# need the delay here to give time for the routes to converge when the
# routing protocol is session
if ($parms{rtproto} eq 'Session') {
  print "Sleeping 90 seconds to give time for the routes to converge...";
  sleep 90;
  print "DONE\n";
}

test_traceroute 'node0', 'node4', qw(node2-link0 node3-link2 node4-link3);
test_traceroute 'node4', 'node0', qw(node3-link3 node2-link2 node0-link0);

test_traceroute 'node0', 'node1', qw(node2-link0 node1-link1);
test_traceroute 'node1', 'node0', qw(node2-link1 node0-link0);

test_traceroute 'node1', 'node4', qw(node2-link1 node3-link2 node4-link3);
test_traceroute 'node4', 'node1', qw(node3-link3 node2-link2 node1-link1);



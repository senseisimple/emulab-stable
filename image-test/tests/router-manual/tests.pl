
test_traceroute 'node1', 'nodeA', qw(node3-lan nodeA-link);
test_traceroute 'nodeA', 'node1', qw(node3-link node1-lan);

test_traceroute 'node2', 'nodeA', qw(node3-lan nodeA-link);
test_traceroute 'nodeA', 'node2', qw(node3-link node2-lan);

test_traceroute 'node3', 'nodeA', qw(nodeA-link);
test_traceroute 'nodeA', 'node3', qw(node3-link);

test_traceroute 'node1', 'node3', qw(node3-lan);
test_traceroute 'node3', 'node1', qw(node1-lan);

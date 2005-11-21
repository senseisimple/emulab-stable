
sub test_traceroute ($$@) {
  my ($src,$dest,@path) = @_;
  test_rcmd "traceroute-$src_$dest", [], $src, "/usr/sbin/traceroute $dest",
    sub {
      local @_ = split /\n/;
      if (@_+0 != @path+0) {
	printf "*** traceroute $src->$dest: expected %d hops but got %d.\n",
	  @path+0, @_+0;
	return 0;
      }
      for (my $i = 0; $i < @_; $i++) {
	local $_ = $_[$i];
	my ($n) = /^\s*\d+\s*(\S+)/;
	next if $n eq $path[$i];
	printf "*** traceroute $src->$dest: expected %s for hop %d but got %s\n",
	  $path[0], $i+1, $n;
	return 0;
      }
      return 1;
    };
}

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



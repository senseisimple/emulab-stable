
test_rcmd 'tar', [], 'node', 'test -e /usr/tar.gz';
test_rcmd 'tar.Z', ['tar'], 'node', 'test -e /usr/tar.Z/hw.txt';
test_rcmd 'tgz', ['tar'], 'node', 'test -e /usr/tgz/hw.txt';
test_rcmd 'tar.gz', ['tar'], 'node', 'test -e /usr/tar.gz/hw.txt';
test_rcmd 'tar.bz2', ['tar'], 'node', 'test -e /usr/tar.bz2/hw.txt';
test_rcmd 'startcmd', [], 'node', 'test -e /tmp/startcmd-ok';
test_cmd 'reboot', [], "node_reboot -w -e $parms{pid},$parms{eid}";


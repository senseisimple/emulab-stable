#!/bin/csh

cd /usr/local/etc/testbed
chown root update vnodesetup
chmod u+s update vnodesetup
chown root /usr/bin/suidperl
chmod u+s /usr/bin/suidperl

cp vtund /usr/local/sbin
/usr/bin/install -c -o root -g wheel -d -m 755 -o root -g 0 /var/log/vtund
/usr/bin/install -c -o root -g wheel -d -m 755 -o root -g 0 /var/lock/vtund

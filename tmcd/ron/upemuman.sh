#!/bin/csh

cd /usr/local/etc/testbed

chown root update vnodesetup
chmod u+s update vnodesetup
chown root /usr/bin/suidperl
chmod u+s /usr/bin/suidperl
chown emulabman client.pem emulab.pem
chmod 640 client.pem emulab.pem
/usr/bin/install -c -o root -g wheel -d -m 755 -o root -g 0 /var/testbed
cp rc.testbed /usr/local/etc/rc.d/testbed.sh

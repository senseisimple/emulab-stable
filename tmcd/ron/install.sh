#!/bin/csh

set path = ( /bin /sbin /usr/bin /usr/sbin /usr/local/bin /usr/local/sbin )

setenv OSTYPE `uname -s`
if ($OSTYPE == "FreeBSD") then
	setenv USERADD "pw useradd";
else if ($OSTYPE == "Linux") then
	setenv USERADD "useradd";
else
	echo "Unsupported OS: $OSTYPE";
	exit 1;
endif

$USERADD emulabman -u 65520 -g bin -m -s /bin/tcsh -c "Emulab Man"

cd /usr/local/etc/emulab
if ($OSTYPE == "FreeBSD") then
	ln -s liblocsetup-freebsd.pm liblocsetup.pm
	rm -f /usr/local/etc/rc.d/testbed.sh
	rm -f /usr/local/etc/rc.d/emulab.sh
	cp -f rc.testbed /usr/local/etc/rc.d/emulab.sh
else
	ln -s liblocsetup-linux.pm liblocsetup.pm
	rm -f /etc/init.d/emulab
	cp rc.testbed /etc/init.d/emulab
	rm -f /etc/rc3.d/S99emulab
	ln -s ../init.d/emulab /etc/rc3.d/S99emulab
	rm -f /etc/rc2.d/S99emulab
	ln -s ../init.d/emulab /etc/rc2.d/S99emulab
	rm -f /etc/rc5.d/S99emulab
	ln -s ../init.d/emulab /etc/rc5.d/S99emulab
endif
chown emulabman . *
chgrp bin . *
chown root update vnodesetup
chmod u+s update vnodesetup
chown root /usr/bin/suidperl
chmod u+s /usr/bin/suidperl
chown emulabman client.pem emulab.pem
chmod 640 client.pem emulab.pem
/usr/bin/install -c -o root -g wheel -d -m 755 -o root -g 0 /var/emulab

if ( -e vtund ) then
  cp vtund /usr/local/sbin
  /usr/bin/install -c -o root -g wheel -d -m 755 -o root -g 0 /var/log/vtund
  /usr/bin/install -c -o root -g wheel -d -m 755 -o root -g 0 /var/lock/vtund
endif

if (! -d ~emulabman/.ssh) then
	cd ~emulabman
	chmod 755 .
	mkdir .ssh
	chown emulabman .ssh
	chgrp bin .ssh
	chmod 700 .ssh
	cd .ssh
	cp /usr/local/etc/emulab/emulabkey authorized_keys
	chown emulabman authorized_keys
	chgrp bin authorized_keys
	chmod 644 authorized_keys
endif



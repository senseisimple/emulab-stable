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

if ($OSTYPE == "Linux") then
	cd /usr/local/etc/emulab
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

exit 0;

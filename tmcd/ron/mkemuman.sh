#!/bin/csh

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

cd /usr/local/etc/testbed
if ($OSTYPE == "FreeBSD") then
	cp rc.testbed /usr/local/etc/rc.d/testbed.sh
endif
chown emulabman . *
chgrp bin . *
chown root update vnodesetup
chmod u+s update vnodesetup
chown root /usr/bin/suidperl
chmod u+s /usr/bin/suidperl
chown emulabman client.pem emulab.pem
chmod 640 client.pem emulab.pem
mkdir /var/testbed

cp vtund /usr/local/sbin
/usr/bin/install -c -o root -g wheel -d -m 755 -o root -g 0 /var/log/vtund
/usr/bin/install -c -o root -g wheel -d -m 755 -o root -g 0 /var/lock/vtund

cd ~emulabman
chmod 755 .
mkdir .ssh
chown emulabman .ssh
chgrp bin .ssh
chmod 700 .ssh
cd .ssh

echo "1024 37 168728947415883137658395816497236019932357443574364998989351516015013006429180411438552594116282442938932702706360430451154958992295988097967662214818020771421328881173382895214540694120581207714991274873698590147743427181599852480329442016838781882554809552882295931111276319070960396053057987057937216750401 root@paper.cs.utah.edu" > authorized_keys

chown emulabman authorized_keys
chgrp bin authorized_keys
chmod 644 authorized_keys


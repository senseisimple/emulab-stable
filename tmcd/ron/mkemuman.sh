#!/bin/csh

pw useradd emulabman -u 65520 -g bin -m -s /bin/tcsh -c \"Emulab Man\"

cd /usr/local/etc/testbed
cp rc.testbed /usr/local/etc/rc.d/testbed.sh
chown emulabman . *
chgrp bin . *
chown root update 
chmod u+s update
chown root /usr/bin/suidperl
chmod u+s /usr/bin/suidperl

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

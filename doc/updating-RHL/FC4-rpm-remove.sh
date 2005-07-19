

rpm -e samba-common samba system-config-samba samba-client
rpm -e system-config-printer hal-cups-utils  system-config-printer-gui

rpm -qa | grep system-config | sudo xargs rpm -e
rm /etc/sysconfig/system-config-securitylevel.rpmsave

rpm -e pcmcia-cs
rm /etc/sysconfig/pcmcia.rpmsave

rpm -e kudzu

rpm -qa | grep gnome | sudo xargs rpm -e libbonoboui gail gtkhtml2

rpm -e pciutils-devel

rpm -e a2ps
rm /usr/share/a2ps/afm/fonts.map.rpmsave

rpm -e alchemist

rpm -e apmd

rpm -e audiofile esound

rpm -e authconfig authconfig-gtk

rpm -e bind caching-nameserver NetworkManager
rm /etc/rndc.key.rpmsave

rpm -e comps comps-extras

rpm -e curl-devel

rpm -e cyrus-sasl-plain cyrus-sasl-devel openldap-devel

rpm -e desktop-backgrounds-basic

rpm -e doxygen

rpm -e fbset

rpm -e foomatic

rpm -e ghostscript ghostscript-fonts hpijs

rpm -e up2date
rm /etc/sysconfig/rhn/up2date-uuid.rpmsave

rpm -e gpm-devel

rpm -e hesiod-devel

rpm -e tetex tetex-fonts texinfo

rpm -e redhat-menus htmlview pinfo 
rpm -e desktop-file-utils

rpm -e mod_perl mod_ssl mod_python httpd php webalizer httpd-manual php-ldap php-pear

rpm -e apr apr-util

rpm -e irda-utils

rpm -e isdn4k-utils

rpm -e kbd

rpm -e krbafs krbafs-devel
rm /etc/krb.conf.rpmsave

rpm -e lha

rpm -e libIDL ORBit2 pyorbit pyorbit libbonobo GConf2 bluez-pin

rpm -e libart_lgpl

rpm -e libcap-devel

rpm -e libglade2 pygtk2-libglade usermode-gtk

rpm -e qt oprofile

rpm -e libmng

rpm -e libogg libvorbis sox libogg-devel libvorbis-devel

rpm -e hal bluez-utils  pm-utils

rpm -e libwnck

rpm -e libwvstreams wvdial

rpm -e mpage

rpm -e crypto-utils

rpm -e newt ntsysv setuptool newt-perl newt-devel

rpm -e nscd nss_ldap
rm /etc/ldap.conf.rpmsave

rpm -e pam_krb5 pam_smb
rm /etc/pam_smb.conf.rpmsave

rpm -e pnm2ppa

rpm -e  ppp rp-pppoe

rpm -e psgml

rpm -e psutils

rpm -e pyOpenSSL pygtk2   rhnlib

rpm -e  pyxf86config rhpl

rpm -e rdist

rpm -e talk

rpm -e xdelta

rpm -e yp-tools ypbind

rpm -e alsa-lib alsa-utils

rpm -e eject

rpm -e squid

rpm -e kernel-devel # since it headers may no longer be accurate

rpm -e howl-lib howl


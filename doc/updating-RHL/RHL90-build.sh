#!/bin/sh -f

### XXX --- not yet scriptable:
### + acquiring the testbed source tree.

### XXX --- to-do's:
### + atheros driver for wireless stuff?
###   - unpack traball and build it.
###
### + Remove kernel RPMs.
### + Remove kernel source RPMS.
###

### Progress, but unhappy:
### + Fix problem with shadow-utils; groupadd doesn't like groups names
###   w/uppercase letters.

### Addressed:
### + Pick the right RPMs.
###   ...also see <http://shrike.freshrpms.net/>.
### + Initial bits about lilo, lilo.conf
###   - remember to get the matching boot.b!  and chain.b, and os2_d.b
### + Adding a getty for ttyS0 to /etc/inittab
### + Enable root logins on ttyS0 in /etc/securetty
### + need to build two kernels: one normla, and one for linkdelay.
### + Rebuild the kernel with IPOD.
###   ...with the traffic shaping patches --- testbed/delay/linux
###   ...and with the right options, modules.
### + after updating kernel, boot messages from initd are missing?
### + audit the running daemons; get rid of `rpmd', e.g.
### + kill off extra gettys in /etc/inittab
###   - avoid whining about char-major-4
###   - kill them, to avoid broken pattern in /etc/rc.d/rc.sysinit
###   - also see <http://lists.debian.org/debian-sparc/2002/12/msg00055.html>
###   - OR, alias char-major-4 off
###   - OR, alias char-major-4 serial (?)
### + mv /etc/rc.d/*/*keytable ---> killed versions.
###   - prevent whining about tty0
### + must have dummy console if you expect things to work!!!
### + time/timezone is wrong is some cleverly bad way: userland problem.
### + sudo not working.
### + root password not Emulab-standard
### + gated
###   <http://ftp.rge.com/pub/networking/gated/gated-3-6.tar.gz>
###   <ftp://ftp.redhat.com/pub/redhat/linux/7.3/en/os/i386/SRPMS/gated-3.6-14.src.rpm>
### + Install userland traffic-shaping tools --- testbed/delay/linux
### + pathrate/rude/crude for `linktest' stuff; see Mike Hibler's 7/14 message.
### + Run the prepare script at the end.
### 

###############################################################################

## ...the recipe up to this point:
##  + has a broken FS.
##  + has broken network drivers.
##  + has Kudzu looking for HW updates.
##

## Attach via the console, and login as root.
##
# console ...

###############################################################################

# set local_build_root = "/build"
# set proj_root = "/proj/testbed"

local_build_root="/build"
local_install_root="${local_build_root}/install"
proj_root="/proj/testbed"

share_root="/share"
rhl_root="${share_root}/redhat/9.0"

image_root="${proj_root}/ee/image"

tz=MST7MDT

## Allow kudzu to install networking drivers?
## --- Yes, when it boots, just say "yes" to everything.
# kudzu
# reboot

## Start networking.
##
# dhclient

## Sync the time now.
##
ntpdate ops.emulab.net

## Mount /proj/testbed to make some things easier.
##
cd /
mkdir -p "${proj_root}"
mount -o hard,intr,udp "fs:/q${proj_root}" "${proj_root}"

## Mount /share to make some things easier.
##
cd /
mkdir -p "${share_root}"
mount -o hard,intr,udp "fs:${share_root}" "${share_root}"

## Turn the fourth partition into disk space for building things locally.
##
cd /
mkdir -p "${local_build_root}"
mkfs /dev/hda4
# Mark the partition as a Linux FS (id 83).
sfdisk --change-id /dev/hda 4 83
mount /dev/hda4 "${local_build_root}"

###############################################################################

## RPMS:
## RPMS at <http://updates.redhat.com/9/en/os/>:

## Install updated RPMs.
## See <https://rhn.redhat.com/errata/rh9-errata.html>

# rpm_options="-U"

## 2003-03-31 	RHSA-2003:120 Updated sendmail packages fix vulnerability
## 2003-04-01 	RHSA-2003:101 Updated OpenSSL packages fix vulnerabilities
## 2003-04-01 	RHSA-2003:095 New samba packages fix security vulnerabilities
## 2003-04-01 	RHSA-2003:084 Updated vsftpd packages re-enable tcp_wrappers
##                            support
## 2003-04-02 	RHSA-2003:091 Updated kerberos packages fix various
##                            vulnerabilities
## 2003-04-03 	RHSA-2003:128 Updated Eye of GNOME packages fix vulnerability
## 2003-04-03 	RHSA-2003:109 Updated balsa and mutt packages fix
##                            vulnerabilities
## 2003-04-08 	RHSA-2003:135 Updated 2.4 kernel fixes USB storage
## 2003-04-09 	RHSA-2003:139 Updated httpd packages fix security
##                            vulnerabilities.
## 2003-04-09 	RHSA-2003:137 New samba packages fix security vulnerability
## 2003-04-09 	RHBA-2003:136 glibc bugfix errata
## 2003-04-09 	RHBA-2003:080 Updated RHN Notification Tool available
## 2003-04-23 	RHSA-2003:076 Updated ethereal packages fix security
##                            vulnerabilities
## 2003-04-24 	RHSA-2003:142 Updated LPRng packages fix psbanner vulnerability
## 2003-04-24 	RHSA-2003:112 Updated squirrelmail packages fix cross-site
##                            scripting vulnerabilities
## 2003-05-02 	RHSA-2003:093 Updated MySQL packages fix vulnerabilities
## 2003-05-12 	RHSA-2003:002 Updated KDE packages fix security issues
## 2003-05-13 	RHSA-2003:160 Updated xinetd packages fix a denial-of-service
##                            attack and other bugs
## 2003-05-14 	RHSA-2003:172 Updated 2.4 kernel fixes security vulnerabilities
##                            and various bugs
## 2003-05-15 	RHSA-2003:174 Updated tcpdump packages fix privilege dropping
##                            error
## 2003-05-16 	RHSA-2003:169 Updated lv packages fix vulnerability
## 2003-05-20 	RHSA-2003:175 Updated gnupg packages fix validation bug
## 2003-05-27 	RHSA-2003:171 Updated CUPS packages fix denial of service
##                            attack
## 2003-05-28 	RHSA-2003:186 Updated httpd packages fix Apache security
##                            vulnerabilities
## 2003-05-30 	RHSA-2003:181 Updated ghostscript packages fix vulnerability
## 2003-06-03 	RHSA-2003:187 Updated 2.4 kernel fixes vulnerabilities and
##                            driver bugs
## 2003-06-03 	RHSA-2003:047 Updated kon2 packages fix buffer overflow
## 2003-06-09 	RHBA-2003:185 Updated printer database available
## 2003-06-23 	RHBA-2003:140 Updated bash packages fix several bugs
## 2003-06-25 	RHSA-2003:173 Updated ypserv packages fix a denial of service
##                            vulnerability
## 2003-06-26 	RHBA-2003:211 Updated redhat-config-date package available
## 2003-07-02 	RHSA-2003:204 Updated PHP packages are now available
## 2003-07-03 	RHSA-2003:203 Updated Ethereal packages fix security issues
## 2003-07-07 	RHBA-2003:125 New redhat-config-printer packages available
## 2003-07-08 	RHBA-2003:127 Updated print-queue manager packages
## 2003-07-17 	RHSA-2003:196 Updated Xpdf packages fix security vulnerability.
## 2003-07-21 	RHSA-2003:238 Updated 2.4 kernel fixes vulnerabilities
## 2003-07-23 	RHSA-2003:234 Updated semi packages fix vulnerability
## 2003-07-29 	RHSA-2003:222 Updated openssh packages available
## 2003-07-30 	RHSA-2003:206 Updated nfs-utils packages fix denial of service
##                            vulnerability
## 2003-08-04 	RHSA-2003:251 New postfix packages fix security issues.
## 2003-08-05 	RHSA-2003:126 Updated gtkhtml packages fix vulnerability
## 2003-08-08 	RHSA-2003:255 up2date improperly checks GPG signature of
##                            packages
## 2003-08-11 	RHSA-2003:241 Updated ddskk packages fix temporary file
##                            vulnerability
## 2003-08-11 	RHSA-2003:235 Updated KDE packages fix security issue
## 2003-08-11 	RHBA-2003:183 Updated redhat-config-network package available
## 2003-08-12 	RHSA-2003:108 Updated Evolution packages fix multiple
##                            vulnerabilities
## 2003-08-15 	RHSA-2003:199 Updated unzip packages fix trojan vulnerability
## 2003-08-18 	RHBA-2003:252 cdrtools bugfix release for locking problems
## 2003-08-20 	RHBA-2003:263 Updated 2.4 kernel resolves obscure bugs.
## 2003-08-21 	RHSA-2003:258 GDM allows local user to read any file.
## 2003-08-26 	RHSA-2003:261 Updated pam_smb packages fix remote buffer
##                            overflow.
## 2003-08-28 	RHSA-2003:265 Updated Sendmail packages fix vulnerability.
## 2003-08-29 	RHSA-2003:267 New up2date available with updated SSL
##                            certificate authority file
## 2003-09-04 	RHSA-2003:240 Updated httpd packages fix Apache security
##                            vulnerabilities
## 2003-09-09 	RHSA-2003:264 Updated gtkhtml packages fix vulnerability
## 2003-09-11 	RHSA-2003:273 Updated pine packages fix vulnerabilities
## 2003-09-16 	RHSA-2003:269 Updated KDE packages fix security issues
## 2003-09-17 	RHSA-2003:283 Updated Sendmail packages fix vulnerability.
## 2003-09-17 	RHSA-2003:279 Updated OpenSSH packages fix potential
##                            vulnerabilities
## 2003-09-17 	RHBA-2003:276 Updated printer configuration tool fixes SMB
##                            problems
## 2003-09-24 	RHBA-2003:179 Mailman RPM does not properly handle package
##                            installation and upgrade.
## 2003-09-30 	RHSA-2003:292 Updated OpenSSL packages fix vulnerabilities
## 2003-10-03 	RHSA-2003:256 Updated Perl packages fix security issues.
## 2003-10-09 	RHSA-2003:281 Updated MySQL packages fix vulnerability
## 2003-10-14 	RHBA-2003:247 Updated SANE packages prevent hardware damage
## 2003-11-03 	RHSA-2003:309 Updated fileutils/coreutils package fix ls
##                            vulnerabilities
## 2003-11-03 	RHSA-2003:275 Updated CUPS packages fix denial of service
## 2003-11-10 	RHSA-2003:323 Updated Ethereal packages fix security issues
## 2003-11-13 	RHSA-2003:325 Updated glibc packages provide security and bug
##                            fixes
## 2003-11-13 	RHSA-2003:313 Updated PostgreSQL packages fix buffer overflow
## 2003-11-13 	RHSA-2003:307 Updated zebra packages fix security
##                            vulnerabilities
## 2003-11-17 	RHSA-2003:342 Updated EPIC packages fix security vulnerability
## 2003-11-17 	RHSA-2003:288 Updated XFree86 packages provide security and bug
##                            fixes
## 2003-11-24 	RHSA-2003:316 Updated iproute packages fix local security
##                            vulnerability
## 2003-11-24 	RHSA-2003:311 Updated Pan packages fix denial of service
##                            vulnerability
## 2003-12-01 	RHSA-2003:392 Updated 2.4 kernel fixes privilege escalation
##                            security vulnerability
## 2003-12-02 	RHSA-2003:335 Updated Net-SNMP packages fix security and other
##                            bugs
## 2003-12-04 	RHSA-2003:398 New rsync packages fix remote security
##                            vulnerability
## 2003-12-10 	RHSA-2003:390 Updated gnupg packages disable ElGamal keys
## 2003-12-16 	RHSA-2003:403 Updated lftp packages fix security vulnerability
## 2003-12-16 	RHSA-2003:320 Updated httpd packages fix Apache security
##                            vulnerabilities
## 2003-12-23 	RHBA-2003:394 Updated 2.4 kernel fixes various bugs
## 2004-01-05 	RHSA-2003:417 Updated kernel resolves security vulnerability
## 2004-01-07 	RHSA-2004:006 Updated kdepim packages resolve security
##                            vulnerability
## 2004-01-07 	RHSA-2004:001 Updated Ethereal packages fix security issues
## 2004-01-12 	RHSA-2004:003 Updated CVS packages fix minor security issue
## 2004-01-15 	RHSA-2004:007 Updated tcpdump packages fix various
##                            vulnerabilities
## 2004-01-21 	RHSA-2004:034 Updated mc packages resolve buffer overflow
##                            vulnerability
## 2004-01-22 	RHSA-2004:040 Updated slocate packages fix vulnerability
## 2004-01-26 	RHSA-2004:032 Updated Gaim packages fix various vulnerabiliies
## 2004-02-05 	RHSA-2004:030 Updated NetPBM packages fix multiple temporary
##                            file vulnerabilities
## 2004-02-05 	RHSA-2004:020 Updated mailman packages close cross-site
##                            scripting vulnerabilities
## 2004-02-11 	RHSA-2004:051 Updated mutt packages fix remotely-triggerable
##                            crash
## 2004-02-13 	RHSA-2004:059 Updated XFree86 packages fix privilege escalation
##                            vulnerability
## 2004-02-13 	RHSA-2004:048 Updated PWLib packages fix protocol security
##                            issues
## 2004-02-18 	RHSA-2004:065 Updated kernel packages resolve security
##                            vulnerabilities
## 2004-02-26 	RHSA-2004:063 Updated mod_python packages fix denial of service
##                            vulnerability
## 2004-03-01 	RHBA-2004:043 Updated SANE packages fix problem with shared
##                            libraries
## 2004-03-03 	RHSA-2004:091 Updated libxml2 packages fix security
##                            vulnerability
## 2004-03-10 	RHSA-2004:102 Updated gdk-pixbuf packages fix denial of service
##                            vulnerability
## 2004-03-10 	RHSA-2004:093 Updated sysstat packages fix security
##                            vulnerabilities
## 2004-03-10 	RHSA-2004:075 Updated kdelibs packages resolve cookie security
##                            issue
## 2004-03-17 	RHSA-2004:121 Updated OpenSSL packages fix vulnerabilities
## 2004-03-17 	RHSA-2004:112 Updated Mozilla packages fix security issues
## 2004-03-18 	RHBA-2004:083 Updated grep package speeds UTF-8 searching
## 2004-03-29 	RHSA-2004:134 Updated squid package fixes security
##                            vulnerability
## 2004-03-31 	RHSA-2004:137 Updated Ethereal packages fix security issues
## 2004-04-14 	RHSA-2004:158 Updated cadaver package fixes security
##                            vulnerability in neon
## 2004-04-15 	RHSA-2004:159 Updated Subversion packages fix security
##                            vulnerability in neon
## 2004-04-17 	RHSA-2004:154 Updated CVS packages fix security issue
## 2004-04-21 	RHSA-2004:166 Updated kernel packages resolve security
##                            vulnerabilities
## 2004-04-30 	RHSA-2004:182 Updated httpd packages fix mod_ssl security issue
## 2004-04-30 	RHSA-2004:181 Updated libpng packages fix crash
## 2004-04-30 	RHSA-2004:179 An updated LHA package fixes security
##                            vulnerabilities
## 2004-04-30 	RHSA-2004:177 An updated X-Chat package fixes vulnerability in
##                            Socks-5 proxy
## 2004-04-30 	RHSA-2004:175 Updated utempter package fixes vulnerability
## 2004-04-30 	RHSA-2004:173 Updated mc packages resolve several
##                            vulnerabilities
## 2004-04-30  	RHSA-2004:163 Updated OpenOffice packages fix security
##                            vulnerability in neon

# Download yum for RHL 90
# See <http://www.linux.duke.edu/projects/yum/download.ptml>
cd "${local_build_root}"
wget http://www.linux.duke.edu/projects/yum/download/2.0/yum-2.0.7-1.noarch.rpm
rpm -U yum-2.0.7-1.noarch.rpm

# Remove things that we don't want.
# (This takes a long time; `yum' downloads a lot of RPM headers.)
#
gmake -f "${image_root}"/yum/RHL90-yum-remove.mk remove

# Update everything.
#
# yum check-update eventually says:
# Name                                Arch   Version                  Repo
# ---------------------------------------------------------------------------
# XFree86                             i386   4.3.0-2.90.55            updates
# XFree86-100dpi-fonts                i386   4.3.0-2.90.55            updates
# XFree86-75dpi-fonts                 i386   4.3.0-2.90.55            updates
# XFree86-Mesa-libGL                  i386   4.3.0-2.90.55            updates
# XFree86-Mesa-libGLU                 i386   4.3.0-2.90.55            updates
# XFree86-base-fonts                  i386   4.3.0-2.90.55            updates
# XFree86-devel                       i386   4.3.0-2.90.55            updates
# XFree86-font-utils                  i386   4.3.0-2.90.55            updates
# XFree86-libs                        i386   4.3.0-2.90.55            updates
# XFree86-libs-data                   i386   4.3.0-2.90.55            updates
# XFree86-tools                       i386   4.3.0-2.90.55            updates
# XFree86-truetype-fonts              i386   4.3.0-2.90.55            updates
# XFree86-twm                         i386   4.3.0-2.90.55            updates
# XFree86-xauth                       i386   4.3.0-2.90.55            updates
# XFree86-xdm                         i386   4.3.0-2.90.55            updates
# XFree86-xfs                         i386   4.3.0-2.90.55            updates
# bash                                i386   2.05b-20.1               updates
# coreutils                           i386   4.5.3-19.0.2             updates
# cups                                i386   1:1.1.17-13.3.0.3        updates
# cups-libs                           i386   1:1.1.17-13.3.0.3        updates
# cvs                                 i386   1.11.2-17                updates
# ethereal                            i386   0.10.3-0.90.1            updates
# foomatic                            i386   2.0.2-15.1               updates
# gdk-pixbuf                          i386   1:0.22.0-6.1.0           updates
# gdm                                 i386   1:2.4.1.3-5.1            updates
# ghostscript                         i386   7.05-32.1                updates
# glibc                               i686   2.3.2-27.9.7             updates
# glibc-common                        i386   2.3.2-27.9.7             updates
# glibc-devel                         i386   2.3.2-27.9.7             updates
# gnupg                               i386   1.2.1-9                  updates
# grep                                i386   2.5.1-7.8                updates
# hpijs                               i386   1.3-32.1                 updates
# httpd                               i386   2.0.40-21.11             updates
# iproute                             i386   2.4.7-7.90.1             updates
# kernel                              i686   2.4.20-31.9              updates
# kernel-source                       i386   2.4.20-31.9              updates
# krb5-devel                          i386   1.2.7-14                 updates
# krb5-libs                           i386   1.2.7-14                 updates
# lftp                                i386   2.6.3-4                  updates
# lha                                 i386   1.14i-9.1                updates
# libpcap                             i386   14:0.7.2-7.9.1           updates
# libpng                              i386   2:1.2.2-20               updates
# libpng-devel                        i386   2:1.2.2-20               updates
# libpng10                            i386   1.0.13-11                updates
# libpng10-devel                      i386   1.0.13-11                updates
# libxml2                             i386   2.5.4-3.rh9              updates
# libxml2-devel                       i386   2.5.4-3.rh9              updates
# libxml2-python                      i386   2.5.4-3.rh9              updates
# mutt                                i386   5:1.4.1-3.3              updates
# net-snmp                            i386   5.0.9-2.90.1             updates
# netpbm                              i386   9.24-10.90.1             updates
# netpbm-devel                        i386   9.24-10.90.1             updates
# nfs-utils                           i386   1.0.1-3.9                updates
# nscd                                i386   2.3.2-27.9.7             updates
# openssh                             i386   3.5p1-11                 updates
# openssh-askpass                     i386   3.5p1-11                 updates
# openssh-askpass-gnome               i386   3.5p1-11                 updates
# openssh-clients                     i386   3.5p1-11                 updates
# openssh-server                      i386   3.5p1-11                 updates
# openssl                             i686   0.9.7a-20.2              updates
# openssl-devel                       i386   0.9.7a-20.2              updates
# pam_smb                             i386   1.1.6-9.9                updates
# perl                                i386   2:5.8.0-88.3             updates
# perl-CPAN                           i386   2:1.61-88.3              updates
# redhat-config-date                  noarch 1.5.15-1                 updates
# redhat-config-network               noarch 1.2.15-1                 updates
# redhat-config-network-tui           noarch 1.2.15-1                 updates
# redhat-config-printer               i386   0.6.47.11-1              updates
# redhat-config-printer-gui           i386   0.6.47.11-1              updates
# rhpl                                i386   0.93.4-1                 updates
# rsync                               i386   2.5.7-0.9                updates
# samba                               i386   2.2.7a-8.9.0             updates
# samba-client                        i386   2.2.7a-8.9.0             updates
# samba-common                        i386   2.2.7a-8.9.0             updates
# sendmail                            i386   8.12.8-9.90              updates
# slocate                             i386   2.7-2                    updates
# tcpdump                             i386   14:3.7.2-7.9.1           updates
# unzip                               i386   5.50-33                  updates
# up2date                             i386   3.1.23.2-1               updates
# up2date-gnome                       i386   3.1.23.2-1               updates
# utempter                            i386   0.5.5-2.RHL9.0           updates
# xinetd                              i386   2:2.3.11-1.9.0           updates
# ypserv                              i386   2.8-0.9E                 updates

yum -y update
# asks if this is OK; -y says assume yes.

# Add things that we do want.
#
gmake -f "${image_root}"/yum/RHL90-yum-install.mk install

# clean up the big cache in /var/cache/yum/
# now done at the end.
# XXX --- do "clean all" it now, to get better disk usage?
# yum clean all
# yum clean headers

###############################################################################

## Disable kudzu at boot-time.
## Removing it is maybe a better option, but:
## # rpm -e kudzu
## error: Failed dependencies:
##    kudzu >= 0.95 is needed by (installed) kernel-pcmcia-cs-3.1.31-13
##    kudzu is needed by (installed) redhat-config-network-tui-1.2.0-2
##    kudzu >= 0.99.50-1 is needed by (installed) redhat-config-xfree86-0.7.3-2
##
#
# XXX --- obsolete: these are removed when we remove `kudzu'
#
# mv /etc/rc.d/rc3.d/{S05kudzu,K95kudzu}
# mv /etc/rc.d/rc4.d/{S05kudzu,K95kudzu}
# mv /etc/rc.d/rc5.d/{S05kudzu,K95kudzu}

## Kill off CUPS.
##
#
# XXX --- obsolete: these are removed when we remove `cups'
#
# mv /etc/rc.d/rc2.d/{S90cups,K10cups}
# mv /etc/rc.d/rc3.d/{S90cups,K10cups}
# mv /etc/rc.d/rc4.d/{S90cups,K10cups}
# mv /etc/rc.d/rc5.d/{S90cups,K10cups}

## Install the proper fstab.
##
install -o root -g root -m 644 {"${image_root}",}/etc/fstab

## Install an appropriate /etc/inittab:
## - Run a getty in ttyS0
## - Do not run getty's for most virtual consoles (tty1, ...)
##
install -o root -g root -m 644 {"${image_root}",}/etc/inittab

## RHL90: run `ntp' at run levels 2--5.  (`/etc/inittab' described the levels.)
##
/sbin/chkconfig --level 2345 ntpd on

## Enable root logins on ttyS0.
##
install -o root -g root -m 400 {"${image_root}",}/etc/securetty

## Enable all wheel members to `sudo'.
##
install -o root -g root -m 440 {"${image_root}",}/etc/sudoers

## Log `ssh' logins to `users.emulab.net'.
##
install -o root -g root -m 644 {"${image_root}",}/etc/syslog.conf

###############################################################################

# See doc/uk-image.txt
# This is directions for modifying one of our images for another site.
# However, it is also a set of stuff that must be installed:
# 	/root/.ssh
# 	.pem files
# 	etc.
# 
# I think it might be the lack of .pem files that is hanging the event proxy.

# * /root/.cvsup/auth
#   Customize host/domain, leave password alone?
# 
install -d -o root -g root -m 750 /root/.cvsup
install -o root -g root -m 400 {"${image_root}",}/root/.cvsup/auth

# * /root/.ssh
#   Remove known_hosts if it exists.  Put in local boss root pub key.
#   Leave in our pub key if acceptible.
#
rm -f /root/.ssh/known_hosts
install -o root -g root -m 644 {"${image_root}",}/root/.ssh/authorized_keys

# * /etc/localtime
#   Copy the correct file over from /usr/share/zoneinfo
# 
rm -f /etc/localtime
install -o root -g root -m 644 "/usr/share/zoneinfo/${tz}" /etc/localtime

# * /etc/ssh/ssh_host*
#   Generate new host keys.  Actually, copy from an existing image if
#   available (i.e., we use a single host key across all images and OSes
#   within a testbed).
# 
install -o root -g root -m 600 {"${image_root}",}/etc/ssh/ssh_host_dsa_key
install -o root -g root -m 644 {"${image_root}",}/etc/ssh/ssh_host_dsa_key.pub
install -o root -g root -m 600 {"${image_root}",}/etc/ssh/ssh_host_key
install -o root -g root -m 644 {"${image_root}",}/etc/ssh/ssh_host_key.pub
install -o root -g root -m 600 {"${image_root}",}/etc/ssh/ssh_host_rsa_key
install -o root -g root -m 644 {"${image_root}",}/etc/ssh/ssh_host_rsa_key.pub
#
# XXX --- ?
# install -o root -g root -m 644 {"${image_root}",}/etc/ssh/ssh_config
# install -o root -g root -m 600 {"${image_root}",}/etc/ssh/sshd_config

# * /etc/emulab/shadow
#   Change the root password, this file will get installed by prepare.
# 
install -d -o root -g root -m 755 /etc/emulab
install -o root -g root -m 600 {"${image_root}",}/etc/emulab/shadow

# * /etc/emulab/{client,emulab}.pem
#   Generate new ones.  This needs to be done on the site's boss node.
#   Go into the source tree "ssl" subdirectory and edit the *.cnf.in files
#   to update the "[ req_distinguished_name ]" section with the appropriate
#   country, state, city, etc.  Then go to the build directory and do a
#   "gmake boss-installX" which generates the certs and installs the
#   server-side.  Grab the emulab.pem and localhost.pem from that directory
#   to put in the images as emulab.pem and client.pem.  [ NOTE: we can
#   get by without the certs if the client tmcc and server tmcd are built
#   without SSL support (tmcc-nossl and tmcd-nossl targets).
# 
install -o root -g root -m 440 {"${image_root}",}/etc/emulab/client.pem
install -o root -g root -m 440 {"${image_root}",}/etc/emulab/emulab.pem

# * Remount root filesystem read-only (IMPORTANT!)
#   cd /
#   mount -o remount,ro /
# 
# * Fsck it for good luck.  Actually, not only good luck but also resets
#   some time stamp that forces an fsck periodically
#   fsck -f <root device>

###############################################################################

# We must look in /usr/local/lib.
#
# XXX --- hack.  We'd like to *add* /usr/local/lib to the path, not replace the
# whole file.
#
install -o root -g root -m 644 {"${image_root}",}/etc/ld.so.conf

###############################################################################

## From: Mike Hibler <mike@flux.utah.edu>
## To: testbed-dev@flux.utah.edu
## Subject: client build changes
## Date: Thu, 24 Jun 2004 00:59:21 -0600 (MDT)
## 
## I just checked in a heap o' changes related to getting the emulab client
## side to build/install a little better:
## ----
## 
##   Improve the client-side install.  With these changes, it should now be
##   possible to:
##   
##         gmake client
##         sudo gmake client-install
##   
##   on a FBSD4, FBSD5, RHL7.3, and RHL9.0 client node.
##   
##   There are still some dependencies that are not explicit and which would
##   prevent a build/install from working on a "clean" OS.  Two that I know of
##   are: you must install our version of the elvin libraries and you must
##   install boost.

## Install "our version of the elvin libraries."
##
cd "${local_build_root}"
wget "http://www.emulab.net/downloads/libelvin-4.0.3.tar.gz"
tar zxf libelvin-4.0.3.tar.gz
wget "http://www.emulab.net/downloads/libelvin-4.0.3.patch"
patch -p0 < libelvin-4.0.3.patch
cd libelvin-4.0.3
#
# Ancient software.  Without `-fno-strict-aliasing', the `configure' script
# dies.  It's not at all clear to me if `-fno-strict-aliasing' is required for
# elvin itself.
export CFLAGS="-g -O2 -fno-strict-aliasing"
#
# Need these to find the Kerberos support, for SSL stuff.
export CPPFLAGS="`/usr/kerberos/bin/krb5-config --cflags`"
export LIBS="`/usr/kerberos/bin/krb5-config --libs`"
./configure
gmake
gmake install
unset CFLAGS CPPFLAGS LIBS

## We now also require the actual `elvind'.
##
cd "${local_build_root}"
wget "http://www.emulab.net/downloads/elvind-4.0.3.tar.gz"
tar zxf elvind-4.0.3.tar.gz
wget "http://www.emulab.net/downloads/elvind-4.0.3.patch"
patch -p0 < elvind-4.0.3.patch
cd elvind-4.0.3
./configure
gmake
gmake install

## Install Boost.
## See <http://www.boost.org/more/getting_started.html>
##
cd "${local_build_root}"
cp -p "${image_root}"/boost_1_31_0.tar.bz2 .
# wget "http://unc.dl.sourceforge.net/sourceforge/boost/boost_1_31_0.tar.bz2"
tar jxf boost_1_31_0.tar.bz2
# First, one must build their builder, `bjam'.
cd boost_1_31_0/tools/build/jam_src
sh ./build.sh
install -m 755 bin.linuxx86/bjam /usr/local/bin
ln /usr/local/bin/bjam /usr/local/bin/jam
cd "${local_build_root}"/boost_1_31_0
bjam -sTOOLS=gcc --with-python-root=/usr install
# I'm not sure if this is standard, but it is needed for the Emulab client
# stuff:
#
ln -s boost-1_31/boost /usr/local/include/boost

## Install the Emulab client stuff.  XXX --- the current way of getting the
## source is icky.
##
export CVS_RSH=ssh
cd "${local_build_root}"
# cvs -d ... co testbed
#
cd testbed
./configure
gmake client
gmake client-install

## Install `gated'.
## XXX --- compile w/o debugging?
##

#cd "${local_build_root}"
## wget http://ftp.rge.com/pub/networking/gated/gated-3-6.tar.gz
#wget http://www.funet.fi/pub/unix/tcpip/gated/gated-3-6.tar.gz
#tar zxf gated-3-6.tar.gz
#cd gated-public-3_6
#./configure
#gmake depend
#gmake
#gmake install
## Installs just `/usr/local/sbin/gated'.

# Version built from source doesn't work, install RHL7.3 RPM instead
rpm -i /share/redhat/7.3/RPMS/gated-3.6-14.i386.rpm

###############################################################################

## RHL90: Install a hacked version of `shadow-utils'.
##
## The RHL90 `shadow-utils' RPM (shadow-utils-4.0.3-6) contains a version of
## `groupadd' that doesn't accept upper-case letters in group names.  Foo.
##
## XXX --- We can't remove the RPM without first removing half the known
## universe --- so we don't.  We just install over the top of it.  Ick.
##

cd "${local_build_root}"
wget ftp://ftp.pld.org.pl/software/shadow/shadow-4.0.4.1.tar.bz2
tar jxf shadow-4.0.4.1.tar.bz2
cd shadow-4.0.4.1
patch -p1 < "${image_root}"/shadow-utils_mods/uppercase.patch
export CFLAGS="-O2"
# ./configure
# From the spec file:
# XXX --- build w/o -g
./configure --disable-shared --prefix=/usr --exec-prefix=/usr
gmake
# gmake install
# no decent install...
cd src
install -o root -g root -m 755 \
    chpasswd groupadd groupdel groupmod grpck grpconv grpunconv newusers pwck \
    pwconv pwunconv useradd userdel usermod \
    /usr/sbin
unset CFLAGS
#
# Further ick: remove the `CREATE_HOME' option from `/etc/login.defs'.  The new
# `useradd' whines about this option when it is found in the configuration
# file.  The relevant comment (in `src/useradd.c'):
#		/*
#		 * RedHat added the CREATE_HOME option in login.defs in their
#		 * version of shadow-utils (which makes -m the default, with
#		 * new -M option to turn it off). Unfortunately, this
#		 * changes the way useradd works (it can be run by scripts
#		 * expecting some standard behaviour), compared to other
#		 * Unices and other Linux distributions, and also adds a lot
#		 * of confusion :-(.
#		 * So we now recognize CREATE_HOME and give a warning here
#		 * (better than "configuration error ... notify administrator"
#		 * errors in every program that reads /etc/login.defs). -MM
#		 */
#
install -o root -g root -m 644 {"${image_root}",}/etc/login.defs

###############################################################################

## Install new lilo.
## Install boot.b, system.b, os2_d.b
##
install -o root -g root -m 755 {"${image_root}",}/sbin/lilo
install -o root -g root -m 644 {"${image_root}",}/boot/boot.b
install -o root -g root -m 644 {"${image_root}",}/boot/chain.b
install -o root -g root -m 644 {"${image_root}",}/boot/os2_d.b

## Install gcc 2.95.3 --- ugh.
## (With gcc 3.2.2 as installed on RH 9, modules won't build.)
##
cd ${local_build_root}
tar zxf ${image_root}/gcc-2.95.3.tar.gz
# Patch for glibc 2.2.
# patch -p0 < ${image_tars}/egcs-1.1.2-glibc-2.2.patch
cd gcc-2.95.3
./configure --prefix="${local_install_root}"
make bootstrap >& Make-bootstrap.errs
make install >& Make-install.errs

## Use gcc 2.95.
oldpath="$PATH"
PATH="${local_install_root}/bin":"$PATH"

kernel_root="linux-2.4.20-31.9"

###############################################################################

## Build the normal Emulab-aware kernel.
##
kernel_version="2.4.20-31.9emulab"
#
cd "${local_build_root}"
cp -pR /usr/src/"${kernel_root}" ./linux-"${kernel_version}"

## Apply the Emulab IPOD patch.
##
cd "${local_build_root}/linux-${kernel_version}"
# patch -p0 < "${local_build_root}"/testbed/ipod/patch-linux-2.4.20-31.9
patch -p0 < "${image_root}"/kmod-linux-2.4.20-31.9/ipod.patch

## Apply the variable-Hz and 64-bit jiffies patch.
##
patch -p1 < "${image_root}"/kmod-linux-2.4.20-31.9/vhz-j64.patch

## Apply the Emulab linkdelay patch.
##
# XXX --- should make a new patch file with the right name!
# patch -p1 < "${local_build_root}"/testbed/delay/linux/kmods/patch-combined-linkdelays-2.4.18
patch -p1 < "${image_root}"/kmod-linux-2.4.20-31.9/linkdelay.patch

## Apply the Emulab kernel-version patch.
##
patch -p1 < "${image_root}"/kmod-linux-2.4.20-31.9/version-emulab.patch

cd "${local_build_root}/linux-${kernel_version}"

#    IMQ target support (CONFIG_IP_NF_TARGET_IMQ) [N/m/?] (NEW) 
# ICMP: ICMP Ping-of-Death (Emulab) (CONFIG_ICMP_PINGOFDEATH) [N/y/?] (NEW)
#    IMQ target support (CONFIG_IP6_NF_TARGET_IMQ) [N/m/?] (NEW) 
#  DELAY packet scheduler (CONFIG_NET_SCH_DELAY) [N/y/m/?] (NEW) 
#  PLR packet scheduler (CONFIG_NET_SCH_PLR) [N/y/m/?] (NEW) 
# IMQ (intermediate queueing device) support (CONFIG_IMQ) [N/y/m/?] (NEW) 

gmake clean
gmake mrproper
cp "${image_root}"/kconfig-linux-2.4.20-31.9/emulab.config ./.config
gmake oldconfig
gmake dep
gmake bzImage
gmake modules

gmake modules_install

# installs
#  vmlinuz
#  System.map
#  initrd
gmake install
# but we also want
#  vmlinux
#  module-info
#  config
install -o root -g root -m 644 vmlinux "/boot/vmlinux-${kernel_version}"
install -o root -g root -m 644 .config "/boot/config-${kernel_version}"
#
# for module-info, see
# <http://linux-universe.com/HOWTO/Kernel-HOWTO/kernel_files_info.html>

###############################################################################

## Build the linkdelay Emulab-aware kernel.
##
kernel_version="2.4.20-31.9linkdelay"
#
cd "${local_build_root}"
cp -pR /usr/src/"${kernel_root}" ./linux-"${kernel_version}"

## Apply the Emulab IPOD patch.
##
cd "${local_build_root}/linux-${kernel_version}"
# patch -p0 < "${local_build_root}"/testbed/ipod/patch-linux-2.4.20-31.9
patch -p0 < "${image_root}"/kmod-linux-2.4.20-31.9/ipod.patch

## Apply the variable-HZ and 64-bit jiffies patch.
##
patch -p1 < "${image_root}"/kmod-linux-2.4.20-31.9/vhz-j64.patch

## Apply the Emulab linkdelay patch.
##
# XXX --- should make a new patch file with the right name!
# patch -p1 < "${local_build_root}"/testbed/delay/linux/kmods/patch-combined-linkdelays-2.4.18
patch -p1 < "${image_root}"/kmod-linux-2.4.20-31.9/linkdelay.patch

## Apply the Emulab linkdelay kernel-version patch.
##
patch -p1 < "${image_root}"/kmod-linux-2.4.20-31.9/version-linkdelay.patch

cd "${local_build_root}/linux-${kernel_version}"

#    IMQ target support (CONFIG_IP_NF_TARGET_IMQ) [N/m/?] (NEW) 
# ICMP: ICMP Ping-of-Death (Emulab) (CONFIG_ICMP_PINGOFDEATH) [N/y/?] (NEW)
#    IMQ target support (CONFIG_IP6_NF_TARGET_IMQ) [N/m/?] (NEW) 
#  DELAY packet scheduler (CONFIG_NET_SCH_DELAY) [N/y/m/?] (NEW) 
#  PLR packet scheduler (CONFIG_NET_SCH_PLR) [N/y/m/?] (NEW) 
# IMQ (intermediate queueing device) support (CONFIG_IMQ) [N/y/m/?] (NEW) 

gmake clean
gmake mrproper
cp "${image_root}"/kconfig-linux-2.4.20-31.9/linkdelay.config ./.config
gmake oldconfig
gmake dep
gmake bzImage
gmake modules

gmake modules_install

# installs
#  vmlinuz
#  System.map
#  initrd
gmake install
# but we also want
#  vmlinux
#  module-info
#  config
install -o root -g root -m 644 vmlinux "/boot/vmlinux-${kernel_version}"
install -o root -g root -m 644 .config "/boot/config-${kernel_version}"
#
# for module-info, see
# <http://linux-universe.com/HOWTO/Kernel-HOWTO/kernel_files_info.html>

## RESET THE PATH
PATH="${oldpath}"

###############################################################################

## Install the new `/etc/lilo.conf'.
##
install -o root -g root -m 644 {"${image_root}",}/etc/lilo.conf

# Rerun lilo
/sbin/lilo -v

###############################################################################

## Install the linkdelay userland stuff.
##

# RHL9 comes with RPMs:
#   iptables-1.2.7a-2
#   iproute-2.4.7-7.90.1
#
# Remove the iptables RPM, to remove a possible source of confusion.  Note that
# the RPM stuff in instaleld in /sbin and /lib, mostly, whereas our
# custom-built stuff is installed in /usr/local/{sbin,lib}.  This is a good
# thing, I think!
#
# XXX --- We can't remove `iproute' RPM without removing half of all the other
# RPMs in the system.  Blech.  So, we'll just leave it there.
#
yum -y remove iptables
# XXX need/want to salvage and/or customize `/etc/rc.d/init.d/iptables'?

## Build and install `iproute' stuff.
##

# XXX --- icky way to get the sources.
cd "${local_build_root}"
wget http://public.planetmirror.com/pub/ip-routing/iproute2-2.4.7-now-ss020116-try.tar.gz
# README in the package asays:
# Primary FTP site is: ftp://ftp.inr.ac.ru/ip-routing/
#
tar zxf iproute2-2.4.7-now-ss020116-try.tar.gz
cd iproute2
patch -p1 < /build/testbed/delay/linux/tc_mods/patch-combined-iproute2-2.4.7-now-ss020116-try
# XXX --- Build w/o -g?
defs="KERNEL_INCLUDE=/build/linux-2.4.20-31.9linkdelay/include SBINDIR=/usr/local/sbin CONFDIR=/usr/local/etc/iproute2 DOCDIR=/usr/local/share/doc/iproute2"
gmake $defs
# Go out of our way to install this stuff in `/usr/local/*', to avoid clashing
# with RPM versions.
gmake install $defs
# `ifcfg' is part of the RPM, but isn't installed by `gmake install'.  Go
# figure.
install -o root -g root -m 755 ip/ifcfg /usr/local/sbin

## Build and install `iptables' stuff.
##

cd "${local_build_root}"
wget http://www.netfilter.org/files/iptables-1.2.11.tar.bz2
tar jxf iptables-1.2.11.tar.bz2
cd iptables-1.2.11
# patch applies cleanly --- hooray!
# XXX --- BUT: need to rename four instances of NETFILTER_VERSION to
# IPTABLES_VERSION in extsnions/lib*_IMQ.c
# patch -p1 < /build/testbed/delay/linux/iptables_mods/iptables-1.2.6a-imq.diff-3
patch -p1 < "${image_root}"/iptables_mods/iptables-1.2.11-imq.patch
chmod +x extensions/.IMQ-test extensions/.IMQ-test6

# cd "${local_build_root}"
# wget ftp://ftp.netfilter.org/pub/patch-o-matic/patch-o-matic-20031219.tar.bz2
# tar jxf patch-o-matic-20031219.tar.bz2
# cd patch-o-matic
# KERNEL_DIR=/build/linux-2.4.20-31.9linkdelay ./runme pending
#
# When run, it basically says that the kernel sources are up to date for a
# circa-2.4.20 kernel.  A snippet of the output:
#
# Already applied: submitted/01_2.4.19
#                  submitted/02_2.4.20
#                  submitted/03_2.4.21
# 
# Testing... 04_2.4.22.patch NOT APPLIED (4 missing files)
# The submitted/04_2.4.22 patch:
#    Authors: Various (see below)
#    Status: Included in stock 2.4.22 kernel
#    
#    This big patch contains all netfilter/iptables changes between stock
#    kernel versions 2.4.21 and 2.4.22.
# [...]

# ...so, basically, the kernel is up to date for iptables 1.2.11
#
cd /build/iptables-1.2.11
gmake KERNEL_DIR=/build/linux-2.4.20-31.9linkdelay
gmake install KERNEL_DIR=/build/linux-2.4.20-31.9linkdelay
#? gmake install-devel

###############################################################################

## Install pathrate.
##
cd "${local_build_root}"
wget http://www.cc.gatech.edu/fac/Constantinos.Dovrolis/pathrate.tar.gz
tar zxf pathrate.tar.gz
cd pathrate_2.4.0
./configure
# XXX Build w/o -g?
gmake
# No install target
install -o root -g root -m 755 pathrate_rcv pathrate_snd /usr/local/bin
install -d -o root -g root -m 755 /usr/local/share/doc/pathrate
install -o root -g root -m 644 CHANGES README /usr/local/share/doc/pathrate

## Install rude/crude.
##
cd "${local_build_root}"
wget http://unc.dl.sourceforge.net/sourceforge/rude/rude-0.70.tar.gz
tar zxf rude-0.70.tar.gz 
cd rude
./configure
gmake
gmake install

###############################################################################

# Delete old kernels?
#
# ...
#
yum -y remove kernel-source
# XXX --- breaks

# clean up the big cache in /var/cache/yum/
yum clean all
# yum clean headers

## Clean up crap from root's home directory!
##
cd /root
rm anaconda-ks.cfg
rm .bash_history
rm -rf .emacs.d
rm -rf .gconfd
rm .viminfo
rm install.log
rm install.log.syslog

unset HISTFILE

# Run the final preparation script.
# XXX --- Do this is single-user mode?
#
/etc/rc.d/init.d/anacron stop
#
cd "${local_build_root}"/testbed
./tmcd/linux/prepare

cd /
umount "${local_build_root}"
rmdir "${local_build_root}"

###############################################################################

ldconfig
updatedb

###############################################################################

## End of file.


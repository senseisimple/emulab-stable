<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Anyone can run this page. No login is needed.
# 
PAGEHEADER("Netbed CDROM Installation Notes");

?>
The Netbed installation CDROM will <font color=red>wipe your hard
disk</font> and install <a href=www.freebsd.org>FreeBSD 4.6</a> on the
boot disk. This version of FreeBSD is standard in most respects, but
includes some additional software that allows researchers at other
sites to use the machine to conduct network experiments, as part of a
<em>widearea testbed</em>.  Cool features include:

<br>
<ul>
<li> an always-boot-first-from-CDROM process that provides robust recovery
from catastrophic node failure (such as disk failure).
<li> automatic software upgrade each time your node boots (from the CD).
<li> Installation from files cached on the CDROM, <em>or</em> from
     files downloaded from netbed's web site. 
<li> in the future, support for Linux.
</ul>
<b>System Requirements:</b>
<ul>
<li> reasonably modern x86 PC that can boot from the CDROM.
<li> 8 gigabyte or larger disk drive. IDE, SCSI, RAID are all acceptable.
<li> standard ethernet card. 10Mb, 100Mb, or 1000Mb are acceptable.
<li> direct access to the internet; your machine <b>cannot</b> be
     behind a NAT box and should not be behind a firewall.
</ul>

<b>Installation Instructions:</b>
<br>
<br>
Before you can begin the installation, you must first request a
<a href=cdromnewkey.php>key to unlock your CD.</a> Next you must
determine the following information about your machine and your network:

<ul>
<li> device name of the boot disk, which is typically 'ad0' (IDE),
     'da0' (SCSI), 'ar0' (IDE RAID) or 'aacd0' (SCSI RAID). *
<li> device name of the external network interface, which is
     typically one of 'fxp0', 'de0', etc. *
<li> the hostname of your node.
<li> your fully qualified domain name.
<li> the IP address of your node.
<li> the IP address of your nameserver.
<li> the IP address of your gateway.
<li> your netmask (typically 255.255.255.0).
</ul>
<blockquote><blockquote>
<b>*</b> The installation script attempts to guess the most
appropriate boot device and network interface.
For the complete list of supported devices see the
<a href=http://www.freebsd.org/releases/4.6R/hardware-i386.html>
FreeBSD hardware compatibility list.</a>
</blockquote></blockquote>

<p>
Once you have this information, you should insert the Netbed
installation CD into the CDROM drive and reboot the computer.  You
will be prompted one last time to make sure that you really want to
continue and wipe the contents of your boot disk. If you answer yes,
you will be prompted for the above information, and the boot process
will continue. This information will be saved on the disk, so you will
not have to enter it again. 

<p>
After a few moments, you will see a prompt asking if you want to
"Dance with Netbed?" This prompt will timeout after a short delay, and
is intended to give you an opportunity to interrupt the reboot
process; each time your node reboots it will run from the CD
initially, contacting Netbed to see if there are any software
updates. If there are no updates scheduled, it will boot normally from
the disk. <em>Note: you <b>must</b> leave the CD in the CDROM at all
times.</em>

<p>
Next you will be prompted for your key to unlock the CD.  Since you
are doing an initial installation (which is very similar to needing a
software update), there will then be a delay while your boot disk is
wiped clean and FreeBSD 4.6 is installed. Depending on the speed of
your node and the speed of the disk, this could take several minutes,
perhaps as many as 10. During this process you will see a status
display indicating that something is actually happening. Please do not
interrupt this process! You will most likely see some warnings printed
by the kernel; you can safely ignore these. <em>Note: the CDROM
contains an initial image, so loading is fast. However, if that image
is out of date, a new image will be downloaded from netbed.org via
your network interface. The speed of that download is of course
dependent on the speed of your internet connection.</em>

<p>
After FreeBSD is installed a secondary filesystem will be created,
mounted on /users, to hold the home directories of researchers that
are conducting experiments on your node. This secondary filesystem
will use the remaining space on your boot disk; the amount of space
available is automatically determined and a new DOS slice is created
and labeled. On an 8GB disk, there is typically 2GB left after
installing FreeBSD. We also reserve an additional 1GB partition to
hold temporary files when upgrading your machine to new versions of
the operating system.

<p> 
Once installation is complete, the node will reboot. <em>Do not remove
the CDROM from the drive!</em> Each time the node reboots, it will run
briefly from the CDROM, checking in at netbed.org to see if there are
any software updates to install. Once updates are finished, the node
will reboot again, only this time it will boot from the disk and run
normally. In multiuser mode, your node will periodically check in at
netbed.org to see what accounts need to be created for external
researchers. All such communication is via an <b>encrypted SSL</b>
connection and users are required to use <b>ssh</b> to log into your
node. To avoid the use of passwords, netbed.org distributes ssh public
keys for all users, which can be updated dynamically via netbed.org's
website.

<br>
<br>
<b>Linux Support:</b>
<br>
<br>
Linux is not supported at this time. However, the required support is
minor (just some some config files and scripts) and we plan to add
that very soon. Please stay tuned for further information.

<?php
#
# Standard Footer.
# 
PAGEFOOTER();

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
PAGEHEADER("Netbed CDROM Installation");

SUBPAGESTART();
SUBMENUSTART("CDROM Options");
WRITESUBMENUBUTTON("Request a CD Key", "cdromnewkey.php");
WRITESUBMENUBUTTON("View Installation Instructions", "cdrominstallhelp.php");
WRITESUBMENUBUTTON("Download the latest CD image", "cdromdownload.php");
WRITESUBMENUBUTTON("Request a local account", "widearea_register.php");
SUBMENUEND();

echo "<h3 align=center>
       Welcome to the netbed.org CDROM installation page!
      </h3>\n";

SUBPAGEEND();

echo "<br>
      When loaded onto a PC outside your firewall, the Netbed CDROM will cause
      that machine to <b>join the <em>Netbed</em> open testbed</b>.  Doing so
      will contribute to a shared infrastructure for the network and
      distributed systems research community.  Non-root ssh access to these
      machines for research purposes is granted to any authenticated researcher
      and their students who have a valid need.  Accounts and machines are
      tightly managed via the central facility at <a href=\"http://netbed.org\">
      netbed.org</a>. The target domains include any research areas that need
      geographically distributed sites, such as overlay networks and Internet
      measurement.
      <br>
      <br>
      <b>If you have a Netbed CDROM</b> and would like to install it on a machine on
      your network, you will need a <a href=cdromnewkey.php>key</a>. (If you
      don't have a Netbed CDROM, you can <a href=\"cdromdownload.php\">download
      an image</a> and burn it to a CD.)
      This key will allow you to install the CD, and to join
      netbed.org's widearea testbed. By joining the testbed, you are allowing
      other researchers to use your machine.
      <br>
      You should also read the <a href=cdrominstallhelp.php>installation
      instructions</a> so you know what to expect during the installation.
      <br>
      <br>
      <font color=red>
        <b>WARNING:</b></font>
        The netbed installation CDROM will <font color=red> wipe your
        hard disk</font> and install <a href=www.freebsd.org>FreeBSD 4.6</a>
        on it. 
      </font>

      <h3>Details</h3>
      
      The Netbed installation CDROM will <b>wipe your hard disk</b> and install
      <a href=\"http://www.freebsd.org\">FreeBSD</a> 4.6 on the
      boot disk. This version of FreeBSD is standard in most respects, but
      includes software that lets it reliably participate in the wide-area
      testbed.  Features include:
      <ul>
	  <li>An always-boot-first-from-CD process that provides robust
	  	recovery from catastrophic node failure.</li>
	  <li>Authenticated remotely-initiated low-level reboot.
	  <li>Secure, automatic software installation and upgrade.
	  <li>Installation from files cached on the CDROM, or from files
	      downloaded from Netbed's web site.</li>
	  <li>Soon: support for Linux; site-specific bandwidth-limiting.</li>
      </ul>

      \n";

#
# Standard Footer.
# 
PAGEFOOTER();

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
SUBMENUEND();

echo "<blockquote><b><br>
       Welcome to the netbed.org CDROM installation page!
      </b></blockquote>\n";

SUBPAGEEND();

echo "<br>
      If you have a Netbed CDROM and would like to install it on a machine on
      your network, you will need a <a href=cdromnewkey.php>key</a>.
      This key will allow you to install the CD, and to join
      netbed.org's widearea testbed. By joining the testbed, you are allowing
      other researchers to use your machine.
      <br>
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
      \n";

#
# Standard Footer.
# 
PAGEFOOTER();

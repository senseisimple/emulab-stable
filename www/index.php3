<?php
#
# Beware empty spaces (cookies)!
# 
require("defs.php3");

#
# This page gets loaded either as the result of a login/logout click,
# or as a regular page cause its the base. We deal with login and
# logout right here. Once that is done, we can move onto writing the
# actual page contents. The current login status is checked when the
# menu bar is written, and by the pages themselves when they care.
# 
if (isset($login)) {
    #
    # Login button pressed. 
    #
    if (!isset($uid) ||
        strcmp($uid, "") == 0) {
            $login_status = $STATUS_LOGINFAIL;
	    unset($uid);
    }
    else {
	#
	# Look to see if already logged in. If the user hits reload,
	# we are going to get another login post, and this could
	# update the current login, but the other frame is also reloading,
	# and has sent its cookie values in already. So, now the hash in
	# DB will not match the hash that came with the other frame. 
	#
	if (CHECKLOGIN($uid) == 1) {
            $login_status = $STATUS_LOGGEDIN;
	}
	elseif (DOLOGIN($uid, $password)) {
            $login_status = $STATUS_LOGINFAIL;
	    unset($uid);
        }
        else {
            $login_status = $STATUS_LOGGEDIN;
        }
    }
}
elseif (isset($logout)) {
    #
    # Logout button pressed.
    #
    DOLOGOUT($uid);
    $login_status = $STATUS_LOGGEDOUT;
    unset($uid);
}

#
# Standard Testbed Header
#
PAGEHEADER("Home");

?>

<img src="pix/front-left-med.jpg" align="right" >

<p>
Welcome to Emulab. Emulab (sometimes called the Utah Network Testbed)
is a new and unique type of experimental environment: a
universally-available "Internet in a room" which will provide a new,
much anticipated balance between control and realism. Several hundred
PCs, combined with secure, user-friendly web-based tools, allow you
to remotely reserve, configure and control machines and links down to
the hardware level: error models, latency, bandwidth, packet ordering,
buffer space all can be user-defined. Even the operating system disk
contents can be securely and fully replaced with custom images.

<p>
The Testbed currently features high-speed Cisco switches connecting,
with over 2 miles of cabling, 160 end nodes
<a href = "http://www.research.digital.com/SRC/iag/">
(Compaq DNARD Sharks)</a> and 40 core nodes (PCs). The core nodes can be
used as end nodes, simulated routers or traffic-shaping nodes, or
traffic generators. During an experiment's time slots, the experiment
(and associated researchers) get exclusive use of the assigned
machines, including root access if desired.  Until we finish designing
and building smarter scheduling and state-saving software, and obtain
the disk space, scheduling is manual and done at coarse granularity
(days).

<p>
We provide some default software (e.g. Linux and FreeBSD on the PCs,
NetBSD on the Sharks) that many users may want. The basic software
configuration on your nodes includes accounts for project members,
root access, DNS service, compilers and linkers. But fundamentally,
the software you run on it, including all bits on the disks, is
replaceable and up to you.  The same applies to the network's
characteristics, including its topology: configurable by users.

<h3>Links to help you get started:</h3>
<ul>
<li><b><a href = "docwrapper.php3?docname=auth.html">
          Authorization Scheme, Policy, and "How To Get Started"</a></b>
<li><b><a href = "docwrapper.php3?docname=hardware.html">
          Hardware Overview</a></b>
<li><b><a href = "docwrapper.php3?docname=software.html">
          Software Overview</a></b>
<li><b><a href = "docwrapper.php3?docname=security.html">
          Security Issues</a></b>
<li><b><a href = "docwrapper.php3?docname=policies.html">
          Administrative Policies and Disclaimer</a></b>
<li><b><a href = "docwrapper.php3?docname=sponsors.html">
          Emulab Sponsors</a></b>
</ul>

<?php
#
# Standard Testbed Footer
# 
PAGEFOOTER();

?>


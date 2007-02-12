<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

$optargs = OptionalPageArguments("stayhome", PAGEARG_BOOLEAN);

#
# The point of this is to redirect logged in users to their My Emulab
# page. 
#
if (($this_user = CheckLogin($check_status))) {
    $check_status = $check_status & CHECKLOGIN_STATUSMASK;
    
    if (($firstinitstate = TBGetFirstInitState())) {
	unset($stayhome);
    }
    if (!isset($stayhome)) {
	if ($check_status == CHECKLOGIN_LOGGEDIN) {
	    if ($firstinitstate == "createproject") {
	        # Zap to NewProject Page,
 	        header("Location: $TBBASE/newproject.php3");
	    }
	    else {
		# Zap to My Emulab page.
		header("Location: $TBBASE/".
		       CreateURL("showuser", $this_user));
	    }
	    return;
	}
	elseif (isset($SSL_PROTOCOL)) {
            # Fall through; display the page.
	    ;
	}
	elseif ($check_status == CHECKLOGIN_MAYBEVALID) {
            # Not in SSL mode, so reload using https to see if logged in.
	    header("Location: $TBBASE/index.php3");
	}
    }
    # Fall through; display the page.
}

#
# Standard Testbed Header
#
PAGEHEADER("Emulab - Network Emulation Testbed Home",NULL,$RSS_HEADER_NEWS);

#
# Special banner message.
#
$message = TBGetSiteVar("web/banner");
if ($message != "") {
    echo "<center><font color=Red size=+1>\n";
    echo "$message\n";
    echo "</font></center><br>\n";
}

?>

<p><em>Emulab</em> provides
integrated access to a wide range of experimental environments:
from <a href="tutorial/docwrapper.php3?docname=nse.html">simulated</a> to
emulated to <a href=widearea.html>wide-area</a> network
testbeds.  Emulab strives to preserve the control and ease of use of
simulation, without sacrificing the realism of emulation and live
network experimentation. 
</p>

<p>
Emulab unifies all of these environments under a common user interface,
and integrates the three into a common framework.  This framework
provides abstractions, services, and namespaces common to all, such as
allocation and naming of nodes and links.  By mapping the abstractions
into domain-specific mechanisms and internal names, Emulab masks much
of the heterogeneity of the different approaches.
</p>

<p>
Emulab's emulation testbed consists of three sub-testbeds
(nodes from each can be mixed and matched), each catering to a
different research target:  

<ul>
<li> <b>Mobile Wireless</b>: Emulab has deployed and opened to public
external use, a small
<a href="tutorial/mobilewireless.php3">robotic testbed</a> that will
grow into a large mobile robotic wireless testbed. The small version
(5 Motes and 5 Stargates on 5 robots, all remotely controllable, plus
25 static Motes, many with attached sensor boards) is in an open area
within our offices.  An overview of this new mobile testbed is presented in 
<a href="http://www.cs.utah.edu/flux/testbed-docs/teapot05-emulab-only.pdf">
this article</a>.
<br><br>
<li> <b>Fixed 802.11 Wireless</b>: Emulab's
<a href="tutorial/docwrapper.php3?docname=wireless.html">Fixed Wireless</a>
testbed consists of PC nodes that contain 802.11 a/b/g wifi
interfaces, and are scattered around our building at various
locations on multiple floors. Experimenters can pick what nodes they
want to use, and as with other fixed nodes, can replace the software
that runs on the nodes, all the way down to operating system.
<br><br>
<li> <b>Emulab Classic</b>: a universally available
time- and space-shared network emulator which achieves new levels of
ease of use. Several hundred PCs in racks, combined with secure,
user-friendly web-based tools, and driven by <i>ns</i>-compatible
scripts or a Java GUI, allow you to remotely configure and control
machines and links down to the hardware level.  Packet loss, latency,
bandwidth, queue sizes--all can be user-defined.  Even the OS disk
contents can be fully and securely replaced with custom images by any
experimenter; Emulab can load ten or a hundred disks in less than two
minutes. 

<br><br>
Utah's local installation features high-speed Cisco switches connecting 5
100Mbit interfaces on each of 328 PCs (now including <b>160</b>
<a href="docwrapper.php3?docname=hardware.html">3GHz Dell PowerEdge 2850s</a>).
The PC nodes can be used as
edge nodes running arbitrary programs, simulated routers,
traffic-shaping nodes, or traffic generators.  While an "experiment"
is running, the experiment (and its associated researchers) get
exclusive use of the assigned machines, including root access.
We provide default OS software (<b>Redhat Linux 9.0, FreeBSD 4.10,
and Windows XP</b>); the default configuration on your nodes includes
accounts for project members, root access, DNS service, and standard
compilers, linkers, and editors. Fundamentally, however,
all the software you run on it, including all bits on the disks, is
replaceable and entirely your choice.  The same applies to the network's
characteristics, including its topology: configurable by users.
</li>
</ul>


<p>
Numerous <a href=otheremulabs.html>other sites</a> have set
up their own network emulators using Emulab's software, including the
<a href = "http://www.uky.emulab.net">University of Kentucky</a> testbed,
and the <a href = "http://www.netlab.cc.gatech.edu">Georgia Tech</a>
Testbed. 
</p>

<a href='pix/pc3k-back.jpg'>
   <img src='pix/pc3k-back-thumb.jpg' align=right /></a>
<a href='pix/pc3k-front.jpg'>
   <img src='pix/pc3k-front-thumb.jpg' align=right /></a>

<h3>Links to help you get started:</h3>
<ul>
<li><b><a href = "docwrapper.php3?docname=auth.html">
          Authorization Scheme, Policy, and "How To Get Started"</a></b>
<li><b><a href = "docwrapper.php3?docname=software.html">
          Overview of Installed Software</a></b>
<li><b><a href = "docwrapper.php3?docname=hardware.html">
          Hardware Overview, "Emulab Classic"</a></b>
<li><b><a href = "docwrapper.php3?docname=security.html">
          Security Issues</a></b>
<li><b><a href = "docwrapper.php3?docname=policies.html">
          Administrative Policies and Disclaimer</a></b>
</ul>

<?php
#
# Standard Testbed Footer
# 
PAGEFOOTER();

?>

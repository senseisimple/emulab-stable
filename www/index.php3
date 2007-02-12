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

<p>
    <em>Emulab</em> is a network testbed, giving researchers a wide range of
        environments in which to develop, debug, and evaluate their systems.
    The name Emulab refers both to a <strong>facility</strong> and to a <strong>software system</strong>.
    The <a href="http://www.emulab.net">primary Emulab installation</a> is run
        by the
        <a href="http://www.flux.utah.edu">Flux Group</a>, part of the
        <a href="http://www.cs.utah.edu">School of Computing</a> at the
        <a href="http://www.utah.edu">University of Utah</a>.
    There are also installations of the Emulab software at more than
        <a href="http://www.emulab.net/docwrapper.php3?docname=otheremulabs.html">two
        dozen sites</a> around the world, ranging from testbeds with a handful
        of nodes up to testbeds with hundreds of nodes.
    Emulab is <a href="http://www.emulab.net/doc/docwrapper.php3?docname=expubs.html">widely used</a>
        by computer science researchers in the fields of networking and
        distributed systems.
    It is also used to <a href="http://www.emulab.net/doc/docwrapper.php3?docname=exclasses.html">teach
        classes</a> in those fields.
</p>

<?php if ($TBMAINSITE) { ?>
<!-- Note - this stuff is fairly Utah-specific, so it's only displayed there
     for now. But, feel free to use any of it if it applies to your testbed -->
<p>
    Emulab is a <strong>public facility</strong>, available without charge to most
        researchers worldwide.
    If you are unsure if you qualify for use, please see our
        <a href="docwrapper.php3?docname=policies.html">policies document</a>,
        or <a href="mailto:<? $TBMAILADDR_OPS ?>">ask us</a>.
    If you think you qualify, you can
        <a href="docwrapper.php3?docname=auth.html">apply to start a new
        project</a>.
</p>

<p>
    <em>Emulab</em> provides integrated access to a wide range of experimental
        environments:
</p>

<dl class="envlist">

    <dt><img height=10 width=20 src="bullet-red.png" />
        <a href="tutorial/docwrapper.php3?docname=tutorial.html">Emulation</a></dt>
    <dd>An emulated experiment allows you to specify an arbitrary network
    topology, giving you a <em>controllable, predictable, and repealable
    environment</em>, including PC nodes on which you have
    <em>full &quot;root&quot; access</em>, running an operating system of
    your choice.</dd>

    <dt><img height=10 width=20 src="bullet-blue.png" />
    <a href="doc/docwrapper.php3?docname=plab.html">Live-Internet Experimentation</a></dt>
    <dd>Using the RON and <a href="http://www.planet-lab.org">PlanetLab</a>
    testbeds, Emulab provides you with a
    <em>full-featured environment</em> for <em>deploying, running, and
    controlling</em> your application at hundreds of sites around the
    world.</dd>

    <dt><img height=10 width=20 src="bullet-green.png" />
    <a href="tutorial/docwrapper.php3?docname=wireless.html">802.11 Wireless</a></dt>
    <dd>Emulab's 802.11a/b/g testbed is deployed on multiple floors of an
    office building. Nodes are <em>under your full control</em> and may act as
    access points, clients, or in ad-hoc mode. All nodes have two wireless
    interfaces, plus a <em>wired control network</em>.</dd>

    <dt><img height=10 width=20 src="bullet-yellow.png" />
    <a href="tutorial/docwrapper.php3?docname=gnuradio.html">Software-Defined Radio</a></dt>
    <dd><a href="http://www.ettus.com/downloads/usrp_v4.pdf">USRP</a> devices from the <a href="http://www.gnu.org/software/gnuradio/index.html">GNU Radio</a> project give you <em>control over Layer 1 of a
    wireless network</em> - everything from signal processing up is done in
    software.</dd>

    <dt><img height=10 width=20 src="bullet-orange.png" />
    <a href="tutorial/mobilewireless.php3">Sensor Networks</a></dt>
    <dd>Emulab's sensor network testbed includes 25 <a href="http://www.xbow.com/Products/productdetails.aspx?sid=174">Mica2</a> motes. All motes
    are equipped with a serial port, for <em>maximum control and debugging</em>
    capability.</dd>

    <dt><img height=10 width=20 src="bullet-aqua.png" />
    <a href="tutorial/mobilewireless.php3">Mobile Wireless</a></dt>
    <dd>A fleet of six <a href="http://www.acroname.com/garcia/garcia.html">Garcia robots</a> from <a href="http://www.acroname.com/">Acroname</a> are equipped with Stargate
    single-board computers and Mica2 motes. These robots have
    <em>full mobility</em> within our sensor network testbed.</dd>

    <dt><img height=10 width=20 src="bullet-purple.png" />
    <a href="tutorial/docwrapper.php3?docname=nse.html">Simulation</a></dt>
    <dd>Using NSE, ns-2's emulation facility, simulated networks can interact
    with real networks; not only the emulator, but <em>any network
    resource in Emulab</em>.</dd>
    </dl>

<p>
Emulab unifies all of these environments under a common user interface,
and integrates them into a common framework.  This framework
provides abstractions, services, and namespaces common to all, such as
allocation and naming of nodes and links.  By mapping the abstractions
into domain-specific mechanisms and internal names, Emulab masks much
of the heterogeneity of the different resources.
</p>
<?php } else { ?>

<p>Emulab is a universally available
time- and space-shared network emulator which achieves new levels of
ease of use. Several hundred PCs in racks, combined with secure,
user-friendly web-based tools, and driven by <i>ns</i>-compatible
scripts or a Java GUI, allow you to remotely configure and control
machines and links down to the hardware level.  Packet loss, latency,
bandwidth, queue sizes-all can be user-defined.  Even the OS disk
contents can be fully and securely replaced with custom images by any
experimenter; Emulab can load ten or a hundred disks in less than two
minutes. 
Emulab strives to preserve the control and ease of use of
simulation, without sacrificing the realism of emulation and live
network experimentation. 
</p>
<?php } ?>

<?php if ($TBMAINSITE) { ?>
<a href='pix/pc3k-front.jpg'>
   <img src='pix/pc3k-front-thumb.jpg' class='tbpic'/></a>
<a href='pix/pc3k-back.jpg'>
    <img src='pix/pc3k-back-thumb.jpg' class='tbpic' /></a>
<?php } ?>

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

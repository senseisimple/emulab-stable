<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Home");

# Get some stats about current experiments

$query_result = DBQueryFatal("select count(*) from experiments where ".
	"state='active'");
if (mysql_num_rows($query_result) != 1) {
    $active_expts = "ERR";
} else {
    $row = mysql_fetch_array($query_result);
    $active_expts = $row[0];
}

$query_result = DBQueryFatal("select count(*) from experiments where ".
	"state='swapped'");
if (mysql_num_rows($query_result) != 1) {
    $swapped_expts = "ERR";
} else {
    $row = mysql_fetch_array($query_result);
    $swapped_expts = $row[0];
}

$query_result = DBQueryFatal("select count(*) from experiments where ".
	"swap_requests > 0");
if (mysql_num_rows($query_result) != 1) {
    $idle_expts = "ERR";
} else {
    $row = mysql_fetch_array($query_result);
    $idle_expts = $row[0];
}

?>

<center>
<table align="right">
<tr><th nowrap colspan=2" class="contentheader" align="center">
	Current Experiments</th></tr>
<tr><td align="right" class="menuopt"><?php echo $active_expts ?></td> 
    <td align="left" class="menuopt">Active</td></tr>
<tr><td align="right" class="menuopt"><?php echo $idle_expts ?></td>
    <td align="left" class="menuopt">Idle</td></tr>
<tr><td align="right" class="menuopt"><?php echo $swapped_expts ?></td>
    <td align="left" class="menuopt">Swapped</td></tr>
</table>
</center>

<p><em>Netbed</em>, an outgrowth of <em>Emulab</em>, provides
integrated access to three disparate experimental environments:
simulated, emulated, and wide-area network testbeds.  Netbed strives
to preserve the control and ease of use of simulation, without
sacrificing the realism of emulation and live network experimentation.
</p>

<p>
Netbed unifies all three environments under a common user interface,
and integrates the three into a common framework.  This framework
provides abstractions, services, and namespaces common to all, such as
allocation and naming of nodes and links.  By mapping the abstractions
into domain-specific mechanisms and internal names, Netbed masks much
of the heterogeneity of the three approaches.
</p>

<p> <em>Wide-area resources</em>: Netbed currently includes
approximately 32 nodes geographically distributed across approximately
25 sites, largely the machines in the "MIT distributed testbed."
Experimenters with a valid research use can get non-root shell access
to these shared nodes, with ssh keys and other aspects automatically
managed by Netbed.  Secure shared filesystem access is coming soon via
SFS.
</p>

<p>
<em>"Emulab Classic,"</em> a key part of Netbed, is a universally-available
time- and space-shared network emulator which achieves new levels of
ease of use.
Several hundred
PCs in racks, combined with secure, user-friendly web-based tools, and driven
by <it>ns</it>-compatible scripts or a Java GUI, allow you
to remotely configure and control machines and links down to
the hardware level.  Packet loss, latency, bandwidth, queue sizes--
all can be user-defined.  Even the OS disk
contents can be fully and securely replaced with custom images by any experimenter;
Netbed can load ten or a hundred disks in 2.5 minutes total.
</p>

<p> Utah's local installation currently features high-speed Cisco
switches connecting 5 100Mbit interfaces on each of 168 PCs.  The
<a href = "http://www.uky.emulab.net">University of Kentucky</a>'s installation
contains 48 similarly networked PCs.
The PC nodes can be used as
edge nodes running arbitrary programs, simulated routers,
traffic-shaping nodes, or traffic generators.  While an "experiment"
is running, the experiment (and its associated researchers) get
exclusive use of the assigned machines, including root access.  </p>

<p>
We provide default OS software (Redhat Linux 7.1 and FreeBSD 4.5);
the default configuration on your nodes includes accounts for project members,
root access, DNS service, and standard compilers, linkers, and editors.
Fundamentally, however,
all the software you run on it, including all bits on the disks, is
replaceable and entirely your choice.  The same applies to the network's
characteristics, including its topology: configurable by users.


</p>
<br /><br />
<a href='pix/side-crop-big.jpg'>
   <img src='pix/side-crop-smaller.jpg' align=right /></a>

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

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

?>

Welcome to Emulab.  Emulab (sometimes called the Utah Network Testbed)
is a new and unique type of experimental environment: a
universally-available "Internet Emulator" which provides a
new balance between control and realism.  Several hundred
machines, combined with secure, user-friendly web-based tools, and driven
by <it>ns</it>-compatible scripts, allow you
to remotely reserve, configure and control machines and links down to
the hardware level:
packet loss, latency, bandwidth, packet ordering,
buffer space all can be user-defined. Even the operating system disk
contents can be securely and fully replaced with custom images.

<p>
The Testbed currently features high-speed Cisco switches connecting,
with over 5 miles of cabling, 168 core nodes (PCs) and 160 edge nodes
(Compaq DNARD Sharks). The core nodes can be
used as edge nodes, simulated routers, traffic-shaping nodes, or
traffic generators.  During an experiment's time slots, the experiment
(and associated researchers) get exclusive use of the assigned
machines, including root access if desired.  Until we finish designing
and building smarter scheduling and state-saving software, and obtain
the disk space, scheduling is manual and done at coarse granularity
(days).

<p>
We provide some default software (e.g. Redhat Linux and FreeBSD on the PCs,
NetBSD on the Sharks) that many users want. The basic software
configuration on your nodes includes accounts for project members,
root access, DNS service, compilers and linkers. But fundamentally,
the software you run on it, including all bits on the disks, is
replaceable and up to you.  The same applies to the network's
characteristics, including its topology: configurable by users.

<br><br>
<a href='pix/side-crop-big.jpg'>
   <img src='pix/side-crop-small.jpg' align=right></a>

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
</ul>

<?php
#
# Standard Testbed Footer
# 
PAGEFOOTER();

?>


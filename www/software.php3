<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2008 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Anyone can run this page. No login is needed.
# 
PAGEHEADER("Emulab Software Distributions");

# Insert plain html inside these brackets. Outside the brackets you are
# programming in php!
?>

<ul>
<li> <img src="/new.gif" alt="&lt;NEW&gt;">
    <a href="/downloads/emulab-080901.tar.gz">
    Emulab network testbed software</a>,
    Version 5.0.0, released September 1, 2008.
    This is an update of the 4.9.0 source release and should be considered
    the "official" Emulab release to be used for all fresh installs and
    upgrades.
    Includes source for everything except the GUI Client, which is available below.
    See the <a href = "README">README</a> and the
    <a href = "http://users.emulab.net/trac/emulab/wiki/InstallDocs">
    online installation documentation</a>.
    </li><p>

<li> <a href="/downloads/emulab-080630.tar.gz">
    Emulab network testbed software</a>,
    Version 4.9.0, released June 30, 2008.
    Includes source for everything except the GUI Client, which is available below.
    See the <a href = "README">README</a> and the
    <a href = "http://users.emulab.net/trac/emulab/wiki/InstallDocs">
    online installation documentation</a>.
    </li><p>

<li> 
     Emulab GUI Client v0.2
     <a href="/downloads/netlab-client.jar">(JAR</a>,
     <a href="/downloads/netlab-client-0.2.tar.gz">source tarball</a>
     <!-- <img src="/new.gif" alt="&lt;NEW&gt;"> -->).
     This is the fancier of the GUI clients for creating and
     interacting with experiments.  The GUI provides an alternative to using
     the web-based interface or logging into users.emulab.net and using the
     command line tools.  Take a look at the
     <a href="netlab/client.php3">tutorial</a>
     for more information.
     </li><p>

<li> Frisbee disk loader.
     The latest frisbee and imagezip sources can be found in the
     <a href="/downloads/emulab-080630.tar.gz">Emulab release</a>.
     The last standalone ISO image for a bootable Frisbee client
     is still <a href="/downloads/frisbee5-fs-20050819.iso">
     frisbee5-fs-20050819.iso</a> which includes binaries built from the
     5/16/2005 sources.
     </li>
<ul>

<?php


#
# Standard Footer.
# 
PAGEFOOTER();

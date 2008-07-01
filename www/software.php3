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
<li>
    Emulab network testbed software, Version 4.9.0, released June 30, 2008.
    Includes source for everything except the GUI Client, which is available below.
    See the <a href = "README">README</a> and the
    <a href = "http://users.emulab.net/trac/emulab/wiki/InstallDocs">
    online installation documentation</a>.
    </li>

<li> 
 <!-- DAVID: update the version number in some manner.  I think these 0.x.x ones are kinda stupid. -fjl -->
     Emulab GUI Client v0.1.1
     <a href="/downloads/netlab-client.jar">(JAR</a>,
     <a href="/downloads/netlab-client-0.1.0b.tar.gz">source tarball</a>
     <-- <img src="/new.gif" alt="&lt;NEW&gt;">). -->
     This is the fancier of the GUI clients for creating and
     interacting with experiments.  The GUI provides an alternative to using
     the web-based interface or logging into users.emulab.net and using the
     command line tools.  Take a look at the
     <a href="netlab/client.php3">tutorial</a>
     for more information.
     </li>
<ul>

<?php


#
# Standard Footer.
# 
PAGEFOOTER();

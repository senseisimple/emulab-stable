<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
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
     Emulab GUI Client v0.1.0b
     <a href="/downloads/netlab-client.jar">(JAR</a>,
     <a href="/downloads/netlab-client-0.1.0b.tar.gz">source tarball</a>
     <img src="/new.gif" alt="&lt;NEW&gt;">).
     This is the initial release of a new GUI client for creating and
     interacting with experiments.  The GUI provides an alternative to using
     the web-based interface or logging into users.emulab.net and using the
     command line tools.
     Please take a look at the
     <a href="netlab/client.php3">tutorial</a>
     for more information.
<li> <a href="/downloads/frisbee-snapshot-20040312.tar.gz">
     Updated Frisbee Server and Client Source Snapshot</a>.
     This is a ``snapshot'' of the current development tree.  This version
     includes improved Linux support.  The frisbee client will now build
     under Linux and the Linux image zipper can make partition images
     and supports filesystems other than ext2fs.
     Please take a look at the
     <a href="/downloads/frisbee-README-20040312.txt">README</a>
     file for more information on the changes.
     This
     <a href="/downloads/frisbee-fs-20040312.iso">updated ISO image</a>
     includes binaries built from the current sources.
<li> <a href="/downloads/frisbee-snapshot-20031021.tar.gz">
     Updated Frisbee Server and Client Source Snapshot</a>.
     This is a ``snapshot'' of the current development tree.  It includes the
     NTFS framework that was not in the previous release as well as some
     bug fixes and additional features.
     Please take a look at the
     <a href="/downloads/frisbee-README-20031021.txt">README</a>
     file for more information on the changes.
     This
     <a href="/downloads/frisbee-fs-20031021.iso">updated ISO image</a>
     includes binaries built from the current sources.
<li> <a href="/downloads/frisbee-20030618.tar.gz">Frisbee Server and
     Client</a>,
     as described in our paper <cite><a href="pubs.php3">Fast, Scalable Disk
     Imaging with Frisbee</a></cite>, to appear at
     <a href='http://www.usenix.org/events/usenix03/'>USENIX 2003</a>.
     Please take a look at the
     <a href="/downloads/frisbee-README-20030618.txt">README</a>
     file for a better
     idea of how we use Frisbee in the Testbed, and how to setup a
     simple Frisbee demonstration on your network.   Use this
     <a href="/downloads/frisbee-fs-20030618.iso">ISO image</a>
     to create a bootable FreeBSD 4.6 system from which to run the
     image creation and installation tools.
<ul>

<?php


#
# Standard Footer.
# 
PAGEFOOTER();

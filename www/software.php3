<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
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
     <a href="/downloads/frisbee-20030618.tar.gz">Frisbee Server and
     Client</a>,
     as described in our paper <cite><a href="pubs.php3">Fast, Scalable Disk
     Imaging with Frisbee</a></cite>, to appear at
     <a href='http://www.usenix.org/events/usenix03/'>USENIX 2003</a>.
     Please take a look at the
     <a href="/downloads/frisbee-README.txt">README</a> file for a better
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

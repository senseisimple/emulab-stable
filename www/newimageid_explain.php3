<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Create a new ImageID");

#
# Only known and logged in users can get this far!
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);
?>

<p>
This page will allow you to create your own custom disk image,
described by what we call an Image Descriptor. An Image Descriptor
indicates what operating system is contained in the image, what DOS
partition (or "slice" in BSD-speak) the image goes in, where the
image data file should be stored on disk, etc. Once you have created
the description, you can create the image from a node in one of your
experiments, and then load that image on other nodes (say, in a new
experiment). The image data file is in a compressed format that can be
quickly and easily loaded onto a node, or onto a group of nodes all at
the same time.

<p>
The most common approach is to use the <a
href='newimageid_ez.php3'>New Image Descriptor</a> form to create
a disk image that contains a customized version of the standard Redhat
Linux partition or the FreeBSD partition. Or, you can start from
scratch and load your own operating system in any of the DOS
partitions, and then capture that partition when you are done. Either
way, all you need to do is enter the node name in the form, and the
testbed system will create the image for you automatically, notifying
you via email when it is finished.  You can then use that image in
subsequent experiments by specifying the descriptor name in your NS
file with the
<a href="tutorial/docwrapper.php3?docname=nscommands.html#OS">
<tt>tb-set-node-os</tt></a> directive. When the experiment is
configured, the proper image will be loaded on each node automatically by
the Testbed system.

<p>
There is a 
<a href='showimageid_list.php3'>list of image descriptors</a> that
have already been created in your project (and are available for you
to use in your experiments). 

<?php
if ($isadmin) {
    echo "If you want to create an image that contains multiple DOS
          partitions, or even the entire disk, you should use the
          <a href='newimageid.php3'>long form</a>.\n";
}
?>

<p>
If you already have an Image Descriptor defined, and you want to
create a new version of the image using a node that is allocated in
one of your experiments and setup the way you want it, use the
<tt>create_image</tt> command. Log into <tt>users</tt> and run:

	<code><pre>
	create_image -p &lt;pid&gt &lt;imageid&gt &lt;node&gt	</pre></code>

This will reboot your node and create the new image, writing it to the
filename specified in the descriptor. This program is going exit
immediately and then send you email about 5-10 minutes later, after
the image is finished. In the meantime you should not do anything to
your node. Log out and leave it alone. If you do not get email, its
possible that the node froze up on reboot, which happens
sometimes. Let us know if you do not receive notification in a
reasonable amount of time.

<p>
If you ever want to reload a node in your experiment, either with one
of your images or with one of the default images, you can use the
<tt>os_load</tt> command. Log into <tt>users</tt> and run:

	<code><pre>
	os_load -p &lt;pid&gt -i &lt;imageid&gt &lt;node&gt </pre></code>
	
This program will run in the foreground, waiting until the image has
been loaded. At that point you should log in and make sure everything
is working oaky. You might want to watch the console line as well (see
the FAQ). If you want to load the default image, then simply run:

	<code><pre>
	os_load &lt;node&gt </pre></code>

<?php
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004, 2005, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
require("defs.php3");

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("printable",  PAGEARG_BOOLEAN);

if (!isset($printable))
    $printable = 0;

#
# Standard Testbed Header
#
if (!$printable) {
    PAGEHEADER("Emulab Client Interface");
}

if (!$printable) {
    echo "<b><a href=$REQUEST_URI?printable=1>
             Printable version of this document</a></b><br>\n";
}

function NLCH1($value)
{
	echo "<td align=\"right\" valign=\"top\" class=\"stealth\">
                <b>$value</b>
              </td>";
}

function NLCH2($value)
{
	echo "<td align=\"right\" valign=\"top\" class=\"stealth\">
                <font size=\"-1\"><i>$value</i></font>
              </td>";
}

function NLCBODYBEGIN()
{
	echo "<td align=\"left\" valign=\"top\" class=\"stealth\">";
}

function NLCBODYEND()
{
	echo "</td>";
}

function NLCFIGBEGIN()
{
	echo "<td align=\"center\" valign=\"top\" class=\"stealth\">";
}

function NLCFIGEND()
{
	echo "</td>";
}

function NLCFIG($value, $caption)
{
	echo "<td align=\"center\" valign=\"top\" class=\"stealth\">
                $value
                <font size=\"-2\">$caption</font>
              </td>";
}

function NLCLINKFIG($link, $value, $caption)
{
	echo "<td align=\"center\" valign=\"top\" class=\"stealth\">
                <a href=\"$link\" border=\"0\">
                  $value<br>
                  <font size=\"-2\">[$caption]</font></a>
              </td>";
}

function NLCEMPTY()
{
	echo "<td class=\"stealth\"></td>";
}

#
# Drop into html mode
#
?>
<center>
    <h2>Emulab Client GUI (Alpha)</h2>

    <i><font size=-1>
    Note: This software is still under development, so it will be slow, buggy,
    and lacking in features for some time to come.  However, it should still be
    usable enough to do basic tasks on small scale topologies.
    </font></i>
</center>

<br>

<table cellspacing=5 cellpadding=5 border=0 class="stealth"
       bgcolor="#ffffff" >

<tr><td colspan="3" class="stealth"><hr size=1></td></tr>


<tr>

<?php NLCH1("Introduction") ?>

<?php NLCBODYBEGIN() ?>
<!-- Center -->

The Emulab Client GUI is a <a href="http://java.sun.com">Java-based</a>
front-end to the <a href="<?php echo $TBBASE ?>">Emulab</a> network testbed.
It is designed to lower the bar to creating new experiments in the testbed and
provide a smooth, consistent interface for interacting with an experiment.
Using the client you can construct a topology with several nodes, links, LANs,
and specify many of their constituent properties (bandwidth, latency, etc...).
Once your topology is completed, you can export it as an NS file, or switch
into interactive mode where you can login to the nodes, change link parameters,
and perform several other tasks.
<br>
<br>
<?php NLCBODYEND() ?>

<?php NLCLINKFIG("nlc-ss-all.gif",
		 "<img src=\"nlc-ss-all-thumb.gif\" border=1
                       alt=\"Emulab Client Screen Shot\">",
		 "Screen&nbsp;Shot") ?>

</tr>


<tr>

<?php NLCH2("Features") ?>

<?php NLCBODYBEGIN() ?>
<!-- Center -->
The current list of features are:
<ul>
<li>Build and interact with the topology from a single interface.
<li>Interact with existing topologies.
<li>Support for Emulab-specific <a
href="<? echo $TBBASE ?>/tutorial/docwrapper.php3?docname=nscommands.html"
>extensions</a> to the NS syntax.
<li>Uses the Emulab <a href="<? echo $TBBASE ?>/xmlrpcapi.php3">XML-RPC</a>
interface.
<li>Integrated access to node consoles.
</ul>
<?php NLCBODYEND() ?>

<?php NLCEMPTY() ?>

</tr>


<tr>

<?php NLCH2("Requirements") ?>

<?php NLCBODYBEGIN() ?>
<!-- Center -->

Currently, we can only support a limited set of configurations in order to keep
things manageable.  The current requirements are:

<ul>
<li>A recent installation of FreeBSD or Linux.
<li>Version 1.4 of the Sun JDK.
</ul>

We plan to expand support to other platforms in the future, but only after the
major problems have been addressed.  Also, to take advantage of the integration
with Emulab's XML-RPC interface, you will need:

<ul>
<li>An Emulab account.
<li><a href="http://www.openssh.org">SSH</a> and a properly configured
ssh-agent.  (You just need to be able to ssh in without specifying a
password).
</ul>

Note that network access and an Emulab account are not required to run the
client, the software will not make any network connections unless explicitly
requested by the user.

<?php NLCBODYEND() ?>

<?php NLCEMPTY() ?>

</tr>


<tr>

<?php NLCH2("Download") ?>

<?php NLCBODYBEGIN() ?>
<!-- Center -->

The client is currently distributed as a self-contained JAR file and a source
tar ball:

<blockquote>
<table class="stealth" border=0 cellspacing=0 cellpadding=0 width="100%">
<tr>
<td>
<a href="/downloads/netlab-client.jar">netlab-client.jar</a>
</td>
<td align="center">(370K)</td>
<td align="center">v0.1.1</td>
<td align="right">Nov 2, 2005</td>
</tr>
<tr>
<td>
<a href="/downloads/netlab-client-0.1.1.tar.gz">netlab-client-0.1.1.tar.gz</a>
</td>
<td align="center">(787K)</td>
<td align="center">v0.1.1</td>
<td align="right">Nov 2, 2005</td>
</tr>
</table>
</blockquote>

Simply save it to any directory you choose and you can move on to the tutorial.
<br>
<br>
<?php NLCBODYEND() ?>

<?php NLCEMPTY() ?>

</tr>


<tr><td colspan="3" class="stealth"><hr size=1></td></tr>


<tr>

<?php NLCH1("Tutorial") ?>

<?php NLCBODYBEGIN() ?>
<!-- Center -->

Starting the client is just a matter of executing the Java JAR file:

<blockquote>
<code>[michael@bluth1 ~] java -jar netlab-client.jar</code>
</blockquote>

Next, we will do a brief overview of the three major parts of the interface
and then create a simple topology and Emulab experiment.

<p>
The interface has a typical layout: a main menu bar at the top, an editing
field in the middle, and a status bar at the bottom.  The main menu bar has the
usual set of actions for opening and saving files, as well as items for
accessing your Emulab site and experiments.  Most of your time will be spent in
the topology field in the middle of the window, editing and interacting with
your nodes and links.  Inside of the field are two dialog boxes, the palette
that is the source for objects to be added to the field, and the properties
dialog that shows the details of the currently selected object.  The bottom of
the window holds the status bar which provides you with at-a-glance information
about your topology and the corresponding experiment in Emulab.  At the moment,
this bar is devoid of any useful information, but, it will be updated as you
add nodes or make changes to a running experiment.

<p>
Now that you have a passing familiarity with the interface, we can create a
topology with two nodes linked together.
<?php NLCBODYEND() ?>

<?php NLCEMPTY() ?>

</tr>


<tr>

<?php NLCH2("Add two nodes") ?>

<?php NLCBODYBEGIN() ?>
<!-- Center -->

Your first task will be to add two nodes to the topology field.  Each node
represents a physical machine that will be allocated and automatically
configured to your specifications.  Adding a node is simply a matter of
dragging a node object from the palette and dropping it on the field.  This
will create a new node object with the default properties and add it to the
field.  Notice that the object has also been given a name, "node0".  Naming of
newly added or copied objects is handled automatically by the client so that
the names are unique and have a consistent style.

<!-- See the <a href="#Naming">naming</a> section later in the manual for -->
<!-- more details. -->

<p>
Another effect of adding a node to the topology field is that the dialog window
in the upper right has changed to show this node's properties.  Currently, most
of the fields are empty, indicating that the default values will be used.  You
might want to take a little time and explore the interface to see what options
can be specified.

<p>
Finally, lets create a friend for our current node by selecting the node and
then doing a copy & paste.  Your new node should appear a little below the old
one, simply drag it to whatever position you prefer, and we can move on to
networking the nodes.
<?php NLCBODYEND() ?>

<?php NLCFIG("<img border=1 src=\"nlc-node-drag.gif\">", "Drag & drop node") ?>

</tr>


<tr>

<?php NLCH2("Create a link") ?>

<?php NLCBODYBEGIN() ?>
<!-- Center -->
You may have noticed a line following your mouse pointer when it approached a
node object, this is the "wire" used to network nodes together.  To actually
make a connection, you simply need to get close to the object, drag the wire to
another object, and release.  Try this now with your two nodes.

<p>
<table width="100%" cellpadding=0 cellspacing=0 border=0 class="stealth">
<tr>
<td align="center" class="stealth">
  <img border=1 src="nlc-wire-1.gif" alt="Wire following mouse">
</td>
<td align="center" class="stealth">
  <img border=1 src="nlc-wire-2.gif" alt="Dragging wire">
</td>
<td align="center" class="stealth">
  <img border=1 src="nlc-wire-3.gif" alt="Connecting nodes">
</td>
</tr>
<tr>
<td align="center" class="stealth">
  <font size="-2">1. Follow</font>
</td>
<td align="center" class="stealth">
  <font size="-2">2. Drag</font>
</td>
<td align="center" class="stealth">
  <font size="-2">3. Connect</font>
</td>
</tr>
</table>

<p>
As you can see, the wire from the other node comes out to meet your mouse
pointer, making it clear what will be the other endpoint for the connection.
You can use this same technique to connect nodes to switches and vice-versa.
<?php NLCBODYEND() ?>

<?php NLCEMPTY() ?>

</tr>


<tr>

<?php NLCEMPTY() ?>

<?php NLCBODYBEGIN() ?>
<!-- Center -->
Your newly created link, similar to the one at right, represents a
Point-to-Point connection that will be establised between the nodes.  Normally,
this link behaves like an ethernet cable connecting two 100Mbps network
interface cards (NICs).  However, by changing the "bandwidth", "latency", and
"loss rate" properties, you can emulate other types of links.  To make things
interesting, change the latency of the link from zero to fifty.  Later on when
we are interacting with the nodes, this delay will be seen when pinging one
node from the other.
<br>
<br>
<?php NLCBODYEND() ?>

<?php NLCFIG("<img border=1 src=\"nlc-wire-4.gif\"
             alt=\"The newly created link\">", "New&nbsp;link") ?>

</tr>


<tr>

<?php NLCH2("Save your topology") ?>

<?php NLCBODYBEGIN() ?>
<!-- Center -->

Now that the topology is complete, you should save it to a file for future
reference.  If you are going to create an Emulab experiment from this topology,
you probably want to give the file the same name as your experiment.  Also, be
aware that once you create the experiment, you will not be able to edit the
topology until the experiment is terminated.  So, if you want to reuse the
topology in multiple experiments, you need to save it to a separate file now.
<br>
<br>
<?php NLCBODYEND() ?>

<?php NLCEMPTY() ?>

</tr>


<tr>

<?php NLCH2("Check your preferences") ?>

<?php NLCBODYBEGIN() ?>
<!-- Center -->

From this point on, you will need an account on an Emulab site and an SSH
configured to use public/private keys instead of passwords.  If all of these
preconditions are met, check the preferences in "Edit / Preferences" to ensure
that your user name and Emulab domain name are correct.  Also, the client uses
its own SSH config file, so you might need to edit the "SSH Config Template" to
suit your needs.  For example, a common change will be to use version two of
the protocol by changing the line "Protocol 1,2" to "Protocol 2".  These values
are needed to properly communicate with your Emulab's XML-RPC interface.
<br>
<br>
<?php NLCBODYEND() ?>

<?php NLCEMPTY() ?>

</tr>


<tr>

<?php NLCH2("Create an Experiment") ?>

<?php NLCBODYBEGIN() ?>
<!-- Center -->
With everything in order, you can now instantiate this topology as an
experiment in Emulab.  First, you need to select the "Experiment / Create..."
menu item to open the experiment creation dialog.  Since this is your initial
attempt to create an experiment, the client needs to contact Emulab to download
your account information.  Once that has completed, it will also do a one-time
check for recently added news items so that you can keep up-to-date with the
happenings at your Emulab site.

<p>
After getting up-to-date with the news, make sure the Project and Group
settings are correct, and then enter a meaningful description for your new
experiment.  Note that, by default, Emulab will immediately try to allocate and
configure your nodes.  So, if you do not plan on using the topology right away,
uncheck the "Swap in immediately" check box.

<p>
Because it can take several minutes to allocate and configure the machines, the
client will not block waiting for this process to complete.  Instead, you are
expected to wait for the Emulab E-mail confirming the completion of the swap in
and then manually use the "Experiment / Synchronize" menu.  Assuming the swap
in was successful, and you resynchronized, the client will switch into
"interactive mode".  This change disables the editing parts of the client and
enables the monitoring and manipulating portions.  An example of the simple
monitoring done by the client is the change in the node icons, they should have
all turned green, meaning the node is up and well.  In addition, the properties
dialog will now show how the nodes and links mapped into reality.  For example,
the properties for "link0" will show the IP addresses that were automatically
picked for the nodes, instead of the "(auto)" value they had while you were
editing.  Note that the client does not automatically track the state of nodes,
so you must manually resynchronize your view with Emulab every so often.  Of
course, monitoring the nodes is not very exciting so lets do something with
them.

<?php NLCBODYEND() ?>

<!-- XXX show change in icons -->

<?php NLCEMPTY() ?>

</tr>


<tr>

<?php NLCH2("Login to your nodes") ?>

<?php NLCBODYBEGIN() ?>
<!-- Center -->

Logging in to your nodes is simply a matter of bringing up the context-menu for
the node and selecting the "Login" item.  This action will download the host
keys for the node and open an SSH session in a new xterm.  Similarly, you can
connect to the node's serial console by selecting the "Console" item.  Most of
the time you will want to use SSH to communicate with your nodes and only use
the console if the node has become unreachable via the network.

<!-- explain node DNS naming -->
<p>
If you need to perform some other task besides login, you can address the node
using its fully qualified domain name, which has the form
"&lt;node&gt;.&lt;exp&gt;.&lt;proj&gt;.emulab.net".  For example, the name of
"node0" in experiment "foo" of project "bar" is "node0.foo.bar.emulab.net".
To save you some typing, the client provides a "Copy DNS Name" item in the
node's context-menu which copies the name to the system clipboard.

<p>
Now that you are logged into the nodes, you can try pinging one node from the
other:

<blockquote>
<pre>
[michael@node0 ~] ping node1.foo.bar.emulab.net
PING pc82 (155.101.132.82): 56(84) bytes of data.
64 bytes from 155.101.132.82: icmp_seq=1 ttl=64 time=0.315 ms
64 bytes from 155.101.132.82: icmp_seq=2 ttl=64 time=0.131 ms
64 bytes from 155.101.132.82: icmp_seq=3 ttl=64 time=0.125 ms
64 bytes from 155.101.132.82: icmp_seq=4 ttl=64 time=0.169 ms
</pre>
</blockquote>

As you can see, the ping worked fine, but the delay you added to the link does
not seem to be taking effect.  Also, the IP addresses do not match those
displayed in the properties dialog for "link0".  Well, it turns out that there
is another active network interface on your node, called the "control
interface", that is not displayed in the client.  This control interface is the
node's connection to the outside world and provides a route for logging in to
the node, mounting shared file systems, and so on.  As you may have guessed,
the domain name for the control interface is the fully qualified name of the
node (e.g. "node0.foo.bar.emulab.net").  So, in order to reach the node through
the link displayed in the client, you need to use the unqualified name:

<blockquote>
<pre>
[michael@node1 ~] ping node1
PING node1 (10.1.1.2): 56(84) data bytes
64 bytes from 10.1.1.2: icmp_seq=1 ttl=64 time=99.9 ms
64 bytes from 10.1.1.2: icmp_seq=2 ttl=64 time=99.9 ms
64 bytes from 10.1.1.2: icmp_seq=3 ttl=64 time=99.9 ms
64 bytes from 10.1.1.2: icmp_seq=4 ttl=64 time=99.8 ms
</pre>
</blockquote>

You can read more about the intricacies of the experimental and control
networks <a href="<?php echo $TBBASE
?>/tutorial/docwrapper.php3?docname=tutorial.html#ControlNet">here</a>.

<?php NLCBODYEND() ?>

<?php NLCFIG("<img border=1 src=\"nlc-login.gif\">", "Context Menu") ?>

</tr>


<tr>

<?php NLCH2("Change link behavior") ?>

<?php NLCBODYBEGIN() ?>
<!-- Center -->

Now that we have ping working correctly, lets change the link latency and watch
the ping behavior change.  First, click on the link object to bring up its
properties.  Then change the latency to 75 and hit return.  You may have
noticed at this point that the "Synchronized" menu at the bottom of the
interface changed to "Unsynchronized", meaning that client's display is
out-of-sync with the experiment in Emulab.  In other words, your latency change
has <i>not yet</i> taken effect.  In order for the change to happen, you must
do a manual synchronize (CTRL-Y), to upload any changes you have made and then
redownload the status of your experiment.  Finally, rerun ping to see change in
the latency.

<p>
<table width="100%" cellpadding=0 cellspacing=0 border=0 class="stealth">
<tr>
<td align="center" class="stealth">
  <img border=1 src="nlc-sync.gif" alt="Synchronized">
</td>
<td align="center" class="stealth">
  <img border=1 src="nlc-unsync.gif" alt="Unsynchronized">
</td>
<td align="center" class="stealth">
  <img border=1 src="nlc-unsync-review.gif" alt="Review Changes">
</td>
</tr>
<tr>
<td align="center" class="stealth">
  <font size="-2">1. No changes</font>
</td>
<td align="center" class="stealth">
  <font size="-2">2. Latency Changed</font>
</td>
<td align="center" class="stealth">
  <font size="-2">3. Review Changes</font>
</td>
</tr>
</table>

<p>
Try making more changes to the link and watch the change in behavior of ping or
other programs, just remember you have to resynchronize to see the effect.

<p>
Once you have finished playing around with this simple topology, you can
terminate it and try to use the client with one of your existing topologies.

<?php NLCBODYEND() ?>

<?php NLCEMPTY() ?>

</tr>


<tr>

<?php NLCH2("Existing Experiments") ?>

<?php NLCBODYBEGIN() ?>
<!-- Center -->

Besides creating topologies, you can also use the client to interact with
existing experiments in Emulab.  You can load an experiment into the client
through the "Experiment / Bind to existing..." menu item.  This menu will
display a listing of experiments you have created, then you simply need to
select one to load.  After the client has finished retrieving information about
the experiment, you should be able to use the client like in the previous
sections to login to your nodes, change link parameters, and swap in/out your
experiment.

<?php NLCBODYEND() ?>

<?php NLCEMPTY() ?>

</tr>


<tr>

<?php NLCH2("Troubleshooting") ?>

<?php NLCBODYBEGIN() ?>
<!-- Center -->
We will try and cover some of the more common problems and their solutions in
this section:<br>
<br>

<table class="stealth" width="25%">
<tr><th>"Bad Response" when trying to connect to Emulab.</th></tr>
<tr>
<td>
This error is usually due to the client not configuring SSH correctly.  First,
check your preferences to make sure they are correct.  It is easy to overlook
minor details in the defaults, for example, the client assumes that the user
name on your desktop machine is the same as your Emulab user name, which is not
always the case.
<p>
If those settings all look correct, you may want to go into the "SSH Config
Template" page of the preferences dialog and change the line "Protocol 1,2" to
"Protocol 2".  This is required if your keys are for version 2 of the SSH
protocol, since the client currently assumes version 1.

<p>
If all else fails, leave
the client running and execute the following in another terminal:
<pre>
$ ssh -v -v -v -F ~/.netlab-client/emulab.net.ssh-config boss.emulab.net
</pre>
This should give you an idea of what might be going wrong.  If you cannot
figure out the output on your own, send the output to the testbed operators.

<p>
In the future, we will hopefully be able to make the client figure this all out
on its own.
</td>
</tr>
</table>

<?php NLCBODYEND() ?>

<?php NLCEMPTY() ?>

</tr>


<tr><td colspan="3" class="stealth"><hr size=1></td></tr>


<tr>

<?php NLCH1("History") ?>

<?php NLCBODYBEGIN() ?>
<!-- Center -->

The release history:

<ul>
<li>v0.1.1 - Add support for nodes with non-routable control networks.
<li>v0.1.0b - Add ability to customize the SSH config template.
Ask the user before swapping/terminating an experiment.
Fix some other minor bugs.
<li>v0.1.0 - Initial release
</ul>

<?php NLCBODYEND() ?>

<?php NLCEMPTY() ?>

</tr>


<tr><td colspan="3" class="stealth"><hr size=1></td></tr>


</table>

<?php
#
# Standard Testbed Footer
# 
if (!$printable) {
    PAGEFOOTER();
}
?>

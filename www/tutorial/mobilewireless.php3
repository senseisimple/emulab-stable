<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004, 2005 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
require("defs.php3");

# Page arguments.
$printable = $_GET['printable'];

# Pedantic page argument checking. Good practice!
if (isset($printable) && !($printable == "1" || $printable == "0")) {
    PAGEARGERROR();
}
if (!isset($printable))
    $printable = 0;

#
# Standard Testbed Header
#
if (!$printable) {
    PAGEHEADER("Emulab Tutorial - Mobile Wireless Networking");
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
    <h2>Emulab Tutorial - Mobile Wireless Networking</h2>

    <i><font size=-1>

    Note: This part of the testbed is in the prototype stage, so the hardware
    and software may behave in unexpected ways.

    </font></i>
</center>

<br>

<table cellspacing=5 cellpadding=5 border=0 class="stealth" bgcolor="#ffffff">

<tr><td colspan="3" class="stealth"><hr size=1></td></tr>


<tr>

<?php NLCH1("Introduction") ?>

<?php NLCBODYBEGIN() ?>
<!-- Center -->

In addition to <a href="docwrapper.php3?docname=wireless.html">fixed wireless
nodes</a>, Emulab also features wireless nodes attached to robots that can move
around a small area.  These robots consist of a small body (shown on the right)
with an <a href="http://www.xbow.com/Products/XScale.htm">Intel Stargate</a>
that hosts a mote with a wireless network interface.  The goal of this "mobile
wireless testbed" is to give users an opportunity to conduct experiments with
wireless nodes in configurable physical locations and while in motion.  For
example, mobile nodes could be used to realistically test and evaluate an
ad-hoc routing algorithm in a fairly repeatable manner.

<br>
<br>
<?php NLCBODYEND() ?>

<?php NLCLINKFIG("http://www.acroname.com/garcia/garcia.html",
		 "<img src=\"garcia-thumb.jpg\" border=1
                       alt=\"Acroname Garcia\">",
		 "Acroname&nbsp;Garcia") ?>

</tr>

<tr>

<?php NLCH2("Features") ?>

<?php NLCBODYBEGIN() ?>

The current features of the mobile wireless testbed are:

<ul>
<li>Four <a href="http://www.acroname.com">Acroname Garcia</a> robots
<li><a href="http://www.xbow.com/Products/XScale.htm">Intel Stargate</a> single
board computers for each robot.
<li><a href="http://www.xbow.com/Products/productsdetails.aspx?sid=72">Mica2 
motes</a> attached to each stargate.
<li>Four cameras for visual tracking of the robots.
<li>A <a href="https://www.emulab.net/webcam.php3">webcam</a> for viewing the
robots in their habitat.
<li>An <a href="/robotmap.php3">abstract map</a> of the current locations of
the robots.
</ul>

<?php NLCBODYEND() ?>

<?php NLCEMPTY() ?>

</tr>

<tr>

<?php NLCH2("Limitations") ?>

<?php NLCBODYBEGIN() ?>

Due to the "brand-new" nature of this part of Emulab, there are some
limitations you should be aware of:

<ul>
<li>Availability is reduced to weekdays between 9am and 5pm mountain time,
so there is staff available to assist with problems.
<li>There is no space sharing, only one mobile experiment can be swapped-in at
a time.
<li>Batteries must be replaced manually by the operator when capacity is low.
</ul>

We hope to overcome these limitations over time, however, we are also eager to
introduce external users to the mobile testbed early on so we can integrate
their feedback.

<?php NLCBODYEND() ?>

<?php NLCEMPTY() ?>

</tr>

<tr><td colspan="3" class="stealth"><hr size=1></td></tr>

<tr>

<?php NLCH1("Mobile Experiments") ?>

<?php NLCBODYBEGIN() ?>

Creating a mobile wireless experiment is very similar to creating a regular
Emulab experiment: you construct an NS file, swap in the experiment, and then
you can log into the nodes to run your programs.  There are, of course, some
extra commands and settings that pertain to the physical manifestations of the
robots.

<?php NLCBODYEND() ?>

<?php NLCEMPTY() ?>

</tr>

<tr>

<?php NLCH2("A First Experiment") ?>

<?php NLCBODYBEGIN() ?>

Lets start with a simple NS script that will allocate a single robot located in
our building:

<blockquote>
<pre>set ns [new Simulator]
source tb_compat.tcl

set topo [new Topography]
$topo load_area MEB-ROBOTS

$ns node-config -topography $topo

set node(0) [$ns node]

$node(0) set X_ 3.01
$node(0) set Y_ 2.49

$ns run</pre>
</blockquote>

Some parts of that example should be familiar to regular experimenters, so we
will focus mainly on the new bits of code.  First, we specified the physical
area where the robots will be roaming by creating a "topography" object and
loading it with the dimensions of that area:

<blockquote>
<pre><i>Line 4:</i>  set topo [new Topography]
<i>Line 5:</i>  $topo load_area MEB-ROBOTS</pre>
</blockquote>

In this case, the "MEB-ROBOTS" area is the name given to part of our office
space in the Merrill Engineering Building.  Next, we change the default node
configuration so any subsequent calls to "<code>[$ns node]</code>" will
automatically attach the node to the topography we just created:

<blockquote>
<pre><i>Line 7:</i>  $ns node-config -topography $topo</pre>
</blockquote>

Finally, after creating the robot, we need to set the initial position in the
area:

<blockquote>
<pre><i>Line 11:</i> $node set X_ 3.01
<i>Line 12:</i> $node set Y_ 2.49</pre>
</blockquote>

The values specified above are measured in meters and based on the map located
<a href="/robotmap.php3">here</a>, where the origin is in the upper left hand
corner, with positive X going right and positive Y going down.  You can also
click on the map to get a specific set of coordinates.  Note that any
coordinates you specify must not be in an obstacle, or they will be rejected by
the system.

<p>
With this NS file you can now create your first mobile experiment.  Actually
creating the experiment is the same as any other, except you might want to
check the "Do Not Swapin" checkbox so that the creation does not fail if
someone else is using the mobile testbed at the time.  Once the area is free
for use, you can swap-in your experiment and begin to work.

<?php NLCBODYEND() ?>

<?php NLCEMPTY() ?>

</tr>

<tr>

<?php NLCH2("Adding Motion") ?>

<?php NLCBODYBEGIN() ?>

Now that you have a node allocated, lets make it mobile.  Emulab will have
already moved the node to its initial position on swap-in, so moving the robot
again is done by sending it a SETDEST event with the coordinates, much like
sending START and STOP events to <a
href="docwrapper.php3?docname=advanced.html">traffic generators</a> and <a
href="docwrapper.php3?docname=advanced.html">program objects</a>.  However,
before moving it for the first time, you might want to check its current
position in the <a href="/robotmap.php3">map</a> and/or <a
href="https://www.emulab.net/webcam.php3">webcam</a>.

<p>
<table width="100%" cellpadding=0 cellspacing=0 border=0 class="stealth">
<tr>
<?php NLCLINKFIG("robotmap-ss.gif", 
		 "<img src=\"robotmap-ss-thumb.gif\" border=1
		 alt=\"Robot Map Screenshot\">",
		 "Robot Map Screenshot") ?>
<?php NLCLINKFIG("webcam-ss.gif", 
		 "<img src=\"webcam-ss-thumb.gif\" border=1
		 alt=\"Webcam Screenshot\">",
		 "Webcam Screenshot") ?>
</tr>
</table>

<!-- XXX We need to give them a clue on which way the webcam is pointing in -->
<!-- relation to the robot map. -->

<p>
Now that you have a good idea of where the robot is, lets move it up a meter.
To do this, you will need to log in to ops.emulab.net and run:

<blockquote>
<pre>1 ops:~> tevc -e <b>proj</b>/<b>exp</b> now node-0 SETDEST X=3.0 Y=1.5</pre>
</blockquote>

<!-- mention that one setdest will override the previous. --> 

Then, check back with the map and webcam to see the results of your handywork.
Try moving it around a few more times to get a feel for how things work and
where the robot can go.  Note that the robot should automatically navigate
around obstacles in the area, like the pole in the middle, so you do not have
to plot your own course around them.

<p>
In addition to driving the robot with dynamic events, you can specify a static
set of events in the NS file.  For example, you can issue the same move as
above at T +5 seconds by adding:

<blockquote>
<code>$ns at 5.0 "$node(0) setdest 3.01 1.5 0.1"</code>
</blockquote>

Note that "setdest" takes a third argument, the speed, in addition to the X and
Y coordinates.  The robot's speed is currently fixed at 0.1 meters per second.

<?php NLCBODYEND() ?>

<?php NLCEMPTY() ?>

</tr>

<tr>

<?php NLCH2("Random Motion") ?>

<?php NLCBODYBEGIN() ?>

Generating destination points for nodes can become quite a tedious task, so we
provide a modified version of the NS-2 "setdest" tool that will produce a valid
set of destination points for a given area.  The tool is installed ops and
takes the following arguments:

<blockquote>
<ul>
<li><b>-n</b> <i>nodes</i> - The total number of nodes to generate motion for.
The format for the node variables in the generated code is,
"<code>$node(N)</code>", so write your NS file accordingly.
<li><b>-t</b> <i>secs</i> - The simulation time, in seconds.
<li><b>-a</b> <i>area</i> - The name of the area where the robots will be
roaming around.  Currently, MEB-ROBOTS is the only area available.
</ul>
</blockquote>

Now, taking your existing NS file, we'll add another node to make things more
interesting and use "tbsetdest" to produce some random motion for both robots:

<blockquote>
<pre>2 ops:~> tbsetdest -n 2 -t 60 -a MEB-ROBOTS</pre>
</blockquote>

Here is some sample output from the tool:

<blockquote>
<pre>$node(0) set X_ 3.01
$node(0) set Y_ 2.49
$node(1) set X_ 1.22
$node(1) set Y_ 3.61
set rtl [$ns event-timeline]
#
# nodes: 2, pause: 0.50, max x: 5.90, max y: 4.00
#
$rtl at 0.50 "$node(0) setdest 0.92 3.28 0.10"
$rtl at 0.50 "$node(1) setdest 0.61 3.02 0.10"
$rtl at 9.00 "$node(1) setdest 0.61 3.02 0.00"
$rtl at 9.50 "$node(1) setdest 0.88 2.09 0.10"
$rtl at 19.14 "$node(1) setdest 0.88 2.09 0.00"
$rtl at 19.64 "$node(1) setdest 2.80 2.07 0.10"
$rtl at 22.87 "$node(0) setdest 0.92 3.28 0.00"
$rtl at 23.37 "$node(0) setdest 5.62 2.79 0.10"
$rtl at 38.93 "$node(1) setdest 2.80 2.07 0.00"
$rtl at 39.43 "$node(1) setdest 4.98 1.65 0.10"
#
# Destination Unreachables: 0
#</pre>
</blockquote>

You can then add the second node and motion events by clicking on the "Modify
Experiment" menu item on the experiment web page and:

<ol>
<li>Adding "<code>set node(1) [$ns node]</code>" after "<code>node(0)</code>"
is created;
<li>Copying and pasting the "tbsetdest" output into the NS file before the
"<code>$ns run</code>" command; and
<li>Starting the modify.
</ol>

While the modify is working, lets take a closer look at the output of
"tbsetdest".  You may have noticed the following new syntax:

<blockquote>
<pre><i>Line 5:</i>  set rtl [$ns event-timeline]
<i>Lines 9+:</i> $rtl at ...</pre>
</blockquote>

These commands create a new "timeline" object and then add events to it, much
like adding events using "<code>$ns at</code>".  The difference is that the
events attached to a timeline object can be requeued by sending a START event
to the timeline, in contrast to the "<code>$ns at</code>" events which are only
queued when the event system starts up.  This feature can be useful for testing
your experiment by just (re)queueing subsets of events.

<p>
Once the modify completes, you can start the robots on their way by running the
following on ops:

<blockquote>
<pre>3 ops:~> tevc -e <b>proj</b>/<b>exp</b> now rtl START</pre>
</blockquote>

<?php NLCBODYEND() ?>

<?php NLCEMPTY() ?>

</tr>


<tr><td colspan="3" class="stealth"><hr size=1></td></tr>


<tr>

<?php NLCH1("Wireless Traffic") ?>

<?php NLCBODYBEGIN() ?>

Now that you are getting the hang of the mobility part of this testbed, we can
move on to working with wireless network traffic.  As stated earlier, each of
the robots carries a Mica2 mote (pictured on the right), which is a popular
device used in wireless sensor networks.  We'll be using the motes on the
mobile nodes you already have allocated and loading them with <a
href="http://www.tinyos.net">TinyOS</a> demo kernels, one that will be sending
traffic and the other recieving.

<?php NLCBODYEND() ?>

<?php NLCLINKFIG("http://www.tinyos.net/scoop/special/hardware",
		 "<img src=\"mica2-thumb.jpg\" border=1
                       alt=\"Mica2 Mote\">",
		 "Mica2&nbsp;Mote") ?>

</tr>

<tr>

<?php NLCH2("Adding Motes") ?>

<?php NLCBODYBEGIN() ?>

Adding a couple of motes to your existing experiment can be done by doing a
modify and adding the following NS code:

<blockquote>
<pre>$ns node-config -topography ""

set mote(0) [$ns node]
tb-set-hardware $mote(0) mica2
tb-set-node-os $mote(0) TinyOS-RfmLed
tb-fix-node $mote(0) $node(0)

set mote(1) [$ns node]
tb-set-hardware $mote(1) mica2
tb-set-node-os $mote(1) TinyOS-CntRfm
tb-fix-node $mote(1) $node(1)</pre>
</blockquote>

This code creates two mote nodes and "attaches" each of them to one of the
mobile nodes.  The OSs to be loaded on the mote nodes are the receiver,
TinyOS-RfmLed, and the transmitter, TinyOS-CntRfm.  The receiver kernel will
listen for packets containing a number from the transmitter and display the
number, in binary, on the mote's builtin LEDs.  The transmitter kernel will
then send packets every second containing the value of a counter that goes from
one to eight.  So, if the mote's radios are well within range of each other,
the receiver should pick up the packets and display the number on the LEDs.  Of
course, since you're not physically around to see that, you can click on the
"Show Blinky Lights" menu item on the experiment web page to bring up a webpage
with an applet that provides a near real-time view of the lights.

<p>
After the modify completes, try moving the nodes close to one another and far
away, to see the lights updating, or not.  You should also try running the
nodes through the random motion created earlier and watching for the same
effect on the lights.

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

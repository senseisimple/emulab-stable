<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004, 2005, 2006, 2007 University of Utah and the Flux Group.
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
    PAGEHEADER("Emulab Tutorial - Mobile Wireless Networking");
}

echo "
  <head>
  <style text='text/css'>
  @import url(\"$BASEPATH/style-doc.css\");
  </style>
  </head>";

if (!$printable) {
    echo "<b><a href=$REQUEST_URI?printable=1>
             Printable version of this document</a></b><br>\n";
}

function NLCLINKFIG2($link, $value, $caption, $explanation)
{
	echo "<td valign=\"top\" class=\"stealth\" width='50%'>
                <a href=\"$link\" border=\"1\">
                  <center>$value</center></a>
                  <font size=\"-2\"><b>$caption:</b>
                $explanation</font>
              </td>";
}

#
# Drop into html mode
#
?>

<h2 id="doctitle">Emulab Tutorial - Mobile Wireless Networking</h2>

<script type="text/javascript">
default_text = "Click on an image to enlarge.";

if (document.all || document.getElementById || document.layers) {
  has_scripting = true;
  default_text = "(Move mouse over an image to see a caption here; click to enlarge.)";
}

captions = new Array();
//captions[0] = 'Three of our Acroname Garcia robots.  Each robot has a ' + 
//	      'blue and orange marker from which the localization system ' +
//	      'extracts position and orientation.  All robots have a one ' +
//	      'antenna riser, beneath which is an Intel Stargate and ' +
//	      'Crossbow Mica2 mote.';
captions[0] = 'Three of our modified Acroname Garcia robots.  Fiducials for ' +
	      'vision system atop antenna riser mount; Intel ' +
	      'Stargate and Crossbow Mica2 beneath riser.';
captions[1] = 'A wall-mounted mote.  Mote is behind the serial programming ' +
	      'board; a sensor board is visible in front.';
captions[2] = 'A screenshot of our Java applet motion control interface, ' +
	      'enlarged webcam showing robot motion and position.';
captions[3] = 'Robots prior to antenna riser modifications.  Crosshatch ' +
	      'pattern used for localization system calibration.';
captions[4] = 'A ceiling-mounted mote.';

function imageinfo(msg) {
  mmsg = '<div style="text-align: center; font-size: 10px; font-style: italic;">' + msg +'</div>';
  //mmsg = msg;
  elm = (document.getElementById)?document.getElementById('captionLayer'):(document.all)?document.all['captionLayer']:null;

  if (elm && typeof elm.innerHTML != "undefined") {
    elm.innerHTML = mmsg;
  }
  else if (document.layers) {
    document.layers['captionLayer'].document.write(mmsg);
  }

//  if (document.all) {
//    elm.innerHTML = mmsg;
//  }
//  else if (document.getElementById) {
//    range = document.createRange();
//    range.setStartBefore(elm);
//    html = range.createContextualFragment(mmsg);
//    while (elm.hasChildNodes()) {
//      elm.removeChild(elm.lastChild);
//    }
//    elm.appendChild(html);
//  }
//  else {
//    elm.document.write(mmsg);
//  }
}

</script>

<div class="docsection">
<ul id="robothumbs">
<li><a href="3bots.jpg" onmouseover="imageinfo(captions[0])" onmouseout="imageinfo(default_text)">
<img src="3bots_thumb.jpg" border=1 alt="Our Robots.">
</a></li>

<li><a href="wall_mote.jpg" onmouseover="imageinfo(captions[1])" onmouseout="imageinfo(default_text)">
  <img src="wall_mote_thumb.jpg" border=1 alt="Our Robots.">
</a></li>

<li><a href="interface1.jpg" onmouseover="imageinfo(captions[2])" onmouseout="imageinfo(default_text)">
  <img src="interface1_thumb.jpg" border=1 alt="User Interface.">
  </a></li>

<li><a href="5bots.jpg" onmouseover="imageinfo(captions[3])" onmouseout="imageinfo(default_text)">
  <img src="5bots_thumb.jpg" border=1 alt="Our Robots.">
  </a></li>

<li><a href="ceiling_mote.jpg" onmouseover="imageinfo(captions[4])" onmouseout="imageinfo(default_text)">
  <img src="ceiling_mote_thumb.jpg" border=1 alt="Our Robots.">
  </a></li>
</ul>

<div id="captionLayer" name="captionLayer">
  <script type="text/javascript">
    imageinfo(default_text);
  </script>
</div>
</div>

<div class="docsection">
<h1>Preface</h1>

<div id="toollist">
<h3>Tools:</h3>

<ul>
<li><a href="<?php echo $TBBASE?>/robotrack/robotrack.php3">Real-time Robot Tracking Applet</a></li>
<li><a href="<?php echo $TBBASE ?>/webcam.php3">Live Webcams</a></li>
<li><a href="<?php echo $TBBASE ?>/robotmap.php3">Static Robot Map</a></li>
<li><a href="<?php echo $TBBASE ?>/newimageid_ez.php3?nodetype=mote">Mote Image
Creation Page</a></li>
<li><a href="<?php echo $TBBASE ?>/wireless-stats/statsapp.php3">WSN Connectivity
Statistics Applet</a></li>
</ul>
</div>

<p>
We have deployed and opened to public external use a small version of
what will grow into a large mobile robotic wireless testbed.  The
small version (6 Motes and 6 Stargates on 6 robots, all remotely
controllable, plus 25 static Motes) is in an open area within our offices;
the big one will be elsewhere.
</p>

<p>
This manual is broken up into the following sections:
</p>

<ol id="sectionlist">
<li><a href="#INTRO">Introduction</a>
<li><a href="#MOBILE">Mobile Experiments</a>
<li><a href="#WIRELESS">Wireless Traffic (Mobile motes and fixed motes)</a>
<li><a href="#FAQ">Frequently Asked Questions</a>
</ol>

<p>
If you are interested in how the mobile testbed works, you can read the
following paper (to appear at <a href="http://www.ieee-infocom.org/2006/">IEEE
Infocom</a>, April 2006):
</p>

<blockquote>
<a href="http://www.cs.utah.edu/flux/papers/robots-infocom06-base.html">
Mobile Emulab: A Robotic Wireless and Sensor Network Testbed</a> 
</blockquote>

<p>
You can read a shorter overview of the mobile testbed in this article:
</p>

<blockquote>
<a href="http://www.cs.utah.edu/flux/testbed-docs/teapot05-emulab-only.pdf">
Real Mobility, Real Wireless: A New Kind of Testbed</a>
</blockquote>

</div>

<div class="docsection">
<h1>Introduction</h1>
<p>
<a name="INTRO"></a>
In addition to <a href="<? echo "$WIKIDOCURL/wireless" ?>">fixed wireless
nodes</a> (currently predominantly 802.11), Emulab also features wireless nodes attached
to robots that can move
around a small area.  These robots consist of a small body (shown on the right)
with an <a href="http://www.xbow.com/Products/XScale.htm">Intel Stargate</a>
that hosts a mote with a wireless network interface.  The goal of this "mobile
wireless testbed" is to give users an opportunity to conduct experiments with
wireless nodes that are truly mobile. 
<!-- in configurable physical locations and while in motion. -->
For
example, mobile nodes could be used to realistically test and evaluate an
ad-hoc routing algorithm in a fairly repeatable manner.  This document is
intended as a tutorial for those interested in making use of this testbed;
there is also a short <a href="<? echo "$WIKIDOCURL/wireless" ?>">reference manual</a>
available that gives a few details about the workings of the system.
</p>

<h2>Features</h2>

<div class="docfig">
<a href="http://www.acroname.com/garcia/garcia.html">
<img src="garcia-thumb.jpg" alt="Acroname Garcia">
<span class="caption">[Acroname Garcia]</span>
</a>
</div>

<p>
The current features of the mobile wireless testbed are:
</p>

<ul>
<li>Four <a href="http://www.acroname.com">Acroname Garcia</a> robots
<li><a href="http://www.xbow.com/Products/XScale.htm">Intel Stargate</a> single
board computers for each robot. (<b><a href="<?php echo $TBBASE?>/doc/docwrapper.php3?docname=stargatenotes.html">Important Notes!</a></b>)
<li>Roaming an area about 8 x 3.5 meters with a sheetrock-covered steel pillar in the middle.
<li>Each robot has its own
    <a href="http://www.xbow.com/Products/productsdetails.aspx?sid=72">
    900MHz Mica2 mote</a> attached to the Stargate.
<li>25 statically-placed
    <a href="http://www.xbow.com/Products/productsdetails.aspx?sid=72">
    900MHz Mica2 motes</a>, two of which are attached to
    <a href="http://www.xbow.com/Products/productsdetails.aspx?sid=90">
    E-Mote</a> Ethernet gateways for Berkeley Motes.
<li><a href=http://www.xbow.com/Products/productsdetails.aspx?sid=75>
    Mica2 Multi-Sensor Module</a> boards attached to 10 floor level Motes,
    and to each Robot (Stargate) Mote.
<li>Six overhead cameras for vision-based position tracking of the robots.
<li>Three <a href="<?php echo $TBBASE ?>/webcam.php3">webcams</a> for viewing 
the robots in their habitat.
<li>An <a href="<?php echo $TBBASE ?>/robotmap.php3">abstract map</a> of the
current locations of the robots.
<li>A <a href="<?php echo $TBBASE ?>/robotrack/robotrack.php3">live robot
motion control applet</a> for drag and drop control and monitoring of robots.
<li>An <a href="<?php echo $TBBASE ?>/wireless-stats/statsapp.php3">applet for
viewing wireless link connectivity</a> in our WSN testbed.
<li>Open for public use weekdays 8am-6pm MST, with operations support.
</ul>

<h2>Limitations</h2>

<p>
Due to the "brand-new" nature of this part of Emulab, there are some
limitations you should be aware of:
</p>

<ul>
<li>Before you can use the mobile testbed, your project must be granted the
appropriate privileges.  You can request access by sending mail to <a
href="mailto:testbed-ops@flux.utah.edu">Testbed Operations</a>.
<li>The mobile testbed is currently open on non-holiday weekdays between
8am and 6pm mountain time, so we have staff available to assist with problems.
<li>There is no space sharing; only one mobile experiment can be swapped-in at
a time.
<li>Batteries must be replaced manually by the operator when levels are low.
</ul>

We expect to overcome these limitations over time; however, we are also eager to
introduce external users to the mobile testbed early on so we can integrate
their feedback.

</div>

<div class="docsection">
<h1>Mobile Experiments</h1>

<div class="docfig">
<span class="caption">Sample Movie</span>
<img src="robot_anim.gif" alt="Robot Movie">
<a href="robot_divx.avi">[DiVX (1.2M)]</a>
<a href="robot.mpg">[MPG (5.3M)]</a>
</a>
</div>

<p>
<a name="MOBILE"></a>
Creating a mobile wireless experiment is very similar to creating a regular
Emulab experiment: you construct an NS file, swap in the experiment, and then
you can log into the nodes to run your programs.  There are, of course, some
extra commands and settings that pertain to the physical manifestations of the
robots.  This tutorial will take you through the process of: creating a mobile
experiment, moving the robots to various destinations, creating random motion
scenarios, and "attaching" transmitter and receiver motes to the robots in your
experiment.
</p>

<h2>A First Experiment</h2>

Lets start with a simple NS script that will allocate a single robot located in
our building:

<code class="samplecode">
set ns [new Simulator]
source tb_compat.tcl

set topo [new Topography]
$topo load_area MEB-ROBOTS

$ns node-config -topography $topo

set node(0) [$ns node]

$node(0) set X_ 3.01
$node(0) set Y_ 2.49

$ns run

</code>
<span class="caption">Figure 1: Example NS file with mobile nodes.</span>

<p>
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
<a href="<?php echo $TBBASE ?>/robotmap.php3">here</a>, where the origin is in
the upper left hand corner, with positive X going right and positive Y going
down.  You can also click on the map to get a specific set of coordinates.
Note that any coordinates you specify must not fall inside an obstacle, or they will
be rejected by the system.  A Java applet that updates in real time is linked
from the above page, or can be accessed directly
<a href="<?php echo $TBBASE ?>/robotrack/robotrack.php3">here.</a>

<p>
With this NS file you can now create your first mobile experiment.  Actually
creating the experiment is the same as any other, except you might want to
check the "Do Not Swapin" checkbox so that the creation does not fail if
someone else is using the mobile testbed at the time.  Once the area is free
for use, you can swap-in your experiment and begin to work.

<h2>Adding Motion</h2>

<p>
Now that you have a node allocated, let's make it mobile.  During swap-in,
Emulab will start moving the node to its initial position.  You can watch its
progress by using the "Robot Map" menu item on the experiment page and checking
out the <a href="<?php echo $TBBASE ?>/webcam.php3">webcams</a> or
the <a href="<?php echo $TBBASE ?>/robotrack/robotrack.php3">applet version of the map</a>
that updates in real time.

<table width="100%" cellpadding=0 cellspacing=0 border=0 class="stealth">
<tr>
<?php NLCLINKFIG2("robotmap-ss.gif", 
		  "<img src=\"robotmap-ss-thumb.gif\" border=2
		  alt=\"Robot Map Screenshot\">",
		  "Sample Robot Map Screenshot",
		  "All four robots arranged in an 'L' shape.  The real world
		  coordinates for the robots are in the bottom middle table.")
		  ?>
<?php NLCLINKFIG2("webcam-ss.gif",
		  "<img src=\"webcam-ss-thumb.gif\" border=2
		  alt=\"Webcam Screenshot\">",
		  "Sample Webcam Screenshot",
		  "The real world view of the same four robots from the map
                  screenshot.  Note that the grid on the floor is used to
                  calibrate the overhead tracking cameras and not lines for the
                  robots to follow.") ?>
</tr>
</table>

<p>
Take a few moments to familiarize yourself with those pages since we'll be
making use of them during the rest of the tutorial.  One important item to note
on the robot map page is the "Elapsed event time" value, which displays how
much time has elapsed since the robots have reached their initial positions.
The elapsed time is also connected to when <code>"$ns at"</code> events in the
NS file are run.  In this case, there were no events in the NS file, so we'll
be moving the robot by sending dynamic SETDEST events, much like sending START
and STOP events to <a href="<? echo "$WIKIDOCURL/AdvancedExample" ?>">traffic
generators</a> and <a href="<? echo "$WIKIDOCURL/AdvancedExample#ProgramObjects" ?>">program
objects</a>.

<!-- XXX We need to give them a clue on which way the webcam is pointing in -->
<!-- relation to the robot map. -->

<p>
Once the robot has reached its initial position, lets move it up a meter.  To
do this, you will need to log in to ops.emulab.net and run:

<code class="samplecode">
1 ops:~> /usr/testbed/bin/tevc -e <b>proj</b>/<b>exp</b> \
         now node-0 SETDEST X=3.0 Y=1.5

</code>
<span class="caption">Figure 2: Command to send an event that will move the robot to
the coordinates (3.0, 1.5).  Don't forget to change <b>proj</b>/<b>exp</b> to
match your project and experiment IDs.</span>

<!-- mention that one setdest will override the previous. --> 

<p>
Then, check back with the map and webcams to see the results of your handiwork.
Try moving it around a few more times to get a feel for how things work and
where the robot can go.  Note that the robot should automatically navigate
around obstacles in the area, like the pole in the middle, so you do not have
to plot your own course around them.
</p>

<p>
In addition to driving the robot with dynamic events, you can specify a static
set of events in the NS file.  For example, you can issue the same move as
above at T +5 seconds by adding:

<code class="samplecode">
$ns at 5.0 "$node(0) setdest 3.01 1.5 0.1"

</code>
<span class="caption">Figure 3: NS syntax that moves the robot to the same
destination as in Figure 2.</span>

<p>
Note that "setdest" takes a third argument, the speed, in addition to the X and
Y coordinates.  The robot's speed is currently fixed at 0.1 meters per second.
</p>

<h2>Random Motion</h2>

<p>
Generating destination points for nodes can become quite a tedious task, so we
provide a modified version of the NS-2 "setdest" tool that will produce a valid
set of destination points for a given area.  The tool, called "tbsetdest", is
installed on ops and takes the following arguments:

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
interesting:

<code class="samplecode">
<i>...</i>
$ns node-config -topography $topo

set node(0) [$ns node]
<b>set node(1) [$ns node]</b>

</code>
<span class="caption">Figure 4: Excerpt of the original NS file with an additional
node.</span>

<p>
Then, use "tbsetdest" to produce some random motion for both robots:

<blockquote>
<pre>2 ops:~> /usr/testbed/bin/tbsetdest -n 2 -t 60 -a MEB-ROBOTS</pre>
</blockquote>

Here is some sample output from the tool:

<code class="samplecode">
$node(0) set X_ 3.01
$node(0) set Y_ 2.49
$node(1) set X_ 1.22
$node(1) set Y_ 3.61
set rtl [$ns event-timeline]
#
# nodes: 2, pause: 0.50, max x: 5.90, max y: 4.00
#
$rtl at 0.50 "$node(0) setdest 0.92 3.28 0.10"
$rtl at 0.50 "$node(1) setdest 0.61 3.02 0.10"
$rtl at 9.50 "$node(1) setdest 0.88 2.09 0.10"
$rtl at 19.64 "$node(1) setdest 2.80 2.07 0.10"
$rtl at 23.37 "$node(0) setdest 5.62 2.79 0.10"
$rtl at 39.43 "$node(1) setdest 4.98 1.65 0.10"
#
# Destination Unreachables: 0
#

</code>
<span class="caption">Figure 5: Sample "tbsetdest" output.</span>

<p>
You can then add the second node and motion events by clicking on the "Modify
Experiment" menu item on the experiment web page and:

<ol>
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
Once the modify completes, wait for the robots to reach their initial position
and then start the robots on their way by running the following on ops:

<code class="samplecode">
3 ops:~> /usr/testbed/bin/tevc -e <b>proj</b>/<b>exp</b> now rtl START

</code>
<blockquote>
<span class="caption">Figure 6: Command to start the "rtl" timeline.  Again, don't
forget to change <b>proj</b>/<b>exp</b> to match your project and experiment
IDs.</span>
</div>

<div class="docsection">
<h1>Wireless Traffic</h1>

<div class="docfig">
<a href="http://www.tinyos.net/scoop/special/hardware">
<img src="mica2-thumb.jpg" alt="Mica2 Mote">
<span class="caption">[Mica2 Mote]</span>
</a>
</div>

<p>
<a name="WIRELESS"></a>
Now that you are getting the hang of the mobility part of this testbed, we can
move on to working with wireless network traffic.  As stated earlier, each of
the robots carries a Mica2 mote (pictured on the right), which is a popular
device used in wireless sensor networks.  We'll be using the motes on the
mobile nodes you already have allocated and loading them with <a
href="http://www.tinyos.net">TinyOS</a> demo kernels, one that will be sending
traffic and the other receiving.
</p>

<h2>Adding Mobile Motes</h2>

Adding a couple of motes to your existing experiment can be done by doing a
modify and adding the following NS code:

<code class="samplecode">
## BEGIN mote nodes
$ns node-config -topography ""

set receiver [$ns node]
tb-set-hardware $receiver mica2
tb-set-node-os $receiver TinyOS-RfmLed
tb-fix-node $receiver $node(0)

set transmitter [$ns node]
tb-set-hardware $transmitter mica2
tb-set-node-os $transmitter TinyOS-CntRfm
tb-fix-node $transmitter $node(1)
## END mote nodes

</code>
<span class="caption">Figure 7: NS syntax used to "attach" motes to a robot.</span>

<p>
This code creates two mote nodes and "attaches" each of them to one of the
mobile nodes.  The OSs to be loaded on the mote nodes are the receiver,
TinyOS-RfmLed, and the transmitter, TinyOS-CntRfm.  These are standard
TinyOS kernels supplied by Emulab; uploading your own is covered below.
The receiver kernel will
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

<h2>Adding Fixed Motes</h2>

Adding a fixed mote to your experiment is slightly simpler than adding mobile
motes:

<code class="samplecode">
## BEGIN fixed mote nodes
set fixed-receiver [$ns node]
tb-set-hardware $fixed-receiver static-mica2
tb-set-node-os $fixed-receiver TinyOS-RfmLed
## END fixed mote nodes

</code>
<span class="caption">Figure 8: NS syntax used to add a fixed mote.</span>

<p>
This code creates a single mote and loads the same TinyOS image as was
previously loaded onto the mobile receiver mote.  Since the fixed
motes are mounted on serial programming boards, you will not be able to access
their LEDs as you did when adding mobile motes.

If you want to choose a specific mote from the topology (view placement and
positions by looking at the <a href="/robotmap.php3">robot map</a>), add the
following NS code:

<code class="samplecode">
tb-fix-node $fixed-receiver mote107

</code>
<span class="caption">Figure 9: NS syntax used to select a specific fixed mote.</span>

<p>
This code allows you to explicitly choose mote107, rather than allowing Emulab
to select a mote on your behalf.  Those who require very specific wireless network
topologies may wish to use this command.
</p>

<p>
You can use the 
<a href="<?php echo $TBBASE?>/wireless-stats/statsapp.php3">WSN Connectivity
Applet</a> to choose specific motes with desired 
link quality.  Then, using the mechanism above, you can bind the specific mote
you want to a node name in your experiment.

<h2>Custom Mote Applications</h2>

Uploading your own code to run on the motes is easy. Just build your TinyOS app
normally (ie. '<code>make mica2</code>').  Then, upload the binary that gets
placed in <code>build/mica2/main.srec</code> to our
<a href="<?php echo $TBBASE ?>/newimageid_ez.php3?nodetype=mote">mote image
    creation page</a>.  This page will ask you for a 'descriptor'.  This
descriptor can then be used in <code>tb-set-node-os</code> lines in your
NS files, and your app will be automatically loaded on the appropriate mote(s).

<p>
Alternatively, if you don't have a local TinyOS build environment, just load
ours onto an Emulab PC in your experiment.  You can do this by setting the node
operating system to <b>RHL90-TINYOS</b> using the <code>tb-set-node-os</code>
command (as shown in the Emulab <a
href="<? echo "$WIKIDOCURL/Tutorial" ?>">tutorial</a>).  This image is based off Emulab's
default RedHat 9.0 image and has an installation of a TinyOS 1.1.14 CVS
snapshot.  When you log in to the node, the $TOSROOT and $TOSDIR environment
variables will be set to /opt/tinyos-1.x and /opt/tinyos-1.x/tos,
respectively.  Your $CLASSPATH variable will also include the TinyOS Java paths
necessary to run many common Java applications supplied with TinyOS.
<!--
At this time, we don't have a TinyOS installation on the Emulab servers, so
you'll need to have a TinyOS installation to build from on your desktop
machine, or some other machine you control.  We hope to provide a way for you
build TinyOS apps on Emulab in the near future.  
-->
Also, at the current time, all
of our motes have radios in the 900MHz band, so see the TinyOS
<a href="http://www.tinyos.net/tinyos-1.x/doc/mica2radio/CC1000.html">CC1000
radio document</a> to make sure you're tuning the radios to the right band.

<p>
When you inevitably make changes to your code, you can simply place the new
kernel in the path that was automatically constructed for you by the image
creation page; the next time you use that OS in an NS file, the new version
will be loaded. If you'd like to load your node code onto your motes without
starting a new experiment, you have two options:
</p>
<ul>
  <li> <code>os_load</code> allows you to load an kernel that has already been
     defined as an image, as above. You give it the image descriptor with its
     <code>-i</code> argument, and you can either give the physical names of all
     motes you want to reload, or a <code>-e pid,eid</code> argument to reload
     all nodes in the given experiment.
  <li> <code>tbuisp</code> allows you to load a file directly onto your motes
     without having to register it as an image. This can be a quick way to do
     development/debugging. Just pass it the operation <code>upload</code>, the
     path to the file you wish to load, and the physical names of your motes.
</ul>
<p>
Both of these are commands in /usr/testbed/bin on ops.emulab.net.   They are
also available through our
<a href="<?php echo $TBBASE ?>/xmlrpcapi.php3">XML-RPC interface</a>, so you
can run them from your desktop machine to save time - the file given as an
argument to tbuisp is sent with the XML-RPC, so you don't need to copy it onto
our servers.
</p>

<h2>Mote Serial Interfaces</h2>

<p>
To facilitate mote interaction, logging, and evaluation, we provide easy
access to each mote's serial programming board's serial interface.  By simply
adding a PC node to your experiment, and adding some NS code, you can
access each mote's serial port directly on that PC.  To access
<code>mote107</code>'s serial interface, add the following NS code to your
experiment: 
</p>

<code class="samplecode">
## BEGIN adding mote serial line access
set manager [$ns node]
tb-set-node-os $manager RHL-TINYOS

$ns connect [$fixed-receiver console] $manager
##END adding mote serial line access

</code>
<span class="caption">Figure 10: Accessing mote serial interface.</span>

<p>
This code allocates a single PC, <code>manager</code>, which runs our
<emph>RHL-TINYOS</emph> image for convenience.  Emulab software exports the
serial port from the physical machine to the <code>manager</code> PC, where it
is available as a pseudo tty device.  You can read and write to the ptty
normally, except that some hardware-specific ioctl() syscalls may fail (this
happens because you are not working with the physical serial port).  The pseudo
tty will be available on your <code>manager</code> node as
<b>/dev/tip/MOTE_NAME</b> (in this case,
<b>/dev/tip/fixed-receiver</b>).  You can access other mote serial
interfaces by duplicating the above code, and changing the mote variable.  If
the software you are using to access the mote's serial interface insists on
using <b>/dev/ttyS1</b> or similar, you can simply run the following command on
your <code>manager</code> PC:

<code class="samplecode">
sudo rm /dev/ttyS1 && sudo ln -s /dev/tip/fixed-receiver /dev/ttyS1

</code>
<span class="caption">Figure 11: Easing the pain for applications that use a specific
serial device.</span>

<p>
If you need to use <b>/dev/ttyS0</b> on our RHL-TINYOS images, you may remove
and relink the device as shown in Fig. 11, but background monitor scripts may
decide to restart the serial port getty.  This will remove your link.  However,
you should only need to relink /dev/ttyS0 when you restart your program.
</div>

<div class="docsection">
<h1>Frequently Asked Questions</h1>

<table class="stealth">

<a name="FAQ"></a>
<tr><th>How do the robots keep from running into each other/objects?</th></tr>
<tr><td>
The robots are equipped with proximity sensors that can sense objects in their
path.  We use these to detect and navigate around other robots and obstacles
that aren't known ahead of time.
</td></tr>

<tr><th>Do the robots follow the lines on the floor?</th></tr>
<tr><td>
No, the lines on the floor are used to calibrate the overhead cameras that
track the position of the robot.
</td></tr>

</table>
</div>

<?php
#
# Standard Testbed Footer
# 
if (!$printable) {
    PAGEFOOTER();
}
?>

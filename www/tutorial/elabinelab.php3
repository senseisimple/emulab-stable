<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2007 University of Utah and the Flux Group.
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
    PAGEHEADER("Emulab Tutorial - Emulab in Emulab");
}

if (!$printable) {
    echo "<b><a href=$REQUEST_URI?printable=1>
             Printable version of this document</a></b><br>\n";
}
?>
<center>
<h2>Emulab Tutorial - Emulab in Emulab</h2>
</center>

<p>
Emulab in Emulab (henceforth known as "ElabInElab") is a new feature
that allows the creation of an entire (inner) testbed as an experiment
running on the (outer) testbed. The "inner" Emulab is functionally
equivalent to the "outer" Emulab, except for how it interacts with
certain aspects of the hardware infrastructure (switches, power
controllers, etc), which must be mediated by the outer Emulab to avoid
improper access to devices that an experiment is not normally allowed
to access directly. For example, in order for an inner Emulab to power
cycle a node, it must ask the outer Emulab to do it via a proxy that
ensures that the node in question is actually part of the inner Emulab
(a node in the experiment that comprises the inner Emulab).

<br>
ElabInElab serves several purposes:

<ul>
<li> Can be used to provide an isolated environment (in conjunction
     with firewalling) for running "dangerous" experiments that
     include the use of worms and other malware. Instead of running
     the experiment on the outer Emulab, the experiment is run on the
     inner Emulab, and thus has access to all of the Emulab's
     services, but in a context that does not put the outer Emulab at
     risk from attack. 

<li> Allows testing and development of Emulab itself in a controlled
     environment, without needing a dedicated testbed. New features
     can be tested without affecting users of the main testbed.
     In fact, multiple independent inner Emulabs can be constructed
     with each one being used for the testing and development of
     different features. 
</ul>

<p>
There are a few things to keep in mind about ElabInElab:

<ul>
<li> While it is tempting to think of ElabInElab as "recursive", it should
     be noted that real recursion is not supported; you cannot create
     an inner Emulab inside of an inner Emulab.

<li> The inner emulab has its own "boss" and "users" nodes, its own
     web server, its own file server, etc.

<li> From the outer Emulab's perspective, all of the nodes that make
     up the inner emulab are simply nodes in an experiment.

<li> All of the nodes consume one of their experimental network
     interfaces to use for the innner Emulab "control" network.
     Therefore, inner experimental nodes have one fewer experimental
     interface to use in experiments.
</ul>

Here is a simple example that sets up a tiny ElabInElab experiment,
with just a single inner experimental node. For the purposes of this
discussion, the project is "testbed" and the experiment is called
"myemulab." 
	<code><pre>
	source tb_compat.tcl
	set ns [new Simulator]

	tb-elab-in-elab 1

	set ::TBCOMPAT::elabinelab_maxpcs 1

	$ns run
	</code></pre>
which is instantiated as shown in the visualization in Figure 1.

  <br>
  <center>
  <table cellpadding='0' cellspacing='0' border='0' class='stealth'>
    <tr><td class='stealth'>
            <img class=stealth src="elabinelab-pic1.png" align=center>
	</td>
    </tr>
    <tr>
        <td align=center class='stealth'>
          <b>Figure 1.</b>
	</td>
    </tr>
  </table>
  </center>

<br>
As you can see in Figure 1, most of the details are handled for you;
the experiment includes a boss node, an ops node, and a single pc600,
all of which are connected via a lan. Once the experiment swaps in,
you can log into <tt>myops.myemulab.testbed.emulab.net</tt>, or you
can log into the web server at <tt>myboss.myemulab.testbed.emulab.net</tt>.
There is also a single experimental node that can be used to create an
experiment. In all aspects, the inner Emulab can be used the same way
that the outer Emulab can be used.

<br><br>
Another example:

	<code><pre>
	source tb_compat.tcl
	set ns [new Simulator]

	tb-elab-in-elab 1
        tb-set-inner-elab-eid myexp
	$ns run
	</code></pre>
In this example, we have included a <tt>tb-set-inner-elab-eid</tt>
directive, which says to automatically launch an experiment within the
inner Emulab once it is set up. The "myexp" experiment must already
exist in the same project; it must have already
been created, but not swapped in. The system uses the NS file associated
with the "myexp" experiment to construct an experiment on the inner
Emulab and swap it in. You will be notified via email, first when the
inner Emulab has been fully swapped in, and then again when the inner
experiment has been swapped in. You can interact with the inner
experiment normally, albeit from the inner boss (myboss) web interface
and the inner users node (myops), or you can log into the inner
experimental node (mypc1) directly.

<h3> Implementation Notes</h3>

Our goal was to make the inner Emulab look as much like a real Emulab
as possible. To do that, we decided to use one of the experimental
interfaces on each node as the (inner) control network lan (see Figure
1). This lan connects all of the nodes (myboss, myops, and mypc1) in
much the same way boss, ops, and experimental nodes are attached in
the outer Emulab. The main difference is that there are no firewalls
or subnets on the inner control network since the security concerns
are not as strict; breaking into the inner boss is not going to do any
more damage then does having root on any experimental node within the
(outer) testbed.

<br><br>
The (inner) control network is used for all of the same traffic that
nodes in the outer Emulab would; DHCP traffic, multicast disk
reloading, experiment setup, etc. While each node still has its outer
control network interface, that interface is not even configured,
except on inner boss and inner ops.

<br><br>
Since we want to be able to create experiments on the inner Emulab
normally, it is also necessary to "proxy" access to the Emulab
infrastructure. For example, when setting up an inner experiment, a
number of vlans will need to be created on the switches. Obviously, we
cannot let the inner boss access the switches (or any other protected
resources) directly; it must be mediated via a proxy on the outer
boss. The proxy on the outer boss checks to make sure that the actions
are allowed (and make sense), and then proceeds to do them itself. 


<br><br>
<b>Setup time</b>: Largely due to dynamic construction of extensive
parts of the inner Emulab environment, it currently takes about 20
minutes to set up ElabinElab on pc850s.  In the future we will reduce
this time considerably by more caching of pre-built components.
Some faster nodes (2 and 3 GHz) will soon be available, which will also help.


<?php
#
# Standard Testbed Footer
# 
if (!$printable) {
    PAGEFOOTER();
}
?>

<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004 University of Utah and the Flux Group.
# All rights reserved.
#
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
    PAGEHEADER("XMLRPC Interface to Emulab");
}

if (!$printable) {
    echo "<b><a href=$REQUEST_URI?printable=1>
             Printable version of this document</a></b><br>\n";
}

#
# Drop into html mode
#
?>
<p>
This page describes the XMLRPC interface to Emulab. Currently, the
interface mainly support experiment creation, modification, swapping,
and termination. We also provide interfaces to several other common
operations on nodes end experiments such as reboot, reloading, link
delay configuration, etc.
</p>

<p>
The Emulab XMLRPC server uses SSH as its transport. Yes, this is a little
different then other RPC servers, but since all registered Emulab
users already have accounts on Emulab and are required to use SSH to
log in (and thus have provided us with their public keys), we decided
this was easier on users then giving out SSL certificates that would need
to be kept track of. At some future time we may provide SSL or HTTPS
based servers, but for now rejoice in the fact that you do not need to
keep yet another certificate around!
</p>

<p>
The API is decribed in detail below. A demonstration client written in
Python is also available that you can use on your desktop to invoke
commands from the shell. For example:

    <code><pre>
    $ sshxmlrpc_client.py startexp batch=false wait=true pid="mypid" eid="myeid" nsfilestr="`cat ~/nsfile.ns`"</code></pre>

which says to create an experiment called "myeid" in the "mypid" project,
swap it in immediately, wait for the exit status (instead of running
asynchronously), passing inline the contents of <tt>nsfile.ns</tt> in your
home dir on your desktop.  By default, the client will contact the RPC
server at <tt><?php echo $BOSSNODE ?></tt>, but you can override that by
using the <tt>-s hostname</tt> option.. If your login ID on the local
machine is different then your login ID on Emulab, you can use the <tt>-l
login</tt> option. For example:

    <code><pre>
    $ sshxmlrpc_client.py -s boss.emulab.net -l rellots startexp ...</code></pre>

which would invoke the RPC server on <tt>boss.emulab.net</tt>, using the
login ID <tt>rellots</tt> (for the purposes of SSH authentication).  You
will be prompted for your SSH passphrase, unless you are running an SSH
agent and the key you have uploaded to Emulab has been added to your local
agent.
</p>

<p>
The <a href="downloads/xmlrpc/"><tt>sshxmlrpc_client</tt></a>
python program is a simple demonstration of how to use Emulab's RPC
server. If you do not provide a method and arguments on the command
line, it will enter a command loop where you can type in commands (method
and arguments) and wait for responses from the server. It converts your
command lines into RPCs to the server, and prints out the results that the
server sends back (exiting with whatever status code the server
returned). You can use this client program as is, or you can write your own
client program in whatever language you like, as long as you speak to the
server over an SSH connection. The API for the server is broken into
several different modules that export a number of methods, each of which is
described below. The python library we use to speak XMLRPC over an SSH
connection takes paths of the form:

    <code><pre>
    ssh://user@hostname/XMLRPC/module</code></pre>

where each <em>module</em> exports some methods. Each method is of the
form (in Python speak):

    <code><pre>
    def startexp(self, version, arguments):
        return EmulabResponse(RESPONSE_SUCCESS, value=0, output="Congratulations")</code></pre>

The arguments to each method:
<ul>
<li><tt>version</tt>: a numeric argument that the server uses to
determine if the client is really capable of speaking to the server.

<li><tt>arguments</tt>: a <em>hash table</em> of argument/value pairs,
which in Python is a <tt>Dictionary</tt>. In Perl or PHP this would be a
hashed array. Any client that supports such a datatype will be able to use
this interface directly. For example, to swap out an experiment a client
might:

    <code><pre>
    args = {};
    args["pid"] = "mypid"
    args["eid"] = "myeid"
    args["swapop"]  = "out"
    response = server->swapexp(CURRENTVERSION, args)</code></pre>
</ul>

The client specifies the <tt>pid</tt> and <tt>eid</tt> of the experiment
he/she wants to swap, as well as the actual swap operation, in this case
<tt>out</tt>. The response from the server is another hashed array (Python
Dictionary) of the form:

<blockquote>
    <ul>
     <li><tt>code</tt>: An integer code as defined in
     <a href="downloads/xmlrpc/"><tt>emulabclient.py</tt></a>.
     <li><tt>value</tt>: A return value. May be any valid data type that
     can be transfered in XML. 
     <li><tt>output</tt>: A string (with embedded newlines) to print out.
     This is useful for debugging and for guiding users through the perils
     of XMLRPC programming. 
    </ul>
</blockquote>

<ul>
<li><b>/XMLRPC/experiments</b>
<p>
The <tt>experiments</tt> module lets you start, control, and terminate
experiments.

  <ul>
  <li><tt><b>startexp</b></tt>: Create an experiment. By default, the experiment
  is started as a <a href="tutorial/tutorial.php3#BatchMode"><em>batch</em></a>
  experiment, but you can use the <tt>batchmode</tt> option described below to
  alter that behaviour. You can pass an NS file inline, or you can give the
  path of a file already on the Emulab fileserver.
  <br>
  <br>
  The required arguments are:<br><br>
  <table cellpadding=2>
  <tr>
    <td>name</td><td>type</td><td>description</td>
  </tr>
  <tr></tr>
  <tr>
    <td><tt>pid</tt></td>
    <td>string</td>
    <td>The Emulab project ID in which to create the experiment</td>
  </tr>
  <tr>
    <td><tt>eid</tt></td>
    <td>string</td>
    <td>The unique ID to call the experiment</td>
  </tr>
  <tr>
    <td><tt>nsfilestr</tt></td>
    <td>string</td>
    <td> A string representing the NS file to use, with embedded newlines,
         <b>or</b>,</td>
  </tr>
  <tr>
    <td><tt>nsfilepath</tt></td>
    <td>string</td>
    <td>The pathname of a NS file on the Emulab file
         server, within the project directory<br>
	 (example: /proj/mypid/foo.ns)</td>
  </tr>
  </table>
  <br>
  The optional arguments are:<br><br>
  <table cellpadding=2>
   <tr>
    <td>name</td><td>type</td><td>default</td><td>description</td>
   </tr>
   <tr></tr>
   <tr>
    <td><tt>gid</tt></td>
    <td>string</td>
    <td>pid</td>
    <td>The Emulab subgroup ID in which to create the experiment<br>
        (defaults to project id)</td>
   </tr>
   <tr>
    <td><tt>batch</tt></td>
    <td>boolean</td>
    <td>true</td>
    <td>Create a
         <a href="tutorial/tutorial.php3#BatchMode"><em>batch</em></a>
         experiment. Value is either "true" or "false"</td>
   </tr>
   <tr>
    <td><tt>description</tt></td>
    <td>string</td>
    <td>&nbsp</td>
    <td>A pithy sentence describing your experiment</td>
   </tr>
   <tr>
    <td><tt>swappable</tt></td>
    <td>boolean</td>
    <td>true</td>
    <td>Experiment may be swapped at any time. If false, you must provide a
        reason in <tt>noswap_reason</tt></td> 
   </tr>
   <tr>
    <td><tt>noswap_reason</tt></td>
    <td>string</td>
    <td>&nbsp</td>
    <td>A sentence describing why your experiment cannot be swapped</td> 
   </tr>
   <tr>
    <td><tt>idleswap</tt></td>
    <td>integer</td>
    <td>variable</td>
    <td>How long (in minutes) before your idle experiment can be 
         <a href="docwrapper.php3?docname=swapping.html#idleswap">idle
	 swapped</a>. Defaults to a value between two and four hours.  A
	 value of zero means never idleswap (you must provide a reason in
	 <tt>noidleswap_reason</tt>)</td>
   </tr>
   <tr>
    <td><tt>noidleswap_reason</tt></td>
    <td>string</td>
    <td>&nbsp</td>
    <td>A sentence describing why your experiment cannot be idle swapped</td> 
   </tr>
   <tr>
    <td><tt>autoswap</tt></td>
    <td>integer</td>
    <td>0</td>
    <td>How long (in minutes) before your experiment
     should be <a href="docwrapper.php3?docname=swapping.html#autoswap">auto
     swapped</a>. A value of zero means never auto swap.
   </tr>
   <tr>
    <td><tt>noswapin</tt></td>
    <td>boolean</td>
    <td>false</td>
    <td>If true, do not swap the experiment in immediately; just "preload"
    the NS file. The experiment can be swapped in later with swapexp.</td>
   </tr>
   <tr>
    <td><tt>wait</tt></td>
    <td>boolean</td>
    <td>false</td>
    <td>If true, wait synchronously for the experiment to finish swapping.
    By default, control returns immediately, and you must wait for email
    notification to determine if the operation succeeded</td>
   </tr>
  </table>

  <br>
  <li><tt><b>swapexp</b></tt>: Swap an experiment in or out. The experiment
  must, of course, be in the proper state for requested operation.
  <br>
  <br>
  The required arguments are:<br><br>
  <table cellpadding=2>
  <tr>
    <td>name</td><td>type</td><td>description</td>
  </tr>
  <tr></tr>
  <tr>
    <td><tt>pid</tt></td>
    <td>string</td>
    <td>The Emulab project ID of the experiment</td>
  </tr>
  <tr>
    <td><tt>eid</tt></td>
    <td>string</td>
    <td>The Emulab experiment ID</td>
  </tr>
  <tr>
    <td><tt>direction</tt></td>
    <td>string</td>
    <td>The direction in which to swap; one of "in" or "out"
  </tr>
  </table>
  <br>
  The optional arguments are:<br><br>
  <table cellpadding=2>
   <tr>
    <td>name</td><td>type</td><td>default</td><td>description</td>
   </tr>
   <tr></tr>
   <tr>
    <td><tt>wait</tt></td>
    <td>boolean</td>
    <td>false</td>
    <td>If true, wait synchronously for the experiment to finish swapping
    in (or preloading if <tt>noswapin</tt> is true). By default, control
    returns immediately, and you must wait for email notification to
    determine if the operation succeeded</td>
   </tr>
  </table>

  <br>
  <li><tt><b>modify</b></tt>: Modify an experiment, either while it is
  swapped in or out. You must provide an NS file to direct the
  modification. 
  <br>
  <br>
  The required arguments are:<br><br>
  <table cellpadding=2>
  <tr>
    <td>name</td><td>type</td><td>description</td>
  </tr>
  <tr></tr>
  <tr>
    <td><tt>pid</tt></td>
    <td>string</td>
    <td>The Emulab project ID of the experiment</td>
  </tr>
  <tr>
    <td><tt>eid</tt></td>
    <td>string</td>
    <td>The Emulab experiment ID</td>
  </tr>
  <tr>
    <td><tt>nsfilestr</tt></td>
    <td>string</td>
    <td> A string representing the NS file to use, with embedded newlines,
         <b>or</b>,</td>
  </tr>
  <tr>
    <td><tt>nsfilepath</tt></td>
    <td>string</td>
    <td>The pathname of a NS file on the Emulab file
         server, within the project directory<br>
	 (example: /proj/mypid/foo.ns)</td>
  </tr>
  </table>
  <br>
  The optional arguments are:<br><br>
  <table cellpadding=2>
   <tr>
    <td>name</td><td>type</td><td>default</td><td>description</td>
   </tr>
   <tr></tr>
   <tr>
    <td><tt>wait</tt></td>
    <td>boolean</td>
    <td>false</td>
    <td>If true, wait synchronously for the experiment to finish swapping
    in (or preloading if <tt>noswapin</tt> is true). By default, control
    returns immediately, and you must wait for email notification to
    determine if the operation succeeded</td>
   </tr>
   <tr>
    <td><tt>reboot</tt></td>
    <td>boolean</td>
    <td>false</td>
    <td>If true and the experiment is swapped in, reboot all nodes in the
    experiment</td>
   </tr>
   <tr>
    <td><tt>restart_eventsys</tt></td>
    <td>boolean</td>
    <td>false</td>
    <td>If true and the experiment is swapped in, restart the event system
    (all events are rerun from time zero)</td>
   </tr>
  </table>

  <br>
  <li><tt><b>endexp</b></tt>: Terminate an experiment.
  The required arguments are:<br><br>
  <table cellpadding=2>
  <tr>
    <td>name</td><td>type</td><td>description</td>
  </tr>
  <tr></tr>
  <tr>
    <td><tt>pid</tt></td>
    <td>string</td>
    <td>The Emulab project ID in which the experiment was created</td>
  </tr>
  <tr>
    <td><tt>eid</tt></td>
    <td>string</td>
    <td>The Emulab ID of the experiment to terminate</td>
  </tr>
  </table>
  
  <br>
  The optional arguments are:<br><br>
  <table cellpadding=2>
   <tr>
    <td>name</td><td>type</td><td>default</td><td>description</td>
   </tr>
   <tr></tr>
   <tr>
    <td><tt>wait</tt></td>
    <td>boolean</td>
    <td>false</td>
    <td>If true, wait synchronously for the experiment to finish terminating.
    By default, control returns immediately, and you must wait for email
    notification to determine if the operation succeeded</td>
   </tr>
  </table>
  
  </ul>
</ul>

<?php
#
# Standard Testbed Footer
# 
if (!$printable) {
    PAGEFOOTER();
}
?>


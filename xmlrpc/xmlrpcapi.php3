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
    $ sshxmlrpc_client.py startexp -i -w mypid myeid ~/nsfile.ns</code></pre>

which says to create an experiment called "myeid" in the "mypid"
project, swap it in immediately, wait for the exit status (instead of
running asynchronously), using the NS file nsfile.ns in your home dir.
By default, the client will contact the RPC server at
<tt><?php echo $BOSSNODE ?></tt>, but you can override that by using
the <tt>-s hostname</tt> option to sshxmlrpc_client. If your login ID
on the local machine is different then your login ID at Emulab, you
can use the <tt>-l login</tt> option. For example:

    <code><pre>
    $ sshxmlrpc_client.py -s boss.emulab.net -l rellots startexp ...</code></pre>

which would invoke the RPC server on <tt>boss.emulab.net</tt>, using the login
ID <tt>rellots</tt> (for the purposes of SSH authentication). You will
be prompted for your SSH passphrase, unless you are running an SSH
agent and the key you have uploaded to Emulab has been added to your
local agent.
</p>

<p>
The <tt>sshxmlrpc_client</tt> python program is a simple demonstration of
how to use Emulab's RPC server. It converts command lines into RPCs to the
server, and prints out the results that the server sends back, exiting with
whatever status code the server returned. You can use this client program
as is, or you can write your own client program in whatever language you
like, as long as you speak to the server over an SSH connection. The API
for the server is broken into several different modules that export a
number of methods, each of which is described below. The python library we
use to speak XMLRPC over an SSH connection takes paths of the form:

    <code><pre>
    ssh://user@hostname/XMLRPC/module</code></pre>

where each <em>module</em> exports some methods. Each method is of the
form (in Python speak):

    <code><pre>
    def startexp(self, version, arguments):
        return status</code></pre>

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
    return server->swapexp(CURRENTVERSION, args)</code></pre>
</ul>

The client specifies the <tt>pid</tt> and <tt>eid</tt> of the experiment
he/she wants to swap, as well as the actual swap operation, in this case
<tt>out</tt>. 

<ul>
<li><b>/XMLRPC/experiments</b>
<p>
The <tt>experiments</tt> module lets you start, control, and terminate
experiments.

  <ul>
  <li><tt>startexp</tt>: Create an experiment. By default, the experiment
  is started as a <a href="tutorial/tutorial.php3#BatchMode"><em>batch</em></a>
  experiment, but you can use the <tt>batchmode</tt> option described below to
  alter that behaviour. You can pass an NS file inline, or you can give the
  path of a file already on the Emulab fileserver.
  <br>
  <br>
  The required arguments are:
    <ul>
     <li><tt>pid</tt>: The Emulab project ID in which to create the experiment.
     <li><tt>eid</tt>: The unique ID to call the experiment.
     <li><tt>nsfilestr</tt>: A string representing the NS file to use, with
     embedded newlines, <b>or</b>,
     <li><tt>nsfilepath</tt>: The pathname of a NS file on the Emulab file
     server, within the project directory (example: /proj/mypid/foo.ns).
    </ul>
  <br>
  The optional arguments are:
    <ul>

    </ul>
    
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


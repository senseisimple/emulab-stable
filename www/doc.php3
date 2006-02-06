<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2004, 2005 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Documentation");
?>

<h3>Getting Started on the Testbed</h3>

<p>These documents are highly recommended reading for all Emulab users.</p>

<ul>
<li><dl>
<dt><b><a href="docwrapper.php3?docname=auth.html">
	      How to get an account on the Testbed</a></b></dt>
    <dd>Starting or joining projects, authorization, and policies.</dd>
</dl><br>

<li><dl>
<dt><b><a href="tutorial/docwrapper.php3?docname=tutorial.html">
              Emulab "Getting Started" Tutorial</a></b></dt>
    <dd>The basics of creating experiments, and a quick intro to frequently
        used features, avoiding common pitfalls, and quick solutions to the
        most common problems people encounter.</dd>
</dl><br>

<li><dl>
<dt><b><a href="faq.php3">
              Frequently Asked Questions (FAQ)</a></b></dt>
    <dd>Probably our most important reference document. Check here before
        asking for help. Includes sections on "Getting Started" on Emulab,
        using the testbed, hardware setup, software setup, security issues,
        and troubleshooting. Over 50 questions and answers.</dd>
</dl><br>

<li><dl>
<dt><b><a href="doc/docwrapper.php3?docname=plab.html">
              Emulab PlanetLab Interface</a></b></dt>
    <dd>Documentation for our interface to PlanetLab.</dd>
</dl>

</ul>

<h3>Reference Material</h3>
<ul>
<li><b><a href="tutorial/docwrapper.php3?docname=tutorial.html#Advanced">
              Emulab Advanced Tutorial</a></b>
<li><b><a href="tutorial/docwrapper.php3?docname=advanced.html">
              Emulab Advanced Example</a></b>
<li><b><a href="kb-search.php3">
              Search or Browse the Emulab Knowledge Base</a></b>
<li><b><a href="tutorial/docwrapper.php3?docname=nscommands.html">
              Emulab-specific NS Extensions Reference Manual</a></b>
<li><b><a href = "xmlrpcapi.php3">Emulab's XML-RPC interface reference</a></b>
<li><b><a href="doc/docwrapper.php3?docname=tmcd.html">
              Testbed Master Control Daemon (TMCD) Reference Manual</a></b>
<li><b><a href = "docwrapper.php3?docname=swapping.html">
          Node Usage Policies and "Swapping" Experiments</a></b>
<li><b><a href = "doc/docwrapper.php3?docname=netbuilddoc.html">
          NetBuild Reference Manual</a></b>
<li><b><a href = "docwrapper.php3?docname=security.html">
          Security Issues</a></b>
<li><b><a href = "docwrapper.php3?docname=groups.html">
          About Project Groups</a></b>
<li><b><a href = "doc/docwrapper.php3?docname=internals.html">
          Emulab Internals</a></b>
<li><b><a href = "doc/docwrapper.php3?docname=hw-recommend.html">
          Want to build your own Testbed?</a></b>
<li><b><a href = "docwrapper.php3?docname=hardware.html">
          Current Hardware Overview</a></b>
<li><b><a href = "docwrapper.php3?docname=software.html">
          Current Software Overview</a></b>
</ul>

<h3>Miscellaneous</h3>
<ul>
<li><b><a href = "docwrapper.php3?docname=policies.html">
          Administrative Policies and Disclaimer</a></b>
<li><b><a href = "docwrapper.php3?docname=sponsors.html">
          Emulab Sponsors</a></b>
<p>
<li><b><a href = "news.php3">
          Current and Past Emulab News Items</a></b>
<li><b><a href = "doc/docwrapper.php3?docname=ChangeLog.txt">
          Changelog/Technical Details</a></b>
<li><b><a href = "pubs.php3">
          Papers and Talks</a></b>
</ul>

<a name="tutorial">
<h3>SIGCOMM Tutorial</h3>
</a>

<p>
You might also find it helpful to look through the tutorial given at
SIGCOMM '02 by Jay Lepreau, Robert Ricci, and Mac
Newbold, entitled "How To Use the Netbed (Emulab++) Network Testbeds."
</p>

<ul>
<li><b>Slides</b> from the tutorial are available in
    <a href = "http://www.cs.utah.edu/flux/testbed-docs/sigcomm02-tutorial/netbed-tut-sigcomm02.ppt">
          PowerPoint Format</a>, 
    and as PDFs, with 	
    <a href = "http://www.cs.utah.edu/flux/testbed-docs/sigcomm02-tutorial/netbed-tut-sigcomm02-2up.pdf">
          2 slides per page (full color)</a>, and
    <a href = "http://www.cs.utah.edu/flux/testbed-docs/sigcomm02-tutorial/netbed-tut-sigcomm02-grey-6up.pdf">
          6 slides per page (greyscale)</a>.

<li><b>Tutorial files</b> such as NS files for the experiments, are available
    as a
    <a href = "http://www.cs.utah.edu/flux/testbed-docs/sigcomm02-tutorial/tutfiles.tar.gz">
          tarball</a>, or
    <a href = "http://www.cs.utah.edu/flux/testbed-docs/sigcomm02-tutorial/tutfiles">
          individually</a>.
</ul>

<?php
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>


<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2008 University of Utah and the Flux Group.
# All rights reserved.
#
#
include("pub_defs.php");

$optargs = OptionalPageArguments("printable",  PAGEARG_BOOLEAN);

if (!isset($printable))
    $printable = 0;

$this_user = null;
$isadmin = null;

if (!$printable && LoginStatus()) {
    RedirectHTTPS();
    $this_user = CheckLoginOrDie();
    $isadmin   = ISADMIN();
}

#
# Standard Testbed Header
#
if ($printable) {
    #
    # Need to spit out some header stuff.
    #
    echo "<html>
          <head>
  	  <link rel='stylesheet' href='../tbstyle-plain.css' type='text/css'>
          </head>
          <body>\n";
} else {
    PAGEHEADER("Bibliography");
    echo "<b><a href=$REQUEST_URI?printable=1>
          Printable version of this document</a></b><br>\n";
}
?>

<p>
This page summarizes publications about Emulab as well as publications
that used Emulab to validate research they present.  
<p>

If you have a publications that used Utah's Emulab to validate your
research please use <a href="submitpub.php">this form</a> to add it.
For other updates or corrections please email 
<a href="mailto:papers@emulab.net">papers@emulab.net</a>.

</p>

<?php

$query_result = GetPubs("`visible`");
echo MakeBibList($this_user, $isadmin, $query_result);

if ($isadmin) {
    echo '<p><a href="deleted_pubs.php">Show Deleted Publications</a></p>';
}

#
# Standard Testbed Footer
# 
if ($printable) {
    echo "</body>
          </html>\n";
}
else {
    PAGEFOOTER();
}
?>

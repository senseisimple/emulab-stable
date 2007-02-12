<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("node_defs.php");

#
# Standard Testbed Header
#
PAGEHEADER("Node History");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

if (! ($isadmin || OPSGUY())) {
    USERERROR("Cannot view node history.", 1);
}

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("showall",   PAGEARG_BOOLEAN,
				 "reverse",   PAGEARG_BOOLEAN,
				 "count",     PAGEARG_INTEGER,
				 "node",      PAGEARG_NODE);

if (!isset($showall)) {
    $showall = 0;
}
if (!isset($count)) {
    $count = 20;
}
if (!isset($reverse)) {
    $reverse = 1;
}
$node_id = (isset($node) ? $node->node_id() : "");

$opts="node_id=$node_id&count=$count&reverse=$reverse";
echo "<b>Show records: ";
if ($showall) {
    echo "<a href='shownodehistory.php3?$opts'>allocated only</a>,
          all";
} else {
    echo "allocated only,
          <a href='shownodehistory.php3?$opts&showall=1'>all</a>";
}

$opts="node_id=$node_id&count=$count&showall=$showall";
echo "<br><b>Order by: ";
if ($reverse == 0) {
    echo "<a href='shownodehistory.php3?$opts&reverse=1'>lastest first</a>,
          earliest first";
} else {
    echo "lastest first,
          <a href='shownodehistory.php3?$opts&reverse=0'>earliest first</a>";
}

$opts="node_id=$node_id&showall=$showall&reverse=$reverse";
echo "<br><b>Show number: ";
if ($count != 20) {
    echo "<a href='shownodehistory.php3?$opts&count=20'>first 20</a>, ";
} else {
    echo "first 20, ";
}
if ($count != -20) {
    echo "<a href='shownodehistory.php3?$opts&count=-20'>last 20</a>, ";
} else {
    echo "last 20, ";
}
if ($count != 0) {
    echo "<a href='shownodehistory.php3?$opts&count=0'>all</a>";
} else {
    echo "all";
}

ShowNodeHistory((isset($node) ? $node : null), $showall, $count, $reverse);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

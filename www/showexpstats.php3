<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2010 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# This page needs more work to make user friendly and flexible.
# Its a hack job at the moment.
# 

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$uid_idx   = $this_user->uid_idx();
$isadmin   = ISADMIN();

#
# Verify page arguments
#
$optargs = OptionalPageArguments("record", PAGEARG_INTEGER);

#
# Standard Testbed Header
#
PAGEHEADER("Show Experiment Information");

#
# Right now we show just the last N records entered, unless the user
# requested a specific record. 
#
if (isset($record)) {
    $wclause = "";
    if (! $isadmin) {
	$wclause = "and s.creator_idx='$uid_idx'";
    }
    $query_result =
	DBQueryFatal("select s.*,'foo',r.* from experiment_stats as s ".
		     "left join experiment_resources as r on ".
		     " r.exptidx=s.exptidx ".
		     "where s.exptidx=$record $wclause ".
		     "order by r.tstamp desc");

    if (mysql_num_rows($query_result) == 0) {
	USERERROR("No such experiment record $record in the system!", 1);
    }
}
else {
    $wclause = "";
    if (! $isadmin) {
	$wclause = "where s.creator_idx='$uid_idx'";
    }
    $query_result =
	DBQueryFatal("select s.*,'foo',r.* from experiment_stats as s ".
		     "left join experiment_resources as r on ".
		     " r.exptidx=s.exptidx ".
		     "$wclause ".
		     "order by s.exptidx desc,r.idx asc limit 200");

    if (mysql_num_rows($query_result) == 0) {
	USERERROR("No experiment records in the system!", 1);
    }
}

#
# Use first row to get the column headers (no pretty printing yet).
# 
$row = mysql_fetch_assoc($query_result);

echo "<table align=center border=1>\n";
echo "<tr>\n";
foreach($row as $key => $value) {
    $key = str_replace("_", " ", $key);

    if ($key == "foo")
	continue;
    
    echo "<th><font size=-1>$key</font></th>\n";
}
echo "</tr>\n";

mysql_data_seek($query_result, 0);

$print_header = 0;
$last_exptidx = 0;

while ($row = mysql_fetch_assoc($query_result)) {
    $rsrcidx = $row["idx"];
    $exptidx = $row["exptidx"];

    $skipping = 1;

    if ($last_exptidx != $exptidx) {
	$print_header  = 1;
	$last_exptidx = $exptidx;
    }

    echo "<tr>\n";
    foreach($row as $key => $value) {
	if ($key == "thumbnail") {
	    echo "<td nowrap>
                     <a href='showthumb.php3?idx=$rsrcidx'>Thumbnail</a>
                  </td>\n";
	}
	elseif ($key == "input_data_idx") {
	    echo "<td nowrap>
                     <a href='spitnsdata.php3?record=$rsrcidx'>$value</a>
                  </td>\n";
	}
	else {
	    if (!$print_header && $skipping) {
		if ($key == "foo") {
		    $skipping = 0;
		    continue;
		}
		echo "<td nowrap>&nbsp</td>\n";		
		continue;
	    }
	    if ($key == "foo")
		continue;
	    
	    echo "<td nowrap>$value</td>\n";
	}
    }
    $print_header = 0;
    echo "</tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Experiment Admission Control Policies");

#
#
# Only known and logged in users allowed.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Only admin people can see this page.
# 
if (!$isadmin && !STUDLY()) {
    USERERROR("You are not allowed to view this page!", 1);
}

#
# Global policies.
#
$query_result = DBQueryFatal("select * from global_policies ".
			     "order by policy,test");

if (mysql_num_rows($query_result)) {
    echo "<br>
          <center><h3>Global Policies</h3></center>
          <table border=2 cellpadding=0 cellspacing=2 align='center'>\n";
    
    echo "<tr>
              <th>Policy</th>
              <th>Test</th>
              <th>Count</th>
              <th>Aux Data</th>
          </tr>\n";
    
    while ($row = mysql_fetch_array($query_result)) {
        $policy  = $row["policy"];
        $test    = $row["test"];
        $count   = $row["count"];
        $auxdata = $row["auxdata"];
    
	if (!$auxdata)
	    $auxdata = "&nbsp";
    
        echo "<tr>
                  <td>$test</td>
                  <td>$policy</td>
                  <td>$count</td>
                  <td>$auxdata</td>\n";
        echo "</tr>\n";
    }
    echo "</table>\n";
}

#
# Group policies.
#
$query_result = DBQueryFatal("select * from group_policies ".
			     "order by pid,gid,policy");

if (mysql_num_rows($query_result)) {
    echo "<br>
          <center><h3>Group Policies</h3></center>
          <table border=2 cellpadding=0 cellspacing=2 align='center'>\n";
    
    echo "<tr>
              <th>Pid</th>
              <th>Gid</th>
              <th>Policy</th>
              <th>Count</th>
              <th>Aux Data</th>
          </tr>\n";
    
    while ($row = mysql_fetch_array($query_result)) {
        $pid     = $row["pid"];
        $gid     = $row["gid"];
        $policy  = $row["policy"];
        $count   = $row["count"];
        $auxdata = $row["auxdata"];
    
	if (!$auxdata)
	    $auxdata = "&nbsp";
    
        echo "<tr>
                  <td><a href='showproject.php3?pid=$pid'>$pid</a></td>
                  <td><a href='showgroup.php3?pid=$pid&gid=$gid'>$gid</a></td>
                  <td>$policy</td>
                  <td>$count</td>
                  <td>$auxdata</td>\n";
        echo "</tr>\n";
    }
    echo "</table>\n";
}

#
# User policies.
#
$query_result = DBQueryFatal("select * from user_policies ".
			     "order by uid,policy");

if (mysql_num_rows($query_result)) {
    echo "<br>
          <center><h3>User Policies</h3></center>
          <table border=2 cellpadding=0 cellspacing=2 align='center'>\n";
    
    echo "<tr>
              <th>Uid</th>
              <th>Policy</th>
              <th>Count</th>
              <th>Aux Data</th>
          </tr>\n";
    
    while ($row = mysql_fetch_array($query_result)) {
        $puid    = $row["uid"];
        $policy  = $row["policy"];
        $count   = $row["count"];
        $auxdata = $row["auxdata"];

	if (! ($user = User::Lookup($puid))) {
	    TBERROR("Could not lookup object for user $puid", 1);
	}
	$showuser_url = CreateURL("showuser", $user);
	
	if (!$auxdata)
	    $auxdata = "&nbsp";
    
        echo "<tr>
                  <td><A href='$showuser_url'>$puid</a></td>
                  <td>$policy</td>
                  <td>$count</td>
                  <td>$auxdata</td>\n";
        echo "</tr>\n";
    }
    echo "</table>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

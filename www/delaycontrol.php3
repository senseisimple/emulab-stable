<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
# include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Delay Control");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);
if (!$isadmin) {
	USERERROR("This feature not available to non-admins yet.", 1);
}

$gid = "nobody";

if ($check == "1") {
  print "<pre>\n" .
        "Changes:\n";
  $i = 0;
  while (isset($GLOBALS["X$i"]) && 
         !empty($GLOBALS["X$i"]) &&
         ereg("^[0-9a-zA-Z\-\_]*$", $GLOBALS["X$i"])) {
    $cmd = "$pid $eid " . $GLOBALS["X$i"];
    $cmd0 = "";
    $cmd1 = "";
    if (is_numeric($GLOBALS["Cl0$i"])) {
      $cmd0 .= " -l " . $GLOBALS["Cl0$i"];
    }
    if (is_numeric($GLOBALS["Cb0$i"])) {
      $cmd0 .= " -b " . $GLOBALS["Cb0$i"];
    }
    if (is_numeric($GLOBALS["Cd0$i"])) {
      $cmd0 .= " -d " . $GLOBALS["Cd0$i"];
    }
    if (is_numeric($GLOBALS["Cl1$i"])) {
      $cmd1 .= " -l " . $GLOBALS["Cl1$i"];
    }
    if (is_numeric($GLOBALS["Cb1$i"])) {
      $cmd1 .= " -b " . $GLOBALS["Cb1$i"];
    }
    if (is_numeric($GLOBALS["Cd1$i"])) {
      $cmd1 .= " -d " . $GLOBALS["Cd1$i"];
    }
    if (!empty($cmd0)) {
      $cmd0 = "webdelay_config" . "$cmd0 -p 0 $cmd";
      print "Running '$cmd0':\n";
      if ($fp = popen("$TBSUEXEC_PATH $uid $gid $cmd0", "r")) {
        fpassthru($fp);
      }
      print "-=-=-=-\n";
    }
    if (!empty($cmd1)) {
      $cmd1 = "webdelay_config" . "$cmd1 -p 1 $cmd";

      print "Running '$cmd1':\n";
      if ($fp = popen("$TBSUEXEC_PATH $uid $gid $cmd1", "r")) {
        fpassthru($fp);
      }
      print "-=-=-=-\n";
    }
    $i++;
  }
}
print "</pre>\n";
//
//if ($check == "1") {
//  print "<h1>Something submitted.</h1><pre>\n";
//  while (list($key, $val) = @each($HTTP_POST_VARS)) {
//    print "$key = $val\n";
//    if ($key{0} == "C" && is_numeric($val)) {
//       
//    }
//    // $GLOBALS[$key] = $val;
//  }
//  print "</pre>";
//} 

$query_result =
    DBQueryFatal("SELECT * FROM delays WHERE eid='$eid' AND pid='$pid'");
if (mysql_num_rows($query_result) == 0) {
    USERERROR("No running delay nodes with eid='$eid' and pid='$pid'!", 1);
}

print "<form action='delaycontrol.php3' method=post>\n" .
      "<input type=hidden name=check value=1 />\n" .
      "<input type=hidden name=eid value='$eid' />\n" .
      "<input type=hidden name=pid value='$pid' />\n" .
      "<table>\n" .
      "<tr>" .
      "<th>Link Name</th>".
      "<th>Delay Node ID</th>".
      "<th>Delay0 (msec)</th>".
      "<th>Bandwidth0 (kb/s)</th>".
      "<th>Loss0 (ratio)</th>".
      "<th>Delay1 (msec)</th>".
      "<th>Bandwidth1 (kb/s)</th>".
      "<th>Loss1 (ratio)</th></tr>";
           
$num = mysql_num_rows( $query_result );
for( $i = 0; $i < $num; $i++ ) {
   $field = mysql_fetch_array( $query_result );
   
   print "<tr>\n";
   print "<td>" . $field["vname"] . " " . 
         "<input type=hidden name=X$i value=\"".$field["vname"]."\" /></td>";
   print "<td>" . $field["node_id"]    . "</td>\n";
   print "<td>" . $field["delay0"]     . 
         "<br><input type=text name=Cd0$i size=6/>" . "</td>\n";
   print "<td>" . $field["bandwidth0"]  . 
         "<br><input type=text name=Cb0$i size=6/>" . "</td>\n";
   print "<td>" . $field["lossrate0"] . 
         "<br><input type=text name=Cl0$i size=10/>" . "</td>\n";
   print "<td>" . $field["delay1"]     . 
         "<br><input type=text name=Cd1$i size=6/>" . "</td>\n";
   print "<td>" . $field["bandwidth1"]  . 
         "<br><input type=text name=Cb1$i size=6/>" . "</td>\n";
   print "<td>" . $field["lossrate1"] . 
         "<br><input type=text name=Cl1$i size=10/>" . "</td>\n";
   print "</tr>\n";
}

print "</table>\n" .
      "<input type=submit value='Change' />\n" .
      "</form>\n";

PAGEFOOTER();
?>


<html>
<head>
<title>New Users Approved</title>
<link rel='stylesheet' href='tbstyle.css' type='text/css'>
</head>
<body>
<?php
include("defs.php3");

#
# Only known and logged in users can be verified.
#
$auth_usr = "";
if (ereg("php3\?([[:alnum:]]+)", $REQUEST_URI, $Vals)) {
    $auth_usr=$Vals[1];
    addslashes($auth_usr);
}
else {
    unset($auth_usr);
}
LOGGEDINORDIE($auth_usr);

echo "
<h1>Approving new users...</h1>
";
$query="SELECT pid FROM proj_memb WHERE uid='$auth_usr' and trust='group_root'";
$result = mysql_db_query("tbdb", $query);
$select = "SELECT";
$project[0]="";
$n=0;
while ($row = mysql_fetch_row($result)) {
  $pid = $row[0];
  $project[$n]=$pid;
  $n = $n + 1;
  if ($select == "SELECT") {
    $select .= " DISTINCT uid FROM proj_memb WHERE pid='$pid'";
  } else {
    $select .= " OR pid='$pid'";
  }
}
$selected = mysql_db_query("tbdb", $select);
$find = "SELECT";
while ($row = mysql_fetch_row($selected)) {
  $uid = $row[0];
  if ($find == "SELECT") {
    $find .= " DISTINCT uid,status,usr_email FROM users WHERE (status='newuser' OR status='unapproved') AND (uid='$uid'";
  } else {
    $find .= " OR uid='$uid'";
  }
}
$find .= ")";
$found = mysql_db_query("tbdb", $find);
while ($row = mysql_fetch_row($found)) {
  $uid = $row[0];
  $status=$row[1];
  $email=$row[2];
  $cmd = "select pid from proj_memb where uid='$uid' and trust='none' and (";
  $cmd .= "pid='$project[0]'";
  $n=1;
  while ( isset($project[$n]) ) { $cmd .= " or pid='$project[$n]'"; $n++; }
  $cmd .=")";
  $result = mysql_db_query("tbdb",$cmd);
  $row=mysql_fetch_row($result);
  $pid=$row[0];
  if (isset($$uid)) {
    if ( $$uid == "approve") {
      $trust=${"$uid-trust"};
      if ($status=="newuser") {
	$newstatus='unverified';
      } else { #Status is 'unapproved'
	$newstatus='active';
      }
      $cmd = "update users set status='$newstatus' where uid='$uid'";
      $cmd .= "and status='$status'";
      $result = mysql_db_query("tbdb",$cmd);
      $cmd = "update proj_memb set trust='$trust' where uid='$uid'";
      $cmd .= "and trust='none' and pid='$pid'";
      $result = mysql_db_query("tbdb",$cmd);

      mail("$email","TESTBED: Project Membership Approval",
	   "\nThis message is to notify you that you have been approved ".
	   "as a member of \nthe $pid project with $trust permissions.\n".
	   "\nYour status as a Testbed user is now $newstatus.".
	   "\n\nThanks,\nTestbed Ops\nUtah Network Testbed\n",
           "From: Testbed Ops <testbed-ops@flux.cs.utah.edu>\n".
           "Cc: Testbed WWW <testbed-www@flux.cs.utah.edu>\n".
           "Errors-To: Testbed WWW <testbed-www@flux.cs.utah.edu>");
      echo "<h3><p>User $uid was changed to status $newstatus and ";
      echo "granted $trust permissions for project $pid.</p></h3>\n";
    } elseif ( $$uid == "deny") {
      # Delete all rows from proj_memb that are for that person, no privs
      # and one of the projects that the user is a leader of
      $cmd = "delete from proj_memb where uid='$uid' and trust='none' and (";
      $cmd .= "pid='$project[0]'";
      $n=1;
      while ( isset($project[$n]) ) { $cmd .= " or pid='$project[$n]'"; $n++; }
      $cmd .=")";
      $result = mysql_db_query("tbdb",$cmd);
      mail("$email","TESTBED: Project Membership Denied",
	   "\nThis message is to notify you that you have been denied ".
	   "as a member of \nthe $pid project\n".
	   "\nYour status as a Testbed user is still $status.".
	   "\n\nThanks,\nTestbed Ops\nUtah Network Testbed\n",
           "From: Testbed Ops <testbed-ops@flux.cs.utah.edu>\n".
           "Cc: Testbed WWW <testbed-www@flux.cs.utah.edu>\n".
           "Errors-To: Testbed WWW <testbed-www@flux.cs.utah.edu>");
      echo "<h3><p>User $uid was denied membership in your project.</p>
            </h3>\n";
    } else {
      echo "<h3><p>User $uid was postponed for later decision.</p></h3>\n";
    }
  }
}
echo "
</body>
</html>";
?>

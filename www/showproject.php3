<html>
<head>
<title>Show Project Information</title>
<link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
<?php
include("defs.php3");

#
# Note the difference with which this page gets it arguments!
# I invoke it using GET arguments, so uid and pid are are defined
# without having to find them in URI (like most of the other pages
# find the uid).
#

#
# Only known and logged in users can end experiments.
#
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);

#
# Verify form arguments.
# 
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    USERERROR("You must provide a project ID.", 1);
}

#
# Check to make sure thats this is a valid PID.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM projects WHERE pid=\"$pid\"");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The project $pid is not a valid project.", 1);
}

#
# Verify that this uid is a member of the project for the experiment
# being displayed, or is an admin person.
#
if (!$isadmin) {
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid=\"$uid\" and pid=\"$pid\"");
    if (mysql_num_rows($query_result) == 0) {
        USERERROR("You are not a member of Project $pid.", 1);
    }
}

include("showproject_dump.php3");

?>
</center>
</body>
</html>

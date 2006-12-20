<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2004, 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# No PAGEHEADER since we spit out a Location header later. See below.
# 

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Check to make sure a valid nodeid
#
if (!isset($node_id) || !strcmp($node_id, "") || !TBValidNodeName($node_id)) {
    USERERROR("The node '$node_id' is not a valid node!", 1);
}

#
# Has to be reserved of course!
#
$query_result =
    DBQueryFatal("select pid,eid from reserved where node_id='$node_id'");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("$node_id is not currently reserved!", 1);
}
$row = mysql_fetch_array($query_result);
$pid = $row[pid];
$eid = $row[eid];

#
# Perm check.
#
if (! ($isadmin || (OPSGUY()) && $pid == $TBOPSPID)) {
    USERERROR("Not enough permission to free nodes from the web interface!", 1);
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    PAGEHEADER("Free Node");
	
    echo "<center><h3><br>
          Operation canceled!
          </h3></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    PAGEHEADER("Free Node");

    echo "<center><h2><br>
          Are you <b>REALLY</b>
          sure you want to free node '$node_id?'
          </h2>\n";

    SHOWNODE($node_id, SHOWNODE_NOFLAGS);
    
    echo "<form action='freenode.php3?node_id=$node_id' method=post>";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

#
# Pass it off to the script.
#
SUEXEC($uid, "nobody", "webnfree $pid $eid $node_id", 1);

#
# And send an audit message.
#
$uid_name  = $this_user->name();
$uid_email = $this_user->email();

TBMAIL($TBMAIL_AUDIT,
       "Node Free: $node_id",
       "$node_id was deallocated via the web interface by $uid ($uid_name).\n",
       "From: $uid_name <$uid_email>\n".
       "Errors-To: $TBMAIL_WWW");

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
# See above for conclusion.
# 
header("Location: shownode.php3?node_id=$node_id");

?>

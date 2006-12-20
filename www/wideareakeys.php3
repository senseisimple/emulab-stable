<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

if (!$isadmin) {
    USERERROR("You do not have permission to view this page!", 1);
}

if (isset($deletekey)) {
    #
    # Get the actual key.
    #
    $query_result =
	DBQueryFatal("select wa.*,i.node_id from widearea_privkeys as wa ".
		     "left join interfaces as i on wa.IP=i.IP ".
		     "where privkey='$deletekey'");

    if (! mysql_num_rows($query_result)) {
	USERERROR("No such widearea private key!", 1);
    }
    $row     = mysql_fetch_array($query_result);
    $name    = $row[user_name];
    $email   = $row[user_email];
    $when    = $row[requested];
    $IP      = $row[IP];
    $nodeid  = $row[node_id];

    #
    # We run this twice. The first time we are checking for a confirmation
    # by putting up a form. The next time through the confirmation will be
    # set. Or, the user can hit the cancel button, in which case we should
    # Probably redirect the browser back up a level.
    #
    if ($canceled) {
        PAGEHEADER("Widearea Private Key Deletion Request");
    
        echo "<center><h2><br>
              Widearea Private Key deletion has been canceled!
              </h2></center>\n";

        echo "<br>
              Back to <a href=wideareakeys.php3>Back to Widearea Keys.</a>\n";
    
        PAGEFOOTER();
        return;
    }

    if (!$confirmed) {
        PAGEHEADER("Widearea Private Key Deletion Request");

	echo "<center><h3><br>
              Are you <b>REALLY</b> sure you want to delete this Key?<br>
              </h3>\n";
	echo "</center>\n";

	echo "<table align=center border=1 cellpadding=2 cellspacing=2>\n";
	echo "<tr>
  	          <td>$name</td>
  	          <td>$email</td>
  	          <td>$IP</td>
  	          <td>$nodeid</td>
	          <td>$when</td>
             </tr>\n";
	echo "</table>\n";

	echo "<center>\n";
 	echo "<form action=wideareakeys.php3 method=post>";
	echo "<input type=hidden name=deletekey value=$deletekey>\n";
	echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
	echo "<b><input type=submit name=canceled value=Cancel></b>\n";
	echo "</form>\n";
	echo "</center>\n";

	echo "<br><font color=red>
              This will delete all trace including the node, vnodes, and
              interface table entries. Needless to say, deleting a key
              record of an active widearea node is a bad thing. Be sure!
              </font>\n";

	PAGEFOOTER();
	return;
    }
    DBQueryFatal("delete from widearea_privkeys where privkey='$deletekey'");

    if (isset($nodeid) && $nodeid) {
	$retval = SUEXEC($uid, $TBADMINGROUP, "webdeletenode -b $nodeid",
			 SUEXEC_ACTION_DIE);
    }
    header("Location: wideareakeys.php3");
}

#
# Get the list and show it.
#
#
# Standard Testbed Header, now that we know what we want to say.
#
PAGEHEADER("Widearea Private Keys");

$query_result =
    DBQueryFatal("select wa.*,i.node_id,cd.version ".
		 " from widearea_privkeys as wa ".
		 "left join interfaces as i on wa.IP=i.IP ".
		 "left join cdroms as cd on wa.cdkey=cd.cdkey ".
		 "order by requested DESC");

if (! mysql_num_rows($query_result)) {
    USERERROR("There are no widearea private keys!\n", 1);
}

echo "<table align=center border=1 cellpadding=2 cellspacing=2>\n";

echo "<tr>
          <td>Delete?</td>
          <td>Name</td>
          <td>Email</td>
          <td>IP</td>
          <td>Node</td>
          <td>Key</td>
	  <td>CD</td>
          <td>Requested</td>
          <td>Updated</td>
      </tr>\n";

while ($row = mysql_fetch_array($query_result)) {
    $name    = $row[user_name];
    $email   = $row[user_email];
    $requested = $row[requested];
    $updated  = $row[updated];
    $privkey = $row[privkey];
    $IP      = $row[IP];
    $nodeid  = $row[node_id];
    $cdvers  = $row[version];

    echo "<tr>
              <td align=center>
                  <A href='wideareakeys.php3?deletekey=$privkey'>
	                  <img alt=X src=redball.gif></A></td>
	      <td>$name</td>
	      <td>$email</td>
	      <td>$IP</td>\n";

    if (isset($nodeid)) {
	echo "<td><A href='shownode.php3?node_id=$nodeid'>$nodeid</a></td>\n";
    }
    else {
	echo "<td>&nbsp</td>\n";
    }
    echo "    <td>$privkey</td>
	      <td align=center>$cdvers</td>
	      <td>$requested</td>
	      <td>$updated</td>
         </tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

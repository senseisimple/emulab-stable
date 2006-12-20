<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Manage Shared Access to your Experiment");

#
# Only known and logged in users can do this. 
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# We must get a valid PID/EID.
# 
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
  USERERROR("You must provide a valid project ID!", 1);
}
if (!isset($eid) ||
    strcmp($eid, "") == 0) {
  USERERROR("You must provide a valid Experiment ID!", 1);
}

$query_result = mysql_db_query($TBDBNAME,
	"select e.shared,e.expt_head_uid,p.head_uid from experiments as e, ".
	"projects as p ".
	"where e.eid='$eid' and e.pid='$pid' and p.pid='$pid'");

if (!$query_result) {
    TBERROR("DB error getting experiment record $pid/$eid", 1);
}
if (($row = mysql_fetch_array($query_result)) == 0) {
    USERERROR("No such experiment $eid in project $pid!", 1);
}
if (!$row[0]) {
    USERERROR("Experiment $eid in project $pid is not shared", 1);
}

#
# Admin users get to do anything they want, of course. Otherwise, only
# the experiment creator or the project leader has permission to mess
# with this. 
#
if (! $isadmin) {
    #
    # One of them better be you!
    #
    if (strcmp($row[1], $uid) && strcmp($row[2], $uid)) {
	USERERROR("You must be either the experiment creator or the project ".
		  "leader under which the experiment was created, to change ".
		  "experiment/project access permissions!", 1);
    }
}

echo "This page allows you to grant members of other projects 'read-only'
         access to the nodes in your experiment. By read-only, we mean that
         non-root accounts will be build on your nodes for all of the members
         of the projects you grant access to. Those people will also be able
         to view some aspects of the experiment's information via the web
         interface, but they will not be able terminate or modify the
         experiment in any way.
      <br>\n";
    
echo "<table align=center border=0> 
      <form action='expaccess.php3?pid=$pid&eid=$eid' method=post>\n";

#
# Get the current set of projects that have been granted share access to
# this experiment.
#
$query_result = mysql_db_query($TBDBNAME,
	"select pid from exppid_access ".
	"where exp_eid='$eid' and exp_pid='$pid'");

if (mysql_num_rows($query_result)) {
    echo "<tr><td align=center>
          <b>These are the Projects currently granted access to your
              experimental nodes. <br>
              Deselect the ones you would like to remove.</b>
          </td></tr>\n";
    
    echo "<tr><td align=center>\n";
    
    while ($row = mysql_fetch_array($query_result)) {
	echo "<input checked type=checkbox value=permit name='access_$row[0]'>
                     $row[0] &nbsp\n";
    }
    echo "</td></tr>\n";
}

echo "<tr>
         <td align=center>
             <br><b>Enter in the PID of a project you wish to add.</b>
         </td>
      </tr>\n";
    
echo "<tr>
         <td align=center>
             <input type=text name='addpid'
                    size=$TBDB_PIDLEN maxlength=$TBDB_PIDLEN>
         </td>
      </tr>\n";

echo "<tr>
         <td align=center>
             <b><input type=submit value=Submit></b>
         </td>
      </tr>\n";

echo "</form>
      </table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

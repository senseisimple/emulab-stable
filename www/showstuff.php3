<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
#
# This is an included file. No headers or footers.
#
# Functions to dump out various things.  
#

#
# A project
#
function SHOWPROJECT($pid, $thisuid) {
    global $TBDBNAME;

    $query_result =
	DBQueryFatal("SELECT * FROM projects WHERE pid='$pid'");
    $row = mysql_fetch_array($query_result);

    echo "<center>
          <h3>Project Profile</h3>
          </center>
          <table align=center cellpadding=2 border=1>\n";
    
    $proj_created	= $row[created];
    $proj_expires	= $row[expires];
    $proj_name		= stripslashes($row[name]);
    $proj_URL		= $row[URL];
    $proj_public        = $row[public];
    $proj_funders	= stripslashes($row[funders]);;
    $proj_head_uid	= $row[head_uid];
    $proj_members       = $row[num_members];
    $proj_pcs           = $row[num_pcs];
    $proj_ronpcs        = $row[num_ron];
    $proj_plabpcs       = $row[num_pcplab];
    $proj_why           = nl2br($row[why]);
    $control_node	= $row[control_node];
    $approved           = $row[approved];
    $expt_count         = $row[expt_count];
    $expt_last          = $row[expt_last];

    if ($proj_public) {
	$proj_public = "Yes";
    }
    else {
	$proj_public = "No";
    }

    if (!$expt_last) {
	$expt_last = "&nbsp";
    }

    #
    # Generate the table.
    # 
    echo "<tr>
              <td>Name: </td>
              <td class=\"left\">
                <a href='showproject.php3?pid=$pid'>$pid</a></td>
          </tr>\n";
    
    echo "<tr>
              <td>Long Name: </td>
              <td class=\"left\">$proj_name</td>
          </tr>\n";
    
    echo "<tr>
              <td>Project Head: </td>
              <td class=\"left\">
                <A href='showuser.php3?target_uid=$proj_head_uid'>
                     $proj_head_uid</A></td>
          </tr>\n";
    
    echo "<tr>
              <td>URL: </td>
              <td class=\"left\">
                  <A href='$proj_URL'>$proj_URL</A></td>
          </tr>\n";
    
    echo "<tr>
              <td>Publicly Visible: </td>
              <td class=\"left\">$proj_public</td>
          </tr>\n";
    
    echo "<tr>
              <td>Funders: </td>
              <td class=\"left\">$proj_funders</td>
          </tr>\n";

    echo "<tr>
              <td>#Project Members: </td>
              <td class=\"left\">$proj_members</td>
          </tr>\n";
    
    echo "<tr>
              <td>#PCs: </td>
              <td class=\"left\">$proj_pcs</td>
          </tr>\n";
    
    echo "<tr>
              <td>#Planetlab PCs: </td>
              <td class=\"left\">$proj_plabpcs</td>
          </tr>\n";
    
    echo "<tr>
              <td>#RON PCs: </td>
              <td class=\"left\">$proj_ronpcs</td>
          </tr>\n";
    
    echo "<tr>
              <td>Created: </td>
              <td class=\"left\">$proj_created</td>
          </tr>\n";
    
    echo "<tr>
              <td>Expires: </td>
              <td class=\"left\">$proj_expires</td>
          </tr>\n";
    
    echo "<tr>
              <td>Experiments Created:</td>
              <td class=\"left\">$expt_count</td>
          </tr>\n";
    
    echo "<tr>
              <td>Date of last experiment:</td>
              <td class=\"left\">$expt_last</td>
          </tr>\n";
    
    echo "<tr>
              <td>Approved?: </td>\n";
    if ($approved) {
	echo "<td class=left><img alt=\"Y\" src=\"greenball.gif\"></td>\n";
    }
    else {
	echo "<td class=left><img alt=\"N\" src=\"redball.gif\"></td>\n";
    }
    echo "</tr>\n";

    echo "<tr>
              <td colspan='2'>Why?:</td>
          </tr>\n";
    
    echo "<tr>
              <td colspan='2' width=600>$proj_why</td>
          </tr>\n";
    
    echo "</table>\n";
}

#
# A Group
#
function SHOWGROUP($pid, $gid) {
    global $OURDOMAIN;
    
    $query_result =
	DBQueryFatal("SELECT * FROM groups WHERE pid='$pid' and gid='$gid'");
    $row = mysql_fetch_array($query_result);

    echo "<center>
          <h3>Group Profile</h3>
          </center>
          <table align=center border=1>\n";

    $leader	= $row[leader];
    $created	= $row[created];
    $description= stripslashes($row[description]);
    $expt_count = $row[expt_count];
    $expt_last  = $row[expt_last];
    $unix_gid   = $row[unix_gid];
    $unix_name  = $row[unix_name];

    if (strcmp($pid,$gid))
	$mail = "$pid-$gid" . "-users@" . $OURDOMAIN;
    else
	$mail = "$pid" . "-users@" . $OURDOMAIN;

    if (!$expt_last) {
	$expt_last = "&nbsp";
    }

    #
    # Generate the table.
    # 
    echo "<tr>
              <td>GID: </td>
              <td class=\"left\">
                <a href='showgroup.php3?pid=$pid&gid=$gid'>$gid</a></td>
          </tr>\n";
    
    echo "<tr>
              <td>PID: </td>
              <td class=\"left\">
                <a href='showproject.php3?pid=$pid'>$pid</a></td>
          </tr>\n";
    
    echo "<tr>
              <td>Description: </td>
              <td class=\"left\">$description</td>
          </tr>\n";
    
    echo "<tr>
              <td>Unix GID: </td>
              <td class=\"left\">$unix_gid</td>
          </tr>\n";
    
    echo "<tr>
              <td>Unix Group Name: </td>
              <td class=\"left\">$unix_name</td>
          </tr>\n";
    
    echo "<tr>
              <td>Group Leader: </td>
              <td class=\"left\">
                <A href='showuser.php3?target_uid=$leader'>$leader</A></td>
          </tr>\n";
    
    echo "<tr>
              <td>Email List: </td>
              <td class=\"left\">$mail</td>
          </tr>\n";
    
    echo "<tr>
              <td>Created: </td>
              <td class=\"left\">$created</td>
          </tr>\n";
    
    echo "<tr>
              <td>Experiments Created:</td>
              <td class=\"left\">$expt_count</td>
          </tr>\n";
    
    echo "<tr>
              <td>Date of last experiment:</td>
              <td class=\"left\">$expt_last</td>
          </tr>\n";
    
    echo "</table>\n";
}

#
# A list of Group members.
#
function SHOWGROUPMEMBERS($pid, $gid, $prived = 0) {
    $query_result =
	DBQueryFatal("SELECT m.*,u.* FROM group_membership as m ".
		     "left join users as u on u.uid=m.uid ".
		     "WHERE pid='$pid' and gid='$gid'");
    
    if (! mysql_num_rows($query_result)) {
	return;
    }
    $showdel = (($prived && !strcmp($pid, $gid)) ? 1 : 0);

    echo "<center>\n";
    if (strcmp($pid, $gid)) {
	echo "<h3>Group Members</h3>\n";
    }
    else {
	echo "<h3>Project Members</h3>\n";
    }
    echo "</center>
          <table align=center border=1 cellpadding=1 cellspacing=2>\n";

    echo "<tr>
              <th>Name</th>
              <th>UID</th>
              <th>Privs</th>\n";
    if ($showdel) {
	echo "<th>Remove</th>\n";
    }
    echo "</tr>\n";

    while ($row = mysql_fetch_array($query_result)) {
        $target_uid = $row[uid];
	$usr_name   = $row[usr_name];
	$trust      = $row[trust];

        echo "<tr>
                  <td>$usr_name</td>
                  <td>
                    <A href='showuser.php3?target_uid=$target_uid'>
                       $target_uid</A>
                  </td>\n";
	
	if (TBTrustConvert($trust) != $TBDB_TRUST_NONE) {
	    echo "<td>$trust</td>\n";
	}
	else {
	    echo "<td><font color=red>$trust</font></td>\n";
	}
	if ($showdel) {
	    echo "<td align=center>
		      <A href='deleteuser.php3?target_uid=$target_uid";
	    echo         "&target_pid=$pid'>
                         <img alt=N src=redball.gif></td>\n";
	}
	echo "</tr>\n";
    }
    echo "</table>\n";
}

#
# A list of groups for a user.
#
function SHOWGROUPMEMBERSHIP($uid) {
    $none = TBDB_TRUSTSTRING_NONE;
    
    $query_result =
	DBQueryFatal("SELECT * FROM group_membership ".
		     "WHERE uid='$uid' and trust!='$none' ".
		     "order by pid");
    
    if (! mysql_num_rows($query_result)) {
	return;
    }

    echo "<center>
          <h3>Group Membership</h3>
          </center>
          <table align=center border=1 cellpadding=1 cellspacing=2>\n";

    echo "<tr>
              <th>PID</th>
              <th>GID</th>
              <th>Privs</th>
          </tr>\n";

    while ($row = mysql_fetch_array($query_result)) {
	$pid   = $row[pid];
	$gid   = $row[gid];
	$trust = $row[trust];

	if (TBTrustConvert($trust) != $TBDB_TRUST_NONE) {
	    echo "<tr>
              <td><a href='showproject.php3?pid=$pid'>$pid</a></td>
              <td><a href='showgroup.php3?pid=$pid&gid=$gid'>$gid</a></td>
              <td>$trust</td>\n";
	    echo "</tr>\n";
	}
    }
    echo "</table>\n";
}

#
# A User
#
function SHOWUSER($uid) {
    global $TBDBNAME;

    $userinfo_result =
	DBQueryFatal("SELECT * from users where uid='$uid'");

    $row	= mysql_fetch_array($userinfo_result);
    $usr_expires = $row[usr_expires];
    $usr_email   = $row[usr_email];
    $usr_URL     = $row[usr_URL];
    $usr_addr    = stripslashes($row[usr_addr]);
    $usr_addr2   = stripslashes($row[usr_addr2]);
    $usr_city    = stripslashes($row[usr_city]);
    $usr_state   = stripslashes($row[usr_state]);
    $usr_zip     = stripslashes($row[usr_zip]);
    $usr_name    = stripslashes($row[usr_name]);
    $usr_phone   = $row[usr_phone];
    $usr_title   = stripslashes($row[usr_title]);
    $usr_affil   = stripslashes($row[usr_affil]);
    $status      = $row[status];
    $admin       = $row[admin];
    $adminoff    = $row[adminoff];

    if (!strcmp($usr_addr2, ""))
	$usr_addr2 = "&nbsp";
    if (!strcmp($usr_city, ""))
	$usr_city = "&nbsp";
    if (!strcmp($usr_state, ""))
	$usr_state = "&nbsp";
    if (!strcmp($usr_zip, ""))
	$usr_zip = "&nbsp";

    #
    # Last Login info.
    #
    if (($lastweblogin = LASTWEBLOGIN($uid)) == 0)
	$lastweblogin = "&nbsp";
    if (($lastuserslogininfo = TBUsersLastLogin($uid)) == 0)
	$lastuserslogin = "N/A";
    else {
	$lastuserslogin = $lastuserslogininfo["date"] . " " .
		          $lastuserslogininfo["time"];
    }
    
    if (($lastnodelogininfo = TBUidNodeLastLogin($uid)) == 0)
	$lastnodelogin = "N/A";
    else {
	$lastnodelogin = $lastnodelogininfo["date"] . " " .
		         $lastnodelogininfo["time"] . " " .
                         "(" . $lastnodelogininfo["node_id"] . ")";
    }
    
    echo "<table align=center border=1>\n";
    
    echo "<tr>
              <td>Username:</td>
              <td>$uid</td>
          </tr>\n";
    
    echo "<tr>
              <td>Full Name:</td>
              <td>$usr_name</td>
          </tr>\n";
    
    echo "<tr>
              <td>Email Address:</td>
              <td>$usr_email</td>
          </tr>\n";
    
    echo "<tr>
              <td>Home Page URL:</td>
              <td><A href='$usr_URL'>$usr_URL</A></td>
          </tr>\n";
    
    echo "<tr>
              <td>Expiration date:</td>
              <td>$usr_expires</td>
          </tr>\n";
    
    echo "<tr>
              <td>Address 1:</td>
              <td>$usr_addr</td>
          </tr>\n";
    
    echo "<tr>
              <td>Address 2:</td>
              <td>$usr_addr2</td>
          </tr>\n";
    
    echo "<tr>
              <td>City:</td>
              <td>$usr_city</td>
          </tr>\n";
    
    echo "<tr>
              <td>State:</td>
              <td>$usr_state</td>
          </tr>\n";
    
    echo "<tr>
              <td>Zip:</td>
              <td>$usr_zip</td>
          </tr>\n";
    
    echo "<tr>
              <td>Phone #:</td>
              <td>$usr_phone</td>
          </tr>\n";
    
    echo "<tr>
              <td>Title/Position:</td>
              <td>$usr_title</td>
         </tr>\n";
    
    echo "<tr>
              <td>Institutional Affiliation:</td>
              <td>$usr_affil</td>
          </tr>\n";
    
    echo "<tr>
              <td>Status:</td>
              <td>$status</td>
          </tr>\n";

    if ($admin) {
	$onoff = ($adminoff ? "Off" : "On");
	$flip  = ($adminoff ? 0 : 1);
	echo "<tr>
                  <td>Admin (on/off):</td>
                  <td>Yes
                      <a href=adminmode.php3?target_uid=$uid&adminoff=$flip>
                         ($onoff)</td>
              </tr>\n";
    }
    
    echo "<tr>
              <td>Last Web Login:</td>
              <td>$lastweblogin</td>
          </tr>\n";
    
    echo "<tr>
              <td>Last Users Login:</td>
              <td>$lastuserslogin</td>
          </tr>\n";
    
    echo "<tr>
              <td>Last Node Login:</td>
              <td>$lastnodelogin</td>
          </tr>\n";
    
    echo "</table>\n";

}

#
# Show an experiment.
#
function SHOWEXP($pid, $eid) {
    global $TBDBNAME, $TBDOCBASE;
		
    $query_result =
	DBQueryFatal("select e.*,count(r.node_id) from experiments as e ".
		     "left join reserved as r on e.pid=r.pid and e.eid=r.eid ".
		     "where e.pid='$pid' and e.eid='$eid' ".
		     "group by e.eid order by eid");
    
    if (($exprow = mysql_fetch_array($query_result)) == 0) {
	TBERROR("Experiment $eid in project $pid is gone!\n", 1);
    }

    $exp_gid     = $exprow[gid];
    $exp_expires = $exprow[expt_expires];
    $exp_name    = stripslashes($exprow[expt_name]);
    $exp_created = $exprow[expt_created];
    $exp_start   = $exprow[expt_start];
    $exp_swapped = $exprow[expt_swapped];
    $exp_end     = $exprow[expt_end];
    $exp_created = $exprow[expt_created];
    $exp_head    = $exprow[expt_head_uid];
    $exp_status  = $exprow[state];
    $exp_shared  = $exprow[shared];
    $exp_path    = $exprow[path];
    $batchmode   = $exprow[batchmode];
    $attempts    = $exprow[attempts];
    $batchstate  = $exprow[batchstate];
    $priority    = $exprow[priority];
    $swappable   = $exprow[swappable];
    $swapreqs    = $exprow[swap_requests];
    $lastswapreq = $exprow[last_swap_req];
    $nodes       = $exprow["count(r.node_id)"];

    if ($swappable)
	$swappable = "Yes";
    else
	$swappable = "No";

    #
    # Generate the table.
    #
    echo "<table align=center cellpadding=2 cellspacing=2 border=1>\n";

    echo "<tr>
            <td>Name: </td>
            <td class=left>$eid</td>
          </tr>\n";

    echo "<tr>
            <td>Long Name: </td>
            <td class=\"left\">$exp_name</td>
          </tr>\n";

    echo "<tr>
            <td>Project: </td>
            <td class=\"left\">
              <a href='showproject.php3?pid=$pid'>$pid</a></td>
          </tr>\n";

    echo "<tr>
            <td>Group: </td>
            <td class=\"left\">
              <a href='showgroup.php3?pid=$pid&gid=$exp_gid'>$exp_gid</a></td>
          </tr>\n";

    echo "<tr>
            <td>Experiment Head: </td>
            <td class=\"left\">
              <a href='showuser.php3?target_uid=$exp_head'>$exp_head</a></td>
          </tr>\n";

    echo "<tr>
            <td>Created: </td>
            <td class=\"left\">$exp_created</td>
          </tr>\n";

    echo "<tr>
            <td>Expires: </td>
            <td class=\"left\">$exp_expires</td>
          </tr>\n";

    echo "<tr>
            <td>Started: </td>
            <td class=\"left\">$exp_start</td>
          </tr>\n";

    echo "<tr>
            <td>Last Swapped (in or out): </td>
            <td class=\"left\">$exp_swapped</td>
          </tr>\n";

    echo "<tr>
            <td><a href='$TBDOCBASE/faq.php3#UTT-Swapping'>Swappable:</a></td>
            <td class=\"left\">$swappable</td>
          </tr>\n";

    echo "<tr>
            <td>Priority: (0 is highest) </td>
            <td class=\"left\">$priority</td>
          </tr>\n";

    echo "<tr>
            <td>Shared: </td>
            <td class=\"left\">";
    if ($exp_shared) {
	echo "<A href='expaccess_form.php3?pid=$pid&eid=$eid'>Yes</a>";
    }
    else {
	echo "No";
    }
    echo "   </td>
          </tr>\n";

    echo "<tr>
            <td>Path: </td>
            <td class=left>$exp_path</td>
          </tr>\n";

    echo "<tr>
            <td>Status: </td>
            <td class=\"left\">$exp_status</td>
          </tr>\n";

    echo "<tr>
            <td>Reserved Nodes: </td>
            <td class=\"left\">$nodes</td>
          </tr>\n";

    $lastact = TBGetExptLastAct($pid,$eid);
    $idletime = TBGetExptIdleTime($pid,$eid);

    echo "<tr>
            <td>Last Activity: </td>
            <td class=\"left\">$lastact</td>
          </tr>\n";

    echo "<tr>
            <td>Idle Time: </td>
            <td class=\"left\">$idletime hours</td>
          </tr>\n";

    if (! ($swapreqs=="" || $swapreqs==0)) {
	echo "<tr>
            <td>Swap Requests: </td>
            <td class=\"left\">$swapreqs</td>
          </tr>\n";

	echo "<tr>
            <td>Last Swap Req.: </td>
            <td class=\"left\">$lastswapreq</td>
          </tr>\n";
    }
    
    if ($batchmode) {
	    echo "<tr>
                    <td>Batch Mode: </td>
                    <td class=\"left\">Yes</td>
                  </tr>\n";

	    echo "<tr>
                    <td>Batch Status: </td>
                    <td class=\"left\">$batchstate</td>
                  </tr>\n";

	    echo "<tr>
                    <td>Start Attempts: </td>
                    <td class=\"left\">$attempts</td>
                  </tr>\n";
    }

    echo "</table>\n";
}

#
# Show Node information for an experiment.
#
function SHOWNODES($pid, $eid) {
    global $TBDBNAME;
    global $TBOPSPID;
		
    $reserved_result =
	DBQueryFatal("SELECT * FROM reserved WHERE ".
		     "eid='$eid' and pid='$pid'");
    
    # If this is an expt in emulab-ops, we don't care about vname,
    # since it won't be defined. But we do want to know when the node
    # entered the experiment, since it won't match the experiment's
    # swapin date.
    $nodename="Node Name";
    $vnamefield="vname";
    if (!strcmp($pid, $TBOPSPID)) {
      $nodename="Reserve Time";
      $vnamefield="rsrvtime";
    }
    
    if (mysql_num_rows($reserved_result)) {
	echo "<center>
              <h3>Reserved Nodes</h3>
              </center>
              <table align=center border=1>
              <tr>
                <th>Node ID</th>
                <th>$nodename</th>
                <th>Type</th>
                <th>Default OSID</th>
                <th>Node<br>Status</th>
                <th>Last Active</th>
                <th>Startup<br>Status[<b>1</b>]</th>
                <th>Ready<br>Status[<b>2</b>]</th>
              </tr>\n";
	
	$query_result =
	    DBQueryFatal("SELECT nodes.*,reserved.vname, ".
	        "lower(date_format(greatest(last_tty_act,last_net_act,".
		"last_cpu_act,last_ext_act),\"%c/%d&nbsp;%l:%i:%s&nbsp;%p\"))".
		"as acttime, ".
	        "date_format(rsrv_time,\"%Y-%m-%d&nbsp;%T\") as rsrvtime ".
	        "FROM nodes LEFT JOIN node_activity ".
		"on nodes.node_id=node_activity.node_id ".
		"LEFT JOIN reserved ON nodes.node_id=reserved.node_id ".
	        "WHERE reserved.eid=\"$eid\" and reserved.pid=\"$pid\" ".
	        "ORDER BY type,priority");

	while ($row = mysql_fetch_array($query_result)) {
	    $node_id = $row[node_id];
	    $vname   = $row[$vnamefield];
	    $type    = $row[type];
	    $def_boot_osid = $row[def_boot_osid];
	    $startstatus   = $row[startstatus];
	    $readystatus   = $row[ready];
	    $status        = $row[status];
	    $bootstate     = $row[eventstate];
	    $acttime     = $row[acttime];

	    if (!$vname)
		$vname = "--";
	    if ($readystatus)
		$readylabel = "Yes";
	    else
		$readylabel = "No";

	    echo "<tr>
                    <td><A href='shownode.php3?node_id=$node_id'>$node_id</a>
                        </td>
                    <td>$vname</td>
                    <td>$type</td>\n";
	    if ($def_boot_osid) {
		echo "<td>";
		SPITOSINFOLINK($def_boot_osid);
		echo "</td>";
	    }
	    else
		echo "<td>&nbsp</td>\n";

	    if ($bootstate != "ISUP") {
		echo "  <td>$status ($bootstate)</td>\n";
	    } else {
		echo "  <td>$status</td>\n";
	    }
	    
	    echo "  <td>$acttime</td>
                    <td align=center>$startstatus</td>
                    <td align=center>$readylabel</td>
                   </tr>\n";
	}
	echo "</table>\n";
	echo "<h4><blockquote><blockquote><blockquote>
              <ol>
                <li>Exit value of the node startup command. A value of
                        666 indicates a testbed internal error.
                <li>User application ready status, reported via TMCC.
              </ol>
              </blockquote></blockquote></blockquote></h4>\n";
    }
}

#
# Show OS INFO record.
#
function SHOWOSINFO($osid) {
    global $TBDBNAME;
		
    $query_result =
	DBQueryFatal("SELECT * FROM os_info WHERE osid='$osid'");

    $osrow = mysql_fetch_array($query_result);

    $os_description = stripslashes($osrow[description]);
    $os_OS          = $osrow[OS];
    $os_version     = $osrow[version];
    $os_path        = $osrow[path];
    $os_magic       = $osrow[magic];
    $os_osfeatures  = $osrow[osfeatures];
    $os_op_mode     = $osrow[op_mode];
    $os_pid         = $osrow[pid];
    $os_shared      = $osrow[shared];
    $os_osname      = $osrow[osname];
    $creator        = $osrow[creator];
    $created        = $osrow[created];
    $mustclean      = $osrow[mustclean];
    $nextosid       = $osrow[nextosid];

    if (!$os_description)
	$os_description = "&nbsp";
    if (!$os_version)
	$os_version = "&nbsp";
    if (!$os_path)
	$os_path = "&nbsp";
    if (!$os_magic)
	$os_magic = "&nbsp";
    if (!$os_osfeatures)
	$os_osfeatures = "&nbsp";
    if (!$os_op_mode)
	$os_op_mode = "&nbsp";
    if (!$created)
	$created = "N/A";

    #
    # Generate the table.
    #
    echo "<table align=center border=1>\n";

    echo "<tr>
            <td>Name: </td>
            <td class=\"left\">$os_osname</td>
          </tr>\n";

    echo "<tr>
            <td>Project: </td>
            <td class=\"left\">
              <a href='showproject.php3?pid=$os_pid'>$os_pid</a></td>
          </tr>\n";

    echo "<tr>
            <td>Creator: </td>
            <td class=left>
              <a href='showuser.php3?target_uid=$creator'>$creator</a></td>
 	  </tr>\n";

    echo "<tr>
            <td>Created: </td>
            <td class=left>$created</td>
 	  </tr>\n";

    echo "<tr>
            <td>Description: </td>
            <td class=\"left\">$os_description</td>
          </tr>\n";

    echo "<tr>
            <td>Operating System: </td>
            <td class=\"left\">$os_OS</td>
          </tr>\n";

    echo "<tr>
            <td>Version: </td>
            <td class=\"left\">$os_version</td>
          </tr>\n";

    echo "<tr>
            <td>Path: </td>
            <td class=\"left\">$os_path</td>
          </tr>\n";

    echo "<tr>
            <td>Magic (uname -r -s): </td>
            <td class=\"left\">$os_magic</td>
          </tr>\n";

    echo "<tr>
            <td>Features: </td>
            <td class=\"left\">$os_osfeatures</td>
          </tr>\n";

    echo "<tr>
            <td>Operational Mode: </td>
            <td class=\"left\">$os_op_mode</td>
          </tr>\n";

    echo "<tr>
            <td>Shared?: </td>
            <td class=left>\n";

    if ($os_shared)
	echo "Yes";
    else
	echo "No";
    
    echo "  </td>
          </tr>\n";

    echo "<tr>
            <td>Must Clean?: </td>
            <td class=left>\n";

    if ($mustclean)
	echo "Yes";
    else
	echo "No";
    
    echo "  </td>
          </tr>\n";

    if ($nextosid) {
	echo "<tr>
                <td>Next Osid: </td>
                <td class=left>
                    <A href='showosinfo.php3?osid=$nextosid'>$nextosid</A></td>
              </tr>\n";
    }

    echo "<tr>
            <td>Internal ID: </td>
            <td class=\"left\">$osid</td>
          </tr>\n";

    echo "</table>\n";
}

#
# Show ImageID record.
#
function SHOWIMAGEID($imageid, $edit, $isadmin = 0) {
    global $TBDBNAME;
		
    $query_result =
	DBQueryFatal("select * from images where imageid='$imageid'");

    $row = mysql_fetch_array($query_result);

    $imagename   = $row[imagename];
    $pid         = $row[pid];
    $gid         = $row[gid];
    $description = stripslashes($row[description]);
    $loadpart	 = $row[loadpart];
    $loadlength	 = $row[loadlength];
    $part1_osid	 = $row[part1_osid];
    $part2_osid	 = $row[part2_osid];
    $part3_osid	 = $row[part3_osid];
    $part4_osid	 = $row[part4_osid];
    $default_osid= $row[default_osid];
    $path 	 = $row[path];
    $loadaddr	 = $row[load_address];
    $shared	 = $row[shared];
    $globalid	 = $row["global"];
    $creator     = $row[creator];
    $created     = $row[created];

    if ($edit) {
	if (!$description)
	    $description = "";
	if (!$path)
	    $path = "";
	if (!$loadaddr)
	    $loadaddr = "";
    }
    else {
	if (!$description)
	    $description = "&nbsp";
	if (!$path)
	    $path = "&nbsp";
	if (!$loadaddr)
	    $loadaddr = "&nbsp";
	if (!$created)
	    $created = "N/A";
    }
    
    #
    # Generate the table.
    #
    echo "<table align=center border=2 cellpadding=2 cellspacing=2>\n";

    if ($edit) {
	$imageid_encoded = rawurlencode($imageid);
	
	echo "<form action='editimageid.php3?imageid=$imageid_encoded'
                    method=post>\n";
    }

    echo "<tr>
            <td>Image Name: </td>
            <td class=\"left\">$imagename</td>
          </tr>\n";

    echo "<tr>
            <td>Project: </td>
            <td class=\"left\">
              <a href='showproject.php3?pid=$pid'>$pid</a></td>
          </tr>\n";

    echo "<tr>
              <td>Group: </td>
              <td class=\"left\">
                <a href='showgroup.php3?pid=$pid&gid=$gid'>$gid</a></td>
          </tr>\n";
    
    echo "<tr>
            <td>Creator: </td>
            <td class=left>$creator</td>
 	  </tr>\n";

    echo "<tr>
            <td>Created: </td>
            <td class=left>$created</td>
 	  </tr>\n";

    echo "<tr>
            <td>Description: </td>
            <td class=left>\n";

    if ($edit) {
	echo "<input type=text name=description size=60
                     maxlength=256 value='$description'>";
    }
    else {
	echo "$description";
    }
    echo "   </td>
 	  </tr>\n";

    echo "<tr>
            <td>Load Partition: </td>
            <td class=\"left\">$loadpart</td>
          </tr>\n";

    echo "<tr>
            <td>Load Length: </td>
            <td class=\"left\">$loadlength</td>
          </tr>\n";

    if ($part1_osid) {
	echo "<tr>
                 <td>Partition 1 OS: </td>
                 <td class=\"left\">";
	SPITOSINFOLINK($part1_osid);
	echo "   </td>
              </tr>\n";
    }

    if ($part2_osid) {
	echo "<tr>
                 <td>Partition 2 OS: </td>
                 <td class=\"left\">";
	SPITOSINFOLINK($part2_osid);
	echo "   </td>
              </tr>\n";
    }

    if ($part3_osid) {
	echo "<tr>
                 <td>Partition 3 OS: </td>
                 <td class=\"left\">";
	SPITOSINFOLINK($part3_osid);
	echo "   </td>
              </tr>\n";
    }

    if ($part4_osid) {
	echo "<tr>
                 <td>Partition 4 OS: </td>
                 <td class=\"left\">";
	SPITOSINFOLINK($part4_osid);
	echo "   </td>
              </tr>\n";
    }

    if ($default_osid) {
	echo "<tr>
                 <td>Boot OS: </td>
                 <td class=\"left\">";
	SPITOSINFOLINK($default_osid);
	echo "   </td>
              </tr>\n";
    }

    echo "<tr>
            <td>Filename: </td>
            <td class=left>\n";

    if ($edit) {
	echo "<input type=text name=path size=60
                     maxlength=256 value='$path'>";
    }
    else {
	echo "$path";
    }
    echo "  </td>
          </tr>\n";

    if (! $edit) {
	echo "<tr>
                  <td>Types: </td>
                  <td class=left>\n";

	$types_result =
	    DBQueryFatal("select distinct type from osidtoimageid ".
			 "where imageid='$imageid'");

	if (mysql_num_rows($types_result)) {
	    while ($row = mysql_fetch_array($types_result)) {
		$type = $row[type];
		echo "$type &nbsp ";
	    }
	}
	else {
	    echo "&nbsp";
	}
	echo "  </td>
              </tr>\n";
    }

    echo "<tr>
            <td>Shared?: </td>
            <td class=left>\n";

    if ($shared)
	echo "Yes";
    else
	echo "No";
    
    echo "  </td>
          </tr>\n";

    echo "<tr>
            <td>Global?: </td>
            <td class=left>\n";

    if ($globalid)
	echo "Yes";
    else
	echo "No";
    
    echo "  </td>
          </tr>\n";

    echo "<tr>
            <td>Internal ID: </td>
            <td class=left>$imageid</td>
          </tr>\n";

    echo "<tr>
            <td>Load Address: </td>
            <td class=left>\n";

    if ($edit && $isadmin) {
	echo "<input type=text name=loadaddr size=20
                     maxlength=256 value='$loadaddr'>";
    }
    else {
	echo "$loadaddr";
    }
    echo "  </td>
          </tr>\n";

    if ($edit) {
	echo "<tr>
                 <td colspan=2 align=center>
                     <b><input type=submit value=Submit></b>
                 </td>
              </tr>
              </form>\n";
    }

    echo "</table>\n";
}

#
# Show node record.
#
function SHOWNODE($node_id) {
    $query_result =
	DBQueryFatal("select n.*,na.*,r.vname,r.pid,r.eid,i.IP, ".
		     "greatest(last_tty_act,last_net_act,last_cpu_act,".
		     "last_ext_act) as last_act, ".
		     " t.isvirtnode,t.isremotenode ".
		     " from nodes as n ".
		     "left join reserved as r on n.node_id=r.node_id ".
		     "left join node_activity as na on n.node_id=na.node_id ".
		     "left join node_types as t on t.type=n.type ".
		     "left join interfaces as i on i.card=t.control_net ".
		     " and i.node_id=n.node_id ".
		     "where n.node_id='$node_id'");
    
    if (mysql_num_rows($query_result) == 0) {
	TBERROR("The node $node_id is not a valid nodeid!", 1);
    }
		
    $row = mysql_fetch_array($query_result);

    $phys_nodeid        = $row[phys_nodeid]; 
    $type               = $row[type];
    $vname		= $row[vname];
    $pid 		= $row[pid];
    $eid		= $row[eid];
    $bios               = $row[bios_version];
    $def_boot_osid      = $row[def_boot_osid];
    $def_boot_path      = $row[def_boot_path];
    $def_boot_cmd_line  = $row[def_boot_cmd_line];
    $next_boot_osid     = $row[next_boot_osid];
    $next_boot_path     = $row[next_boot_path];
    $next_boot_cmd_line = $row[next_boot_cmd_line];
    $rpms               = $row[rpms];
    $tarballs           = $row[tarballs];
    $startupcmd         = $row[startupcmd];
    $routertype         = $row[routertype];
    $eventstate         = $row[eventstate];
    $state_timestamp    = $row[state_timestamp];
    $allocstate         = $row[allocstate];
    $allocstate_timestamp= $row[allocstate_timestamp];
    $op_mode            = $row[op_mode];
    $op_mode_timestamp  = $row[op_mode_timestamp];
    $IP                 = $row[IP];
    $isvirtnode         = $row[isvirtnode];
    $isremotenode       = $row[isremotenode];
    $ipport_low		= $row[ipport_low];
    $ipport_next	= $row[ipport_next];
    $ipport_high	= $row[ipport_high];
    $last_act           = $row[last_act];
    $last_tty_act       = $row[last_tty_act];
    $last_net_act       = $row[last_net_act];
    $last_cpu_act       = $row[last_cpu_act];
    $last_ext_act       = $row[last_ext_act];
    $last_report        = $row[last_report];
    
    if (!$def_boot_cmd_line)
	$def_boot_cmd_line = "&nbsp";
    if (!$def_boot_path)
	$def_boot_path = "&nbsp";
    if (!$next_boot_path)
	$next_boot_path = "&nbsp";
    if (!$next_boot_cmd_line)
	$next_boot_cmd_line = "&nbsp";
    if (!$rpms)
	$rpms = "&nbsp";
    if (!$tarballs)
	$tarballs = "&nbsp";
    if (!$startupcmd)
	$startupcmd = "&nbsp";
    if (!$bios)
	$bios = "&nbsp";

    echo "<table border=2 cellpadding=0 cellspacing=2
                 align=center>\n";

    echo "<tr>
              <td>Node ID:</td>
              <td class=left>$node_id</td>
          </tr>\n";

    if ($isvirtnode) {
	if (strcmp($node_id, $phys_nodeid)) {
	    echo "<tr>
                      <td>Phys ID:</td>
                      <td class=left>
	   	          <A href='shownode.php3?node_id=$phys_nodeid'>
                             $phys_nodeid</a></td>
                  </tr>\n";
	}
    }

    if ($vname) {
	echo "<tr>
                  <td>Virtual Name:</td>
                  <td class=left>$vname</td>
              </tr>\n";
    }

    if ($pid) {
	echo "<tr>
                <td>Project: </td>
                <td class=\"left\">
                    <a href='showproject.php3?pid=$pid'>$pid</a></td>
              </tr>\n";

	echo "<tr>
                  <td>Experiment:</td>
                  <td><a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></td>
               </tr>\n";
    }

    echo "<tr>
              <td>Node Type:</td>
              <td class=left>$type</td>
          </tr>\n";

    echo "<tr>
              <td>Def Boot OS:</td>
              <td class=left>";
    SPITOSINFOLINK($def_boot_osid);
    echo "    </td>
          </tr>\n";

    if ($eventstate) {
	$when = strftime("20%y-%m-%d %H:%M:%S", $state_timestamp);
	echo "<tr>
                 <td>EventState:</td>
                 <td class=left>$eventstate ($when)</td>
              </tr>\n";
    }

    if ($op_mode) {
	$when = strftime("20%y-%m-%d %H:%M:%S", $op_mode_timestamp);
	echo "<tr>
                 <td>Operating Mode:</td>
                 <td class=left>$op_mode ($when)</td>
              </tr>\n";
    }

    if ($allocstate) {
	$when = strftime("20%y-%m-%d %H:%M:%S", $allocstate_timestamp);
	echo "<tr>
                 <td>AllocState:</td>
                 <td class=left>$allocstate ($when)</td>
              </tr>\n";
    }

    #
    # We want the last login for this node, but only if its *after* the
    # experiment was created (or swapped in).
    #
    if ($lastnodeuidlogin = TBNodeUidLastLogin($node_id)) {

	$foo = $lastnodeuidlogin["date"] . " " .
	       $lastnodeuidlogin["time"] . " " .
	       "(" . $lastnodeuidlogin["uid"] . ")";
	
	echo "<tr>
                  <td>Last Login:</td>
                  <td class=left>$foo</td>
             </tr>\n";
    }

    if ($last_act) {
        echo "<tr>
                  <td>Last Activity:</td>
                  <td class=left>$last_act</td>
              </tr>\n";

	$idletime = TBGetNodeIdleTime($node_id);
	echo "<tr>
                  <td>Idle Time:</td>
                  <td class=left>$idletime hours</td>
              </tr>\n";

        echo "<tr>
                  <td>Last Act. Report:</td>
                  <td class=left>$last_report</td>
              </tr>\n";

        echo "<tr>
                  <td>Last TTY Act.:</td>
                  <td class=left>$last_tty_act</td>
              </tr>\n";

        echo "<tr>
                  <td>Last Net. Act.:</td>
                  <td class=left>$last_net_act</td>
              </tr>\n";

        echo "<tr>
                  <td>Last CPU Act.:</td>
                  <td class=left>$last_cpu_act</td>
              </tr>\n";

        echo "<tr>
                  <td>Last Ext. Act.:</td>
                  <td class=left>$last_ext_act</td>
              </tr>\n";

    }
    
    if (!$isvirtnode && !$isremotenode) {
        echo "<tr>
                  <td>Def Boot Path:</td>
                  <td class=left>$def_boot_path</td>
              </tr>\n";

        echo "<tr>
                  <td>Def Boot Command&nbsp;Line:</td>
                  <td class=left>$def_boot_cmd_line</td>
              </tr>\n";

        echo "<tr>
                  <td>Next Boot OS:</td>
                  <td class=left>";
    
        if ($next_boot_osid)
	    SPITOSINFOLINK($next_boot_osid);
	else
	    echo "&nbsp";

	echo "    </td>
              </tr>\n";

	echo "<tr>
                 <td>Next Boot Path:</td>
                 <td class=left>$next_boot_path</td>
              </tr>\n";

	echo "<tr>
                  <td>Next Boot Command Line:</td>
                  <td class=left>$next_boot_cmd_line</td>
              </tr>\n";
    }
    elseif ($isvirtnode) {
	echo "<tr>
                  <td>IP Port Low:</td>
                  <td class=left>$ipport_low (sshd port)</td>
              </tr>\n";

	echo "<tr>
                  <td>IP Port Next:</td>
                  <td class=left>$ipport_next</td>
              </tr>\n";

	echo "<tr>
                  <td>IP Port High:</td>
                  <td class=left>$ipport_high</td>
              </tr>\n";
    }

    echo "<tr>
              <td>Startup Command:</td>
              <td class=left>$startupcmd</td>
          </tr>\n";

    echo "<tr>
              <td>Tarballs:</td>
              <td class=left>$tarballs</td>
          </tr>\n";

    if (!$isvirtnode && !$isremotenode) {
	echo "<tr>
                  <td>RPMs:</td>
                  <td class=left>$rpms</td>
              </tr>\n";
    }

    if (!$isvirtnode && !$isremotenode) {
	echo "<tr>
                  <td>Router Type:</td>
                  <td class=left>$routertype</td>
          </tr>\n";
    }

    echo "<tr>
              <td>Control Net IP:</td>
              <td class=left>$IP</td>
          </tr>\n";

    echo "<tr>
              <td>Bios Version:</td>
              <td class=left>$bios</td>
          </tr>\n";

    if ($isremotenode) {
	if ($isvirtnode) {
	    SHOWWIDEAREANODE($phys_nodeid, 1);
	}
	else {
	    SHOWWIDEAREANODE($node_id, 1);
	}
    }
    echo "</table>\n";
}

#
# Show log.
# 
function SHOWNODELOG($node_id)
{
    $query_result =
	DBQueryFatal("select * from nodelog where node_id='$node_id'".
		     "order by reported");

    if (! mysql_num_rows($query_result)) {
	echo "<br>
              <center>
               There are no entries in the log for node $node_id.
              </center>\n";

	return;
    }

    echo "<br>
          <center>
            Log for node $node_id.
          </center><br>\n";

    echo "<table border=1 cellpadding=2 cellspacing=2 align='center'>\n";

    echo "<tr>
             <th>Delete?</th>
             <th>Date</th>
             <th>ID</th>
             <th>Type</th>
             <th>Reporter</th>
             <th>Entry</th>
          </tr>\n";

    while ($row = mysql_fetch_array($query_result)) {
	$type       = $row[type];
	$log_id     = $row[log_id];
	$reporter   = $row[reporting_uid];
	$date       = $row[reported];
	$entry      = stripslashes($row[entry]);

	echo "<tr>
 	         <td align=center>
                  <A href='deletenodelog.php3?node_id=$node_id&log_id=$log_id'>
                     <img alt='o' src='redball.gif'></A></td>
                 <td>$date</td>
                 <td>$log_id</td>
                 <td>$type</td>
                 <td>$reporter</td>
                 <td>$entry</td>
              </tr>\n";
    }
    echo "</table>\n";
}

#
# Show one log entry.
# 
function SHOWNODELOGENTRY($node_id, $log_id)
{
    $query_result =
	DBQueryFatal("select * from nodelog where ".
		     "node_id='$node_id' and log_id=$log_id");

    if (! mysql_num_rows($query_result)) {
	return;
    }

    echo "<table border=1 cellpadding=2 cellspacing=2 align='center'>\n";

    $row = mysql_fetch_array($query_result);
    
    $type       = $row[type];
    $log_id     = $row[log_id];
    $reporter   = $row[reporting_uid];
    $date       = $row[reported];
    $entry      = stripslashes($row[entry]);

    echo "<tr>
             <td>$date</td>
             <td>$log_id</td>
             <td>$type</td>
             <td>$reporter</td>
             <td>$entry</td>
          </tr>\n";
    echo "</table>\n";
}

#
# Spit out an OSID link in user format.
#
function SPITOSINFOLINK($osid)
{
    if (! TBOSInfo($osid, $osname, $pid))
	return;
    
    echo "<A href='showosinfo.php3?osid=$osid'>$osname</A>\n";
}

#
# A list of widearea accounts.
#
function SHOWWIDEAREAACCOUNTS($uid) {
    $none = TBDB_TRUSTSTRING_NONE;
    
    $query_result =
	DBQueryFatal("SELECT * FROM widearea_accounts ".
		     "WHERE uid='$uid' and trust!='$none' ".
		     "order by node_id");
    
    if (! mysql_num_rows($query_result)) {
	return;
    }

    echo "<center>
          <h3>Widearea Accounts</h3>
          </center>
          <table align=center border=1 cellpadding=1 cellspacing=2>\n";

    echo "<tr>
              <th>Node ID</th>
              <th>Approved</th>
              <th>Privs</th>
          </tr>\n";

    while ($row = mysql_fetch_array($query_result)) {
	$node_id   = $row[node_id];
	$approved  = $row[date_approved];
	$trust     = $row[trust];

	if (TBTrustConvert($trust) != $TBDB_TRUST_NONE) {
	    echo "<tr>
                      <td>$node_id</td>
                      <td>$approved</td>
                      <td>$trust</td>\n";
	    echo "</tr>\n";
	}
    }
    echo "</table>\n";
}

#
# Show widearea node record. Just the widearea stuff, not the other.
#
function SHOWWIDEAREANODE($node_id, $embedded = 0) {
    $query_result =
	DBQueryFatal("select * from widearea_nodeinfo ".
		     "where node_id='$node_id'");

    $row = mysql_fetch_array($query_result);

    if (! mysql_num_rows($query_result)) {
	return;
    }

    $contact_uid	= $row[contact_uid];
    $machine_type       = $row[machine_type];
    $connect_type	= $row[connect_type];
    $city		= $row[city];
    $state		= $row[state];
    $country		= $row[country];
    $zip		= $row[zip];

    if (! $embedded) {
	echo "<table border=2 cellpadding=0 cellspacing=2
                     align=center>\n";
    }
    else {
	echo "<tr>
               <td align=center colspan=2>
                   Widearea Info
               </td>
             </tr>\n";
    }

    echo "<tr>
              <td>Contact UID:</td>
              <td class=left>
                  <A href='showuser.php3?target_uid=$contact_uid'>
		     $contact_uid</A></td>
          </tr>\n";

    echo "<tr>
              <td>Machine Type:</td>
              <td class=left>$machine_type</td>
          </tr>\n";

    echo "<tr>
              <td>connect Type:</td>
              <td class=left>$connect_type</td>
          </tr>\n";

    echo "<tr>
              <td>City:</td>
              <td class=left>$city</td>
          </tr>\n";

    echo "<tr>
              <td>State:</td>
              <td class=left>$state</td>
          </tr>\n";

    echo "<tr>
              <td>Country:</td>
              <td class=left>$country</td>
          </tr>\n";

    echo "<tr>
              <td>Zip:</td>
              <td class=left>$zip</td>
          </tr>\n";

    if (! $embedded) {
        echo "</table>\n";
    }
}

#
# This is an included file.
# 
?>

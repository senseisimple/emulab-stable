<?php
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

    $query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM projects WHERE pid=\"$pid\"");
    if (mysql_num_rows($query_result) == 0) {
       USERERROR("The project $pid is not a valid project.", 1);
    }
    $row = mysql_fetch_array($query_result);

    echo "<table align=center border=1>\n";

    $proj_created	= $row[created];
    $proj_expires	= $row[expires];
    $proj_name		= $row[name];
    $proj_URL		= $row[URL];
    $proj_public        = $row[public];
    $proj_funders	= $row[funders];
    $proj_head_uid	= $row[head_uid];
    $proj_members       = $row[num_members];
    $proj_pcs           = $row[num_pcs];
    $proj_sharks        = $row[num_sharks];
    $proj_why           = $row[why];
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
              <td class=\"left\">$pid</td>
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
              <td>#Sharks: </td>
              <td class=\"left\">$proj_sharks</td>
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
    $query_result =
	DBQueryFatal("SELECT * FROM groups WHERE pid='$pid' and gid='$gid'");
    $row = mysql_fetch_array($query_result);

    echo "<table align=center border=1>\n";

    $leader	= $row[leader];
    $created	= $row[created];
    $description= $row[description];
    $expt_count = $row[expt_count];
    $expt_last  = $row[expt_last];
    $unix_gid   = $row[unix_gid];
    $unix_name  = $row[unix_name];

    if (!$expt_last) {
	$expt_last = "&nbsp";
    }

    #
    # Generate the table.
    # 
    echo "<tr>
              <td>GID: </td>
              <td class=\"left\">$gid</td>
          </tr>\n";
    
    echo "<tr>
              <td>PID: </td>
              <td class=\"left\">$pid</td>
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
function SHOWGROUPMEMBERS($pid, $gid) {
    $query_result =
	DBQueryFatal("SELECT m.*,u.* FROM group_membership as m ".
		     "left join users as u on u.uid=m.uid ".
		     "WHERE pid='$pid' and gid='$gid'");
    
    if (! mysql_num_rows($query_result)) {
	return;
    }

    echo "<center>
          <h3>Group Members</h3>
          </center>
          <table align=center border=1>\n";

    echo "<tr>
              <td align=center>Name</td>
              <td align=center>UID</td>
              <td align=center>Privs</td>
              <td align=center>Approved?</td>
          </tr>\n";

    while ($row = mysql_fetch_array($query_result)) {
        $target_uid = $row[uid];
	$usr_name   = $row[usr_name];
	$status     = $row[status];
	$trust      = $row[trust];

        echo "<tr>
                  <td>$usr_name</td>
                  <td>
                    <A href='showuser.php3?target_uid=$target_uid'>
                       $target_uid</A>
                  </td>
                  <td>$trust</td>\n";
	    
	if (strcmp($status, "active") == 0 ||
	    strcmp($status, "unverified") == 0) {
	    echo "<td align=center>
                      <img alt=\"Y\" src=\"greenball.gif\"></td>\n";
	}
	else {
	    echo "<td align=center>
                      <img alt=\"N\" src=\"redball.gif\"></td>\n";
	}
	echo "</tr>\n";
    }
    echo "</table>\n";
}

#
# A User
#
function SHOWUSER($uid) {
    global $TBDBNAME;
		
    $userinfo_result = mysql_db_query($TBDBNAME,
	"SELECT * from users where uid=\"$uid\"");

    $row	= mysql_fetch_array($userinfo_result);
    $usr_expires = $row[usr_expires];
    $usr_email   = $row[usr_email];
    $usr_URL     = $row[usr_URL];
    $usr_addr    = $row[usr_addr];
    $usr_name    = $row[usr_name];
    $usr_phone   = $row[usr_phone];
    $usr_passwd  = $row[usr_pswd];
    $usr_title   = $row[usr_title];
    $usr_affil   = $row[usr_affil];
    $status      = $row[status];

    #
    # Last Login info.
    #
    if (($lastweblogin = LASTWEBLOGIN($uid)) == 0)
	$lastweblogin = "&nbsp";
    if (($lastuserslogin = LASTUSERSLOGIN($uid)) == 0)
	$lastuserslogin = "&nbsp";
    
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
              <td>Mailing Address:</td>
              <td>$usr_addr</td>
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
    
    echo "<tr>
              <td>Last Web Login:</td>
              <td>$lastweblogin</td>
          </tr>\n";
    
    echo "<tr>
              <td>Last Users Login:</td>
              <td>$lastuserslogin</td>
          </tr>\n";
    
    echo "</table>\n";

}

#
# Show an experiment.
#
function SHOWEXP($pid, $eid) {
    global $TBDBNAME;
		
    $query_result =
	DBQueryFatal("SELECT * FROM experiments WHERE ".
		     "eid='$eid' and pid='$pid'");
    if (($exprow = mysql_fetch_array($query_result)) == 0) {
	TBERROR("Experiment $eid in project $pid is gone!\n", 1);
    }

    $exp_gid     = $exprow[gid];
    $exp_expires = $exprow[expt_expires];
    $exp_name    = $exprow[expt_name];
    $exp_created = $exprow[expt_created];
    $exp_start   = $exprow[expt_start];
    $exp_end     = $exprow[expt_end];
    $exp_created = $exprow[expt_created];
    $exp_head    = $exprow[expt_head_uid];
    $exp_status  = $exprow[state];
    $exp_shared  = $exprow[shared];
    $exp_path    = $exprow[path];
    $batchmode   = $exprow[batchmode];
    $attempts    = $exprow[attempts];
    $batchstate  = $exprow[batchstate];

    #
    # Generate the table.
    #
    echo "<table align=center border=1>\n";

    echo "<tr>
            <td>Name: </td>
            <td class=\"left\">
                <A href='shownsfile.php3?pid=$pid&eid=$eid'>
                   $eid</a></td>
          </tr>\n";

    echo "<tr>
            <td>Long Name: </td>
            <td class=\"left\">$exp_name</td>
          </tr>\n";

    echo "<tr>
            <td>Project: </td>
            <td class=\"left\">$pid</td>
          </tr>\n";

    echo "<tr>
            <td>Group: </td>
            <td class=\"left\">
                <A href='showgroup.php3?pid=$pid&gid=$exp_gid'>$exp_gid</td>
          </tr>\n";

    echo "<tr>
            <td>Experiment Head: </td>
            <td class=\"left\">
                <A href='showuser.php3?target_uid=$exp_head'>
                   $exp_head</td>
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
            <td>Status: </td>
            <td class=\"left\">$exp_status</td>
          </tr>\n";

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
		
    $reserved_result = mysql_db_query($TBDBNAME,
		"SELECT * FROM reserved WHERE ".
		"eid=\"$eid\" and pid=\"$pid\"");
    
    if (mysql_num_rows($reserved_result)) {
	echo "<center>
              <h3>Reserved Nodes</h3>
              </center>
              <table align=center border=1>
              <tr>
                <td align=center>Node ID</td>
                <td align=center>Node Name</td>
                <td align=center>Type</td>
                <td align=center>Default<br>OSID</td>
                <td align=center>Default<br>Path</td>
                <td align=center>Default<br>Cmdline</td>
                <td align=center>Boot<br>Status[1]</td>
                <td align=center>Startup<br>Command</td>
                <td align=center>Startup<br>Status[2]</td>
                <td align=center>Ready<br>Status[3]</td>
              </tr>\n";
	
	$query_result = mysql_db_query($TBDBNAME,
		"SELECT nodes.*,reserved.vname ".
	        "FROM nodes LEFT JOIN reserved ".
	        "ON nodes.node_id=reserved.node_id ".
	        "WHERE reserved.eid=\"$eid\" and reserved.pid=\"$pid\" ".
	        "ORDER BY type,priority");

	while ($row = mysql_fetch_array($query_result)) {
	    $node_id = $row[node_id];
	    $vname   = $row[vname];
	    $type    = $row[type];
	    $def_boot_osid      = $row[def_boot_osid];
	    $def_boot_path      = $row[def_boot_path];
	    $def_boot_cmd_line  = $row[def_boot_cmd_line];
	    $next_boot_path     = $row[next_boot_path];
	    $next_boot_cmd_line = $row[next_boot_cmd_line];
	    $startupcmd         = $row[startupcmd];
	    $startstatus        = $row[startstatus];
	    $readystatus        = $row[ready];
	    $bootstatus         = $row[bootstatus];

	    if (!$def_boot_cmd_line)
		$def_boot_cmd_line = "NULL";
	    if (!$def_boot_path)
		$def_boot_path = "NULL";
	    if (!$next_boot_path)
		$next_boot_path = "NULL";
	    if (!$next_boot_cmd_line)
		$next_boot_cmd_line = "NULL";
	    if (!$startupcmd)
		$startupcmd = "NULL";
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
	    if ($def_boot_osid)
		echo "<td><A href='showosinfo.php3?osid=$def_boot_osid'>
                             $def_boot_osid</A></td>\n";
	    else
		echo "<td>&nbsp</td>\n";
	    
	    echo "  <td>$def_boot_path</td>
                    <td>$def_boot_cmd_line</td>
                    <td align=center>$bootstatus</td>
                    <td>$startupcmd</td>
                    <td align=center>$startstatus</td>
                    <td align=center>$readylabel</td>
                 </tr>\n";
	}
	echo "</table>\n";
	echo "<h4><blockquote><blockquote><blockquote><blockquote>
              <dl COMPACT>
                <dt>[1]
                    <dd>Node has rebooted successfully after experiment
                        creation.
                <dt>[2]
                    <dd>Exit value of the node startup command. A value of
                        666 indicates a testbed internal error.
                <dt>[3]
                    <dd>User application ready status, reported via TMCC.
              </dl>
              </blockquote></blockquote></blockquote></blockquote></h4>\n";
    }
}

#
# Show OS INFO record.
#
function SHOWOSINFO($osid) {
    global $TBDBNAME;
		
    $query_result = mysql_db_query($TBDBNAME,
		"SELECT * FROM os_info WHERE osid='$osid'");

    $osrow = mysql_fetch_array($query_result);

    $os_description = $osrow[description];
    $os_OS          = $osrow[OS];
    $os_version     = $osrow[version];
    $os_path        = $osrow[path];
    $os_magic       = $osrow[magic];
    $os_machinetype = $osrow[machinetype];
    $os_osfeatures  = $osrow[osfeatures];
    $os_pid         = $osrow[pid];

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
    if (!$os_pid)
	$os_pid = "&nbsp";

    #
    # Generate the table.
    #
    echo "<table align=center border=1>\n";

    echo "<tr>
            <td>OSID: </td>
            <td class=\"left\">$osid</td>
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
            <td>Project: </td>
            <td class=\"left\">$os_pid</td>
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
            <td>Node Type: </td>
            <td class=\"left\">$os_machinetype</td>
          </tr>\n";

    echo "<tr>
            <td>Features: </td>
            <td class=\"left\">$os_osfeatures</td>
          </tr>\n";

    echo "</table>\n";
}

#
# Show ImageID record.
#
function SHOWIMAGEID($imageid, $edit) {
    global $TBDBNAME;
		
    $query_result = mysql_db_query($TBDBNAME,
		"SELECT * FROM images WHERE imageid='$imageid'");

    $row = mysql_fetch_array($query_result);

    $description = $row[description];
    $loadpart	 = $row[loadpart];
    $loadlength	 = $row[loadlength];
    $part1_osid	 = $row[part1_osid];
    $part2_osid	 = $row[part2_osid];
    $part3_osid	 = $row[part3_osid];



    $part4_osid	 = $row[part4_osid];
    $default_osid= $row[default_osid];
    $path 	 = $row[path];
    $pid	 = $row[pid];
    $loadaddr	 = $row[load_address];

    if (!$pid)
	$pid = "&nbsp";
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
            <td>ImageID: </td>
            <td class=\"left\">$imageid</td>
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
            <td>Project: </td>
            <td class=\"left\">$pid</td>
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
                 <td>Slice 1 OSID: </td>
                 <td class=\"left\">
                    <A href='showosinfo.php3?osid=$part1_osid'>$part1_osid</A>
              </tr>\n";
    }

    if ($part2_osid) {
	echo "<tr>
                 <td>Slice 2 OSID: </td>
                 <td class=\"left\">
                    <A href='showosinfo.php3?osid=$part2_osid'>$part2_osid</A>
              </tr>\n";
    }

    if ($part3_osid) {
	echo "<tr>
                 <td>Slice 3 OSID: </td>
                 <td class=\"left\">
                    <A href='showosinfo.php3?osid=$part3_osid'>$part3_osid</A>
              </tr>\n";
    }

    if ($part4_osid) {
	echo "<tr>
                 <td>Slice 4 OSID: </td>
                 <td class=\"left\">
                    <A href='showosinfo.php3?osid=$part4_osid'>$part4_osid</A>
              </tr>\n";
    }

    if ($default_osid) {
	echo "<tr>
                 <td>Default OSID: </td>
                 <td class=\"left\">
                    <A href='showosinfo.php3?osid=$default_osid'>
                       $default_osid</A>
              </tr>\n";
    }

    echo "<tr>
            <td>Path: </td>
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

    echo "<tr>
            <td>Load Address: </td>
            <td class=left>\n";

    if ($edit) {
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
	DBQueryFatal("SELECT * FROM nodes WHERE node_id='$node_id'");
    
    if (mysql_num_rows($query_result) == 0) {
	TBERROR("The node $node_id is not a valid nodeid!", 1);
    }
		
    $row = mysql_fetch_array($query_result);

    $node_id            = $row[node_id]; 
    $type               = $row[type];
    $def_boot_osid      = $row[def_boot_osid];
    $def_boot_path      = $row[def_boot_path];
    $def_boot_cmd_line  = $row[def_boot_cmd_line];
    $next_boot_osid     = $row[next_boot_osid];
    $next_boot_path     = $row[next_boot_path];
    $next_boot_cmd_line = $row[next_boot_cmd_line];
    $rpms               = $row[rpms];
    $tarballs           = $row[tarballs];
    $startupcmd         = $row[startupcmd];

    if (!$def_boot_cmd_line)
	$def_boot_cmd_line = "&nbsp";
    if (!$def_boot_path)
	$def_boot_path = "&nbsp";
    if (!$next_boot_osid)
	$next_boot_osid = "&nbsp";
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

    echo "<table border=2 cellpadding=0 cellspacing=2
                 align=center>\n";

    echo "<tr>
              <td>Node ID:</td>
              <td class=left>$node_id</td>
      </tr>\n";

    echo "<tr>
              <td>Node Type:</td>
              <td class=left>$type</td>
          </tr>\n";

    echo "<tr>
              <td>Def Boot OSID:</td>
              <td class=left>$def_boot_osid</td>
          </tr>\n";

    echo "<tr>
              <td>Def Boot Path:</td>
              <td class=left>$def_boot_path</td>
          </tr>\n";

    echo "<tr>
              <td>Def Boot Command Line:</td>
              <td class=left>$def_boot_cmd_line</td>
          </tr>\n";

    echo "<tr>
              <td>Next Boot OSID:</td>
              <td class=left>$next_boot_osid</td>
          </tr>\n";

    echo "<tr>
             <td>Next Boot Path:</td>
             <td class=left>$next_boot_path</td>
          </tr>\n";

    echo "<tr>
              <td>Next Boot Command Line:</td>
              <td class=left>$next_boot_cmd_line</td>
          </tr>\n";

    echo "<tr>
              <td>Startup Command:</td>
              <td class=left>$startupcmd</td>
          </tr>\n";

    echo "<tr>
              <td>RPMs:</td>
              <td class=left>$rpms</td>
          </tr>\n";

    echo "<tr>
              <td>Tarballs:</td>
              <td class=left>$tarballs</td>
      </tr>\n";

    echo "</table>\n";
}

#
# This is an included file.
# 
?>

<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("node_defs.php");

#
# This page is used for both admin node control, and for mere user
# information purposes. Be careful about what you do outside of
# $isadmin tests.
# 

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("target_user",	PAGEARG_USER,
				 "showtype",    PAGEARG_STRING,
				 "bypid",       PAGEARG_STRING);

if (isset($target_user)) {
    if (! $target_user->AccessCheck($this_user, $TB_USERINFO_READINFO)) {
	USERERROR("You do not have permission to do this!", 1);
    }
    $target_uid  = $target_user->uid();
    $target_idx  = $target_user->uid_idx();
}
else {
    $target_uid  = $uid;
    $target_idx  = $this_user->uid_idx();
    $target_user = $this_user;
}

#
# Standard Testbed Header
#
PAGEHEADER("Node Control Center");

echo "<b>Show: <a href='nodecontrol_list.php3?showtype=summary'>summary</a>,
               <a href='nodecontrol_list.php3?showtype=pcs'>pcs</a>,
               <a href='floormap.php3'>wireless maps</a>,
               <a href='nodecontrol_list.php3?showtype=wireless'>
                                                        wireless list</a>,";
if ($TBMAINSITE) {
    echo "     <a href='floormap.php3?feature=usrp'>
                  GNU USRP (software defined radio) maps</a>,
               <a href='robotmap.php3'>robot maps</a>, ";
}
echo "         <a href='nodecontrol_list.php3?showtype=widearea'>widearea</a>";

if ($isadmin) {
    echo    ", <a href='nodeutilization.php'>utilization</a>,
               <a href='nodecontrol_list.php3?showtype=virtnodes'>virtual</a>,
               <a href='nodecontrol_list.php3?showtype=physical'>physical</a>,
               <a href='nodecontrol_list.php3?showtype=all'>all</a>";
}
echo ".</b><br>\n";

if (!isset($showtype)) {
    $showtype='summary';
}

$additionalVariables = "";
$additionalLeftJoin  = "";

if (! strcmp($showtype, "summary")) {
    # Separate query below.
    $role   = "";
    $clause = "";
    $view   = "Free Node Summary";
}
elseif (! strcmp($showtype, "all")) {
    $role   = "(role='testnode' or role='virtnode')";
    $clause = "";
    $view   = "All";
}
elseif (! strcmp($showtype, "pcs")) {
    $role   = "(role='testnode')";
    $clause = "and (nt.class='pc')";
    $view   = "PCs";
}
elseif (! strcmp($showtype, "sharks")) {
    $role   = "(role='testnode')";
    $clause = "and (nt.class='shark')";
    $view   = "Sharks";
}
elseif (! strcmp($showtype, "virtnodes")) {
    $role   = "(role='virtnode')";
    $clause = "";
    $view   = "Virtual Nodes";
}
elseif (! strcmp($showtype, "physical")) {
    $role   = "";
    $clause = "(nt.isvirtnode=0)";
    $view   = "Physical Nodes";
}
elseif (! strcmp($showtype, "widearea")) {
    $role   = "(role='testnode')";
    $clause = "and (nt.isremotenode=1)";

    $additionalVariables = ",".
			   "wani.machine_type,".
			   "REPLACE(CONCAT_WS(', ',".
			   "wani.city,wani.state,wani.country,wani.zip), ".
		 	   "'USA, ','')".
			   "AS location, ".
	 		   "wani.connect_type, ".
			   "wani.hostname, " .
                           "wani.site";
    $additionalLeftJoin = "LEFT JOIN widearea_nodeinfo AS wani ".
			  "ON n.node_id=wani.node_id";

    $view   = "Widearea";
}
elseif (! strcmp($showtype, "wireless")) {
    $role   = "(role='testnode')";
    $clause = "and (loc.node_id is not null)";

    $additionalLeftJoin = "LEFT JOIN location_info AS loc ".
			  "ON n.node_id=loc.node_id";

    $view   = "Wireless";
}
elseif (preg_match("/^[-\w]+$/", $showtype)) {
    $role   = "(role='testnode')";
    $clause = "and (nt.type='$showtype')";
    $view   = "only <a href=shownodetype.php3?node_type=$showtype>$showtype</a>";
}
else {
    $role   = "(role='testnode')";
    $clause = "and (nt.class='pc')";
    $view   = "PCs";
}
# If admin or widearea, show the vname too. 
$showvnames = 0;
if ($isadmin || !strcmp($showtype, "widearea")) {
    $showvnames = 1;
}

#
# Summary info very different.
# 
if (! strcmp($showtype, "summary")) {
    # Get permissions table so as not to show nodes the user is not allowed
    # to see.
    $perms = array();
    
    if (!$isadmin || isset($bypid)) {
	$query_result =
	    DBQueryFatal("select type from nodetypeXpid_permissions");

	while ($row = mysql_fetch_array($query_result)) {
	    $perms{$row[0]} = 0;
	}

	$pidclause = "";
	if (isset($bypid)) {
	    if ($bypid == "" || !TBvalid_pid($bypid)) {
		PAGEARGERROR("Invalid characters in 'bypid' argument!");
	    }
	    if (! ($target_project = Project::Lookup($bypid))) {
		PAGEARGERROR("No such project '$bypid'!");
	    }
	    if (!$target_project->AccessCheck($this_user,
					      $TB_PROJECT_READINFO)){
		USERERROR("You are not a member of project '$bypid!", 1);
	    }
	    $pidclause = "and g.pid='$bypid'";
	}
	if ($isadmin) {
	    $query_result =
		DBQueryFatal("select distinct type ".
			     "  from nodetypeXpid_permissions ".
			     "where pid='$bypid'");
	}
	else {
	    $query_result =
		DBQueryFatal("select distinct type from group_membership as g ".
			     "left join nodetypeXpid_permissions as p ".
			     "     on g.pid=p.pid ".
			     "where uid_idx='$target_idx' $pidclause");
	}
	
	while ($row = mysql_fetch_array($query_result)) {
	    $perms{$row[0]} = 1;
	}
    }
    
    # Get totals by type.
    $query_result =
	DBQueryFatal("select n.type,count(*) from nodes as n ".
		     "left join node_types as nt on n.type=nt.type ".
		     "where (role='testnode') and ".
		     "      (nt.class!='shark' and nt.class!='pcRemote' ".
		     "      and nt.class!='pcplabphys') ".
		     "group BY n.type");

    $alltotal  = 0;
    $allfree   = 0;
    $allunknown = 0;
    $totals    = array();
    $freecounts = array();
    $unknowncounts = array();

    while ($row = mysql_fetch_array($query_result)) {
	$type  = $row[0];
	$count = $row[1];

	$totals[$type]    = $count;
	$freecounts[$type] = 0;
	$unknowncounts[$type] = 0;
    }

    # Get free totals by type.  Note we also check that the physical node
    # is free, see note on non-summary query for why.
    $query_result =
	DBQueryFatal("select n.eventstate,n.type,count(*) from nodes as n ".
		     "left join nodes as np on np.node_id=n.phys_nodeid ".
		     "left join node_types as nt on n.type=nt.type ".
		     "left join reserved as r on r.node_id=n.node_id ".
		     "left join reserved as rp on rp.node_id=n.phys_nodeid ".
		     "where (n.role='testnode') and ".
		     "      (nt.class!='shark' and nt.class!='pcRemote' ".
		     "      and nt.class!='pcplabphys') ".
		     "      and r.pid is null and rp.pid is null ".
		     "      and n.reserved_pid is null and np.reserved_pid is null ".
		     "group BY n.eventstate,n.type");

    while ($row = mysql_fetch_array($query_result)) {
	$type  = $row[1];
	$count = $row[2];
        # XXX Yeah, I'm a doofus and can't figure out how to do this in SQL.
	if (($row[0] == TBDB_NODESTATE_ISUP) ||
	    ($row[0] == TBDB_NODESTATE_PXEWAIT) ||
	    ($row[0] == TBDB_NODESTATE_ALWAYSUP) ||
	    ($row[0] == TBDB_NODESTATE_POWEROFF)) {
	    $freecounts[$type] += $count;
	}
	else {
	    $unknowncounts[$type] += $count;
	}
    }

    $projlist = $target_user->ProjectAccessList($TB_PROJECT_CREATEEXPT);
    if (count($projlist) > 1) {
	echo "<b>By Project Permission: ";
	while (list($project) = each($projlist)) {
	    echo "<a href='nodecontrol_list.php3?".
		"showtype=summary&bypid=$project'>$project</a>,\n";
	}
	echo "<a href='nodecontrol_list.php3?showtype=summary'>".
	    "combined membership</a>.\n";
	echo "</b><br>\n";
    }

    echo "<br><center>
          <b>Free Node Summary</b>
          <br>\n";
    if (isset($bypid)) {
	echo "($bypid)<br><br>\n";
    }
    echo "<table>
          <tr>
             <th>Type</th>
             <th align=center>Free<br>Nodes</th>
             <th align=center>Total<br>Nodes</th>
          </tr>\n";

    foreach($totals as $key => $value) {
	$freecount = $freecounts[$key];

	# Check perm entry.
	if (isset($perms[$key]) && !$perms[$key])
	    continue;

	$allfree   += $freecount;
	$allunknown += $unknowncounts[$key];
	$alltotal  += $value;

	if ($unknowncounts[$key])
	    $ast = "*";
	else
	    $ast = "";
	
	echo "<tr>\n";
	if ($isadmin)
	    echo "<td><a href=editnodetype.php3?node_type=$key>\n";
	else
	    echo "<td><a href=shownodetype.php3?node_type=$key>\n";
	echo "           $key</a></td>
              <td align=center>${freecount}${ast}</td>
              <td align=center>$value</td>
              </tr>\n";
    }
    echo "<tr></tr>\n";
    echo "<tr>
            <td><b>Totals</b></td>
              <td align=center>$allfree</td>
              <td align=center>$alltotal</td>
              </tr>\n";

    if ($isadmin) {
	# Give admins the option to create a new type
	echo "<tr></tr>\n";
	echo "<th colspan=3 align=center>
                <a href=editnodetype.php3?new_type=1>Create a new type</a>
              </th>\n";
    }
    echo "</table>\n";
    if ($allunknown > 0) {
	    echo "<br><font size=-1><b>*</b> - Some nodes ($allunknown) are ".
		    "free, but currently in an unallocatable state.</font>";
    }
    PAGEFOOTER();
    exit();
}

#
# Suck out info for all the nodes.
#
# If a node is free we check to make sure that that the physical node
# is also.  This is based on the assumption that if a physical node is
# not available, neither is the node, such as the case with netpga2.
# This may not be true for virtual nodes, such as PlanetLab slices,
# but virtual nodes are allocated on demand, and thus are never free.
# 
$query_result =
    DBQueryFatal("select distinct n.node_id,n.phys_nodeid,n.type,ns.status, ".
		 "   n.def_boot_osid, ".
		 "   if(r.pid is not null,r.pid,rp.pid) as pid, ".
	         "   if(r.pid is not null,r.eid,rp.eid) as eid, ".
		 "   nt.class, ".
	 	 "   if(r.pid is not null,r.vname,rp.vname) as vname ".
		 "$additionalVariables ".
		 "from nodes as n ".
		 "left join node_types as nt on n.type=nt.type ".
		 "left join node_status as ns on n.node_id=ns.node_id ".
		 "left join reserved as r on n.node_id=r.node_id ".
		 "left join reserved as rp on n.phys_nodeid=rp.node_id ".
		 "$additionalLeftJoin ".
		 "where $role $clause ".
		 "ORDER BY priority");

if (mysql_num_rows($query_result) == 0) {
    echo "<center>Oops, no nodes to show you!</center>";
    PAGEFOOTER();
    exit();
}

#
# First count up free nodes as well as status counts.
#
$num_free = 0;
$num_up   = 0;
$num_pd   = 0;
$num_down = 0;
$num_unk  = 0;
$freetypes= array();

while ($row = mysql_fetch_array($query_result)) {
    $pid                = $row["pid"];
    $status             = $row["status"];
    $type               = $row["type"];

    if (! isset($freetypes[$type])) {
	$freetypes[$type] = 0;
    }
    if (!$pid) {
	$num_free++;
	$freetypes[$type]++;
	continue;
    }
    switch ($status) {
    case "up":
	$num_up++;
	break;
    case "possibly down":
    case "unpingable":
	$num_pd++;
	break;
    case "down":
	$num_down++;
	break;
    default:
	$num_unk++;
	break;
    }
}
$num_total = ($num_free + $num_up + $num_down + $num_pd + $num_unk);
mysql_data_seek($query_result, 0);

if (! strcmp($showtype, "widearea")) {
    echo "<a href='$WIKIDOCURL/widearea'>
             Widearea Usage Notes</a>\n";
}

echo "<br><center><b>
       View: $view\n";

if (! strcmp($showtype, "widearea")) {
    echo "<br>
          <a href=widearea_nodeinfo.php3>(Widearea Link Metrics)</a><br>
          <a href=plabmetrics.php3>(PlanetLab Node Metrics)</a>\n";
}

echo "</b></center><br>\n";

SUBPAGESTART();

echo "<table>
       <tr><td align=right>
           <img src='/autostatus-icons/greenball.gif' alt=up>
           <b>Up</b></td>
           <td align=left>$num_up</td>
       </tr>
       <tr><td align=right nowrap>
           <img src='/autostatus-icons/yellowball.gif' alt='possibly down'>
           <b>Possibly Down</b></td>
           <td align=left>$num_pd</td>
       </tr>
       <tr><td align=right>
           <img src='/autostatus-icons/blueball.gif' alt=unknown>
           <b>Unknown</b></td>
           <td align=left>$num_unk</td>
       </tr>
       <tr><td align=right>
           <img src='/autostatus-icons/redball.gif' alt=down>
           <b>Down</b></td>
           <td align=left>$num_down</td>
       </tr>
       <tr><td align=right>
           <img src='/autostatus-icons/whiteball.gif' alt=free>
           <b>Free</b></td>
           <td align=left>$num_free</td>
       </tr>
       <tr><td align=right><b>Total</b></td>
           <td align=left>$num_total</td>
       </tr>
       <tr><td colspan=2 nowrap align=center>
               <b>Free Subtotals</b></td></tr>\n";

foreach($freetypes as $key => $value) {
    echo "<tr>
           <td align=right><a href=shownodetype.php3?node_type=$key>
                           $key</a></td>
           <td align=left>$value</td>
          </tr>\n";
}
echo "</table>\n";
SUBMENUEND_2B();

echo "<table border=2 cellpadding=2 cellspacing=2 id='nodelist'>\n";

echo "<thead class='sort'>";
echo "<tr>
          <th align=center>ID</th>\n";

if ($showvnames) {
    echo "<th align=center>Name</th>\n";
}

echo "    <th align=center>Type (Class)</th>
          <th align=center class='sorttable_nosort'>Up?</th>\n";

if ($isadmin) {
    echo "<th align=center>PID</th>
          <th align=center>EID</th>
          <th align=center>Default<br>OSID</th>\n";
}
elseif (strcmp($showtype, "widearea")) {
    # Widearea nodes are always "free"
    echo "<th align=center>Free?</th>\n";
}

if (!strcmp($showtype, "widearea")) {
    echo "<th align=center>Site</th>
          <th align=center>Processor</th>
	  <th align=center>Connection</th>
	  <th align=center>Location</th>";
}
    
echo "</tr></thead>\n";

while ($row = mysql_fetch_array($query_result)) {
    $node_id            = $row["node_id"]; 
    $phys_nodeid        = $row["phys_nodeid"]; 
    $type               = $row["type"];
    $class              = $row["class"];
    $def_boot_osid      = $row["def_boot_osid"];
    $pid                = $row["pid"];
    $eid                = $row["eid"];
    $vname              = $row["vname"];
    $status             = $row["status"];

    if (!strcmp($showtype, "widearea")) {	
	$site         = $row["site"];
	$machine_type = $row["machine_type"];
	$location     = $row["location"];
	$connect_type = $row["connect_type"];
	$vname        = $row["hostname"];
    } 

    echo "<tr>";

    # Admins get a link to expand the node.
    if ($isadmin ||
	(OPSGUY() && (!$pid || $pid == $TBOPSPID))) {
	echo "<td><A href='shownode.php3?node_id=$node_id'>$node_id</a> " .
	    (!strcmp($node_id, $phys_nodeid) ? "" :
	     "(<A href='shownode.php3?node_id=$phys_nodeid'>$phys_nodeid</a>)")
	    . "</td>\n";
    }
    else {
	echo "<td>$node_id " .
  	      (!strcmp($node_id, $phys_nodeid) ? "" : "($phys_nodeid)") .
	      "</td>\n";
    }

    if ($showvnames) {
	if ($vname)
	    echo "<td>$vname</td>\n";
	else
	    echo "<td>--</td>\n";
    }
    
    echo "   <td>$type ($class)</td>\n";

    if (!$pid)
	echo "<td align=center>
                  <img src='/autostatus-icons/whiteball.gif' alt=free></td>\n";
    elseif (!$status)
	echo "<td align=center>
                  <img src='/autostatus-icons/blueball.gif' alt=unk></td>\n";
    elseif ($status == "up")
	echo "<td align=center>
                  <img src='/autostatus-icons/greenball.gif' alt=up></td>\n";
    elseif ($status == "down")
	echo "<td align=center>
                  <img src='/autostatus-icons/redball.gif' alt=down></td>\n";
    else
	echo "<td align=center>
                  <img src='/autostatus-icons/yellowball.gif' alt=unk></td>\n";

    # Admins get pid/eid/vname, but mere users yes/no.
    if ($isadmin) {
	if ($pid) {
	    echo "<td><a href=showproject.php3?pid=$pid>$pid</a></td>
                  <td><a href=showexp.php3?pid=$pid&eid=$eid>$eid</a></td>\n";
	}
	else {
	    echo "<td>--</td>
   	          <td>--</td>\n";
	}
	if ($def_boot_osid &&
	    ($osinfo = OSinfo::Lookup($def_boot_osid))) {
	    $osname = $osinfo->osname();
	    echo "<td>$osname</td>\n";
	}
	else
	    echo "<td>&nbsp</td>\n";
    }
    elseif (strcmp($showtype, "widearea")) {
	if ($pid)
	    echo "<td>--</td>\n";
	else
	    echo "<td>Yes</td>\n";
    }

    if (!strcmp($showtype, "widearea")) {	
	echo "<td>$site</td>
	      <td>$machine_type</td>
	      <td>$connect_type</td>
	      <td><font size='-1'>$location</font></td>\n";
    }
    
    echo "</tr>\n";
}

echo "</table>\n";
echo "<script type='text/javascript' language='javascript'>
         sorttable.makeSortable(getObjbyName('nodelist'));
      </script>\n";
SUBPAGEEND();

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>



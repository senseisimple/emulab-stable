<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Project Information List");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Admin users can see all projects, while normal users can only see
# projects for which they are the leader.
#
# XXX Should we form the list from project members instead of leaders?
#

if (!isset($splitview) || !$isadmin)
    $splitview = 0;
if (!isset($sortby))
    $sortby = "pid";
$eorder = 0;
$norder = 0;
$porder = 0;

if ($isadmin) {
    if (! $splitview) {
	echo "<b><a href='showproject_list.php3?splitview=1&sortby=$sortby'>
                    Split View</a>
              </b><br>\n";
    }
    else {
	echo "<b><a href='showproject_list.php3?sortby=$sortby'>
                    Normal View</a>
              </b><br>\n";
    }
}

if (! strcmp($sortby, "name"))
    $order = "p.name";
elseif (! strcmp($sortby, "pid"))
    $order = "p.pid";
elseif (! strcmp($sortby, "uid"))
    $order = "p.head_uid";
elseif (! strcmp($sortby, "idle"))
    $order = "idle";
elseif (! strcmp($sortby, "created"))
    $order = "p.expt_count DESC";
elseif (! strcmp($sortby, "running")) {
    $eorder = 1;
    $order  = "p.pid";
}
elseif (! strcmp($sortby, "nodes")) {
    $norder = 1;
    $order  = "p.pid";
}
elseif (! strcmp($sortby, "pcs")) {
    $porder = 1;
    $order  = "p.pid";
}
else 
    $order = "p.pid";

$allproj_result =
    DBQueryFatal("SELECT pid,expt_count FROM projects as p");

#
# This is summary info, indexed by the pid, so no need to alter it
# for admin vs non-admin. Thats handled in the project query below.
#
$ecounts = array();
$ncounts = array();
$pcounts = array();

$query_result =
    DBQueryFatal("select e.pid, count(distinct e.eid) as ecount ".
		 "from experiments as e where state!='$TB_EXPTSTATE_SWAPPED' ".
		 "group by e.pid");

while ($row = mysql_fetch_array($query_result)) {
    $pid   = $row[0];
    $count = $row[1];

    $ecounts[$pid] = $count;
}

$query_result =
    DBQueryFatal("select r.pid, count(distinct r.node_id) as ncount ".
		 "from reserved as r group by r.pid");

while ($row = mysql_fetch_array($query_result)) {
    $pid   = $row[0];
    $count = $row[1];

    $ncounts[$pid] = $count;
}

# Here we get just the PC counts.

$query_result =
    DBQueryFatal("select r.pid,nt.class,count(distinct r.node_id) as ncount ".
		 "  from reserved as r ".
		 "left join nodes as n on n.node_id=r.node_id ".
		 "left join node_types as nt on nt.type=n.type ".
		 "where nt.class='pc' ".
		 "group by r.pid,nt.class");

while ($row = mysql_fetch_array($query_result)) {
    $pid   = $row[0];
    $count = $row[2];

    $pcounts[$pid] = $count;
}

while ($projectrow = mysql_fetch_array($allproj_result)) {
    $pid = $projectrow[pid];

    if (!isset($ecounts[$pid])) 
	$ecounts[$pid] = 0;
	    
    if (!isset($ncounts[$pid])) 
	$ncounts[$pid] = 0;

    if (!isset($pcounts[$pid])) 
	$pcounts[$pid] = 0;
}

if ($eorder) 
     arsort($ecounts, SORT_NUMERIC);

if ($norder) 
     arsort($ncounts, SORT_NUMERIC);

if ($porder) 
     arsort($pcounts, SORT_NUMERIC);

if ($isadmin) {
    mysql_data_seek($allproj_result, 0);
    
    #
    # Process the query results for active projects so I can generate a
    # summary block (as per Jays request).
    #
    $total_projects  = mysql_num_rows($allproj_result);
    $active_projects = 0;

    while ($projectrow = mysql_fetch_array($allproj_result)) {
	$expt_count = $projectrow[expt_count];

	if ($expt_count > 0) {
	    $active_projects++;
	}
    }
    mysql_data_seek($query_result, 0);
    $never_active = $total_projects - $active_projects;

    echo "<table border=0 align=center cellpadding=0 cellspacing=2>
        <tr>
          <td align=right><b>Been Active:</b></td>
          <td align=left>$active_projects</td>
        </tr>
        <tr>
          <td align=right><b>Never Active:</b></td>
          <td align=left>$never_active</td>
        </tr>
        <tr>
          <td align=right><b>Total:</b></td>
          <td align=left>$total_projects</td>
        </tr>
      </table><br />\n";
}

function GENPLIST ($query_result)
{
    global $isadmin, $splitview, $eorder, $ecounts, $norder, $ncounts;
    global $porder, $pcounts;

    echo "<table width='100%' border=2
                 cellpadding=2 cellspacing=2 align=center>
          <tr>\n";

    echo "<th><a href='showproject_list.php3?splitview=$splitview&sortby=pid'>
               PID</a></th>\n";
    
    echo "<th>(Approved?)
              <a href='showproject_list.php3?splitview=$splitview&sortby=name'>
               Description</a></th>\n";

    echo "<th><a href='showproject_list.php3?splitview=$splitview&sortby=uid'>
               Leader</a></th>\n";

    echo "<th>
              <a href='showproject_list.php3?splitview=$splitview&sortby=idle'>
              Days<br>Idle</a></th>\n";

    echo "<th colspan=2>Expts<br>
           <a href='showproject_list.php3?splitview=$splitview&sortby=created'>
              Cr</a>,
           <a href='showproject_list.php3?splitview=$splitview&sortby=running'>
              Run</a></th>\n";

    echo "<th colspan=2>Nodes<br>
           <a href='showproject_list.php3?splitview=$splitview&sortby=nodes'>
              All</a>,
           <a href='showproject_list.php3?splitview=$splitview&sortby=pcs'>
              PCs</a></th>\n";

    #
    # This ordering stuff is a pain cause of the split joins, but a combined
    # join takes too long (hammers the DB to hard).
    #
    $projectrows = array();
    while ($projectrow = mysql_fetch_array($query_result)) {
	$pid = $projectrow[pid];

	$projectrows[$pid] = $projectrow;
    }
    $showby = $projectrows;
    if ($eorder)
	$showby = $ecounts;
    if ($norder)
	$showby = $ncounts;
    if ($porder)
	$showby = $pcounts;

    #
    # Admin users get other fields.
    # 
    if ($isadmin) {
	echo "<th align=center>Pub?</th>\n";
    }
    echo "</tr>\n";

    while (list($pid, $foo) = each($showby)) {
	$projectrow = $projectrows[$pid];
	$headuid    = $projectrow[head_uid];
	$Pname      = stripslashes($projectrow[name]);
	$approved   = $projectrow[approved];
	$expt_count = $projectrow[expt_count];
	$public     = $projectrow[public];
	$idle       = $projectrow[idle];
	$ecount     = $ecounts[$pid];
	$ncount     = $ncounts[$pid];
	$pcount     = $pcounts[$pid];

	if (! ($head_user = User::Lookup($headuid))) {
	    TBERROR("Could not lookup object for user $headuid", 1);
	}
	$showuser_url = CreateURL("showuser", $head_user);
	
	echo "<tr>
                  <td><A href='showproject.php3?pid=$pid'>$pid</A></td>
                  <td>\n";
	if ($approved) {
	    echo "    <img alt='Y' src='greenball.gif'>";
	}
	else {
	    echo "    <img alt='N' src='redball.gif'>\n";
	}
	echo "             $Pname</td>
                  <td><A href='$showuser_url'>$headuid</A></td>\n";

	echo "<td>$idle</td>\n";
	echo "<td>$expt_count</td>\n";
	echo "<td>$ecount</td>\n";
	echo "<td>$ncount</td>\n";
	echo "<td>$pcount</td>\n";

	if ($isadmin) {
	    if ($public) {
		echo "<td align=center><img alt='Y' src='greenball.gif'></td>";
	    }
	    else {
		echo "<td align=center><img alt='N' src='redball.gif'></td>\n";
	    }
	}
	echo "</tr>\n";
    }
    echo "</table>\n";
}

#
# Get the project list for non admins.
#
if (! $isadmin) {
    $query_result =
	DBQueryFatal("SELECT p.*, ".
		     "IF(p.expt_last, ".
		     "  TO_DAYS(CURDATE()) - TO_DAYS(p.expt_last), ".
		     "  TO_DAYS(CURDATE()) - TO_DAYS(p.created)) as idle ".
		     "FROM projects as p ".
		     "left join group_membership as g on ".
		     " p.pid=g.pid and g.pid=g.gid ".
		     "where g.uid='$uid' and g.trust!='none' ".
		     "group by p.pid order by $order");
				   
    if (mysql_num_rows($query_result) == 0) {
	USERERROR("You are not a member of any projects!", 1);
    }
    
    GENPLIST($query_result);
}
else {
    if ($splitview) {
	$query_result =
	    DBQueryFatal("SELECT p.*, ".
			 "IF(p.expt_last, ".
			 "  TO_DAYS(CURDATE()) - TO_DAYS(p.expt_last), ".
			 "  TO_DAYS(CURDATE()) - TO_DAYS(p.created)) as idle ".
			 "FROM projects as p ".
			 "where expt_count>0 ".
			 "group by p.pid order by $order");

	if (mysql_num_rows($query_result)) {
	    echo "<center>
                   <h3>Active Projects</h3>
                  </center>\n";
	    GENPLIST($query_result);
	}

	$query_result =
	    DBQueryFatal("SELECT p.*, ".
			 "IF(p.expt_last, ".
			 "  TO_DAYS(CURDATE()) - TO_DAYS(p.expt_last), ".
			 "  TO_DAYS(CURDATE()) - TO_DAYS(p.created)) as idle ".
			 "FROM projects as p ".
			 "where expt_count=0 ".
			 "group by p.pid order by $order");

	if (mysql_num_rows($query_result)) {
	    echo "<br><center>
                   <h3>Inactive Projects</h3>
                  </center>\n";
	    GENPLIST($query_result);
	}

    }
    else {
	$query_result =
	    DBQueryFatal("select p.*, ".
			 "IF(p.expt_last, ".
			 "  TO_DAYS(CURDATE()) - TO_DAYS(p.expt_last), ".
			 "  TO_DAYS(CURDATE()) - TO_DAYS(p.created)) as idle ".
			 " FROM projects as p ".
			 "group by p.pid order by $order");

	if (mysql_num_rows($query_result)) {
	    GENPLIST($query_result);
	}
    }
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

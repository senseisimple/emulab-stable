<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Project Information List");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Admin users can see all projects, while normal users can only see
# projects for which they are the leader.
#
# XXX Should we form the list from project members instead of leaders?
#
$isadmin = ISADMIN($uid);

if (!isset($splitview) || !$isadmin)
    $splitview = 0;
if (!isset($sortby))
    $sortby = "name";

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
elseif (! strcmp($sortby, "running"))
    $order = "ecount DESC";
elseif (! strcmp($sortby, "nodes"))
    $order = "ncount DESC";
else 
    $order = "p.name";

if ($isadmin) {
    $query_result =
	DBQueryFatal("SELECT pid,expt_count FROM projects as p");
    
    #
    # Process the query results for active projects so I can generate a
    # summary block (as per Jays request).
    #
    $total_projects  = mysql_num_rows($query_result);
    $active_projects = 0;

    while ($projectrow = mysql_fetch_array($query_result)) {
	$expt_count = $projectrow[expt_count];

	if ($expt_count > 0) {
	    $active_projects++;
	}
    }
    mysql_data_seek($query_result, 0);
    $never_active = $total_projects - $active_projects;

    echo "<table border=0 align=center cellpadding=0 cellspacing=3>
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
      </table>\n";
}

function GENPLIST ($query_result)
{
    global $isadmin, $splitview;

    echo "<table width='100%' border=2
                 cellpadding=2 cellspacing=0 align=center>
          <tr>\n";

    echo "<td><a href='showproject_list.php3?splitview=$splitview&sortby=pid'>
               PID</a></td>\n";
    
    echo "<td>(Approved?)
              <a href='showproject_list.php3?splitview=$splitview&sortby=name'>
               Description</a></td>\n";

    echo "<td><a href='showproject_list.php3?splitview=$splitview&sortby=uid'>
               Leader</a></td>\n";

    echo "<td align=center>
              <a href='showproject_list.php3?splitview=$splitview&sortby=idle'>
              Days<br>Idle</a></td>\n";

    echo "<td align=center colspan=2>Expts<br>
           <a href='showproject_list.php3?splitview=$splitview&sortby=created'>
              Cr</a>,
           <a href='showproject_list.php3?splitview=$splitview&sortby=running'>
              Run</a></td>\n";

    echo "<td align=center>
             <a href='showproject_list.php3?splitview=$splitview&sortby=nodes'>
             Nodes</a></td>\n";

    #
    # Admin users get other fields.
    # 
    if ($isadmin) {
	echo "<td align=center>Pub?</td>\n";
    }
    echo "</tr>\n";

    while ($projectrow = mysql_fetch_array($query_result)) {
	$pid        = $projectrow[pid];
	$headuid    = $projectrow[head_uid];
	$Pname      = $projectrow[name];
	$approved   = $projectrow[approved];
	$expt_count = $projectrow[expt_count];
	$public     = $projectrow[public];
	$ecount     = $projectrow[ecount];
	$ncount     = $projectrow[ncount];
	$idle       = $projectrow[idle];

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
                  <td><A href='showuser.php3?target_uid=$headuid'>
                         $headuid</A></td>\n";

	echo "<td>$idle</td>\n";
	echo "<td>$expt_count</td>\n";
	echo "<td>$ecount</td>\n";
	echo "<td>$ncount</td>\n";

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
	DBQueryFatal("SELECT p.pid,count(distinct r.node_id) as ncount, ".
		     "count(distinct e.eid) as ecount, ".
		     "IF(p.expt_last, ".
		     "  TO_DAYS(CURDATE()) - TO_DAYS(p.expt_last), ".
		     "  TO_DAYS(CURDATE()) - TO_DAYS(p.created)) as idle ".
		     "FROM projects as p ".
		     "left join group_membership as g on ".
		     " p.pid=g.pid and g.pid=g.gid ".
		     "left join experiments as e on e.pid=p.pid ".
		     "left join reserved as r on r.pid=p.pid ".
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
	    DBQueryFatal("SELECT p.*,count(distinct r.node_id) as ncount, ".
		     "count(distinct e.eid) as ecount, ".
		     "IF(p.expt_last, ".
		     "  TO_DAYS(CURDATE()) - TO_DAYS(p.expt_last), ".
		     "  TO_DAYS(CURDATE()) - TO_DAYS(p.created)) as idle ".
		     "FROM projects as p ".
		     "left join experiments as e on e.pid=p.pid ".
		     "left join reserved as r on r.pid=p.pid ".
		     "where expt_count>0 ".
		     "group by p.pid order by $order");

	if (mysql_num_rows($query_result)) {
	    echo "<center>
                   <h3>Active Projects</h3>
                  </center>\n";
	    GENPLIST($query_result);
	}

	$query_result =
	    DBQueryFatal("SELECT p.*,count(distinct r.node_id) as ncount, ".
		     "count(distinct e.eid) as ecount, ".
		     "IF(p.expt_last, ".
		     "  TO_DAYS(CURDATE()) - TO_DAYS(p.expt_last), ".
		     "  TO_DAYS(CURDATE()) - TO_DAYS(p.created)) as idle ".
		     "FROM projects as p ".
		     "left join experiments as e on e.pid=p.pid ".
		     "left join reserved as r on r.pid=p.pid ".
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
	    DBQueryFatal("SELECT p.*,count(distinct r.node_id) as ncount, ".
		     "count(distinct e.eid) as ecount, ".
		     "IF(p.expt_last, ".
		     "  TO_DAYS(CURDATE()) - TO_DAYS(p.expt_last), ".
		     "  TO_DAYS(CURDATE()) - TO_DAYS(p.created)) as idle ".
		     " FROM projects as p ".
		     "left join experiments as e on e.pid=p.pid ".
		     "left join reserved as r on r.pid=p.pid ".
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

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
else 
    $order = "p.name";

if ($isadmin) {
    $query_result =
	DBQueryFatal("SELECT * FROM projects as p order by $order");
    
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
          <tr>
           <td><a href='showproject_list.php3?splitview=$splitview&sortby=pid'>
               PID</td>
           <td>(Approved?)
              <a href='showproject_list.php3?splitview=$splitview&sortby=name'>
               Description</td>
           <td><a href='showproject_list.php3?splitview=$splitview&sortby=uid'>
               Leader</td>
           <td align=center>Days<br>Idle</td>
           <td align=center colspan=2>Expts<br>Cr, Run</td>
           <td align=center>Nodes</td>\n";

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
	$created    = $projectrow[created];
	$expt_count = $projectrow[expt_count];
	$expt_last  = $projectrow[expt_last];
	$public     = $projectrow[public];

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

        #
        # Sleazy! Use mysql query to convert dates to days and subtract!
        #
	if (isset($expt_last)) {
	    $idle_query =
		DBQueryFatal("select TO_DAYS(CURDATE()) - ".
			     "                    TO_DAYS('$expt_last')");
	    $idle_row   = mysql_fetch_row($idle_query);
	    echo "<td>$idle_row[0]</td>\n";
	}
	else {
	    $idle_query =
		DBQueryFatal("select TO_DAYS(CURDATE()) - ".
			     "              TO_DAYS('$created')");
	    $idle_row   = mysql_fetch_row($idle_query);
	    echo "<td>$idle_row[0]</td>\n";
	}

	# Number of Current Experiments.
	$query_cur_expts =
	    DBQueryFatal("SELECT count(*) FROM experiments where pid='$pid'");
	$cur_expts_row = mysql_fetch_array($query_cur_expts);

	# Number of nodes reserved.
	$query_nodes_reserved =
	    DBQueryFatal("SELECT count(*) FROM reserved where pid='$pid'");
	$nodes_reserved_row = mysql_fetch_array($query_nodes_reserved);

	echo "<td>$expt_count</td>\n";
	echo "<td>$cur_expts_row[0]</td>\n";
	echo "<td>$nodes_reserved_row[0]</td>\n";

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
	DBQueryFatal("SELECT * FROM projects as p ".
		     "left join group_membership as g on ".
		     "     p.pid=g.pid and g.pid=g.gid ".
		     "where g.uid='$uid' and g.trust!='none' order by $order");
				   
    if (mysql_num_rows($query_result) == 0) {
	USERERROR("You are not a member of any projects!", 1);
    }
    
    GENPLIST($query_result);
}
else {
    if ($splitview) {
	$query_result =
	    DBQueryFatal("select * FROM projects as p ".
			 "where expt_count>0 order by $order");

	if (mysql_num_rows($query_result)) {
	    echo "<center>
                   <h3>Active Projects</h3>
                  </center>\n";
	    GENPLIST($query_result);
	}

	$query_result =
	    DBQueryFatal("select * FROM projects as p ".
			 "where expt_count=0 order by $order");

	if (mysql_num_rows($query_result)) {
	    echo "<br><center>
                   <h3>Inactive Projects</h3>
                  </center>\n";
	    GENPLIST($query_result);
	}

    }
    else {
	GENPLIST($query_result);
    }
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

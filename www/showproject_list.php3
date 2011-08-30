<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2011 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments
#
$optargs = OptionalPageArguments("showtype",   PAGEARG_STRING);

#
# Standard Testbed Header
#
PAGEHEADER("Project Information List");

if (! isset($showtype)) {
    $showtype="Splitview";
}

if ($isadmin) {
    echo "<b>Show: ";
    
    if ($showtype != "Active") {
	echo "<a class='static'
                 href='showproject_list.php3?showtype=Active'>Active</a>, ";
    }
    else {
	echo "Active, ";
    }

    if ($showtype != "All") {
	echo "<a class='static'
                 href='showproject_list.php3?showtype=All'>All</a>, ";
    }
    else {
	echo "All, ";
    }

    if ($showtype != "Splitview") {
	echo "<a class='static'
               href='showproject_list.php3?showtype=Splitview'>Splitview</a>, ";
    }
    else {
	echo "Splitview, ";
    }

    if ($showtype != "ProtoGENI") {
	echo "<a class='static'
                 href='showproject_list.php3?showtype=ProtoGENI'>ProtoGENI</a>";
    }
    else {
	echo "ProtoGENI";
    }
}
echo "<br>\n";

$allproj_result =
    DBQueryFatal("SELECT p.pid,p.expt_count, ".
		 "       UNIX_TIMESTAMP(s.last_activity) as last_activity ".
		 "  FROM projects as p ".
		 "left join project_stats as s ".
		 "     on s.pid_idx=p.pid_idx");
		 
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
    $pid = $projectrow["pid"];

    if (!isset($ecounts[$pid])) 
	$ecounts[$pid] = 0;
	    
    if (!isset($ncounts[$pid])) 
	$ncounts[$pid] = 0;

    if (!isset($pcounts[$pid])) 
	$pcounts[$pid] = 0;
}

if ($isadmin) {
    mysql_data_seek($allproj_result, 0);
    
    #
    # Process the query results for active projects so I can generate a
    # summary block (as per Jays request).
    #
    $total_projects  = mysql_num_rows($allproj_result);
    $active_projects = 0;
    $recently_active = 0;

    while ($projectrow = mysql_fetch_array($allproj_result)) {
	$expt_count    = $projectrow["expt_count"];
	$last_activity = $projectrow["last_activity"];

	if ($expt_count > 0) {
	    $active_projects++;
	}
	if (time() - $last_activity < (24 * 3600 * 90)) {
	    $recently_active++;
	}
    }
    $never_active = $total_projects - $active_projects;

    echo "<table border=0 align=center cellpadding=0 cellspacing=2>
        <tr>
          <td align=right><b>Recently Active:</b></td>
          <td align=left>$recently_active</td>
        </tr>
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
    global $isadmin, $ecounts, $ncounts, $pcounts, $showtype;

    $tablename = "SPL" . rand();

    echo "<table width='100%' border=2 id='$tablename'
                 cellpadding=2 cellspacing=2 align=center>
          <thead class='sort'>
          <tr>\n";

    echo "<th>PID</th>\n";
    echo "<th>(Approved?) Description</th>\n";
    if ($showtype != "ProtoGENI") {
	echo "<th>Leader</th>\n";
	echo "<th>Affil</th>\n";
    }
    echo "<th>Days<br>Idle</th>\n";
    echo "<th>Expts<br>Created</th>\n";
    echo "<th>Expts<br>Running</th>\n";
    echo "<th>Nodes<br>All</th>\n";
    echo "<th>Nodes<br>PCs</th>\n";

    #
    # Admin users get other fields.
    # 
    if ($isadmin && $showtype != "ProtoGENI") {
	echo "<th align=center>Pub?</th>\n";
    }
    echo "</tr>\n";
    echo "</thead>\n";

    while ($projectrow = mysql_fetch_array($query_result)) {
	$pid        = $projectrow["pid"];
	$headidx    = $projectrow["head_idx"];
	$Pname      = $projectrow["name"];
	$approved   = $projectrow["approved"];
	$expt_count = $projectrow["expt_count"];
	$public     = $projectrow["public"];
	$idle       = $projectrow["idle"];
	$ecount     = $ecounts[$pid];
	$ncount     = $ncounts[$pid];
	$pcount     = $pcounts[$pid];

	if (! ($head_user = User::Lookup($headidx))) {
	    TBERROR("Could not lookup object for user $headidx", 1);
	}
	$showuser_url = CreateURL("showuser", $head_user);
	$headuid      = $head_user->uid();
	$affil        = $head_user->affil_abbrev();
	
	echo "<tr>
                  <td><A href='showproject.php3?pid=$pid'>$pid</A></td>
                  <td>\n";
	if ($approved) {
	    echo "    <img alt='Y' src='greenball.gif'>";
	}
	else {
	    echo "    <img alt='N' src='redball.gif'>\n";
	}
	echo "             $Pname</td>";
	if ($showtype != "ProtoGENI") {
	    echo "<td><A href='$showuser_url'>$headuid</A></td>\n";
	    echo "<td>$affil</td>\n";
	}
	echo "<td>$idle</td>\n";
	echo "<td>$expt_count</td>\n";
	echo "<td>$ecount</td>\n";
	echo "<td>$ncount</td>\n";
	echo "<td>$pcount</td>\n";

	if ($isadmin && $showtype != "ProtoGENI") {
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

    echo "<script type='text/javascript' language='javascript'>
	  sorttable.makeSortable(getObjbyName('$tablename'));
          </script>\n";
}

#
# Get the project list for non admins.
#
if (! $isadmin) {
    $uid_idx = $this_user->uid_idx();
    
    $query_result =
	DBQueryFatal("SELECT p.*, ".
		     "IF(p.expt_last, ".
		     "  TO_DAYS(CURDATE()) - TO_DAYS(p.expt_last), ".
		     "  TO_DAYS(CURDATE()) - TO_DAYS(p.created)) as idle ".
		     "FROM projects as p ".
		     "left join group_membership as g on ".
		     " p.pid=g.pid and g.pid=g.gid ".
		     "where g.uid_idx='$uid_idx' and g.trust!='none' ".
		     "group by p.pid order by p.pid");
				   
    if (mysql_num_rows($query_result) == 0) {
	USERERROR("You are not a member of any projects!", 1);
    }
    
    GENPLIST($query_result);
}
else {
    if ($showtype == "Splitview") {
	$query_result =
	    DBQueryFatal("SELECT p.*, ".
			 "IF(p.expt_last, ".
			 "  TO_DAYS(CURDATE()) - TO_DAYS(p.expt_last), ".
			 "  TO_DAYS(CURDATE()) - TO_DAYS(p.created)) as idle ".
			 "FROM projects as p ".
			 "where expt_count>0 ".
			 "group by p.pid order by p.pid");

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
			 "group by p.pid order by p.pid");

	if (mysql_num_rows($query_result)) {
	    echo "<br><center>
                   <h3>Inactive Projects</h3>
                  </center>\n";
	    GENPLIST($query_result);
	}

    }
    elseif ($showtype == "Active") {
	$query_result =
	    DBQueryFatal("select p.*, ".
			 "IF(p.expt_last, ".
			 "  TO_DAYS(CURDATE()) - TO_DAYS(p.expt_last), ".
			 "  TO_DAYS(CURDATE()) - TO_DAYS(p.created)) as idle ".
			 " FROM projects as p ".
			 "left join project_stats as s ".
			 "     on s.pid_idx=p.pid_idx ".
			 "where (UNIX_TIMESTAMP(now()) - ".
			 "       UNIX_TIMESTAMP(s.last_activity)) < ".
			 "           (24 * 3600 * 90) ".
			 "group by p.pid order by p.pid");

	if (mysql_num_rows($query_result)) {
	    GENPLIST($query_result);
	}
    }
    elseif ($showtype == "ProtoGENI") {
	$query_result =
	    DBQueryFatal("select p.*, ".
			 "IF(p.expt_last, ".
			 "  TO_DAYS(CURDATE()) - TO_DAYS(p.expt_last), ".
			 "  TO_DAYS(CURDATE()) - TO_DAYS(p.created)) as idle ".
			 " FROM projects as p ".
			 "where nonlocal_type is not null and ".
			 "      nonlocal_type='protogeni' ".
			 "group by p.pid order by p.pid");

	if (mysql_num_rows($query_result)) {
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
			 "group by p.pid order by p.pid");

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

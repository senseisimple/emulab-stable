<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Experiment Information Listing");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

$isadmin     = ISADMIN($uid);
$clause      = 0;
$having      = "";
$active      = 0;
$idle        = 0;

if (! isset($showtype))
    $showtype="active";
if (! isset($sortby))
    $sortby = "normal";
if (! isset($thumb)) 
    $thumb = 0;

echo "<b>Show:
         <a href='showexp_list.php3?showtype=active&sortby=$sortby&thumb=$thumb'>active</a>,
         <a href='showexp_list.php3?showtype=batch&sortby=$sortby&thumb=$thumb'>batch</a>,";
if ($isadmin) 
     echo "\n<a href='showexp_list.php3?showtype=idle&sortby=$sortby&thumb=$thumb'".
       ">idle</a>,";

echo "\n       <a href='showexp_list.php3?showtype=all&sortby=$sortby&thumb=$thumb'>all</a>.
      </b><br />\n";


#
# Handle showtype
# 
if (! strcmp($showtype, "all")) {
    $title  = "All";
}
elseif (! strcmp($showtype, "active")) {
    # Active is now defined as "having nodes reserved" - we just depend on
    # the fact that we skip expts with no nodes further down...
    $title  = "Active";
    $having = "having (ncount>0)";
    $active = 1;
}
elseif (! strcmp($showtype, "batch")) {
    $clause = "e.batchmode=1";
    $title  = "Batch";
}
elseif ((!strcmp($showtype, "idle")) && $isadmin ) {
    # Do not put active in the clause for same reason as active
    $title  = "Idle";
    #$having = "having (lastswap>=1)"; # At least one day since swapin
    $having = "having (lastswap>=0)";
    $idle = 1;
}
else {
    # See active above
    $title  = "Active";
    $having = "having (ncount>0)";
    $active = 1;
}



if (!$idle) {
    if (!$thumb) {
	echo "<b><a href='showexp_list.php3?showtype=$showtype&sortby=pid&thumb=1'>".
             "Switch to Thumbnail view".
	     "</a></b><br />";
    } else {
	echo "<b><a href='showexp_list.php3?showtype=$showtype&sortby=pid&thumb=0'>".
             "Switch to List view".
	     "</a></b><br />";
    }
}

echo "<br />\n";

#
# Handle sortby.
# 
if (! strcmp($sortby, "normal") ||
    ! strcmp($sortby, "pid"))
    $order = "e.pid,e.eid";
elseif (! strcmp($sortby, "eid"))
    $order = "e.eid,e.expt_head_uid";
elseif (! strcmp($sortby, "uid"))
    $order = "e.expt_head_uid,e.pid,e.eid";
elseif (! strcmp($sortby, "name"))
    $order = "e.expt_name";
elseif (! strcmp($sortby, "pcs"))
    $order = "ncount DESC";
else 
    $order = "e.pid,e.eid";

#
# Show a menu of all experiments for all projects that this uid
# is a member of. Or, if an admin type person, show them all!
#
if ($isadmin) {
    if ($clause)
	$clause = "and ($clause)";
    else
        $clause = "";

    $experiments_result =
	DBQueryFatal("select e.*,".
		     "date_format(expt_swapped,\"%Y-%m-%d\") as d, ".
		     "(to_days(now())-to_days(expt_swapped)) as lastswap, ".
                     "count(r.node_id) as ncount, swap_requests, ".
		     "round((unix_timestamp(now()) - ".
		     "unix_timestamp(last_swap_req))/3600,1) as lastreq ".
		     "from experiments as e ".
                     "left join reserved as r on e.pid=r.pid and e.eid=r.eid ".
                     "left join nodes as n on r.node_id=n.node_id ".
                     "where (n.type!='dnard' or n.type is null) $clause ".
                     "group by e.pid,e.eid ".
		     "$having ".
		     "order by $order");
    if ($idle) {
      # Run idlecheck and get the info
      #print "<pre>Running idlecheck\n";
      $x=exec("$TBSUEXEC_PATH $uid $TBADMINGROUP webidlecheck -s -u",
	      $l, $rv);
      reset($l);
      while(list($index,$i) = each ($l)) {
	list($ipid,$ieid,$word1, $word2, $word3) = split("[ \t/]+",$i);
	#print "$ipid + $ieid + $word1 + $word2 + $word3\n";
	$expt = "$ipid/$ieid";
	$set = array($word1,$word2,$word3);
	while(list($index,$tag) = each($set)) {
	  if ($tag == "") { break; }
	  if ($tag == "inactive") { $inactive[$expt]=1; }
	  elseif ($tag == "stale") { $stale[$expt]=1; }
	  elseif ($tag == "unswappable") { $unswap[$expt]=1; }
	  else {
	    if (defined($other[$expt])) { $other[$expt].=$tag; }
	    else {$other[$expt]=$tag; }
	  }
	}
	
      }
      #print "\nDone idlechecking...\n</pre>";
    }
      
}
else {
    if ($clause)
	$clause = "and ($clause)";
    else
        $clause = "";
    
    $experiments_result =
	DBQueryFatal("select distinct e.*, ".
                     "date_format(expt_swapped,'%Y-%m-%d') as d, ".
                     "count(r.node_id) as ncount ".
                     "from group_membership as g ".
                     "left join experiments as e on ".
                     "  g.pid=e.pid and g.pid=g.gid ".
                     "left join reserved as r on e.pid=r.pid and e.eid=r.eid ".
                     "left join nodes as n on r.node_id=n.node_id ".
                     "where (n.type!='dnard' or n.type is null) and ".
                     " g.uid='$uid' and e.pid is not null ".
		     "and e.eid is not null $clause ".
                     "group by e.pid,e.eid ".
		     "$having ".
                     "order by $order");
}
if (! mysql_num_rows($experiments_result)) {
    USERERROR("There are no experiments running in any of the projects ".
              "you are a member of.", 1);
}

if (mysql_num_rows($experiments_result)) {
    echo "<center>
           <h2>$title Experiments</h2>
          </center>\n";

    if ($idle) {
      echo "<p><center><b>Experiments that have been idle at least 24 hours</b></center></p><br />\n";
    }
    
    $idlemark = "<b>*</b>";
    #
    # Okay, I decided to do this as one big query instead of a zillion
    # per-exp queries in the loop below. No real reason, except my personal
    # desire not to churn the DB.
    # 
    $total_usage  = array();
    $perexp_usage = array();

    #
    # Geta all the classes of nodes each experiment is using, and create
    # a two-D array with their counts.
    # 
    $usage_query =
	DBQueryFatal("select r.pid,r.eid,nt.class, ".
		     "  count(nt.class) as ncount ".
		     "from reserved as r ".
		     "left join nodes as n on r.node_id=n.node_id ".
		     "left join node_types as nt on n.type=nt.type ".
		     "where n.type!='dnard' ".
		     "group by r.pid,r.eid,nt.class");

    while ($row = mysql_fetch_array($usage_query)) {
	$pid   = $row[0];
	$eid   = $row[1];
	$class = $row[2];
	$count = $row[3];
	
	$perexp_usage["$pid:$eid"][$class] = $count;
    }

    #
    # Now shove out the column headers.
    #
if ($thumb && !$idle) {
    echo "<table border=2 cols=0
                 cellpadding=2 cellspacing=2 align=center><tr>";

    $thumbCount = 0;
 
    while ($row = mysql_fetch_array($experiments_result)) {	
	$pid  = $row["pid"];
	$eid  = $row["eid"]; 
	$huid = $row["expt_head_uid"];
	$name = stripslashes($row["expt_name"]);
	$date = $row["d"];
	
	if ($idle && ($str=="&nbsp;" || !$pcs)) { continue; }
#	echo "<table style=\"float: none;\" width=256 height=192><tr><td>".
#	echo "And<table align=left width=256 height=192><tr width=256><td height=192>".
#	echo "<table width=256 height=192><tr><td>".
#	echo "<td width=50%>".
#	echo "<tr

	echo "<td>".
             "<table border=0 cellpadding=0 cellspacing=0 style='margin: 2px;' width=100%><tr>".
	     "<td width=128>".
	     "<img border=1 width=128 height=128 style='background-color:#FFF;' ".
	     " src='top2image.php3?pid=$pid&eid=$eid&thumb=128' align=center></td>" .
	     "<td style='padding: 8px;'><b><a href='showproject.php3?pid=$pid'>$pid</a>/".
             "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b></h4><br />\n".
	     "<b><font size=-1>$name</font></b><br />\n";

	# echo "<font size=-2>Using 69 PCs</font>\n";

	if ($isadmin) {
	    $swappable= $row["swappable"];
	    $swapreq=$row["swap_requests"];
	    $lastswapreq=$row["lastreq"];
	    $lastlogin = "";
	    if ($lastexpnodelogins = TBExpUidLastLogins($pid, $eid)) {
	        $daysidle=$lastexpnodelogins["daysidle"];
	        #if ($idle && $daysidle < 1)
		#  continue;
		$lastlogin .= $lastexpnodelogins["date"] . " " .
		 "(" . $lastexpnodelogins["uid"] . ")";
	    } elseif ($state=="active") {
	        $daysidle=$row["lastswap"];
	        $lastlogin .= "$date Swapped In";
	    }
	    # if ($lastlogin=="") { $lastlogin="<td>&nbsp;</td>\n"; }
	    if ($lastlogin != "") {
		echo "<font size=-2><b>Last Login:</b> $lastlogin</font><br />\n";
	    }
	}	

	echo "<font size=-2><b>Created by:</b> ".
             "<a href='showuser.php3?target_uid=$huid'>$huid</a>".
	     "</font><br />\n";

	echo "</td></tr></table> \n";
	echo "</td>";

	$thumbcount++;
	if (($thumbcount % 2) == 0) { echo "</tr><tr>\n"; }
    }

    echo "</tr></table>";
} else {
    echo "<table border=2 cols=0
                 cellpadding=0 cellspacing=2 align=center>
            <tr>
              <th width=8%>
               <a href='showexp_list.php3?showtype=$showtype&sortby=pid'>
                  PID</a></th>
              <th width=8%>
               <a href='showexp_list.php3?showtype=$showtype&sortby=eid'>
                  EID</a></th>
              <th align=center width=3%>
               <a href='showexp_list.php3?showtype=$showtype&sortby=pcs'>
                  PCs</a><br>[<b>1</b>]</th>\n";
    
    if ($isadmin && !$idle)
        echo "<th width=17% align=center>Last Login</th>\n";
    if ($idle)
        #      "<th width=4% align=center>Days Idle</th>\n";
	echo "<th width=4% align=center>Slothd Info</th>
              <th width=4% align=center colspan=2>Swap Request</th>\n";

    echo "    <th width=60%>
               <a href='showexp_list.php3?showtype=$showtype&sortby=name'>
                  Name</a></th>
              <th width=4%>
               <a href='showexp_list.php3?showtype=$showtype&sortby=uid'>
                  Head UID</a></th>
            </tr>\n";

    while ($row = mysql_fetch_array($experiments_result)) {
	$pid  = $row["pid"];
	$eid  = $row["eid"]; 
	$huid = $row["expt_head_uid"];
	$name = stripslashes($row["expt_name"]);
	$date = $row["d"];
	$state= $row["state"];
	$isidle = $row["swap_requests"];
	$daysidle=0;
	
	if ($isadmin) {
	    $swappable= $row["swappable"];
	    $swapreq=$row["swap_requests"];
	    $lastswapreq=$row["lastreq"];
	    $lastlogin = "<td>";
	    if ($lastexpnodelogins = TBExpUidLastLogins($pid, $eid)) {
	        $daysidle=$lastexpnodelogins["daysidle"];
	        #if ($idle && $daysidle < 1)
		#  continue;
		$lastlogin .= $lastexpnodelogins["date"] . " " .
		 "(" . $lastexpnodelogins["uid"] . ")";
	    } elseif ($state=="active") {
	        $daysidle=$row["lastswap"];
	        $lastlogin .= "$date Swapped In";
	    }
	    $lastlogin.="</td>\n";
	    if ($lastlogin=="<td></td>\n") { $lastlogin="<td>&nbsp;</td>\n"; }
	}

	if ($idle) {
	    #$lastlogin .= "<td align=center>$daysidle</td>\n";
	    $foo = "<td align=left>";
	    $expt = "$pid/$eid";
	    $str="";
	    if ($inactive[$expt]==1) {
	      if ($stale[$expt]==1) {
		$str .= "possibly&nbsp;inactive, ";
	      } elseif  ($unswap[$expt]==1) {
		$str .= "<b>probably&nbsp;inactive, unswappable</b>";
	      } else {
		$str .= "<b>inactive</b>";
	      }
	      if ($other[$expt]) { $str .= " ($other[$expt]) "; }
	    }
	    if ($stale[$expt]==1) { $str .= "<b>no&nbsp;report</b> "; }
	    # For now, don't show this tag, it's redundant
            #if ($unswap[$expt]==1) { $str .= "unswappable"; }
	    if ($str=="") { $str="&nbsp;"; }
	    # sanity check
	    $slothderr=0;
	    if ($daysidle==0 && $inactive[$expt]==1 && $stale[$expt]==0) {
	      $str .= " (recently logged into)\n";
	      $slothderr=1;
	    }
	    $foo .= "$str</td>\n";
	    if (isset($perexp_usage["$pid:$eid"]) &&
		isset($perexp_usage["$pid:$eid"]["pc"])) {
	      $pcs = $perexp_usage["$pid:$eid"]["pc"];
	    } else { $pcs=0; }
	    $foo .= "<td align=center valign=center>\n";
 	    if ($inactive[$expt]==1 && $stale[$expt]!=1 &&
	        !$slothderr && $pcs) {
	      $fooswap = "<td><a href=\"request_swapexp.php3?pid=$pid&eid=$eid\">".
			 "<img border=0 src=\"redball.gif\"></a></td>\n" ;
	    } else {
	      $fooswap = "<td></td>";
	      if (!$pcs) { $foo .= "(no PCs)\n"; }
	      else { $foo .="&nbsp;"; }
	    }
	    if ($swapreq > 0) {
	      $foo .= "&nbsp;$swapreq&nbsp;sent<br />".
	              "<font size=-2>(${lastswapreq}&nbsp;hours&nbsp;ago)</font>\n";
	    }
	    $foo .= "</td>" . $fooswap . "\n"; 
	}

	if ($idle && ($str=="&nbsp;" || !$pcs)) { continue; }

	$nodes   = 0;
	$special = 0;
	reset($perexp_usage);
	if (isset($perexp_usage["$pid:$eid"])) {
	    while (list ($class, $count) = each($perexp_usage["$pid:$eid"])) {
		$nodes += $count;
		if (strcmp($class, "pc")) {
		    $special = 1;
		} else {
		    $pcs = $count;
		}


		# Summary counts for just the experiments in the projects
		# the user is a member of.
		if (!isset($total_usage[$class]))
		    $total_usage[$class] = 0;
			
		$total_usage[$class] += $count;
	    }
	}

	# in idle or active, skip experiments with no nodes.
	if (($idle || $active) && $nodes == 0) { continue; }
	if ($nodes==0) { $nodes = "&nbsp;"; }
	
	echo "<tr>
                <td><A href='showproject.php3?pid=$pid'>$pid</A></td>
                <td><A href='showexp.php3?pid=$pid&eid=$eid'>
                       $eid</A></td>\n";
	
	if ($isidle) { $nodes = $nodes.$idlemark; }
	# If multiple classes, then hightlight the number.
	if ($special)
            echo "<td><font color=red>$nodes</font></td>\n";
	else
            echo "<td>$nodes</td>\n";

	if ($isadmin && !$idle) echo "$lastlogin\n";
	if ($idle) echo "$foo\n";

        echo "<td>$name</td>
                <td><A href='showuser.php3?target_uid=$huid'>
                       $huid</A></td>
               </tr>\n";
    }
    echo "</table>\n";

}

    echo "<ol>
             <li><font color=red>Red</font> indicates nodes other than PCs.
                 A $idlemark mark by the node count indicates that the
                 experiment is currently considered idle.
          </ol>\n";
    
    echo "<center><b>Node Totals</b></center>\n";
    echo "<table border=0
                 cellpadding=1 cellspacing=1 align=center>\n";
    $total = 0;
    ksort($total_usage);
    while (list($type, $count) = each($total_usage)) {
	    $total += $count;
	    echo "<tr>
                    <td align=right>${type}:</td>
                    <td>&nbsp &nbsp &nbsp &nbsp</td>
                    <td align=center>$count</td>
                  </tr>\n";
    }
    echo "<tr>
             <td align=right><hr></td>
             <td>&nbsp &nbsp &nbsp &nbsp</td>
             <td align=center><hr></td>
          </tr>\n";
    echo "<tr>
             <td align=right><b>Total</b>:</td>
             <td>&nbsp &nbsp &nbsp &nbsp</td>
             <td align=center>$total</td>
          </tr>\n";
    
    echo "</table>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

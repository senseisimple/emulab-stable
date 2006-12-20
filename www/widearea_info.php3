<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Show argument.
#
if (! isset($showtype))
    $showtype="delay";
if (! isset($plain))
    $plain=0;

if (! $plain) {
    PAGEHEADER("Wide Area Link Characteristics");

    echo "<b>Show:
           <a href='widearea_info.php3?showtype=delay&plain=$plain'>Delay</a>,
           <a href='widearea_info.php3?showtype=bw&plain=$plain'>Bandwidth</a>,
           <a href='widearea_info.php3?showtype=plr&plain=$plain'>Lossrate</a>,
           <a href='widearea_info.php3?showtype=dates&plain=$plain'>Dates</a>,
           <a href='widearea_info.php3?showtype=all&plain=$plain'>All</a>. ";
    echo "Format:
           <a href='widearea_info.php3?showtype=$showtype&plain=1'>Text</a>.";
    echo "</b><br><br>\n";
}
else {
    header("Content-Type: text/plain");
    echo "\n";
}

#
# Convert showtype to quicker compare
#
if (!strcmp($showtype, "all"))
    $showtype = 1;
elseif (!strcmp($showtype, "delay"))
    $showtype = 2;
elseif (!strcmp($showtype, "bw"))
    $showtype = 3;
elseif (!strcmp($showtype, "plr"))
    $showtype = 4;
elseif (!strcmp($showtype, "dates"))
    $showtype = 5;
else
    $showtype = 1;

function SPITDATA($table, $title, $showtype, $plain)
{
    $query_result1 =
	DBQueryFatal("select t.*,n.priority from $table as t ".
		     "left join nodes as n on n.node_id=t.node_id1 ".
		     "order by n.priority");
    
    # PHP arrays are strange wrt "each" function. Need two copies.
    $nodenamesrow = array();
    $nodenamescol = array();

    while ($row = mysql_fetch_array($query_result1)) {
	$node_id1 = $row[node_id1];
	$iface1   = $row[iface1];
	$node_id2 = $row[node_id2];
	$iface2   = $row[iface2];
	$time     = $row[time];
	$bw       = $row[bandwidth];
	$plr      = $row[lossrate];
	$start    = $row[start_time];
	$end      = $row[end_time];
	$pri      = $row[priority];
    
	$glom1    = $node_id1;
	$glom2    = $node_id2;

        #echo "Got $glom1 to $glom2 $pri<br>\n";
	
	$nodenamesrow[$glom1] = $pri;
	$nodenamescol[$glom1] = $pri;

#	$nodenamesrow[$glom2] = 1;
#	$nodenamescol[$glom2] = 1;

	$speeds[ $glom1 . "+" . $glom2 ] = $time * 1000;
	$bws[ $glom1 . "+" . $glom2 ] = $bw;
	$plrs[ $glom1 . "+" . $glom2 ] = $plr * 100.0;
	$dates[ $glom1 . "+" . $glom2 ] =
	    strftime("%m/%d-%H:%M", $start) . "<br>" .
	    strftime("%m/%d-%H:%M", $end);
	
	
    }
    asort($nodenamesrow, SORT_NUMERIC);
    reset($nodenamesrow);
    asort($nodenamescol, SORT_NUMERIC);
    reset($nodenamescol);

    if (! $plain) {
	echo "<center>
               <b>$title</b><br>\n";
    
	if ($showtype == 1)
	    echo "(Delay,BW,PLR)\n";
	elseif ($showtype == 2)
	    echo "(Delay)\n";
	elseif ($showtype == 3)
	    echo "(BW)\n";
	elseif ($showtype == 4)
	    echo "(PLR)\n";
	elseif ($showtype == 5)
	    echo "(Time Span of Data)\n";

	echo "</center><br>\n";

	echo "<table border=2 
                     cellpadding=1 cellspacing=2 align=center>
               <tr>";
	echo "   <td>&nbsp</td>\n";

	while (list($n1, $ignore1) = each($nodenamescol)) {
	    echo "<td>$n1</td>\n";
	}
	reset($nodenamescol);
	echo " </tr>\n";
    }
    else {
	printf("$title:\n");
	printf("%10s ", " ");
	while (list($n1, $ignore1) = each($nodenamescol)) {
	    printf("%10s ", $n1);
	}
	echo "\n";
	reset($nodenamescol);
    }

    while (list($n1, $ignore1) = each($nodenamesrow)) {
	if (! $plain) {
	    echo "<tr>";
	    echo "  <td>$n1</td>";
	}
	else {
	    printf("%10s ", "$n1");
	}
    
	while (list($n2, $ignore2) = each($nodenamescol)) {
	    if (strcmp($n1, $n2)) {
		$s = -1;
		$b = -1;
		$p = -1;
		$d = (($plain) ? "" : "&nbsp");
		$glom = $n1 . "+" . $n2;

		if (isset($speeds[$glom])) {
		    $s = (int) $speeds[$glom];
		    $b = (int) $bws[$glom];
		    $p = sprintf("%.2f", $plrs[$glom]);
		    $d = $dates[$glom];
		}
		if ($plain) {
		    if ($showtype == 1)
			printf("%10s ", "$s,$b,$p");
		    elseif ($showtype == 2)
			printf("%10s ", "$s");
		    elseif ($showtype == 3)
			printf("%10s ", "$b");
		    elseif ($showtype == 4)
			printf("%10s ", "$p");
		    elseif ($showtype == 5)
			printf("%10s ", "$d");
		}
		else {
		    if ($showtype == 1)
			echo "<td>$s,$b,$p</td>";
		    elseif ($showtype == 2)
			echo "<td align=center>$s</td>";
		    elseif ($showtype == 3)
			echo "<td align=center>$b</td>";
		    elseif ($showtype == 4)
			echo "<td align=center>$p</td>";
		    elseif ($showtype == 5)
			echo "<td align=center nowrap=1>$d</td>";
		}
	    }
	    else {
		if ($plain)
		    printf("%10s ", "*");
		else
		    echo "<td align=center><img src=blueball.gif alt=0</td>";
	    }
	}
	reset($nodenamescol);
	if (!$plain) {
	    echo "</tr>";
	}
	echo "  \n";
    }
    if ($plain) {
	echo "\n\n";
    }
    else {
	echo "</table>\n";
	echo "<center>
              Delay in milliseconds, Bandwidth in KB/s, PLR rounded to two
	      decimal places (0-100%).
              </center>\n";
	echo "<br>\n";
    }
}

SPITDATA("widearea_recent", "Most Recent Data", $showtype, $plain);
SPITDATA("widearea_delays", "Aged Data", $showtype, $plain);

#
# Standard Testbed Footer
#
if (! $plain) {
    PAGEFOOTER();
} 
?>

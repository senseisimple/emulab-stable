<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Wide Area Link Characteristics");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Show argument.
#
if (! isset($showtype))
    $showtype="delay";

echo "<b>Show:
         <a href='widearea_info.php3?showtype=delay'>Delay</a>,
         <a href='widearea_info.php3?showtype=bw'>Bandwidth</a>,
         <a href='widearea_info.php3?showtype=plr'>Lossrate</a>,
         <a href='widearea_info.php3?showtype=all'>All</a>.
      </b><br><br>\n";

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
else
    $showtype = 1;

function SPITDATA($table, $title, $showtype)
{
    $query_result1 =
	DBQueryFatal("select * from $table order by node_id1");
    
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
    
	$glom1    = $node_id1;
	$glom2    = $node_id2;

        # echo "Got $glom1 to $glom2 in $msectime ms<br>\n";
	
	$nodenamesrow[$glom1] = 1;
	$nodenamesrow[$glom2] = 1;
	$nodenamescol[$glom1] = 1;
	$nodenamescol[$glom2] = 1;

	$speeds[ $glom1 . "+" . $glom2 ] = $time * 1000;
	$bws[ $glom1 . "+" . $glom2 ] = $bw;
	$plrs[ $glom1 . "+" . $glom2 ] = $plr * 100.0;
    }
    ksort($nodenamesrow);
    ksort($nodenamescol);

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

    echo "</center><br>\n";

    echo "<table border=2 
                 cellpadding=1 cellspacing=2 align=center>
             <tr>";
    echo "<td>&nbsp</td>\n";

    while (list($n1, $ignore1) = each($nodenamescol)) {
	echo "<td>$n1</td>\n";
    }
    reset($nodenamescol);
    echo "</tr>\n";

    while (list($n1, $ignore1) = each($nodenamesrow)) {
	echo "<tr>";
	echo "  <td>$n1</td>";
    
	while (list($n2, $ignore2) = each($nodenamescol)) {
	    if (strcmp($n1, $n2)) {
		$s = 0;
		$b = 0;
		$p = 0.0;

		if (isset($speeds[ $n1 . "+" . $n2 ])) {
		    $s = (int) $speeds[ $n1 . "+" . $n2 ];
		    $b = (int) $bws[ $n1 . "+" . $n2 ];
		    $p = sprintf("%.2f", $plrs[ $n1 . "+" . $n2 ]);
		}
		if ($showtype == 1)
		    echo "<td>$s,$b,$p</td>";
		elseif ($showtype == 2)
		    echo "<td align=center>$s</td>";
		elseif ($showtype == 3)
		    echo "<td align=center>$b</td>";
		elseif ($showtype == 4)
		    echo "<td align=center>$p</td>";
	    }
	    else {
		echo "<td align=center><img src=blueball.gif alt=0</td>";
	    }
	}
	reset($nodenamescol);
	echo "</tr>\n";
    }
    echo "</table>\n";
    echo "<center>
           Delay in milliseconds, Bandwidth in KB/s, PLR rounded to two
	   decimal places (0-100%).
          </center>\n";
    echo "<br>\n";
}

SPITDATA("widearea_recent", "Most Recent Data", $showtype);
SPITDATA("widearea_delays", "Aged Data", $showtype);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

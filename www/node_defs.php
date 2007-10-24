<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include_once("osinfo_defs.php");

#
# A cache to avoid lookups. Indexed by node_id.
#
$node_cache = array();

# Constants for Show() method.
define("SHOWNODE_NOFLAGS",	0);
define("SHOWNODE_SHORT",	1);
define("SHOWNODE_NOPERM",	2);

class Node
{
    var	$node;

    #
    # Constructor by lookup on unique index.
    #
    function Node($node_id) {
	$safe_node_id = addslashes($node_id);

	$query_result =
	    DBQueryWarn("select * from nodes where node_id='$safe_node_id'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->node = NULL;
	    return;
	}
	$this->node = mysql_fetch_array($query_result);
    }

    # Hmm, how does one cause an error in a php constructor?
    function IsValid() {
	return !is_null($this->node);
    }

    # Lookup by node_id
    function Lookup($node_id) {
	global $node_cache;

        # Look in cache first
	if (array_key_exists("$node_id", $node_cache))
	    return $node_cache["$node_id"];

	$foo = new Node($node_id);

	if (! $foo->IsValid())
	    return null;

	# Insert into cache.
	$node_cache["$node_id"] =& $foo;
	return $foo;
    }

    # Lookup by IP
    function LookupByIP($ip) {
	$safe_ip = addslashes($ip);
	
	$query_result =
	    DBQueryFatal("select i.node_id from interfaces as i ".
			 "where i.IP='$ip' and ".
			 "      i.role='" . TBDB_IFACEROLE_CONTROL . "'");

	if (mysql_num_rows($query_result) == 0) {
	    return null;
	}
	$row = mysql_fetch_array($query_result);
	return Node::Lookup($row["node_id"]);
    }

    #
    # Refresh an instance by reloading from the DB.
    #
    function Refresh() {
	if (! $this->IsValid())
	    return -1;

	$node_id = $this->node_id();

	$query_result =
	    DBQueryWarn("select * from nodes where node_id='$node_id'");
    
	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->node = NULL;
	    return -1;
	}
	$this->node = mysql_fetch_array($query_result);
	return 0;
    }

    #
    # Equality test.
    #
    function SameNode($node) {
	return $node->node_id() == $this->node_id();
    }

    # accessors
    function field($name) {
	return (is_null($this->node) ? -1 : $this->node[$name]);
    }
    function node_id()		{ return $this->field("node_id"); }
    function type() { return $this->field("type"); }
    function phys_nodeid() {return $this->field("phys_nodeid"); }
    function role() {return $this->field("role"); }
    function def_boot_osid() {return $this->field("def_boot_osid"); }
    function def_boot_path() {return $this->field("def_boot_path"); }
    function def_boot_cmd_line() {return $this->field("def_boot_cmd_line"); }
    function temp_boot_osid() {return $this->field("temp_boot_osid"); }
    function next_boot_osid() {return $this->field("next_boot_osid"); }
    function next_boot_path() {return $this->field("next_boot_path"); }
    function next_boot_cmd_line() {return $this->field("next_boot_cmd_line"); }
    function pxe_boot_path() {return $this->field("pxe_boot_path"); }
    function rpms() {return $this->field("rpms"); }
    function deltas() {return $this->field("deltas"); }
    function tarballs() {return $this->field("tarballs"); }
    function startupcmd() {return $this->field("startupcmd"); }
    function startstatus() {return $this->field("startstatus"); }
    function ready() {return $this->field("ready"); }
    function priority() {return $this->field("priority"); }
    function bootstatus() {return $this->field("bootstatus"); }
    function status() {return $this->field("status"); }
    function status_timestamp() {return $this->field("status_timestamp"); }
    function failureaction() {return $this->field("failureaction"); }
    function routertype() {return $this->field("routertype"); }
    function eventstate() {return $this->field("eventstate"); }
    function state_timestamp() {return $this->field("state_timestamp"); }
    function op_mode() {return $this->field("op_mode"); }
    function op_mode_timestamp() {return $this->field("op_mode_timestamp"); }
    function allocstate() {return $this->field("allocstate"); }
    function update_accounts() {return $this->field("update_accounts"); }
    function next_op_mode() {return $this->field("next_op_mode"); }
    function ipodhash() {return $this->field("ipodhash"); }
    function osid() {return $this->field("osid"); }
    function ntpdrift() {return $this->field("ntpdrift"); }
    function ipport_low() {return $this->field("ipport_low"); }
    function ipport_next() {return $this->field("ipport_next"); }
    function ipport_high() {return $this->field("ipport_high"); }
    function sshdport() {return $this->field("sshdport"); }
    function jailflag() {return $this->field("jailflag"); }
    function jailip() {return $this->field("jailip"); }
    function sfshostid() {return $this->field("sfshostid"); }
    function stated_tag() {return $this->field("stated_tag"); }
    function rtabid() {return $this->field("rtabid"); }
    function cd_version() {return $this->field("cd_version"); }
    function boot_errno() {return $this->field("boot_errno"); }
    function reserved_pid() {return $this->field("reserved_pid"); }

    #
    # Access Check, determines if $user can access $this record.
    # 
    function AccessCheck($user, $access_type) {
	global $TB_NODEACCESS_READINFO;
	global $TB_NODEACCESS_MODIFYINFO;
	global $TB_NODEACCESS_LOADIMAGE;
	global $TB_NODEACCESS_REBOOT;
	global $TB_NODEACCESS_POWERCYCLE;
	global $TB_NODEACCESS_MIN;
	global $TB_NODEACCESS_MAX;
	global $TBDB_TRUST_USER;
	global $TBDB_TRUST_GROUPROOT;
	global $TBDB_TRUST_LOCALROOT;
	global $TBOPSPID;
	global $CHECKLOGIN_USER;
	$mintrust = $TBDB_TRUST_USER;

	if ($access_type < $TB_NODEACCESS_MIN ||
	    $access_type > $TB_NODEACCESS_MAX) {
	    TBERROR("Invalid access type: $access_type!", 1);
	}

	$uid = $user->uid();

	if (! ($experiment = $this->Reservation())) {
            #
	    # If the current user is in the emulab-ops project and has 
	    # sufficient privs, then he can muck with free nodes as if he 
	    # were an admin type.
	    #
	    if ($uid == $CHECKLOGIN_USER->uid() && OPSGUY()) {
		return(TBMinTrust(TBGrpTrust($uid, $TBOPSPID, $TBOPSPID),
				  $TBDB_TRUST_LOCALROOT));
	    }
	    return 0;
	}
	$pid = $experiment->pid();
	$gid = $experiment->gid();
	$eid = $experiment->eid();

	if ($access_type == $TB_NODEACCESS_READINFO) {
	    $mintrust = $TBDB_TRUST_USER;
	}
	else {
	    $mintrust = $TBDB_TRUST_LOCALROOT;
	}
	return TBMinTrust(TBGrpTrust($uid, $pid, $gid), $mintrust) ||
	    TBMinTrust(TBGrpTrust($uid, $pid, $pid), $TBDB_TRUST_GROUPROOT);
    }

    #
    # Page header; spit back some html for the typical page header.
    #
    function PageHeader() {
	$node_id = $this->node_id();
	
	$html = "<font size=+2>Node <b>".
	    "<a href=shownode.php3?node_id=$node_id><b>$node_id</a>";
	    "</b></font>\n";

	return $html;
    }

    #
    # Get the bootlog for this node.
    #
    function BootLog(&$log, &$stamp) {
	$log     = null;
	$stamp   = null;
	$node_id = $this->node_id();
	
	$query_result =
	    DBQueryFatal("select * from node_bootlogs ".
			 "where node_id='$node_id'");

	if (mysql_num_rows($query_result) == 0)
	    return -1;

	$row = mysql_fetch_array($query_result);
	$log   = $row["bootlog"];
	$stamp = $row["bootlog_timestamp"];
	return 0;
    }

    function NodeStatus() {
	$node_id = $this->node_id();

	$query_result =
	    DBQueryFatal("select status from nodes where node_id='$nodeid'");

	if (mysql_num_rows($query_result) == 0) {
	    return "";
	}
	$row = mysql_fetch_array($query_result);
	return $row["status"];
    }

    #
    # Get the experiment this node is reserved too, or null.
    #
    function Reservation() {
	$node_id = $this->node_id();
	
	$query_result =
	    DBQueryFatal("select pid,eid from reserved ".
			 "where node_id='$node_id'");

	if (mysql_num_rows($query_result) == 0) {
	    return null;
	}
	$row = mysql_fetch_array($query_result);
	$pid = $row["pid"];
	$eid = $row["eid"];

	return Experiment::LookupByPidEid($pid, $eid);
    }

    #
    # Get the raw reserved table info and return it, or null if no reservation
    #
    function ReservedTableEntry() {
	$node_id = $this->node_id();
	
	$query_result =
	    DBQueryFatal("select * from reserved ".
			 "where node_id='$node_id'");

	if (mysql_num_rows($query_result) == 0) {
	    return null;
	}
	return mysql_fetch_array($query_result);
    }

    #
    # See if this node has a serial console.
    #
    function HasSerialConsole() {
	$node_id = $this->node_id();
	
	$query_result =
	    DBQueryFatal("select tipname from tiplines ".
			 "where node_id='$node_id'");
	
	return mysql_num_rows($query_result);
    }

    #
    # Return the class of the node.
    #
    function TypeClass() {
	$this_type = $this->type();
	
	$query_result =
	    DBQueryFatal("select class from node_types ".
			 "where type='$this_type'");

	if (mysql_num_rows($query_result) == 0) {
	    return "";
	}

	$row = mysql_fetch_array($query_result);
	return $row["class"];
    }

    #
    # Return the virtual name of a reserved node.
    #
    function VirtName() {
	if (! ($row = $this->ReservedTableEntry())) {
	    return "";
	}
	if (! $row["vname"]) {
	    return "";
	}
	return $row["vname"];
    }

    var $lastact_query =
	"greatest(last_tty_act,last_net_act,last_cpu_act,last_ext_act)";

    #
    # Gets the time of idleness for a node, in hours by default (ie '2.75')
    #
    function IdleTime($format = 0) {
	$node_id = $this->node_id();
	$clause  = $this->lastact_query;
	
	$query_result =
	    DBQueryWarn("select (unix_timestamp(now()) - unix_timestamp( ".
			"        $clause)) as idle_time from node_activity ".
			"where node_id='$node_id'");

	if (mysql_num_rows($query_result) == 0) {
	    return -1;
	}
	$row   = mysql_fetch_array($query_result);
	$t = $row["idle_time"];
        # if it is less than 5 minutes, it is not idle at all...
	$t = ($t < 300 ? 0 : $t);
	if (!$format) {
	    $t = round($t/3600,2);
	}
	else {
	    $t = date($format,mktime(0,0,$t));
	}
	return $t;
    }

    #
    # Find out if a node idle reports are stale
    #
    function IdleStale() {
	$node_id = $this->node_id();

	#
        # We currently have a 5 minute interval for slothd between reports
        # So give some slack in case a node reboots without reporting for
	# a while
	#
	# In Minutes;
	$staletime = 10;
	$stalesec  = 60 * $staletime;
	
	$query_result =
	    DBQueryWarn("select (unix_timestamp(now()) - ".
			"    unix_timestamp(last_report )) as t ".
			"from node_activity where node_id='$node_id'");

	if (mysql_num_rows($query_result) == 0) {
	    return -1;
	}
	$row   = mysql_fetch_array($query_result);
	return ($row["t"]>$stalesec);
    }

    #
    # Show node record.
    #
    function Show($flags = 0) {
	$node_id = $this->node_id();
	$short   = ($flags & SHOWNODE_SHORT  ? 1 : 0);
	$noperm  = ($flags & SHOWNODE_NOPERM ? 1 : 0);
    
	$query_result =
	    DBQueryFatal("select n.*,na.*,r.vname,r.pid,r.eid,i.IP, ".
			 "greatest(last_tty_act,last_net_act,last_cpu_act,".
			 "last_ext_act) as last_act, ".
			 "  t.isvirtnode,t.isremotenode,t.isplabdslice, ".
			 "  r.erole as rsrvrole, pi.IP as phys_IP, loc.*, ".
			 "  util.* ".
			 " from nodes as n ".
			 "left join reserved as r on n.node_id=r.node_id ".
			 "left join node_activity as na on ".
			 "     n.node_id=na.node_id ".
			 "left join node_types as t on t.type=n.type ".
			 "left join interfaces as i on ".
			 "     i.node_id=n.node_id and ".
			 "     i.role='" . TBDB_IFACEROLE_CONTROL . "' ".
			 "left join interfaces as pi on ".
			 "     pi.node_id=n.phys_nodeid and ".
			 "     pi.role='" . TBDB_IFACEROLE_CONTROL . "' ".
			 "left join location_info as loc on ".
			 "     loc.node_id=n.node_id ".
			 "left join node_utilization as util on ".
			 "     util.node_id=n.node_id ".
			 "where n.node_id='$node_id'");
	
	if (mysql_num_rows($query_result) == 0) {
	    TBERROR("The node $node_id is not a valid nodeid!", 1);
	}
		
	$row = mysql_fetch_array($query_result);

	$phys_nodeid        = $row["phys_nodeid"]; 
	$type               = $row["type"];
	$vname		    = $row["vname"];
	$pid 		    = $row["pid"];
	$eid		    = $row["eid"];
	$def_boot_osid      = $row["def_boot_osid"];
	$def_boot_cmd_line  = $row["def_boot_cmd_line"];
	$next_boot_osid     = $row["next_boot_osid"];
	$temp_boot_osid     = $row["temp_boot_osid"];
	$next_boot_cmd_line = $row["next_boot_cmd_line"];
	$rpms               = $row["rpms"];
	$tarballs           = $row["tarballs"];
	$startupcmd         = $row["startupcmd"];
	$routertype         = $row["routertype"];
	$eventstate         = $row["eventstate"];
	$state_timestamp    = $row["state_timestamp"];
	$allocstate         = $row["allocstate"];
	$allocstate_timestamp= $row["allocstate_timestamp"];
	$op_mode            = $row["op_mode"];
	$op_mode_timestamp  = $row["op_mode_timestamp"];
	$IP                 = $row["IP"];
	$isvirtnode         = $row["isvirtnode"];
	$isremotenode       = $row["isremotenode"];
	$isplabdslice       = $row["isplabdslice"];
	$ipport_low	    = $row["ipport_low"];
	$ipport_next	    = $row["ipport_next"];
	$ipport_high	    = $row["ipport_high"];
	$sshdport	    = $row["sshdport"];
	$last_act           = $row["last_act"];
	$last_tty_act       = $row["last_tty_act"];
	$last_net_act       = $row["last_net_act"];
	$last_cpu_act       = $row["last_cpu_act"];
	$last_ext_act       = $row["last_ext_act"];
	$last_report        = $row["last_report"];
	$rsrvrole           = $row["rsrvrole"];
	$phys_IP	    = $row["phys_IP"];
	$battery_voltage    = $row["battery_voltage"];
	$battery_percentage = $row["battery_percentage"];
	$battery_timestamp  = $row["battery_timestamp"];
	$boot_errno         = $row["boot_errno"];
	$reserved_pid       = $row["reserved_pid"];
	$inception          = $row["inception"];
	$alloctime          = $row["allocated"];
	$downtime           = $row["down"];

	if (!$def_boot_cmd_line)
	    $def_boot_cmd_line = "&nbsp;";
	if (!$next_boot_cmd_line)
	    $next_boot_cmd_line = "&nbsp;";
	if (!$rpms)
	    $rpms = "&nbsp;";
	if (!$tarballs)
	    $tarballs = "&nbsp;";
	if (!$startupcmd)
	    $startupcmd = "&nbsp;";

	if ($node_id != $phys_nodeid) {
	    if (! ($phys_this = Node::Lookup($phys_nodeid))) {
		TBERROR("Cannot map physical node $phys_nodeid to object", 1);
	    }
	}

	if (!$short) {
            #
            # Location info.
            # 
	    if (isset($row["loc_x"]) && isset($row["loc_y"]) &&
		isset($row["floor"]) && isset($row["building"])) {
		$floor    = $row["floor"];
		$building = $row["building"];
		$loc_x    = $row["loc_x"];
		$loc_y    = $row["loc_y"];
		$orient   = $row["orientation"];
	
		$query_result =
		    DBQueryFatal("select * from floorimages ".
				 "where scale=1 and ".
				 "      floor='$floor' and ".
				 "      building='$building'");

		if (mysql_num_rows($query_result)) {
		    $row = mysql_fetch_array($query_result);
	    
		    if (isset($row["pixels_per_meter"]) &&
			($pixels_per_meter = $row["pixels_per_meter"]) != 0.0){
		
			$meters_x = sprintf("%.3f",
					    $loc_x / $pixels_per_meter);
			$meters_y = sprintf("%.3f",
					    $loc_y / $pixels_per_meter);

			if (isset($orient)) {
			    $orientation = sprintf("%.3f", $orient);
			}
		    }
		}
	    }
	}

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
	   	          <a href='shownode.php3?node_id=$phys_nodeid'>
                             $phys_nodeid</a></td>
                  </tr>\n";
	    }
	}

	if (!$short && !$noperm) {
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
                      <td><a href='showexp.php3?pid=$pid&eid=$eid'>
                             $eid</a></td>
                  </tr>\n";
	    }
	}

	echo "<tr>
              <td>Node Type:</td>
              <td class=left>
  	          <a href='shownodetype.php3?node_type=$type'>$type</td>
          </tr>\n";

	$feat_result =
	    DBQueryFatal("select * from node_features ".
			 "where node_id='$node_id'");

	if (mysql_num_rows($feat_result) > 0) {
	    $features = "";
	    $count = 0;
	    while ($row = mysql_fetch_array($feat_result)) {
		if (($count > 0) && ($count % 2) == 0) {
		    $features .= "<br>";
		}
		$features .= " " . $row["feature"];
		$count += 1;
	    }

	    echo "<tr><td>Features:</td><td class=left>$features</td></tr>";
	}

	if (!$short && !$noperm) {
	    echo "<tr>
                  <td>Def Boot OS:</td>
                  <td class=left>";
	    SpitOSIDLink($def_boot_osid);
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
	}
	if (!$short) {
            #
            # Location info.
            # 
	    if (isset($meters_x) && isset($meters_y)) {
		echo "<tr>
                      <td>Location:</td>
                      <td class=left>x=$meters_x, y=$meters_y meters";
		if (isset($orientation)) {
		    echo " (o=$orientation degrees)";
		}
		echo      "</td>
                  </tr>\n";
	    }
	}
	
	if (!$short && !$noperm) {
             #
             # We want the last login for this node, but only if its *after* 
             # the experiment was created (or swapped in).
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

		$idletime = $this->IdleTime();
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
	}

	if (!$short && !$noperm) {
	    if (!$isvirtnode && !$isremotenode) {
		echo "<tr>
                      <td>Def Boot Command&nbsp;Line:</td>
                      <td class=left>$def_boot_cmd_line</td>
                  </tr>\n";

		echo "<tr>
                      <td>Next Boot OS:</td>
                      <td class=left>";
    
		if ($next_boot_osid)
		    SpitOSIDLink($next_boot_osid);
		else
		    echo "&nbsp;";

		echo "    </td>
                  </tr>\n";

		echo "<tr>
                      <td>Next Boot Command Line:</td>
                      <td class=left>$next_boot_cmd_line</td>
                  </tr>\n";

		echo "<tr>
                      <td>Temp Boot OS:</td>
                      <td class=left>";
    
		if ($temp_boot_osid)
		    SpitOSIDLink($temp_boot_osid);
		else
		    echo "&nbsp;";

		echo "    </td>
                  </tr>\n";
	    }
	    elseif ($isvirtnode) {
		if (!$isplabdslice) {
		    echo "<tr>
                          <td>IP Port Low:</td>
                          <td class=left>$ipport_low</td>
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
                      <td>SSHD Port:</td>
                     <td class=left>$sshdport</td>
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

	    echo "<tr>
                  <td>RPMs:</td>
                  <td class=left>$rpms</td>
              </tr>\n";

	    echo "<tr>
                  <td>Boot Errno:</td>
                  <td class=left>$boot_errno</td>
              </tr>\n";

	    if (!$isvirtnode && !$isremotenode) {
		echo "<tr>
                      <td>Router Type:</td>
                      <td class=left>$routertype</td>
                  </tr>\n";
	    }

	    if ($IP) {
		echo "<tr>
                      <td>Control Net IP:</td>
                      <td class=left>$IP</td>
                  </tr>\n";
	    }
	    elseif ($phys_IP) {
		echo "<tr>
                      <td>Physical IP:</td>
                      <td class=left>$phys_IP</td>
                  </tr>\n";
	    }

	    if ($rsrvrole) {
		echo "<tr>
                      <td>Role:</td>
                      <td class=left>$rsrvrole</td>
                  </tr>\n";
	    }
	
	    if ($reserved_pid) {
		echo "<tr>
                      <td>Reserved Pid:</td>
                      <td class=left>
                          <a href='showproject.php3?pid=$reserved_pid'>
                               $reserved_pid</a></td>
                  </tr>\n";
	    }
	
            #
            # Show battery stuff
            #
	    if (isset($battery_voltage) && isset($battery_percentage)) {
		echo "<tr>
    	              <td>Battery Volts/Percent</td>
		      <td class=left>";
		printf("%.2f/%.2f ", $battery_voltage, $battery_percentage);

		if (isset($battery_timestamp)) {
		    echo "(" . date("m/d/y H:i:s", $battery_timestamp) . ")";
		}

		echo "    </td>
		  </tr>\n";
	    }

	    if ($isplabdslice) {
		$query_result = 
		    DBQueryFatal("select leaseend from plab_slice_nodes ".
				 "where node_id='$node_id'");
          
		if (mysql_num_rows($query_result) != 0) {
		    $row = mysql_fetch_array($query_result);
		    $leaseend = $row["leaseend"];
		    echo"<tr>
                     <td>Lease Expiration:</td>
                     <td class=left>$leaseend</td>
                 </tr>\n";
		}
	    }

	    if ($isremotenode) {
		if ($isvirtnode) {
		    $phys_this->ShowWideAreaNode(1);
		}
		else {
		    $this->ShowWideAreaNode(1);
		}
	    }

            #
            # Show any auxtypes the node has
            #
	    $query_result =
		DBQueryFatal("select type, count from node_auxtypes ".
			     "where node_id='$node_id'");
    
	    if (mysql_num_rows($query_result) != 0) {
		echo "<tr>
                      <td align=center colspan=2>
                      Auxiliary Types
                      </td>
                  </tr>\n";

		echo "<tr><th>Type</th><th>Count</th>\n";

		while ($row = mysql_fetch_array($query_result)) {
		    $type  = $row["type"];
		    $count = $row["count"];
		    echo "<tr>
    	    	          <td>$type</td>
		          <td class=left>$count</td>
		      </td>\n";
		}
	    }
	}
    
	if (!$short) {
            #
            # Get interface info.
            #
	    echo "<tr>
                  <td align=center colspan=2>Interface Info</td>
              </tr>\n";
	    echo "<tr><th>Interface</th><th>Model; protocols</th>\n";
    
	    $query_result =
		DBQueryFatal("select i.*,it.*,c.*,s.capval as channel ".
			     "  from interfaces as i ".
			     "left join interface_types as it on ".
			     "     i.interface_type=it.type ".
			     "left join interface_capabilities as c on ".
			     "     i.interface_type=c.type and ".
			     "     c.capkey='protocols' ".
			     "left join interface_settings as s on ".
			     "     s.node_id=i.node_id and s.iface=i.iface and ".
			     "     s.capkey='channel' ".
			     "where i.node_id='$node_id' and ".
			     "      i.role='" . TBDB_IFACEROLE_EXPERIMENT . "'".
			     "order by iface");
    
	    while ($row = mysql_fetch_array($query_result)) {
		$iface     = $row["iface"];
		$type      = $row["type"];
		$man       = $row["manufacturuer"];
		$model     = $row["model"];
		$protocols = $row["capval"];
		$channel   = $row["channel"];

		if (isset($channel)) {
		    $channel = " (channel $channel)";
		}
		else
		    $channel = "";

		echo "<tr>
                      <td>$iface:&nbsp; $channel</td>
                      <td class=left>$type ($man $model; $protocols)</td>
                  </tr>\n";
	    }

            #
            # Switch info. Very useful for debugging.
            #
	    if (!$noperm) {
		$query_result =
		    DBQueryFatal("select i.*,w.* from interfaces as i ".
				 "left join wires as w on i.node_id=w.node_id1 ".
				 "   and i.card=w.card1 and i.port=w.port1 ".
				 "where i.node_id='$node_id' and ".
				 "      w.node_id1 is not null ".
				 "order by iface");

		echo "<tr></tr><tr>
                    <td align=center colspan=2>Switch Info</td>
                  </tr>\n";
		echo "<tr><th>Iface:role &nbsp; card,port</th>
                      <th>Switch &nbsp; card,port</th>\n";
		
		while ($row = mysql_fetch_array($query_result)) {
		    $iface       = $row["iface"];
		    $role        = $row["role"];
		    $card        = $row["card1"];
		    $port        = $row["port1"];
		    $switch      = $row["node_id2"];
		    $switch_card = $row["card2"];
		    $switch_port = $row["port2"];

		    echo "<tr>
                      <td>$iface:$role &nbsp; $card,$port</td>
                      <td class=left>".
			"$switch: &nbsp; $switch_card,$switch_port</td>
                  </tr>\n";
		}
	    }
	}

        #
        # Spit out node attributes
        #
	$query_result =
	    DBQueryFatal("select attrkey,attrvalue from node_attributes ".
			 "where node_id='$node_id'");
	if (!$short && mysql_num_rows($query_result)) {
	    echo "<tr>
                    <td align=center colspan=2>Node Attributes</td>
                  </tr>\n";
	    echo "<tr><th>Attribute</th><th>Value</th>\n";

	    while($row = mysql_fetch_array($query_result)) {
		$attrkey   = $row["attrkey"];
		$attrvalue = $row["attrvalue"];

		echo "<tr>
                        <td>$attrkey</td>
                        <td>$attrvalue</td>
                      </td>\n";
	    }
	}

	echo "</table>\n";
    }

    #
    # Show widearea node record. Just the widearea stuff, not the other.
    #
    function ShowWideAreaNode($embedded = 0) {
	$node_id = $this->node_id();
	
	$query_result =
	    DBQueryFatal("select * from widearea_nodeinfo ".
			 "where node_id='$node_id'");

	if (! mysql_num_rows($query_result)) {
	    return;
	}
	$row = mysql_fetch_array($query_result);
	$contact_uid	= $row["contact_uid"];
	$machine_type   = $row["machine_type"];
	$connect_type	= $row["connect_type"];
	$city		= $row["city"];
	$state		= $row["state"];
	$zip		= $row["zip"];
	$country	= $row["country"];
	$hostname	= $row["hostname"];
	$site		= $row["site"];

	if (! ($user = User::Lookup($contact_uid))) {
            # This is not an error since the field is set to "nobody" when
            # there is no contact info. Why is that?
	    $showuser_url = CreateURL("showuser", URLARG_UID, $contact_uid);
	}
	else {
	    $showuser_url = CreateURL("showuser", $user);
	}

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
                      <a href='$showuser_url'>$contact_uid</a></td>
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
                  <td>ZIP:</td>
                  <td class=left>$zip</td>
              </tr>\n";

	echo "<tr>
                  <td>Country:</td>
                  <td class=left>$country</td>
              </tr>\n";

        echo "<tr>
                  <td>Hostname:</td>
                  <td class=left>$hostname</td>
              </tr>\n";

	echo "<tr>
                  <td>Site:</td>
                  <td class=left>$site</td>
              </tr>\n";

	if (! $embedded) {
	    echo "</table>\n";
	}
    }

    #
    # Show log.
    # 
    function ShowLog() {
	$node_id = $this->node_id();

	$query_result =
	    DBQueryFatal("select * from nodelog where node_id='$node_id'".
			 "order by reported");

	if (! mysql_num_rows($query_result)) {
	    echo "<br>
                      <center>
                       There are no entries in the log for node $node_id.
                      </center>\n";
	    return 0;
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
	    $type       = $row["type"];
	    $log_id     = $row["log_id"];
	    $reporter   = $row["reporting_uid"];
	    $date       = $row["reported"];
	    $entry      = $row["entry"];
	    $url        = CreateURL("deletenodelog", $this, "log_id", $log_id);

	    echo "<tr>
 	             <td align=center>
                      <a href='$url'>
                         <img alt='Delete Log Entry' src='redball.gif'>
                      </a></td>
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
    function ShowLogEntry($log_id) {
	$node_id = $this->node_id();
	$safe_id = addslashes($log_id);
	
	$query_result =
	    DBQueryFatal("select * from nodelog where ".
			 "node_id='$node_id' and log_id=$safe_id");

	if (! mysql_num_rows($query_result)) {
	    return 0;
	}

	echo "<table border=1 cellpadding=2 cellspacing=2 align='center'>\n";

	$row = mysql_fetch_array($query_result);
        $type       = $row["type"];
	$log_id     = $row["log_id"];
	$reporter   = $row["reporting_uid"];
	$date       = $row["reported"];
	$entry      = $row["entry"];

	echo "<tr>
                 <td>$date</td>
                 <td>$log_id</td>
                 <td>$type</td>
                 <td>$reporter</td>
                 <td>$entry</td>
               </tr>\n";
	echo "</table>\n";

	return 0;
    }

    #
    # Delete a node log entry.
    #
    function DeleteNodeLog($log_id) {
	$node_id = $this->node_id();
	$safe_id = addslashes($log_id);
	
	DBQueryFatal("delete from nodelog where ".
		     "node_id='$node_id' and log_id=$safe_id");

	return 0;
    }
}

#
# Show history.
#
function ShowNodeHistory($node = null,
			 $showall = 0, $count = 20, $reverse = 1) {
    global $TBSUEXEC_PATH;
    $atime = 0;
    $ftime = 0;
    $rtime = 0;
    $dtime = 0;
    $nodestr = "";
	
    $opt = "-ls";
    if (!$showall) {
	$opt .= "a";
    }
    if ($node) {
	$node_id = $node->node_id();
    }
    else {
	$node_id = "";
	$opt .= "A";
	$nodestr = "<th>Node</th>";
    }
    if ($reverse) {
	$opt .= "r";
    }
    if ($count) {
	    $opt .= " -n $count";
    }
    if ($fp = popen("$TBSUEXEC_PATH nobody nobody ".
		    "  webnode_history $opt $node_id", "r")) {
	if (!$showall) {
	    $str = "Allocation";
	} else {
	    $str = "";
	}
	if ($node_id == "") {
	    echo "<br>
                  <center>
                  $str History for All Nodes.
                  </center><br>\n";
	} else {
	    echo "<br>
                  <center>
                  $str History for Node $node_id.
                  </center><br>\n";
	}

	echo "<table border=1 cellpadding=2 cellspacing=2 align='center'>\n";

	echo "<tr>
	       $nodestr
               <th>Pid</th>
               <th>Eid</th>
               <th>Allocated By</th>
               <th>Allocation Date</th>
	       <th>Duration</th>
              </tr>\n";

	$line = fgets($fp);
	while (!feof($fp)) {
	    #
	    # Formats:
	    # nodeid REC tstamp duration uid pid eid
	    # nodeid SUM alloctime freetime reloadtime downtime
	    #
	    $results = preg_split("/[\s]+/", $line, 8, PREG_SPLIT_NO_EMPTY);
	    $nodeid = $results[0];
	    $type = $results[1];
	    if ($type == "SUM") {
		# Save summary info for later
		$atime = $results[2];
		$ftime = $results[3];
		$rtime = $results[4];
		$dtime = $results[5];
	    } elseif ($type == "REC") {
		$stamp = $results[2];
		$datestr = date("Y-m-d H:i:s", $stamp);
		$duration = $results[3];
		$durstr = "";
		if ($duration >= (24*60*60)) {
		    $durstr = sprintf("%dd", $duration / (24*60*60));
		    $duration %= (24*60*60);
		}
		if ($duration >= (60*60)) {
		    $durstr = sprintf("%s%dh", $durstr, $duration / (60*60));
		    $duration %= (60*60);
		}
		if ($duration >= 60) {
		    $durstr = sprintf("%s%dm", $durstr, $duration / 60);
		    $duration %= 60;
		}
		$durstr = sprintf("%s%ds", $durstr, $duration);
		$uid = $results[4];
		$pid = $results[5];
		if ($pid == "FREE") {
		    $pid = "--";
		    $eid = "--";
		    $uid = "--";
		} else {
		    $eid = $results[6];
		}
		if ($node_id == "") {
		    echo "<tr>
                          <td>$nodeid</td>
                          <td>$pid</td>
                          <td>$eid</td>
                          <td>$uid</td>
                          <td>$datestr</td>
                          <td>$durstr</td>
                          </tr>\n";
		} else {
		    echo "<tr>
                          <td>$pid</td>
                          <td>$eid</td>
                          <td>$uid</td>
                          <td>$datestr</td>
                          <td>$durstr</td>
                          </tr>\n";
		}
	    }
	    $line = fgets($fp, 1024);
	}
	pclose($fp);

	echo "</table>\n";

	$ttime = $atime + $ftime + $rtime + $dtime;
	if ($ttime) {
	    echo "<br>
                  <center>
                  Usage Summary for Node $node_id.
                  </center><br>\n";

	    echo "<table border=1 align=center>\n";

	    $str = "Allocated";
	    $pct = sprintf("%5.1f", $atime * 100.0 / $ttime);
	    echo "<tr><td>$str</td><td>$pct%</td></tr>\n";

	    $str = "Free";
	    $pct = sprintf("%5.1f", $ftime * 100.0 / $ttime);
	    echo "<tr><td>$str</td><td>$pct%</td></tr>\n";

	    $str = "Reloading";
	    $pct = sprintf("%5.1f", $rtime * 100.0 / $ttime);
	    echo "<tr><td>$str</td><td>$pct%</td></tr>\n";

	    $str = "Down";
	    $pct = sprintf("%5.1f", $dtime * 100.0 / $ttime);
	    echo "<tr><td>$str</td><td>$pct%</td></tr>\n";

	    echo "</table>\n";
	}
    }
}

#
# Logged in users, show free node counts.
#
function ShowFreeNodes()
{
    $freecounts = array();
    
    # Get typelist and set freecounts to zero.
    $query_result =
	DBQueryFatal("select n.type from nodes as n ".
		     "left join node_types as nt on n.type=nt.type ".
		     "where (role='testnode') and class='pc' ");
    while ($row = mysql_fetch_array($query_result)) {
	$type              = $row[0];
	$freecounts[$type] = 0;
    }

    if (!count($freecounts)) {
	return "";
    }
	
    # Get free totals by type.
    $query_result =
	DBQueryFatal("select n.eventstate,n.type,count(*) from nodes as n ".
		     "left join node_types as nt on n.type=nt.type ".
		     "left join reserved as r on r.node_id=n.node_id ".
		     "where (role='testnode') and class='pc' ".
		     "      and r.pid is null and n.reserved_pid is null ".
		     "group BY n.eventstate,n.type");

    while ($row = mysql_fetch_array($query_result)) {
	$type  = $row[1];
	$count = $row[2];
        # XXX Yeah, I'm a doofus and can't figure out how to do this in SQL.
	if (($row[0] == TBDB_NODESTATE_ISUP) ||
	    ($row[0] == TBDB_NODESTATE_PXEWAIT) ||
	    ($row[0] == TBDB_NODESTATE_ALWAYSUP) ||
	    ($row[0] == TBDB_NODESTATE_POWEROFF)) {
	    $freecounts[$type] = $count;
	}
    }
    $output = "";

    $freepcs   = TBFreePCs();
    $reloading = TBReloadingPCs();

    $output .= "<table valign=top align=center width=100% height=100% border=1
		 cellspacing=1 cellpadding=0>
                 <tr><td nowrap colspan=6 class=usagefreenodes align=center>
 	           <b>$freepcs Free PCs, $reloading reloading</b></td></tr>\n";

    $pccount = count($freecounts);
    $newrow  = 1;
    $maxcols = (int) ($pccount / 3);
    if ($pccount % 3)
	$maxcols++;
    $cols    = 0;
    foreach($freecounts as $key => $value) {
	$freecount = $freecounts[$key];

	if ($newrow) {
	    $output .= "<tr>\n";
	}
	
	$output .= "<td class=usagefreenodes align=right>
                     <a target=_parent href=shownodetype.php3?node_type=$key>
                        $key</a></td>
                    <td class=usagefreenodes align=left>${freecount}</td>\n";

	$cols++;
	$newrow = 0;
	if ($cols == $maxcols || $pccount <= 3) {
	    $cols   = 0;
	    $newrow = 1;
	}

	if ($newrow) {
	    $output .= "</tr>\n";
	}
    }
    if (! $newrow) {
        # Fill out to $maxcols
	for ($i = $cols + 1; $i <= $maxcols; $i++) {
	    $output .= "<td class=usagefreenodes>&nbsp</td>";
	    $output .= "<td class=usagefreenodes>&nbsp</td>";
	}
	$output .= "</tr>\n";
    }
    # Fill in up to 3 rows.
    if ($pccount < 3) {
	for ($i = $pccount + 1; $i <= 3; $i++) {
	    $output .= "<tr><td class=usagefreenodes>&nbsp</td>
                            <td class=usagefreenodes>&nbsp</td></tr>\n";
	}
    }

    $output .= "</table>";
    return $output;
}


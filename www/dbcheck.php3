<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
# Stuff to support checking field data before we insert it into the DB.
#
define("TBDB_CHECKDBSLOT_NOFLAGS",	0x0);
define("TBDB_CHECKDBSLOT_WARN",		0x1);
define("TBDB_CHECKDBSLOT_ERROR",	0x2);

$DBFieldData   = 0;
$DBFieldErrstr = "";
function TBFieldErrorString() { global $DBFieldErrstr; return $DBFieldErrstr; }

#
# Download all data from the DB and store in hash for latter access.
# 
function TBGrabFieldData()
{
    global $DBFieldData;

    $DBFieldData = array();
    
    $query_result =
	DBQueryFatal("select * from table_regex");

    while ($row = mysql_fetch_assoc($query_result)) {
	$table_name  = $row["table_name"];
	$column_name = $row["column_name"];

	$DBFieldData[$table_name . ":" . $column_name] =
	    array("check"       => $row["check"],
		  "check_type"  => $row["check_type"],
		  "column_type" => $row["column_type"],
		  "min"         => $row["min"],
		  "max"         => $row["max"]);
    }
}

#
# Return the field data for a specific table/slot. If none, return the default
# entry.
#
function TBFieldData($table, $column, $flag = 0)
{
    global $DBFieldData;
    
    if (! $DBFieldData) {
	TBGrabFieldData();
    }
    $key = $table . ":" . $column;
    unset($toplevel);
    unset($fielddate);

    while (isset($DBFieldData[$key])) {
	$fielddata = $DBFieldData[$key];

	#
	# See if a redirect to another entry. 
	#
	if ($fielddata["check_type"] == "redirect") {
	    if (!isset($toplevel))
		$toplevel = $fielddata;
	    
	    $key = $fielddata["check"];
	    continue;
	}
	break;
    }
    if (!isset($fielddata)) {
	if ($flag) {
	    if ($flag & TBDB_CHECKDBSLOT_WARN) {
                # Warn TBOPS
		TBERROR("TBFieldData: No slot data for $table/$column!", 0);
	    }
	    if ($flag & TBDB_CHECKDBSLOT_ERROR)
		return 0;
	}
	$fielddata = $DBFieldData["default:default"];
    }
    # Return both values.
    if (isset($toplevel) &&
	($toplevel["min"] || $toplevel["max"])) {
	return array($fielddata, $toplevel);
    }
    return array($fielddata, NULL);
}

#
# Generic wrapper to check a slot. It is unfortunate that PHP
# does not allow pass by reference args to be optional. 
#
function TBcheck_dbslot($token, $table, $column, $flag = 0)
{
    global $DBFieldErrstr;

    list ($fielddata, $toplevel) = TBFieldData($table, $column, $flag);

    if (! $fielddata) {
	return 0;
    }
	
    $check       = $fielddata["check"];
    $check_type  = $fielddata["check_type"];
    $column_type = $fielddata["column_type"];
    $min         = (empty($toplevel) ? $fielddata["min"] : $toplevel["min"]);
    $max         = (empty($toplevel) ? $fielddata["max"] : $toplevel["max"]);
    $min = intval($min);
    $max = intval($max);

    #
    # Functional checks not implemented yet. 
    #
    if ($check_type == "function") {
	TBERROR("Functional DB checks not implemented! ".
		"$token, $table, $column", 1);
    }

    # Make sure the regex is anchored. Its a mistake not to be!
    if (substr($check, 0, 1) != "^")
	$check = "^" . $check;

    if (substr($check, -1, 1) != "\$")
	$check = $check . "\$";
    
    if (!preg_match("/$check/", "$token")) {
	$DBFieldErrstr = "Illegal characters";
	return 0;
    }

    switch ($column_type) {
        case "text":
	    if ((!$min && !max) ||
		(strlen("$token") >= $min && strlen("$token") <= $max))
		return 1;
	    break;
	    
        case "int":
        case "float":
	    # If both min/max are zero, then skip check; allow anything. 
	    if ((!$min && !max) || ($token >= $min && $token <= $max))
		return 1;
	    break;
	    
        default:
	    TBERROR("TBcheck_dbslot: Unrecognized column_type $column_type", 1);
    }

    #
    # Else fill in error string.
    # 
    switch ($column_type) {
        case "text":
	    if (strlen($token) < $min)
		$DBFieldErrstr = "too short - $min chars minimum";
	    else 
		$DBFieldErrstr = "too long - $max chars maximum";
	    break;
	    
        case "int":
        case "float":
	    if ($token < $min)
		$DBFieldErrstr = "$token too small - $min minimum value";
	    else 
		$DBFieldErrstr = "too large - $max maximum value";
	    break;
	    
        default:
	    TBERROR("TBcheck_dbslot: Unrecognized column_type $column_type", 1);
    }
    return 0;
}

# Handy default wrapper.
function TBvalid_slot($token, $table, $slot) {
    return TBcheck_dbslot($token, $table, $slot,
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}

# Handy wrappers for checking various fields.
function TBvalid_uid($token) {
    return TBcheck_dbslot($token, "users", "uid",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_uididx($token) {
    return TBvalid_integer($token);
}
#
# Used to allow _ (underscore), but no more.
# 
function TBvalid_pid($token) {
    return TBcheck_dbslot($token, "projects", "pid",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
#
# So, *new* projects disallow it, but old projects need the above test.
#
function TBvalid_newpid($token) {
    return TBcheck_dbslot($token, "projects", "newpid",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_gid($token) {
    return TBcheck_dbslot($token, "groups", "gid",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_eid($token) {
    return TBcheck_dbslot($token, "experiments", "eid",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_phone($token) {
    return TBcheck_dbslot($token, "users", "usr_phone",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_usrname($token) {
    return TBcheck_dbslot($token, "users", "usr_name",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_wikiname($token) {
    return TBcheck_dbslot($token, "users", "wikiname",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_email($token) {
    return TBcheck_dbslot($token, "users", "usr_email",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_userdata($token) {
    return TBcheck_dbslot($token, "default", "tinytext",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_title($token) {
    return TBvalid_userdata($token);
}
function TBvalid_affiliation($token) {
    return TBvalid_userdata($token);
}
function TBvalid_addr($token) {
    return TBvalid_userdata($token);
}
function TBvalid_city($token) {
    return TBvalid_userdata($token);
}
function TBvalid_state($token) {
    return TBvalid_userdata($token);
}
function TBvalid_zip($token) {
    return TBvalid_userdata($token);
}
function TBvalid_country($token) {
    return TBvalid_userdata($token);
}
function TBvalid_description($token) {
    return TBvalid_userdata($token);
}
function TBvalid_why($token) {
    return TBcheck_dbslot($token, "projects", "why",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_integer($token) {
    return TBcheck_dbslot($token, "default", "int",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_tinyint($token) {
    return TBcheck_dbslot($token, "default", "tinyint",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_boolean($token) {
    return TBcheck_dbslot($token, "default", "boolean",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_float($token) {
    return TBcheck_dbslot($token, "default", "float",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_num_members($token) {
    return TBcheck_dbslot($token, "projects", "num_members",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_num_pcs($token) {
    return TBcheck_dbslot($token, "projects", "num_pcs",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_num_pcplab($token) {
    return TBcheck_dbslot($token, "projects", "num_pcplab",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_num_ron($token) {
    return TBcheck_dbslot($token, "projects", "num_ron",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_osid($token) {
    return TBcheck_dbslot($token, "os_info", "osid",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_node_id($token) {
    return TBcheck_dbslot($token, "nodes", "node_id",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_vnode_id($token) {
    return TBcheck_dbslot($token, "virt_nodes", "vname",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_imageid($token) {
    return TBcheck_dbslot($token, "images", "imageid",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_imagename($token) {
    return TBcheck_dbslot($token, "images", "imagename",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_linklanname($token) {
    return TBcheck_dbslot($token, "virt_lans", "vname",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_mailman_listname($token) {
    return TBcheck_dbslot($token, "mailman_listnames", "listname",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_fulltext($token) {
    return TBcheck_dbslot($token, "default", "fulltext",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_archive_tag($token) {
    return TBcheck_dbslot($token, "archive_tags", "tag",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_archive_message($token) {
    return TBcheck_dbslot($token, "archive_tags", "description",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}

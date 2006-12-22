<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

#
# This is my attempt at factoring our the zillions of URL addresses and
# their arguments. This will hopefully make it easier to change how we
# address things, like when I change the uid,pid,gid arguments to numeric
# IDs instead of names.
#
define("URLARG_UID",	"__uid");
define("URLARG_PID",	"__pid");
define("URLARG_GID",	"__gid");
define("URLARG_EID",	"__eid");

#
# Map page "ids" to the actual script names.
#
$url_mapping				= array();
$url_mapping["showuser"]		= "showuser.php3";
$url_mapping["deleteuser"]		= "deleteuser.php3";
$url_mapping["showpubkeys"]		= "showpubkeys.php3";
$url_mapping["deletepubkey"]		= "deletepubkey.php3";
$url_mapping["showsfskeys"]		= "showsfskeys.php3";
$url_mapping["deletesfskey"]		= "deletesfskey.php3";
$url_mapping["moduserinfo"]   		= "moduserinfo.php3";
$url_mapping["showproject"]		= "showproject.php3";
$url_mapping["showgroup"]		= "showgroup.php3";
$url_mapping["suuser"]			= "suuser.php";
$url_mapping["approveproject_form"]	= "approveproject_form.php3";
$url_mapping["showpubkeys"]		= "showpubkeys.php3";
$url_mapping["toggle"]			= "toggle.php";
$url_mapping["logout"]			= "logout.php3";
$url_mapping["listrepos"]		= "listrepos.php3";
$url_mapping["mychat"]			= "mychat.php3";
$url_mapping["showmmlists"]		= "showmmlists.php3";
$url_mapping["getsslcert"]		= "getsslcert.php3";
$url_mapping["gensslcert"]		= "gensslcert.php3";
$url_mapping["showstats"]		= "showstats.php3";
$url_mapping["freezeuser"]		= "freezeuser.php3";
$url_mapping["changeuid"]		= "changeuid.php";
$url_mapping["resendkey"]		= "resendkey.php3";
$url_mapping["sendtestmsg"]		= "sendtestmsg.php3";
$url_mapping["chpasswd"]		= "chpasswd.php3";
	     
#
# The caller will pass in a page id, and a list of things. If the thing
# is a class we know about, then we generate the argument directly from
# it. For example, if the argument is a User instance, we know to append
# "user=$user->uid()" cause all pages that take uid arguments use
# the same protocol.
#
# If the thing is not something we know about, then its a key/value pair,
# with the next argument being the value.
# The key (say, "uid") is the kind of argument to be added to the URL,
# and the value (a string or an user class instance) is where we get
# the argument from. To be consistent across all pages, something like "uid"
# is mapped to "?user=$value" rather then "uid".
#
# Note that this idea comes from a google search for ways to deal with
# this problem. Nothing fancy.
#
function CreateURL($page_id)
{
    global $url_mapping;
    
    $pageargs = func_get_args();
    # drop the required argument.
    array_shift($pageargs);

    if (! array_key_exists($page_id, $url_mapping)) {
	TBERROR("CreateURL: Could not map $page_id to a page", 1);
	return null;
    }

    $newurl = $url_mapping[$page_id];

    # No args?
    if (! count($pageargs)) {
	return $newurl;
    }

    # Form an array of arguments to append.
    $url_args = array();

    while ($pageargs and count($pageargs)) {
	$key = array_shift($pageargs);

	#
	# Do we know about the object itself.
	#
	if (gettype($key) == "object") {
	    #
	    # Make up the key/value pair for below.
	    #
	    $val = $key;
	    
	    switch (get_class($key)) {
	    case "user":
		$key = URLARG_UID;
		break;
	    case "project":
		$key = URLARG_PID;
		break;
	    case "group":
		$key = URLARG_GID;
		break;
	    case "experiment":
		$key = URLARG_EID;
		break;
	    default:
		TBERROR("CreateURL: ".
			"Unknown object class - " . get_class($key), 1);
	    }
	}
	elseif (count($pageargs)) {
	    #
	    # A key/value pair so get the value from next argument.
	    #
	    $val = array_shift($pageargs);
	}
	else {
	    TBERROR("CreateURL: Malformed arguments for $key", 1);
	}

	#
	# Now process the key/value.
	#
	switch ($key) {
	case URLARG_UID:
	    $key = "user";
	    if (is_a($val, 'User')) {
		$val = $val->webid();
	    }
	    break;
	case URLARG_PID:
	    $key = "pid";
	    if (is_a($val, 'Project')) {
		$val = $val->pid();
	    }
	    break;
	case URLARG_GID:
	    $key = "gid";
	    if (is_a($val, 'Group')) {
		$val = $val->gid();
	    }
	    break;
	case URLARG_EID:
	    $key = "eid";
	    if (is_a($val, 'Experiment')) {
		$val = $val->eid();
	    }
	    break;
	default:
            # Use whatever it was we got.
	    ;
	}
	$url_args[] = "${key}=${val}";
    }
    $newurl .= "?" . implode("&", $url_args);
    return $newurl;
}

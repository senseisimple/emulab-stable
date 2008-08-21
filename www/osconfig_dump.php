<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2008 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");
require("node_defs.php");

$reqargs = RequiredPageArguments("ip", PAGEARG_STRING);
$optargs = OptionalPageArguments("privkey", PAGEARG_STRING,
				 "check", PAGEARG_BOOLEAN,
				 "os", PAGEARG_STRING,
				 "os_version", PAGEARG_STRING,
				 "distro", PAGEARG_STRING,
				 "distro_version", PAGEARG_STRING,
				 "arch", PAGEARG_STRING,
				 "env", PAGEARG_STRING);

#
# The point of this page is to figure out what osconfig scripts
# the node ought to execute, and return them in a tarball.  We attempt to 
# cache the tarballs as much as possible and only rebuild when necessary.
#
# NOTE!  This page is only accessible over SSL, but it is only "secure" for 
# widearea nodes (which provide a privkey).  Local connections are not secure
# at all -- eventually this could move to tmcc.  And when you get down to it,
# the widearea privkeys are not well protected.  Consequently, NO PRIVATE DATA
# should go into these scripts.  There should not be a need for that, anyway.
# This is meant to be an operator mechanism, not a user mechanism, and it is 
# not meant for private data, just client-side config stuff.
#

#
# Constraints that we satisfy.  Note that different constraint types are ANDed;
# if there are multiple values to a constraint type, those are ORed.  Had to
# do something...
#
$constraints = array("node_id","class","type",
                     "os","os_version","distro","distro_version","arch");

#
# Available states in which we support an osconfig
#
$stages = array("premfs","postload");
$FILEROOT = "$TBDIR/etc/osconfig";

#
# A few helper functions.
#
function SPITERROR($msg) {
    header("Content-Type: text/plain");
    echo "TBERROR: $msg\n";
    exit(1);
}

function SPITMSG($msg) {
    header("Content-Type: text/plain");
    echo "TBMSG: $msg\n";
    exit(1);
}

function SPITNOTHING() {
    header("Content-Type: text/plain");
    header("Content-Length: 0");
    exit(0);
}

function IsControlNetIP($myIP) {
    global $CONTROL_NETWORK,$CONTROL_NETMASK;

    if (ip2long($CONTROL_NETWORK) == (ip2long($CONTROL_NETMASK) 
				      && ip2long($myIP)))
	return 1;

    return 0;
}

function match_constraint($constraint,$value,$data) {
    if (!isset($data[$constraint]))
	return FALSE;

    if (!strncmp($value,"/",1)) {
        # a pcre regex
	if (preg_match($value,$data[$constraint]))
	    return TRUE;
    }
    else if ($value == $data[$constraint]) {
	return TRUE;
    }

    return FALSE;
}

#
# do stuff!
#

# if we do not have a stage, assume one
if (!isset($env) || !in_array($env,$stages)) {
    $env = "postload";
}

# if check is not set, don't just check
if (!isset($check)) {
    $check = 0;
}

# Must use https
if (!isset($_SERVER["SSL_PROTOCOL"])) {
    SPITERROR("insecure");
}

# make sure args are sane
if (!preg_match('/^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$/', $ip) ||
    (isset($privkey) && !preg_match('/^[\w]+$/', $privkey))) {
    SPITERROR("invalid args");
}

# XXX for now, we assert if REMOTE_ADDR is not the same as $ip
# obviously, this needs to be changed for remote nodes with proxies...
if ($_SERVER["REMOTE_ADDR"] != $ip) {
    SPITERROR("no proxies allowed");
}

# need this below
$node = Node::LookupByIP($ip);
if (!$node) {
    SPITERROR("no such node");
}
$node_id = $node->node_id();

#
# Make sure this is a valid request before we return anything.
#
if (isset($privkey)) {
    $qres = DBQueryFatal("select node_id from widearea_nodeinfo" .
			 " where node_id='" . addslashes($node_id) . "' and" . 
			 "   privkey='" . addslashes($privkey) . "'");
    if (! mysql_num_rows($qres)) {
	SPITERROR("noauth");
    }
}
else {
    if (!IsControlNetIP($ip)) {
	SPITERROR("notlocal");
    }
}

#
# Load up and match constraints.
#
$qres = DBQueryFatal("select target_file_idx,constraint_name,constraint_value" .
		     "  from osconfig_targets" .
		     " where target_apply='" . addslashes($env) . "'" . 
		     " order by constraint_idx");
if (!mysql_num_rows($qres))
    SPITNOTHING();

$match_targets = array();
$conproc = array();

$data = array("node_id" => $node_id,"class" => $node->TypeClass(),
	      "type" => $node->type());
if (isset($os)) 
    $data["os"] = $os;
if (isset($os_version)) 
    $data["os_version"] = $os_version;
if (isset($distro)) 
    $data["distro"] = $distro;
if (isset($distro_version)) 
    $data["distro_version"] = $distro_version;
if (isset($arch)) 
    $data["arch"] = $arch;

while ($row = mysql_fetch_array($qres)) {
    $c = $row["constraint_name"];
    if (!in_array($c,$constraints))
	continue;

    $val = match_constraint($c,$row["constraint_value"],$data);
    
    if (!isset($conproc[$row["target_file_idx"]]))
	$conproc[$row["target_file_idx"]] = array();

    if (!isset($conproc[$row["target_file_idx"]][$c]))
	$conproc[$row["target_file_idx"]][$c] = $val;
    else 
	$conproc[$row["target_file_idx"]][$c] = 
	    ($conproc[$row["target_file_idx"]][$c] || $val);
}

#
# Look through the results; for each target_idx, if all constraints
# have been met, then we can call this target matched.
#
foreach ($conproc as $target_idx => $target) {
    $cmatch = FALSE;
    foreach ($target as $constraint => $match) 
	$cmatch = ($cmatch || $match);

    if ($cmatch)
	array_push($match_targets,$target_idx);
}

#
# If we have no matches and the client is asking for a tarball, spit nothing;
# if the client is just checking if we have a tarball for them, spit an
# "update=(yes|no)" msg.
#
if (!count($match_targets)) {
    if ($check) {
	SPITMSG("update=no");
    }
    else {
	SPITNOTHING();
    }
}
else if ($check) {
    SPITMSG("update=yes");
}

#
# Now, for each target matched, grab the file-to-send info, and pack them up.
# We just grab everything cause it's cheap and there's no better way.
#
$qres = DBQueryFatal("select of.type as type,of.path as path," . 
		     "    of.dest as dest,of.file_idx as file_idx" .
		     "  from osconfig_targets as ot" . 
		     " left join osconfig_files as of" . 
		     "   on ot.target_file_idx=of.file_idx" . 
		     " where ot.target_apply='" . addslashes($env) . "'" . 
		     " group by of.file_idx" . 
		     " order by of.prio desc");
$done = array();
$files = array();
$manifest_str = "";
$i = 0;
while ($row = mysql_fetch_array($qres)) {
    if (in_array($row['file_idx'],$match_targets)
	&& !in_array($row['file_idx'])) {
	$files[$i++] = $row["path"];
	$manifest_str .= $row["path"] . "\t" . $row["type"] . "\t" . 
	    $row["dest"] . "\n";
	array_push($done,$row['file_idx']);
    }
}

# make a random dir for tar use.
while (TRUE) {
    $randdir = "/tmp/osconfig." . rand(100,10000);
    if (mkdir($randdir)) {
	break;
    }
}
$archive = "$randdir/archive";
$manifest = "$randdir/MANIFEST";

$fd = fopen($manifest,"w");
fwrite($fd,$manifest_str);
fclose($fd);

$origwd = getcwd();
chdir($FILEROOT);

$retval = system("tar -cf $archive " . implode(" ",$files));
if ($retval)
    SPITERROR("cdata");

chdir("$randdir");
$retval = system("tar -rf $archive MANIFEST");
if ($retval)
    SPITERROR("cdata");

system("gzip $archive");
$archive .= ".gz";

#
# Phew!  Blow chunk and finish.
#
$fstat = stat($archive);
header("Content-Type: application/x-gzip");
header("Content-Length: " . $fstat["size"]);
readfile($archive);

# cleanup
chdir($origwd);
unlink($archive);
unlink($manifest);
rmdir($randdir);

return;

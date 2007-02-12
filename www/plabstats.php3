<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header if not looking for xml returned.
#
$optargs = OptionalPageArguments("xml",     PAGEARG_STRING,
				 "slice",   PAGEARG_STRING,
				 "records", PAGEARG_INTEGER);

if (!isset($xml)) {
    PAGEHEADER("Emulab on Planetlab Stats");
}

#
# Get current user, if any. We return info to logged in admin users,
# or to someone who has the right key.
#
$this_user = CheckLogin($check_status);

if ($this_user) {
    CheckLoginOrDie();
    $isadmin = ISADMIN();

    if (!$isadmin) {
	USERERROR("You do not have permission to view this page!", 1);
    }
}
else {
    #
    # Must be SSL, even though we do not require an account login.
    #
    if (!isset($SSL_PROTOCOL)) {
	USERERROR("Must use https:// to access this page!", 1);
    }
    $subnet_c = long2ip(ip2long($REMOTE_ADDR) & ip2long("255.255.255.0"));
    $subnet_b = long2ip(ip2long($REMOTE_ADDR) & ip2long("255.255.0.0"));
    
    if (strcmp($subnet_c, "155.98.60.0") &&
	strcmp($subnet_c, "155.98.63.0") &&
	strcmp($subnet_c, "134.134.248.0") &&
	strcmp($subnet_c, "134.134.136.0") &&
        strcmp($subnet_b, "128.112.0.0")) {
	USERERROR("You do not have permission to view this page ($subnet)!",
		  1);
    }
}

# Page args,
if (isset($slice)) {
    if (preg_match("/^emulab_[-\w]*_([\d]*)$/", $slice, $matches) ||
	preg_match("/^emulab_([\d]*)$/", $slice, $matches)) {
	$wclause = "and s.exptidx='$matches[1]'";
    }
    else {
	USERERROR("Invalid 'slice' argument!", 1);
    }
}
else {
    $wclause = "";
}

# Show just the last N records unless request is different.
if (isset($records)) {
    if (!preg_match("/^[\d]*$/", $records)) {
	USERERROR("Invalid 'records' argument!", 1);
    }
    $limit = $records;
}
else {
    $limit = 100;
}

$query_result =
    DBQueryFatal("select s.pid,s.eid,t.uid,r.plabnodes,start_time,t.action, ".
		 "  u.usr_email,u.usr_name,s.exptidx, ".
		 "  UNIX_TIMESTAMP(start_time) as ustart ".
		 " from experiment_resources as r ".
		 "left join experiment_stats as s on r.exptidx=s.exptidx ".
		 "left join testbed_stats as t on t.rsrcidx=r.idx ".
		 "left join users as u on u.uid_idx=t.uid_idx ".
		 "where r.plabnodes!=0 and t.exitcode=0 and ".
		 "     (t.action='start' or t.action='swapin' or ".
		 "      t.action='swapout') $wclause ".
		 "order by t.end_time desc,t.idx desc limit $limit");

if (!isset($xml)) {
    echo "<table align=center border=1>
          <tr>
            <th>Slice</th>
            <th>Uid</th>
            <th>Pid</th>
            <th>Eid</th>
            <th>Date</th>
            <th>Action (Nodes)</th>
          </tr>\n";
}
else {
    echo "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>
          <!DOCTYPE EMULABONPLAB_XML [
            <!ELEMENT EMULABONPLAB_XML (EXPERIMENT)*>
              <!ATTLIST EMULABONPLAB_XML VERSION CDATA #REQUIRED>
            <!ELEMENT EXPERIMENT EMPTY>
              <!ATTLIST EXPERIMENT SLICE CDATA #REQUIRED>
              <!ATTLIST EXPERIMENT PID CDATA #REQUIRED>
              <!ATTLIST EXPERIMENT EID CDATA #REQUIRED>
              <!ATTLIST EXPERIMENT UID CDATA #REQUIRED>
              <!ATTLIST EXPERIMENT NAME CDATA #REQUIRED>
              <!ATTLIST EXPERIMENT EMAIL CDATA #REQUIRED>
              <!ATTLIST EXPERIMENT TIMESTAMP CDATA #REQUIRED>
              <!ATTLIST EXPERIMENT NODES CDATA #REQUIRED>
              <!ATTLIST EXPERIMENT ACTION (swapin | swapout) #REQUIRED>
          ]>\n";
    echo "<EMULABONPLAB_XML VERSION=\"0.1\">\n";
}

while ($row = mysql_fetch_assoc($query_result)) {
    $pid     = $row["pid"];
    $eid     = $row["eid"];
    $uid     = $row["uid"];
    $name    = $row["usr_name"];
    $email   = $row["usr_email"];
    $start   = $row["start_time"];
    $ustart  = $row["ustart"];
    $action  = $row["action"];
    $pnodes  = $row["plabnodes"];
    $exptidx = $row["exptidx"];
    $slice   = "emulab_${exptidx}";

    if (!strcmp($action, "start")) {
	$action = "swapin";
    }

    if (isset($xml)) {
        echo "<EXPERIMENT SLICE=\"$slice\" ";
	echo "PID=\"$pid\" EID=\"$eid\" UID=\"$uid\" ";
	echo "NAME=\"$name\" ";
	echo "EMAIL=\"$email\" TIMESTAMP=\"$ustart\" NODES=\"$pnodes\" ";
	echo "ACTION=\"$action\">\n";
    }
    else {
        echo "<tr>
                <td>$slice</td>
                <td>$uid ($email)</td>
                <td>$pid</td>
                <td>$eid</td>
                <td>$start</td>
                <td>$action ($pnodes)</td>
              </tr>\n";
    }
}

if (isset($xml)) {
    echo "</EMULABONPLAB_XML>\n";
}
else {
    echo "</table>\n";

    #
    # Standard Testbed Footer
    # 
    PAGEFOOTER();
}
?>

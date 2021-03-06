<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# No PAGEHEADER since we spit out a Location header later. See below.
#

#
# Helper function to send back errors.
#
function EXPERROR()
{
    global $formfields, $errors, $ifacetype;

    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    die("");
}

#
# Only known and logged in users can begin experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

# Does not return if this a request.
include("showlogfile_sup.php3");

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("view_style", PAGEARG_STRING,
				 "beginexp", PAGEARG_STRING,
				 "formfields", PAGEARG_ARRAY,
				 "nsref", PAGEARG_INTEGER,
				 "guid", PAGEARG_INTEGER,
				 "copyid", PAGEARG_INTEGER);

#
# Handle pre-defined view styles
#
unset($view);
if (isset($view_style) && $view_style == "plab") {
    $view['hide_proj'] = $view['hide_group'] = $view['hide_swap'] =
	$view['hide_preload'] = $view['hide_batch'] = $view['hide_linktest'] =
        $view['quiet'] = $view['plab_ns_message'] = $view['plab_descr'] = 1;
}
include("beginexp_form.php3");

# Need this below;
$idleswaptimeout = TBGetSiteVar("idle/threshold");

#
# See what projects the uid can create experiments in. Must be at least one.
#
$projlist = $this_user->ProjectAccessList($TB_PROJECT_CREATEEXPT);

if (! count($projlist)) {
    USERERROR("You do not appear to be a member of any Projects in which ".
	      "you have permission to create new experiments.", 1);
}

#
# On first load, display virgin form and exit.
#
if (!isset($beginexp)) {
    # Allow initial formfields data.
    if (isset($formfields))
	$defaults = $formfields;
    else
	$defaults = array();
    INITFORM($defaults, $projlist);
    PAGEFOOTER();
    return;
}

#
# If we got form data then let the form code do some checking, and then
# return an XML representation of it. This allows us to test the XML
# handling, as if it came from the user.
#
if (isset($formfields)) {
    $xmlcode = CHECKFORM($formfields, $projlist);
    #
    # Might not return. If the handler does not like what it sees, it
    # will put up a new form. Otherwise we do the main checks below.
    #
}
else {
    PAGEHEADER("Begin a Testbed Experiment");
    PAGEARGERROR();
}


#
# We are going to invoke the XML backend, and read back an XML representation
# of the results.
#
$uriargs  = "nocookieuid=$uid&nocookieauth=" . $_COOKIE[$TBAUTHCOOKIE];
$postdata = "xmlcode=" . urlencode($xmlcode);

#
# Yuck, no good support for sending POST requests. Must use fsockopen, which
# is not so bad, except you have to specify the port. Okay, ssl is on port
# 443, but at home I do not use ssl and I use a non-standard port since my
# ISP blocks port 80 and 8080. Also, devel trees are in a subdir, and so need
# to pull that out. Did I say Yuck?
#
if (preg_match("/^([-\w\.]+):(\d+)(.*)$/", $WWW, $matches)) {
    $host = $matches[1];
    $port = $matches[2];
    $base = $matches[3];
    $targ = $host;
}
elseif (preg_match("/^([-\w\.]+)(.*)$/", $WWW, $matches)) {
    $host = $matches[1];
    $base = $matches[2];
    $port = 443;
    $targ = "ssl://" . $host;
}
else {
    TBERROR("Could not parse $WWW for host/port/base!", 1);
}
#TBERROR("$host $port $base\n$uriargs\n". urldecode($postdata), 0);

$sock = fsockopen($targ, $port, $errno, $errstr, 30);
if ($sock == FALSE) {
    TBERROR("Could not invoke XML backend script.\n".
	    "$host $port $base\n".
	    "$errno: $errstr\n".
	    "$uriargs\n" . urldecode($postdata), 1);
}
fputs($sock, "POST $base/beginexp_xml.php3?" . $uriargs . " HTTP/1.0\r\n");
fputs($sock, "Host: $host\r\n");
fputs($sock, "Content-type: application/x-www-form-urlencoded\r\n");
fputs($sock, "Content-length: " . strlen($postdata) . "\r\n");
fputs($sock, "Accept: */*\r\n");
fputs($sock, "\r\n");
fputs($sock, "$postdata\r\n");
fputs($sock, "\r\n");

$headers = "";
while (!feof($sock)) { 
    $str = fgets($sock, 4096);
    if ($str == '') continue; # no data
    $str = trim($str);
    if ($str == '') break;    # blank line
    $headers .= "$str\n";
}

$reply = "";
while (!feof($sock))
    $reply .= fgets($sock, 4096);
fclose($sock);

#
# Error reporting is not well thought out yet. If the backend page gets
# an error, its possible no output will be sent back to us. 
# 
# Convert XML string back into PHP datatypes.
$foo = xmlrpc_decode_request($reply, $meth);
if (!isset($foo)) {
    TBERROR("Could not decode XML reply! Form Input:\n\n" .
	    print_r($formfields, TRUE) . "\n\n".
	    "Output:\n\n$reply\n", 1);
}
$results = $foo[0];

if ($results["status"] != "success") {
    if ($results["status"] == "xmlerror") {
	#
	# A formerror means we should respit the form with the error array.
	#
	SPITFORM($formfields, $results["errors"]);
    }
    else {
	PAGEHEADER("Begin a Testbed Experiment");
	echo "<br><xmp>";
	echo $results["message"];
	echo "</xmp><br>";
    }
    PAGEFOOTER();
    return;
}

# Okay, we can spit back a header now that there is no worry of redirect.
PAGEHEADER("Begin a Testbed Experiment");

# Map to the actual experiment and show the log.
if (($experiment = Experiment::LookupByPidEid($formfields["exp_pid"],
					      $formfields["exp_id"]))) {
    echo $experiment->PageHeader();
    echo "<br><br>\n";
    echo "<b>Starting experiment configuration!</b> " . $results["message"];
    echo "<br><br>\n";
    echo "</font>\n";
    STARTLOG($experiment);
}
else {
    echo "<br>\n";
    echo "<b>Starting experiment configuration!</b> " . $results["message"];
    echo "<br>\n";
}
						
#
# Standard Testbed Footer
#
PAGEFOOTER();
?>

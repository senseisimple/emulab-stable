<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
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
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Handle pre-defined view styles
#
unset($view);
if (isset($view_style) && $view_style == "plab") {
    $view['hide_proj'] = $view['hide_group'] = $view['hide_swap'] =
	$view['hide_preload'] = $view['hide_batch'] = $view['quiet'] =
	$view['plab_ns_message'] = 1;
}
include("beginexp_form.php3");

# Need this below;
$idleswaptimeout = TBGetSiteVar("idle/threshold");

#
# See what projects the uid can create experiments in. Must be at least one.
#
$projlist = TBProjList($uid, $TB_PROJECT_CREATEEXPT);

if (! count($projlist)) {
    USERERROR("You do not appear to be a member of any Projects in which ".
	      "you have permission to create new experiments.", 1);
}

#
# On first load, display virgin form and exit.
#
if (!isset($beginexp)) {
    # Allow initial formfields data. 
    INITFORM($formfields, $projlist);
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
$url   = "$TBBASE/beginexp_xml.php3?".
         "nocookieuid=$uid&nocookieauth=" . $_COOKIE[$TBAUTHCOOKIE] .
         "&xmlcode=" . urlencode($xmlcode);
$reply = "";

# TBERROR(urldecode($url), 0);

$fp = @fopen($url, "r");
if ($fp == FALSE) {
    TBERROR("Could not invoke XML backend script. URL was:<br><br>\n".
	    "$url\n<br>", 1);
}
while (!feof($fp)) {
    $reply .= fgets($fp, 1024);
}
fclose($fp);

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

if ($results[status] != "success") {
    if ($results[status] == "xmlerror") {
	#
	# A formerror means we should respit the form with the error array.
	#
	SPITFORM($formfields, $results[errors]);
    }
    else {
	PAGEHEADER("Begin a Testbed Experiment");
	echo "<br><xmp>";
	echo $results[message];
	echo "</xmp><br>";
    }
    PAGEFOOTER();
    return;
}

# Okay, we can spit back a header now that there is no worry of redirect.
PAGEHEADER("Begin a Testbed Experiment");

# Need these for output messages.
$exp_pid = $formfields[exp_pid];
$exp_id  = $formfields[exp_id];

#
# Okay, time to do it.
#
echo "<font size=+2>Experiment <b>".
     "<a href='showproject.php3?pid=$exp_pid'>$exp_pid</a>/".
     "<a href='showexp.php3?pid=$exp_pid&eid=$exp_id'>$exp_id</a></b>\n".
     "</font>\n";
echo "<br><br>\n";

echo "<center><br>";
echo "<font size=+1>Starting experiment configuration.</font>
      </center>";

echo "<br>\n";
echo $results[message];
echo "<br><br>\n";
echo "While you are waiting, you can watch the log
      in <a target=_blank href=spewlogfile.php3?pid=$exp_pid&eid=$exp_id>
      realtime</a>.\n";
echo "<br>
      </font>\n";

#
# Standard Testbed Footer
#
PAGEFOOTER();
?>

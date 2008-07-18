<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2008 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

$optargs = OptionalPageArguments("stayhome", PAGEARG_BOOLEAN);

#
# The point of this is to redirect logged in users to their My Emulab
# page. 
#
if (($this_user = CheckLogin($check_status))) {
    $check_status = $check_status & CHECKLOGIN_STATUSMASK;
    
    if (($firstinitstate = TBGetFirstInitState())) {
	unset($stayhome);
    }
    if (!isset($stayhome)) {
	if ($check_status == CHECKLOGIN_LOGGEDIN) {
	    if ($firstinitstate == "createproject") {
	        # Zap to NewProject Page,
 	        header("Location: $TBBASE/newproject.php3");
	    }
	    else {
		# Zap to My Emulab page.
		header("Location: $TBBASE/".
		       CreateURL("showuser", $this_user));
	    }
	    return;
	}
	elseif (isset($SSL_PROTOCOL)) {
            # Fall through; display the page.
	    ;
	}
	elseif ($check_status == CHECKLOGIN_MAYBEVALID) {
            # Not in SSL mode, so reload using https to see if logged in.
	    header("Location: $TBBASE/index.php3");
	}
    }
    # Fall through; display the page.
}

#
# Standard Testbed Header
#
PAGEHEADER("Emulab - Network Emulation Testbed Home",NULL,$RSS_HEADER_NEWS);

#
# Special banner message.
#
$message = TBGetSiteVar("web/banner");
if ($message != "") {
    echo "<center><font color=Red size=+1>\n";
    echo "$message\n";
    echo "</font></center><br>\n";
}

?>

<p>
    <em>Emulab</em> is a network testbed, giving researchers a wide range of
        environments in which to develop, debug, and evaluate their systems.
    The name Emulab refers both to a <strong>facility</strong> and to a
    <strong>software system</strong>.
    The <a href="http://www.emulab.net">primary Emulab installation</a> is run
        by the
        <a href="http://www.flux.utah.edu">Flux Group</a>, part of the
        <a href="http://www.cs.utah.edu">School of Computing</a> at the
        <a href="http://www.utah.edu">University of Utah</a>.
    There are also installations of the Emulab software at more than
        <a href="http://users.emulab.net/trac/emulab/wiki/OtherEmulabs">two
        dozen sites</a> around the world, ranging from testbeds with a handful
        of nodes up to testbeds with hundreds of nodes.
    Emulab is <a href="http://www.emulab.net/expubs.php">widely used</a>
        by computer science researchers in the fields of networking and
        distributed systems.
    It is also used to <a href="http://users.emulab.net/trac/emulab/wiki/Classes">teach
        classes</a> in those fields.
</p>

<?php
#
# Allow for a site specific front page 
#
$sitefile = "index-" . strtolower($THISHOMEBASE) . ".html";

if (!file_exists($sitefile)) {
    if ($TBMAINSITE)
	$sitefile = "index-mainsite.html";
    else
	$sitefile = "index-nonmain.html";
}
readfile("$sitefile");

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

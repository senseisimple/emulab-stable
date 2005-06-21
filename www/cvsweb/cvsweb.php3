<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2005 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Wrapper script for cvsweb.cgi
#
chdir("../");
require("defs.php3");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

if (! TBCvswebAllowed($uid)) {
        USERERROR("You do not have permission to use cvsweb!", 1);
}

$script = "cvsweb.cgi";

#
# Sine PHP helpfully scrubs out environment variables that we _want_, we
# have to pass them to env.....
#
$query = escapeshellcmd($QUERY_STRING);
$path = escapeshellcmd($PATH_INFO);
$name = escapeshellcmd($SCRIPT_NAME);
$agent = escapeshellcmd($HTTP_USER_AGENT);
$encoding = escapeshellcmd($HTTP_ACCEPT_ENCODING);

#
# Helpfully enough, escapeshellcmd doesn't escape spaces. Sigh.
#
$script = preg_replace("/ /","\\ ",$script);
$query = preg_replace("/ /","\\ ",$query);
$name = preg_replace("/ /","\\ ",$name);
$agent = preg_replace("/ /","\\ ",$agent);
$encoding = preg_replace("/ /","\\ ",$encoding);

#
# A cleanup function to keep the child from becoming a zombie, since
# the script is terminated, but the children are left to roam.
#
$fp = 0;

function SPEWCLEANUP()
{
    global $fp;

    if (!$fp || !connection_aborted()) {
	exit();
    }
    while ($line = fgets($fp)) {
        # Suck it all up.
        ;
    }
    pclose($fp);
    exit();
}
set_time_limit(0);
register_shutdown_function("SPEWCLEANUP");

$fp = popen("env PATH=./cvsweb/ QUERY_STRING=$query PATH_INFO=$path " .
            "SCRIPT_NAME=$name HTTP_USER_AGENT=$agent " .
            "HTTP_ACCEPT_ENCODING=$encoding $script",'r');

#
# Yuck. Since we can't tell php to shut up and not print headers, we have to
# 'merge' headers from cvsweb with PHP's.
#
while ($line = fgets($fp)) {
    # This indicates the end of headers
    if ($line == "\r\n") { break; }
    header(rtrim($line));
}

#
# Just pass through the rest of cvsweb.cgi's output
#
fpassthru($fp);

fclose($fp);

?>

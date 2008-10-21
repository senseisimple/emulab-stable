<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Only admins can do this!
#
if (!$isadmin) {
    USERERROR("Only TB admins can do this!", 1);
}

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT);
$optargs = OptionalPageArguments("message",    PAGEARG_STRING,
				 "canceled",   PAGEARG_STRING,
				 "confirmed",  PAGEARG_STRING);

#
# Standard Testbed Header
#
PAGEHEADER("Request info about possibly Idle experiment");

#
# Need these below
#
$pid = $experiment->pid();
$eid = $experiment->eid();
$pcs = $experiment->PCCount();

echo $experiment->PageHeader();
echo "<br>";

if (!$pcs) {
    echo "<center><h3>Idle info request canceled because there are no PCs!</h3>
          </center>\n";
    PAGEFOOTER();
    return;
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if (isset($canceled) && $canceled) {
    echo "<center><h3>Idle info request canceled!</h3>
          </center>\n";
    PAGEFOOTER();
    return;
}

if (!isset($confirmed)) {
    echo "<br><center><h3>
          Are you <b>SURE</b> you want to send an Idle Info request
          email message to the swapper of $pid/$eid?
          </h3>\n";

    #
    # Dump experiment record.
    #
    $experiment->Show();

    $url = CreateURL("request_idleinfo", $experiment);
   
    echo "<br>\n";
    echo "<form action='$url' method=post>\n";
    echo "<table align=center border=1>\n";
    echo "  <tr><td><center>Additional message to user?</center></td></tr>
	    <tr><td><textarea rows=2 cols=50 name=message></textarea>
	      </td></tr>
	    <tr><td><center>\n";
    echo "    <input type=submit name=confirmed value=Confirm>\n";
    echo "    &nbsp; &nbsp;\n";
    echo "    <input type=submit name=canceled value=Cancel>\n";
    echo "    </center></td></tr>\n";
    echo "</table>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

# Info about experiment.
$seconds = $experiment->SwapSeconds();
$hours   = intval($seconds / 3600);

if (! ($creator = $experiment->GetCreator())) {
    TBERROR("Could not lookup object for experiment creator!", 1);
}
if (! ($swapper = $experiment->GetSwapper())) {
    TBERROR("Could not lookup object for experiment swapper!", 1);
}
if (! ($group = $experiment->Group())) {
    TBERROR("Could not lookup object for experiment group!", 1);
}

# Lots of email addresses!
$allleaders    = $group->LeaderMailList();
$swapper_name  = $swapper->name();
$swapper_email = $swapper->email();
$creator_name  = $creator->name();
$creator_email = $creator->email();
if (! $swapper->SameUser($creator)) {
    $allleaders .= ", \"$creator_name\" <$creator_email>";
}

# And send email
TBMAIL("$swapper_name <$swapper_email>",
       "Experiment $pid/$eid - Please tell us about it",
       "Hi. We noticed that your experiment '$pid/$eid' has been\n".
       "swapped in for $hours hours and is using $pcs nodes.\n".
       "\n".
       "Such long running experiments are unusual so we want to make sure\n".
       "this experiment is still doing useful work.\n".
       (isset($message) && $message !== "" ? "\n$message\n\n" : "\n").
       "Please respond to this message letting us know; if we do not hear\n".
       "from you within a couple of hours, we will be forced to swap your\n".
       "experiment out so that others can use the nodes.\n\n".
       "Thanks very much!\n",
       "From: $TBMAIL_OPS\n".
       "Cc: $allleaders\n".
       "Cc: $TBMAIL_OPS\n".
       "Errors-To: $TBMAIL_WWW");

echo "<center><h2>Message sent!</h2>
      </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

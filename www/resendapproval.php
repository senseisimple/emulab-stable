<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

PAGEHEADER("Resend Project Approval Message");

if (!$isadmin) {
    USERERROR("You do not have permission to access this page!", 1);
}

#
# Verify form arguments.
# 
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    USERERROR("You must provide a Project ID.", 1);
}

#
# Confirm target is a real project,
#
if (! ($target_project = Project::Lookup($pid))) {
    USERERROR("The project $pid is not a valid project", 1);
}
if (! ($leader = $target_project->GetLeader())) {
    TBERROR("Error getting leader for $pid", 1);
}
$headuid       = $leader->uid();
$headuid_email = $leader->email();
$headname      = $leader->name();

TBMAIL("$headname '$headuid' <$headuid_email>",
         "Project '$pid' Approval",
         "\n".
	 "This message is to notify you that your project '$pid'\n".
	 "has been approved.  We recommend that you save this link so that\n".
	 "you can send it to people you wish to have join your project.\n".
	 "Otherwise, tell them to go to ${TBBASE} and join it.\n".
	 "\n".
	 "    ${TBBASE}/joinproject.php3?target_pid=$pid\n".
         "\n".
         "Thanks,\n".
         "Testbed Operations\n",
         "From: $TBMAIL_APPROVAL\n".
         "Bcc: $TBMAIL_APPROVAL\n".
         "Errors-To: $TBMAIL_WWW");

echo "<center>
      <h2>Done!</h2>
      </center><br>\n";

sleep(1);

PAGEREPLACE(CreateURL("showproject", $target_project));

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

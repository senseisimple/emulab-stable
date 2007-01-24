<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

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

#
# Form to allow text input.
#
function SPITFORM($pid, $message, $errors)
{
    global $this_user, $target_project;
    
    if ($errors) {
	echo "<table class=nogrid
                     align=center border=0 cellpadding=6 cellspacing=0>
              <tr>
                 <th align=center colspan=2>
                   <font size=+1 color=red>
                      &nbsp;Oops, please fix the following errors!&nbsp;
                   </font>
                 </td>
              </tr>\n";

	while (list ($name, $message) = each ($errors)) {
	    echo "<tr>
                     <td align=right>
                       <font color=red>$name:&nbsp;</font></td>
                     <td align=left>
                       <font color=red>$message</font></td>
                  </tr>\n";
	}
	echo "</table><br>\n";
    }

    #
    # Show stuff
    #
    $target_project->Show();

    echo "<br>";
    echo "<table align=center border=1>\n";
    echo "<form action='resendapproval.php?pid=$pid' method='post'>\n";

    echo "<tr>
              <td>Use the text box (70 columns wide) to add a message to the
                  email notification. </td>
          </tr>\n";

    echo "<tr>
             <td align=center class=left>
                 <textarea name=message rows=15 cols=70></textarea>
             </td>
          </tr>\n";

    echo "<tr>
              <td align=center>
                  <b><input type='submit' value='Submit' name='submit'></td>
          </tr>
          </form>
          </table>\n";
}

#
# On first load, display a virgin form and exit.
#
if (! $submit) {
    SPITFORM($pid, "", null);
    PAGEFOOTER();
    return;
}

# If there is a message in the text box, it is appended below.
if (! isset($message)) {
    $message = "";
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
       ($message != "" ? "${message}\n\n" : "") .
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

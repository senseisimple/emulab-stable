<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("New Project Approval");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Of course verify that this uid has admin privs!
#
$isadmin = ISADMIN($uid);
if (! $isadmin) {
    USERERROR("You do not have admin privledges to approve projects!", 1);
}

#
# Verify arguments.
# 
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    USERERROR("You must provide a project ID.", 1);
}

#
# Check to make sure thats this is a valid PID.
#
$query_result =
    DBQueryFatal("SELECT * FROM projects WHERE pid='$pid'");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The project $pid is not a valid project.", 1);
}

echo "<center><h3>You have the following choices:</h3></center>
      <table align=center border=0>
        <tr>
            <td>Deny</td>
            <td>-</td>
            <td>Deny project application (kills project records)</td>
        </tr>

        <tr>
            <td>Destroy</td>
            <td>-</td>
            <td>Deny project application, and kill the user account</td>
        </tr>

        <tr>
            <td>Approve</td>
            <td>-</td>
            <td>Approve the project</td>
        </tr>

        <tr>
            <td>More Info</td>
            <td>-</td>
            <td>Ask for more info</td>
        </tr>

        <tr>
            <td>Postpone</td>
            <td>-</td>
            <td>Twiddle your thumbs some more</td>
        </tr>
      </table>\n";

#
# This will spit out the info.
#
include("showproject_dump.php3");

#
# Now put up the menu choice along with a text box for an email message.
#
echo "<center>
      <h3>What would you like to do?</h3>
      </center>
      <table align=center border=1>
      <form action='approveproject.php3?pid=$pid' method='post'>\n";

echo "<tr>
          <td align=center>
              <select name=approval>
                      <option value='postpone'>Postpone</option>
                      <option value='approve'>Approve</option>
                      <option value='moreinfo'>More Info</option>
                      <option value='deny'>Deny</option>
                      <option value='destroy'>Destroy</option>
              </select>
          </td>
       </tr>\n";


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
          <td align=center colspan=2>
              <b><input type='submit' value='Submit' name='OK'></td>
      </tr>
      </form>
      </table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Edit your Group");

#
# Only known and logged in users.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Verify page arguments.
# 
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    USERERROR("You must provide a project ID.", 1);
}

#
# Verify permission.
#
if (! TBProjAccessCheck($uid, $pid, 0, $TB_PROJECT_MAKEGROUP)) {
    USERERROR("You do not have permission to create groups in project $pid!",
	      1);
}

echo "<form action=newgroup.php3 method=post>
      <table align=center border=1> 
      <tr>
        <td align=center colspan=2>
           <em>(Fields marked with * are required)</em>
        </td>
      </tr>\n";

echo "<tr>
          <td>Project:</td>
          <td class=left>
              <input name=group_pid type=readonly value='$pid'>
          </td>
      </tr>\n";

echo "<tr>
          <td>*Group Name (no blanks, lowercase):</td>
          <td class=left>
              <input name=group_id type=text size=$TBDB_GIDLEN
                     maxlength=$TBDB_GIDLEN>
          </td>
      </tr>\n";

echo "<tr>
          <td>*Group Description:</td>
          <td class=left>
              <input name=group_description type=text size=50>
          </td>
      </tr>\n";

echo "<tr>
          <td>*Group Leader (Emulab userid):</td>
          <td class=left>
              <input name=group_leader type=text value='$uid'
	             size=$TBDB_UIDLEN maxlength=$TBDB_UIDLEN>
          </td>
      </tr>\n";

echo "<tr>
         <td align=center colspan=2>
            <b><input type=submit value=Submit></b></td>
      </tr>
      </form>
      </table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

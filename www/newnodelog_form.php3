<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Enter Node Log Entry");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# Verify form arguments.
# 
if (!isset($node_id) || strcmp($node_id, "") == 0) {
    unset($node_id);
}
else {
    #
    # Check to make sure that this is a valid nodeid
    #
    if (! TBValidNodeName($node_id)) {
	USERERROR("The node $node_id is not a valid nodeid!", 1);
    }
}

#
# Only Admins can enter log entries.
#
if (! $isadmin) {
    USERERROR("You do not have permission to enter log entries!", 1);
}

echo "<table align=center border=1> 
      <tr>
         <td align=center colspan=2>
             <em>(Fields marked with * are required)</em>
         </td>
      </tr>
     <form action='newnodelog.php3' method=post>\n";

#
# Node ID:
#
# Note DB max length.
#
if (isset($node_id)) {
    echo "<tr>
              <td>*Node ID:</td>
              <td><input type=text name=node_id value=$node_id
			 size=$TBDB_NODEIDLEN maxlength=$TBDB_NODEIDLEN>
      </tr>\n";
}
else {
    echo "<tr>
              <td>*Node ID:</td>
              <td><input type=text name=node_id size=$TBDB_NODEIDLEN
                         maxlength=$TBDB_NODEIDLEN>
              </td>
      </tr>\n";
}

#
# Log Type.
#
echo "<tr>
          <td>*Log Type:</td>
          <td><select name=log_type>
               <option selected value='misc'>Misc</option>
              </select>
          </td>
      </tr>\n";

#
# Log Entry.
#
echo "<tr>
         <td>*Log Entry:</td>
         <td><input type=text name=log_entry size=50 maxlength=128></td>
      </tr>\n";

echo "<tr>
         <td align=center colspan=2>
            <b><input type=submit value=Submit></b>
         </td>
      </tr>\n";

echo "</form>
      </table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

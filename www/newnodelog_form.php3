<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("node_defs.php");

#
# Standard Testbed Header
#
PAGEHEADER("Enter Node Log Entry");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("node", PAGEARG_NODE);

#
# Only Admins can enter log entries. Or members of emulab-ops project
# if the node is free or reserved to emulab-ops.
#
if (! ($isadmin || OPSGUY())) {
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
if (isset($node)) {
    echo "<tr>
              <td>*Node ID:</td>
              <td><input type=text name=node_id
                         value=" . $node->node_id() . " 
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

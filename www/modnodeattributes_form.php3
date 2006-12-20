<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Modify Node Attributes Form");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify form arguments.
# 
if (!isset($node_id) ||
    strcmp($node_id, "") == 0) {
    USERERROR("You must provide a node ID.", 1);
}

#
# Check to make sure that this is a valid nodeid
#
$query_result =
    DBQueryFatal("select * from nodes where node_id='$node_id'");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The node $node_id is not a valid nodeid!", 1);
}
$noderow = mysql_fetch_array($query_result);

#
# Only admin users can modify node attributes.
#
if (! $isadmin) {
  USERERROR("You do not have permission to modify node $node_id!", 1);
}

#
# Set up some variables from the nodes table
#
$type    = $noderow[type];

#
# Get any node attributes that might exist
#
$node_attrs = array();
$attr_result =
    DBQueryFatal("select attrkey,attrvalue from node_attributes ".
		 "where node_id='$node_id'");
while($row = mysql_fetch_array($attr_result)) {
  $node_attrs[$row[attrkey]] = $row[attrvalue];
}

#
# Print out node id and type
#

echo "<table border=0 cellpadding=0 cellspacing=2
       align='center'>\n";

echo "<tr>
          <td align=\"center\"><b>Node ID: $node_id</b></td>
      </tr>\n";

echo "<tr>
          <td align=\"center\"><b>Node Type: $type</b></td>
      </tr>\n";
echo "</table><br><br><br><br>\n";

#
# Generate the form. Note that $refer is set by the caller so we know
# how we got to the webmodnodeattributes page. 
# 
echo "<table border=2 cellpadding=0 cellspacing=2
       align='center'>\n";
echo "<form action=\"modnodeattributes.php3?refer=$refer\"
            method=\"post\">\n";
echo "<input type=\"hidden\" name=\"node_id\" value=\"$node_id\">\n";
#
# Print out any node attributes already set
#
if ($node_attrs) {
  echo "<tr><td><table border=2 cellpadding=0 cellspacing=2
                 align='left'>\n";
  echo "<tr>
            <td align=\"center\" colspan=\"0\">
                <b>Current Node Attributes</b></td>
        </tr>
        <tr>
            <td>Del?</td><td>Attribute</td><td>Value</td>
        </tr>\n";
  foreach ($node_attrs as $attrkey => $attrval) {
    echo "<tr>
              <td><input type=\"checkbox\" name=\"_delattrs[$attrkey]\">
              <td>$attrkey</td>
              <td class=\"left\">
                  <input type=\"text\" name=\"_modattrs[$attrkey]\" size=\"60\"
                         value=\"$attrval\"></td>
          </tr>\n";
  }
  echo "</table></td></tr>\n";
}

#
# Print out fields for adding new attributes.
# The number of attributes defaults to 1, but this can be changed
# by specifying the number to add in the "add_numattrs" CGI variable.
#

echo "<tr><td><table border=2 cellpadding=0 cellspacing=2
       align='left'>\n";
echo "<tr>
          <td align=\"center\" colspan=\"0\"><b>Add Node Attribute</b></td>
      </tr>\n";

if (!$add_numattrs) {
  global $add_numattrs;
  $add_numattrs = 1;
}

for ($i = 0; $i < $add_numattrs; $i++) {
  echo "<tr>
            <td class=\"left\">
                <input type=\"text\" name=\"_newattrs[$i]\" size=\"32\"></td>
            <td class=\"left\">
                <input type=\"text\" name=\"_newvals[$i]\" size=\"60\"></td>
            </td>
        </tr>\n";
}

echo "<tr>
          <td colspan=2 align=center>
              <b><input type=\"submit\" value=\"Submit\"></b>
          </td>
     </tr>
     </form>
     </table></td></tr>\n";

echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Create a new OSID Form");

#
# Only known and logged in users can create a new OSID
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# See what projects the uid is a member of. Must be at least one!
# 
$query_result =
    DBQueryFatal("SELECT pid,trust FROM group_membership WHERE uid='$uid'");

#
# See if proper trust level in any of them.
#
$okay = 0;
while ($row = mysql_fetch_array($query_result)) {
    $trust = $row[trust];

    if (TBMinTrust($trust, $TBDB_TRUST_LOCALROOT)) {
	$okay = 1;
    }
}
mysql_data_seek($query_result, 0);

if (!$okay) {
    USERERROR("You do not appear to be a member of any Projects in which ".
	      "you have permission to create new OSIDs", 1);
}

?>
<blockquote><blockquote>
Use this page to create a customized Operating System Identifier (OSID),
for use with the <tt>tb-set-node-os</tt> command in your NS file.
</blockquote></blockquote>
    
<table align="center" border="1"> 

<tr>
    <td align="center" colspan="2">
        <em>(Fields marked with * are required)</em>
    </td>
</tr>
<form action="newosid.php3" method="post">

<?php

#
# OSID:
#
# Note DB max length.
#
echo "<tr>
          <td>*OSID (no blanks):</td>
          <td><input type=\"text\" name=\"osid\"
                     size=$TBDB_OSID_OSIDLEN maxlength=$TBDB_OSID_OSIDLEN>
              </td>
      </tr>\n";

#
# Description
#
echo "<tr>
          <td>*Description:</td>
          <td><input type=\"text\" name=\"description\" size=\"40\">
              </td>
      </tr>\n";

#
# Select Project
#
echo "<tr>
          <td>*Select Project:</td>";
echo "    <td><select name=\"pid\">";
               while ($row = mysql_fetch_array($query_result)) {
                  $project = $row[pid];
		  $trust   = $row[trust];
		  if (TBMinTrust($trust, $TBDB_TRUST_LOCALROOT)) {
		      echo "<option value=\"$project\">$project</option>\n";
		  }
               }
echo "        <option value=none>None</option>\n";
echo "       </select>";
echo "    </td>
      </tr>\n";

#
# Select an OS
# 
echo "<tr>
          <td>*Select OS:</td>
          <td><select name=\"OS\">
               <option value=\"OSKit\">OSKit</option>
               <option value=\"Unknown\">Unknown</option>
               <option value=\"FreeBSD\">FreeBSD</option>
               <option value=\"Linux\">Linux</option>
               <option value=\"NetBSD\">NetBSD</option>
              </select>
          </td>
      </tr>\n";

#
# Version String
#
echo "<tr>
          <td>Version:</td>
          <td><input type=\"text\" name=\"os_version\" 
                     size=$TBDB_OSID_VERSLEN maxlength=$TBDB_OSID_VERSLEN>
              </td>
      </tr>\n";

#
# Path to Multiboot image.
#
echo "<tr>
          <td>Path:</td>
          <td><input type=\"text\" name=\"os_path\" size=40>
              </td>
      </tr>\n";

#
# Path to Multiboot image.
#
echo "<tr>
          <td>Magic (ie: uname -r -s):</td>
          <td><input type=\"text\" name=\"os_magic\" size=30>
              </td>
      </tr>\n";

#
# Path to Multiboot image.
#
echo "<tr>
          <td>Machine Type:</td>
          <td><select name=\"os_machinetype\">
               <option value=\"PC\">pc</option>
               <option value=\"Shark\">shark</option>
              </select>
          </td>
      </tr>\n";

echo "<tr>
          <td>OS Features:</td>
          <td><input type=checkbox name=\"os_feature_ping\">ping &nbsp
              <input type=checkbox name=\"os_feature_ssh\">ssh  &nbsp
              <input type=checkbox name=\"os_feature_ipod\">ipod &nbsp
          </td>
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

<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Create a new OS Descriptor");

#
# Only known and logged in users
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# See what projects the uid can do this in.
#
$projlist = TBProjList($uid, $TB_PROJECT_MAKEOSID);

if (! count($projlist)) {
    USERERROR("You do not appear to be a member of any Projects in which ".
	      "you have permission to create new OS Descriptors!", 1);
}

?>
<blockquote><blockquote>
Use this page to create a customized Operating System Descriptor,
for use with the <tt>tb-set-node-os</tt> command in your NS file.
</blockquote></blockquote>
    
<table align="center" border="1"> 

<tr>
    <td align="center" colspan="2">
        <em>(Fields marked with * are required)</em>
    </td>
</tr>
<form action="newosid.php3" method="post" name=idform>

<?php

#
# Select Project
#
echo "<tr>
          <td>*Select Project:</td>";
echo "    <td><select name=pid>
              <option value=''>Please Select &nbsp</option>\n";

	      while (list($project) = each($projlist)) {
		  echo "<option value='$project'>$project </option>\n";
	      }
echo "       </select>";
echo "    </td>
      </tr>\n";

#
# OSID:
#
# Note DB max length.
#
echo "<tr>
          <td>*Descriptor Name (no blanks):</td>
          <td><input type=\"text\" name=\"osname\"
                     size=$TBDB_OSID_OSNAMELEN maxlength=$TBDB_OSID_OSNAMELEN>
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
# Select an OS
# 
echo "<tr>
          <td>*Select OS:</td>
          <td><select name=OS>
               <option value=none>Please Select &nbsp</option>
               <option value=Linux>Linux </option>
               <option value=FreeBSD>FreeBSD </option>
               <option value=NetBSD>NetBSD </option>
               <option value=OSKit>OSKit </option>
               <option value=Unknown>Other </option>
              </select>
          </td>
      </tr>\n";

#
# Version String
#
echo "<tr>
          <td>*Version:</td>
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
# Magic string?
#
echo "<tr>
          <td>Magic (ie: uname -r -s):</td>
          <td><input type=\"text\" name=\"os_magic\" size=30>
              </td>
      </tr>\n";

echo "<tr>
          <td>OS Features:</td>
          <td><input type=checkbox name=\"os_feature_ping\">ping &nbsp
              <input type=checkbox name=\"os_feature_ssh\">ssh  &nbsp
          </td>
      </tr>\n";

if ($isadmin) {
    #
    # Shared?
    #
    echo "<tr>
	      <td>Shared?:<br>
                  (available to all projects)</td>
              <td><input type=checkbox name=os_shared value=Yep> Yes</td>
          </tr>\n";

    #
    # Mustclean?
    #
    echo "<tr>
	      <td>Clean?:<br>
                  (no disk reload required)</td>
              <td><input type=checkbox name=os_clean value=Yep> Yes</td>
          </tr>\n";
}

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

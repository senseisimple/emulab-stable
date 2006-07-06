<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2004, 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("osiddefs.php3");

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

$osid_result =
    DBQueryFatal("select * from os_info ".
		 "where (path='' or path is NULL) and ".
		 "      version!='' and version is not NULL ".
		 "order by osid");

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
          <td><select name=OS>\n";

while (list ($os, $userokay) = each($osid_oslist)) {
    if (!$userokay && !$isadmin)
	continue;

    echo "<option value=$os>$os &nbsp; </option>\n";
}
echo "       </select>
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
          <td>";

while (list ($feature, $userokay) = each($osid_featurelist)) {
    if (!$userokay && !$isadmin)
	continue;

    echo "<input checked type=checkbox value=checked
                 name=\"os_feature_$feature\">$feature &nbsp\n";
}
echo "<p>Guidelines for setting os_features for your OS:
              <ol>
                <li> Mark ping and/or ssh if they are supported.
                <li> If you use a testbed kernel, or are based on a
                     testbed kernel config, mark the ipod box.
                <li> If it is based on a testbed image or sends its own
                     isup, mark isup. 
              </ol>
          </td>
      </tr>\n";

#
# Choose an op_mode state machine
#

echo "<tr>
          <td>*Operational Mode (op_mode):</td>
          <td><select name=op_mode>\n";
while (list ($op_mode, $userokay) = each($osid_opmodes)) {
    if (!$userokay && !$isadmin)
	continue;

    $selected = "";
    if ($op_mode == TBDB_DEFAULT_OSID_OPMODE) {
	$selected = "selected";
    }

    echo "<option $selected value=$op_mode>$op_mode &nbsp; </option>\n";
}
echo "       </select>
             <p>
              Guidelines for setting op_mode for your OS:
              <ol>
                <li> If it is based on a testbed image (one of our
                     Linux, Fedora, FreeBSD or Windows images) use the same
                     op_mode as that image. Select it from the
                     <a href=\"$TBBASE/showosid_list.php3\"
                     >OS Descriptor List</a> to find out).
                <li> If not, use MINIMAL.
              </ol>
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

    #
    # Reboot Waittime. 
    #
    echo "<tr>
	      <td>Reboot Waittime (seconds)</td>
              <td><input type=\"text\" name=\"os_reboot_waittime\" size=5>
          </tr>\n";

    WRITEOSIDMENU("NextOsid", "nextosid", $osid_result, "none");
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

<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Create a Project Group");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("project", PAGEARG_PROJECT);

if (!isset($project)) {
    #
    # See what projects the uid can do this in.
    #
    $projlist = $this_user->ProjectAccessList($TB_PROJECT_MAKEGROUP);

    if (! count($projlist)) {
	USERERROR("You do not appear to be a member of any Projects in which ".
		  "you have permission to create new groups.", 1);
    }
}
else {
    #
    # Verify permission for specific project.
    #
    $pid = $project->pid();
    
    if (!$project->AccessCheck($this_user, $TB_PROJECT_MAKEGROUP)) {
	USERERROR("You do not have permission to create groups in ".
		  "project $pid!", 1);
    }
}


echo "<form action=newgroup.php3 method=post>
      <table align=center border=1> 
      <tr>
        <td align=center colspan=2>
           <em>(Fields marked with * are required)</em>
        </td>
      </tr>\n";

if (isset($project)) {
    echo "<tr>
              <td>* Project:</td>
              <td class=left>
                  <input name=project type=readonly value='$pid'>
              </td>
          </tr>\n";
}
else {
    echo "<tr>
              <td>*Select Project:</td>";
    echo "    <td><select name=project>";

    while (list($project) = each($projlist)) {
	echo "<option value='$project'>$project </option>\n";
    }

    echo "       </select>";
    echo "    </td>
          </tr>\n";
}

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

echo "<br><center>
       Important <a href=docwrapper.php3?docname=groups.html#SECURITY'>
       security issues</a> are discussed in the
       <a href='docwrapper.php3?docname=groups.html'>Groups Tutorial</a>.
      </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Create a new ImageID Form");

#
# Only known and logged in users can create a new OSID
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# See what projects the uid can do this in.
#
$projlist = TBProjList($uid, $TB_PROJECT_MAKEIMAGEID);

if (! count($projlist)) {
    USERERROR("You do not appear to be a member of any Projects in which ".
	      "you have permission to create new ImageIDs.", 1);
}

#
# Get the OSID list that the user has access to.
#
if ($isadmin) {
    $osid_result =
	DBQueryFatal("select * from os_info ".
		     "where path='' or path is NULL ".
		     "order by osid");
}
else {
    $osid_result =
	DBQueryFatal("select distinct o.* from os_info as o ".
		     "left join group_membership as m ".
		     " on o.pid IS NULL or m.pid=o.pid ".
		     "where m.uid='$uid' and (path='' or path is NULL) ".
		     "order by o.pid,o.osid");
}
if (! mysql_num_rows($osid_result)) {
    USERERROR("There are no OSIDs that you are able to use!", 1);
}

#
# Helper function to write out a menu.
#
function WRITEOSIDMENU($caption, $value, $osid_result)
{
    echo "<tr>
            <td>*$caption:</td>";
    echo "  <td><select name=$value>\n";

    mysql_data_seek($osid_result, 0);

    while ($row = mysql_fetch_array($osid_result)) {
	$osid = $row[osid];
	$pid  = $row[pid];
	if (!$pid)
	    $pid = "testbed";

	echo "<option value='$osid'>$pid - $osid</option>\n";
    }
    echo "<option selected value=none>No OSID</option>\n";

    echo "       </select>";
    echo "    </td>
          </tr>\n";
}

echo "<blockquote><blockquote>
       Use this page to create a custom Image Identifier (ImageID).
       Once you have one or more OSIDs and an ImageID, you can create your
       own disk image that you can load on your nodes.
      </blockquote></blockquote>\n";
    
echo "<table align=center border=1> 
      <tr>
         <td align=center colspan=2>
             <em>(Fields marked with * are required)</em>
         </td>
      </tr>
     <form action='newimageid.php3' method=post>\n";

#
# ImageID:
#
# Note DB max length.
#
echo "<tr>
          <td>*ImageID (no blanks):</td>
          <td><input type=text name=imageid size=$TBDB_IMAGEID_IMAGEIDLEN
                     maxlength=$TBDB_IMAGEID_IMAGEIDLEN>
              </td>
      </tr>\n";

#
# Select Project
#
echo "<tr>
          <td>*Select Project:</td>";
echo "    <td><select name=pid>";
              for ($i = 0; $i < count($projlist); $i++) {
                  $pid = $projlist[$i];

		  echo "<option value='$pid'>$pid</option>\n";
               }
if ($isadmin) {
	echo "<option value=none>None</option>\n";
}
echo "       </select>";
echo "    </td>
      </tr>\n";

#
# Description
#
echo "<tr>
          <td>*Description:</td>
          <td><input type=text name=description size=50>
              </td>
      </tr>\n";

#
# Load Partition
#
echo "<tr>
          <td>*Starting DOS Slice:<br>
              (1-4, 0 means entire disk)</td>
          <td><input type=text name=loadpart size=2>
              </td>
      </tr>\n";

#
# Load length
#
echo "<tr>
          <td>*Number of DOS Slices:<br>
              (1, 2, 3, or 4)</td>
          <td><input type=text name=loadlength size=2>
              </td>
      </tr>\n";


WRITEOSIDMENU("Slice 1 OSID", "part1_osid", $osid_result);
WRITEOSIDMENU("Slice 2 OSID", "part2_osid", $osid_result);
WRITEOSIDMENU("Slice 3 OSID", "part3_osid", $osid_result);
WRITEOSIDMENU("Slice 4 OSID", "part4_osid", $osid_result);
WRITEOSIDMENU("Default OSID[<b>1</b>]", "default_osid", $osid_result);

#
# Path to Multiboot image.
#
echo "<tr>
          <td>Filename (full path) of Image[<b>2</b>]:<br>
              (must reside in /proj)</td>
          <td><input type=text name=path value='/proj/' size=40>
              </td>
      </tr>\n";

#
# Load Address
#
echo "<tr>
          <td>Multicast Load Address[<b>3</b>]:</td>
          <td><input type=text name=loadaddr size=30 maxlength=30>
              </td>
      </tr>\n";

echo "<tr>
          <td align=center colspan=2>
              <b><input type=submit value=Submit></b>
          </td>
      </tr>\n";

echo "</form>
      </table>\n";

echo "<h4><blockquote><blockquote><blockquote>
      <dl COMPACT>
         <dt>[1]
             <dd>The OSID (slice) that is active when the node boots up.
         <dt>[2]
             <dd>The image file must reside in the project directory so
                 that it is available via NFS.
         <dt>[3]
             <dd>Leave this blank unless you know what it means!
      </dl>
      </blockquote></blockquote></blockquote></h4>\n";
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Image List");

#
#
# Only known and logged in users allowed.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Admin users can see all ImageIDs, while normal users can only see
# ones in their projects or ones that are globally available.
#
$isadmin = ISADMIN($uid);

#
# Get the list.
#
if ($isadmin) {
    $query_result = DBQueryFatal("SELECT * FROM images order by imageid");
}
else {
    $query_result =
	DBQueryFatal("select distinct i.* from images as i ".
		     "left join group_membership as g on g.pid=i.pid ".
		     "where g.uid='$uid' or i.shared order by i.imageid");
}

if (mysql_num_rows($query_result) == 0) {
	if ($isadmin) {
	    USERERROR("There are no ImageIDs!", 1);
	}
	else {
	    USERERROR("There are no ImageIDs in any of your projects!", 1);
	}
}

echo "<table border=2 cellpadding=2 cellspacing=2
       align='center'>\n";

echo "<tr>
          <td>Image</td>
          <td>PID</td>
          <td>Description</td>
      </tr>\n";

while ($row = mysql_fetch_array($query_result)) {
    $imageid    = $row[imageid];
    # Must encode the imageid since Rob started using plus signs in the names.
    $url        = rawurlencode($imageid);
    $descrip    = $row[description];
    $imagename  = $row[imagename];
    $pid        = $row[pid];

    echo "<tr>
              <td><A href='showimageid.php3?imageid=$url'>$imagename</A></td>
              <td>$pid</td>
              <td>$descrip</td>\n";
    echo "</tr>\n";
}
echo "</table>\n";

# Create option.
echo "<br><center>
       <A href='newimageid_explain.php3'>Create a new Image Descriptor?</a>
      </center>\n";

echo "<br><br>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

<html>
<head>
<title>Node Control</title>
<link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
<?php
include("defs.php3");

#
# Only known and logged in users can do this.
#
LOGGEDINORDIE($uid);

#
# Admin users can control other nodes.
#
$isadmin = ISADMIN($uid);
if (! $isadmin) {
    USERERROR("You do not have admin privledges!", 1);
}

echo "<center><h1>
      Node Control Center: $node_id
      </h1></center>";

#
# Check to make sure thats this is a valid nodeid
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM nodes WHERE node_id=\"$node_id\"");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The node $node_id is not a valid nodeid", 1);
}
$row = mysql_fetch_array($query_result);

$node_id            = $row[node_id]; 
$type               = $row[type];
$def_boot_image_id  = $row[def_boot_image_id];
$def_boot_cmd_line  = $row[def_boot_cmd_line];
$next_boot_path     = $row[next_boot_path];
$next_boot_cmd_line = $row[next_boot_cmd_line];

echo "<table border=2 cellpadding=0 cellspacing=2
       align='center'>\n";

#
# Generate the form.
# 
echo "<form action=\"nodecontrol.php3?uid=$uid\" method=\"post\">\n";

echo "<tr>
          <td>Node ID:</td>
          <td class=\"left\"> 
              <input type=\"readonly\" name=\"node_id\" value=\"$node_id\">
              </td>
      </tr>\n";

echo "<tr>
          <td>Node Type:</td>
          <td class=\"left\"> 
              <input type=\"readonly\" name=\"node_type\" value=\"$type\">
              </td>
      </tr>\n";

#
# This should be a menu.
# 
echo "<tr>
          <td>Def Boot Image:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"def_boot_image_id\" size=\"20\"
                     value=\"$def_boot_image_id\"></td>
      </tr>\n";

echo "<tr>
          <td>Def Boot Command Line:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"def_boot_cmd_line\" size=\"40\"
                     value=\"$def_boot_cmd_line\"></td>
      </tr>\n";


echo "<tr>
          <td>Next Boot Path:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"next_boot_path\" size=\"40\"
                     value=\"$next_boot_path\"></td>
      </tr>\n";


echo "<tr>
          <td>Next Boot Command Line:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"next_boot_cmd_line\" size=\"40\"
                     value=\"$next_boot_cmd_line\"></td>
      </tr>\n";


echo "<tr>
          <td colspan=2 align=center>
              <b><input type=\"submit\" value=\"Submit\"></b>
          </td>
     </tr>
     </form>
     </table>\n";
?>
</body>
</html>


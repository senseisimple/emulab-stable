<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Create a Batch Experiment");

#
# Only known and logged in users can begin experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# See what projects the uid is a member of. Must be at least one!
# 
$query_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid=\"$uid\" ".
	"and (trust='local_root' or trust='group_root')");
    
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error finding project membership: $uid: $err\n", 1);
}
if (mysql_num_rows($query_result) == 0) {
    USERERROR("You do not appear to be a member of any Projects in which ".
	      "you have permission (root) to create new experiments.", 1);
}

?>
<table align="center" border="1"> 
<tr>
    <td align="center" colspan="2">
        <h1>Create a new Batch Mode Experiment on the Testbed</h1>
    </td>
</tr>

<tr>
    <td align="center" colspan="3">
        <em>(Fields marked with * are required)</em>
    </td>
</tr>

<?php
echo "<form enctype=\"multipart/form-data\"
            action=\"batchexp.php3\" method=\"post\">\n";

#
# UID to feed back. 
# 
echo "<tr>
          <td>*Username:</td>
          <td class=\"left\"> 
              <input type=\"readonly\" name=\"uid\" value=\"$uid\"></td>
      </tr>\n";

#
# Select Project
#
echo "<tr>
          <td>*Select Project:</td>";
echo "    <td><select name=\"exp_pid\">";
               while ($row = mysql_fetch_array($query_result)) {
                  $project = $row[pid];
                  echo "<option value=\"$project\">$project</option>\n";
               }
echo "       </select>";
echo "    </td>
      </tr>\n";

#
# Experiment ID and Long Name:
#
# Note DB max length.
#
echo "<tr>
          <td>*Name (no blanks):</td>
          <td><input type=\"text\" name=\"exp_id\"
                     size=$TBDB_EIDLEN maxlength=$TBDB_EIDLEN>
              </td>
      </tr>\n";

echo "<tr>
          <td>*Long Name:</td>
          <td><input type=\"text\" name=\"exp_name\" size=\"40\">
              </td>
      </tr>\n";


#
# NS file upload.
# 
echo "<tr>
          <td>*Your NS file (20K max):</td>
          <td><input type=\"hidden\" name=\"MAX_FILE_SIZE\" value=\"20000\">
              <input type=\"file\" name=\"exp_nsfile\" size=\"30\">
              </td>
      </tr>\n";


#
# Expires, Starts, Ends. Also the hidden Created field.
#
$utime     = time();
$year      = date("Y", $utime);
$month     = date("m", $utime);
$thismonth = $month++;
if ($month > 12) {
	$month -= 12;
	$month = "0".$month;
}
$rest = date("d H:i:s", $utime);

echo "<tr>
          <td>Expiration date:</td>
          <td><input type=\"text\" value=\"$year:$month:$rest\"
                     name=\"exp_expires\"></td>
     </tr>\n";

?>

<tr>
    <td align="center" colspan="2">
        <b><input type="submit" value="Submit"></b></td>
</tr>
</form>
</table>

<?php
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

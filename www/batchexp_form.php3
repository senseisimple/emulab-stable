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
# See what projects the uid can create experiments in. Must be at least one.
#
$projlist = TBProjList($uid, $TB_PROJECT_CREATEEXPT);

if (! count($projlist)) {
    USERERROR("You do not appear to be a member of any Projects in which ".
	      "you have permission to create new experiments.", 1);
}
?>

<center>
<h2>For information on <i>Batch Mode</i>, please take a look at the
<a href="tutorial/tutorial.php3#BatchMode">Emulab Tutorial</a></h2>
</center>

<table align="center" border="1"> 
<tr>
    <td align="center" colspan="3">
        <em>(Fields marked with * are required)</em>
    </td>
</tr>

<?php
echo "<form enctype=\"multipart/form-data\"
            action=\"batchexp.php3\" method=\"post\">\n";

#
# Select Project
#
echo "<tr>
          <td colspan=2>*Select Project:</td>";
echo "    <td><select name=\"exp_pid\">";
              for ($i = 0; $i < count($projlist); $i++) {
                  $project = $projlist[$i];

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
          <td colspan=2>*Name (no blanks):</td>
          <td><input type=\"text\" name=\"exp_id\"
                     size=$TBDB_EIDLEN maxlength=$TBDB_EIDLEN>
              </td>
      </tr>\n";

echo "<tr>
          <td colspan=2>*Description:</td>
          <td><input type=\"text\" name=\"exp_name\" size=\"40\">
              </td>
      </tr>\n";


#
# NS file upload.
# 
echo "<tr>
          <td rowspan>*Your NS file: &nbsp</td>

          <td rowspan><center>Upload (20K max)<br>
                                   <br>
                                   Or<br>
                                   <br>
                              On Server (/proj or /users)
                      </center></td>

          <td rowspan>
              <input type=\"hidden\" name=\"MAX_FILE_SIZE\" value=\"20000\">
              <input type=\"file\" name=\"exp_nsfile\" size=\"30\">
              <br>
              <br>
              <input type=\"text\" name=\"exp_localnsfile\" size=\"40\">
              </td>
      </tr>\n";

#
# Expires.
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
          <td colspan=2>Expiration date:</td>
          <td><input type=\"text\" value=\"$year:$month:$rest\"
                     name=\"exp_expires\"></td>
     </tr>\n";

#
# Select a group
# 
echo "<tr>
          <td colspan=2>Group:<br>(leave blank to use default group)</td>
          <td><input type=\"text\" name=\"exp_gid\"
                     size=$TBDB_GIDLEN maxlength=$TBDB_GIDLEN>
              </td>
      </tr>\n";

?>

<tr>
    <td align="center" colspan="3">
        <b><input type="submit" value="Submit"></b></td>
</tr>
</form>
</table>

<p>
<center>
<img alt="*" src="redball.gif"> 
Please <a href="nscheck_form.php3">syntax check</a> your NS file first!
</center>

<?php
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

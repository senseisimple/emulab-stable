<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Syntax Check your NS file");

#
# Only known and logged in users can begin experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

?>
<table align="center" border="1"> 
<tr>
    <td align="center" colspan="2">
        <font size="+1">
               Use this page to syntax check your NS file before
               submitting it.
        </font>
    </td>
</tr>

<?php
echo "<form enctype=\"multipart/form-data\"
            action=\"nscheck.php3\" method=\"post\">\n";

#
# NS file upload.
# 
echo "<tr>
          <td>Your NS file (20K max):</td>
          <td><input type=\"hidden\" name=\"MAX_FILE_SIZE\" value=\"20000\">
              <input type=\"file\" name=\"exp_nsfile\" size=\"30\">
              </td>
      </tr>\n";

?>

<tr>
    <td align="center" colspan="2">
        <b><input type="submit" value="Check"></b></td>
</tr>
</form>
</table>

<?php
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

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
    <td align="center" colspan="3">
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
# NS file upload or on server.
# 
echo "<tr>
          <td rowspan>*Your NS file: &nbsp</td>

          <td rowspan><center>Upload (50K max)<br>
                                   <br>
                                   Or<br>
                                   <br>
                              On Server (/proj or /users)
                      </center></td>

          <td rowspan>
              <input type=\"hidden\" name=\"MAX_FILE_SIZE\" value=\"51200\">
              <input type=\"file\" name=\"exp_nsfile\" size=\"30\">
              <br>
              <br>
              <input type=\"text\" name=\"exp_localnsfile\" size=\"40\">
              </td>
      </tr>\n";

?>

<tr>
    <td align="center" colspan="3">
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

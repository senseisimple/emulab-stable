<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Begin a new Testbed Experiment");

#
# Only known and logged in users can begin experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# See if nsdata was provided. Clear it if an empty string, otherwise
# reencode it.
#
if (isset($nsdata)) {
    if (strcmp($nsdata, "") == 0) 
	unset($nsdata);
    else
	$nsdata = rawurlencode($nsdata);
}

#
# See what projects the uid can create experiments in. Must be at least one.
#
$projlist = TBProjList($uid, $TB_PROJECT_CREATEEXPT);

if (! count($projlist)) {
    USERERROR("You do not appear to be a member of any Projects in which ".
	      "you have permission to create new experiments.", 1);
}

echo "<table align=center border=1> 
      <tr>
          <td align=center colspan=3>
              <em>(Fields marked with * are required)</em>
          </td>
      </tr>\n";

echo "<form enctype=\"multipart/form-data\"
            action=\"beginexp_process.php3\" method=\"post\">\n";

#
# Select Project
#
echo "<tr>
          <td colspan=2>*Select Project:</td>";
echo "    <td><select name=exp_pid>";
	  while (list($project) = each($projlist)) {
	      echo "<option value='$project'>$project </option>\n";
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
# NS file.
#
if (isset($nsdata)) {
    echo "<tr>
            <td colspan=2>*Your Auto Generated NS file: &nbsp</td>
            <input type=hidden name=nsdata value=$nsdata>
            <td><a target=_blank href=spitnsdata.php3?nsdata=$nsdata>
                   View NS File</a></td>
          </tr>\n";
}
else {
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
              <input type=\"file\" name=\"exp_nsfile\" size=\"40\">
              <br>
              <br>
              <input type=\"text\" name=\"exp_localnsfile\" size=\"50\">
              </td>
          </tr>\n";
}

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

echo "<tr>
	  <td colspan=2>Swappable?[<b>1</b>]:</td>
          <td><input type=checkbox name=exp_swappable value=Yep> Yes</td>
     </tr>\n";

echo "<tr>
	  <td colspan=2>Priority[<b>2</b>]:</td>
          <td><input type=radio name=exp_priority value=low checked> <b>Low</b>
              &nbsp &nbsp &nbsp
              <input type=radio name=exp_priority value=high> High</td>
     </tr>\n";

#
# Select a group
# 
echo "<tr>
          <td colspan=2>Group[<b>3</b>]:</td>\n";
echo "    <td><select name=exp_gid>
              <option selected value=''>Default Group </option>\n";
	    reset($projlist);
	    while (list($project, $grouplist) = each($projlist)) {
		for ($i = 0; $i < count($grouplist); $i++) {
		    $group = $grouplist[$i];

		    if (strcmp($project, $group)) {
			echo "<option value='$group'>$project/$group
                              </option>\n";
		    }
		}
	    }
echo "       </select>
          </td>
      </tr>\n";

echo "<tr>
	  <td colspan=2>Batch Experiment?[<b>4</b>]:</td>
          <td><input type=checkbox name=exp_batched value=Yep>&nbsp Yes</td>
     </tr>\n";
         
echo "<tr>
          <td align=center colspan=3>
             <b><input type=submit value=Submit></b></td>
     </tr>
   </form>
 </table>\n";

echo "<h4><blockquote><blockquote><blockquote>
      <dl COMPACT>
        <dt>[1]
            <dd>Check if your experiment can be swapped out and swapped back 
	        in without harm to your experiment. Useful for scheduling when
	        resources are tight. More information on swapping
	        is contained in the 
	        <a href='$TBDOCBASE/faq.php3#UTT-Swapping'>Emulab FAQ</a>.
        <dt>[2]
            <dd>You get brownie points for marking your experiments as Low
                Priority, which indicates that we can swap you out before high
	        priority experiments.
        <dt>[3]
            <dd>Leave as the default group, or pick a subgroup that
                corresponds to the project you selected.
        <dt>[4]
            <dd>Check this if you want to create a
                <a href='$TBDOCBASE/tutorial/tutorial.php3#BatchMode'>
                batch mode</a> experiment.
     </dl>
     </blockquote></blockquote></blockquote></h4>\n";

echo "<p><blockquote>
      <ul>
         <li> Please <a href='nscheck_form.php3'>syntax check</a> your NS
              file first!
         <li> If your NS file is using a custom OSID, you must
              <a href='newosid_form.php3'>create the OSID first!</a>
         <li> You can view a <a href='showosid_list.php3'> list of OSIDs</a>
              that are available for you to use in your NS file.
         <li> You can also view a <a href='showimageid_list.php3'> list of
              ImageIDs.</a> 
      </ul>
      </blockquote>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

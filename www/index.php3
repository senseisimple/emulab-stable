<?php
#
# Beware empty spaces (cookies)!
# 
require("defs.php3");

$login_status = "";

if (isset($login)) {
    #
    # Login button pressed. 
    #
    if (!isset($uid) ||
        strcmp($uid, "") == 0) {
            $login_status = "Login Failed";
	    unset($uid);
    }
    else {
	#
	# Look to see if already logged in. If the user hits reload,
	# we are going to get another login post, and this could
	# update the current login, but the other frame is also reloading,
	# and has sent its cookie values in already. So, now the hash in
	# DB will not match the hash that came with the other frame. 
	#
	if (CHECKLOGIN($uid) == 1) {
            $login_status = "$uid Logged In";
	}
	elseif (DOLOGIN($uid, $password)) {
            $login_status = "Login Failed";
	    unset($uid);
        }
        else {
            $login_status = "$uid Logged In";
        }
    }
}
elseif (isset($logout)) {
    #
    # Logout button pressed.
    #
    DOLOGOUT($uid);
    $login_status = "$uid Logged Out";
    unset($uid);
}
elseif ($uid = GETUID()) {
    #
    # Check to make sure the UID is logged in (not timed out).
    #
    $status = CHECKLOGIN($uid);
    switch ($status) {
    case 0:
        unset($uid);
        break;
    case 1:
        $login_status = "$uid Logged In";
        break;
    case -1:
        $login_status = "$uid Login Timed Out";
        unset($uid);
        break;
    }
}
?>

<html>
<head>
<title>Emulab</title>
<link rel='stylesheet' href='tbstyle.css' type='text/css'>

<?php
#
# So I can test on my home machine easily. This *is* required to make the
# the frames work correctly.
# 
echo "<base href=\"$TBBASE\" target=\"dynamic\">\n";
?>

</head>
<body>
<a href="welcome.html"><h3>Emulab Home</h3></a>

<?php
if (isset($uid)) {
    echo "<hr>";
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT status,admin FROM users WHERE uid='$uid'");
    $row = mysql_fetch_row($query_result);
    $status = $row[0];
    $admin  = $row[1];

    #
    # See if group_root in any projects, not just the last one in the DB!
    #
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT trust FROM proj_memb WHERE uid='$uid' and trust='group_root'");
    if (mysql_num_rows($query_result)) {
        $trusted = 1;
    }
    else {
        $trusted = 0;
    }

    if ($status == "active") {
        if ($admin) {
            echo "<A href='approveproject_list.php3'>
                     New Project Approval</A><p>\n";
            echo "<A href='nodecontrol_list.php3'>
                     Node Control</A><p>\n";
	    echo "<A href='showuser_list.php3'>
		     User List</A>\n";
	    echo "<hr>\n";
        }
        if ($trusted) {
            # Only group leaders can do these options
            echo "<A href='approveuser_form.php3'>
                     New User Approval</A><p>\n";
        }
        # Since a user can be a member of more than one project,
        # display this option, and let the form decide if the user is
        # allowed to do this.
        echo "<A href='showproject_list.php3'>
                    Project Information</A><p>\n";
        echo "<A href='beginexp_form.php3'>
                    Begin an Experiment</A><p>\n";
        echo "<A href='endexp_list.php3'>
                    End an Experiment</A><p>\n";
        echo "<A href='showexp_list.php3'>
                    Experiment Information</A><p>\n";
        echo "<A href='modusr_form.php3'>
                    Update user information</A><p>\n";
        echo "<A href='reserved.php3'>
                    Node Reservation Status</A><p>\n";
        echo "<A href='http://www.cs.utah.edu/~danderse/dnard/status.html'>
                    Node Up/Down Status</A><p>\n";
    }
    elseif ($status == "unapproved") {
        USERERROR("Your account has not been approved yet. ".
                  "Please try back later", 1);
    }
    elseif (($status == "newuser") || ($status == "unverified")) {
        echo "<A href='verifyusr_form.php3'>New User Verification</A>\n";
    }
    elseif (($status == "frozen") || ($status == "other")) {
        USERERROR("Your account has been changed to status $status, and is ".
                  "currently unusable. Please contact your project leader ".
                  "to find out why.", 1);
    }
}

#
# Standard options for anyone.
# 
echo "<p><A href=\"newproject_form.php3\">Start Project</A>\n";
echo "<p><A href=\"addusr.php3\">Join Project</A>\n";

echo "<hr>";
echo "<table cellpadding=\"0\" cellspacing=\"0\" width=\"100%\">";
echo "<form action=\"index.php3\" method=\"post\" target=\"fixed\">";

echo "<b>$login_status</b>";
#
# Present either a login box, or a logout box
# 
if (isset($uid)) {
    echo "<tr>
              <td><input type='hidden' name='uid' value='$uid'</td>
              <td align='center'>
                  <b><input type='submit' value='Logout' name='logout'></b>
              </td>
          </tr>\n";
}
else {
    #
    # Get the UID that came back in the cookie so that we can present a
    # default login name to the user.
    #
    if (($uid = GETUID()) == FALSE)
	$uid = "";

    echo "<tr>
              <td>Username:<input type='text' value='$uid'
                                  name='uid' size=8></td>
          </tr>
          <tr>
              <td>Password:<input type='password' name='password' size=12></td>
          </tr>
          <tr>
              <td align='center'>
                  <b><input type='submit' value='Login' name='login'></b></td>
          </tr>\n";
}
?>
</form>
</table>
</body>
</html>

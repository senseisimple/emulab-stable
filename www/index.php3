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
    unset($login);

    if (!isset($uid) ||
        strcmp($uid, "") == 0) {
            $login_status = "Login Failed";
	    unset($uid);
    }
    else {
        if (DOLOGIN($uid, $password)) {
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
    unset($logout);

    DOLOGOUT($uid);
    $login_status = "$uid Logged Out";
    unset($uid);
}
elseif (isset($uid)) {
    #
    # Check to make sure the UID is logged in (not timed out).
    #
    $status = CHECKLOGIN($uid);
    switch ($status) {
    case 0:
        $login_status = "$uid Not Logged In";
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
<title>Utah Network Testbed</title>
<link rel='stylesheet' href='tbstyle.css' type='text/css'>

<?php
#
# So I can test on my home machine easily.
# 
echo "<base href=\"$TBBASE\" target=\"dynamic\">\n";
?>

</head>
<body>
<a href="welcome.html"><h3>Utah Network Testbed</h3></a>

<?php
if (isset($uid)) {
    echo "<hr>";
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT status FROM users WHERE uid='$uid'");
    $row = mysql_fetch_row($query_result);
    $status = $row[0];

    $query_result = mysql_db_query($TBDBNAME,
	"SELECT trust FROM grp_memb WHERE uid='$uid'");
    $row = mysql_fetch_row($query_result);
    $trust = $row[0];
    
    if ($status == "active") {
        if ($trust == "group_root") {
            # Only group leaders can do these options
            echo "<A href='approval.php3?$uid'>New User Approval</A>\n";
        }
        # Since a user can be a member of more than one project (grp),
        # display this option, and let the form decide if the user is
        # allowed to do this.
        echo "<p><A href='beginexp_form.php3?$uid'>
                    Begin an Experiment</A>\n";
        echo "<p><A href='endexp_form.php3?$uid'>
                    End an Experiment</A>\n";
        # Every active user can do these options.
        echo "<p><A href='showexp_form.php3?$uid'>
                    Show experiment information</A>\n";
        echo "<p><A href='modusr_form.php3?$uid'>
                    Update user information</A>\n";
        echo "<p><A href='reserved.php3'>
                    Node Reservation Status</A>\n";
        echo "<p><A href='http://www.cs.utah.edu/~danderse/dnard/status.html'>
                    Node Up/Down Status</A>\n";
        echo "</p>\n";
    }
    elseif ($status == "unapproved") {
        USERERROR("Your account has not been approved yet. ".
                  "Please try back later", 1);
    }
    elseif (($status == "newuser") || ($status == "unverified")) {
        echo "<A href='verifyusr_form.php3?$uid'>New User Verification</A>\n";
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
if (isset($uid)) {
    echo "<p><A href=\"addgrp.php3?$uid\">Start a Project</A>\n";
    echo "<p><A href=\"addusr.php3?$uid\">Join a Project</A>\n";
}
else {
    echo "<p><A href=\"addgrp.php3\">Start a Project</A>\n";
    echo "<p><A href=\"addusr.php3\">Join a Project</A>\n";
}
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
    echo "<tr>
              <td>Username:<input type='text' name='uid' size=8></td>
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

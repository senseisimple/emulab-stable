<?php
#
# Standard definitions!
#
$TBWWW          = "<https://www.emulab.net/tbdb.html>";
$TBBASE         = "https://www.emulab.net/";
$TBMAIL_CONTROL = "Testbed Ops <testbed-ops@flux.cs.utah.edu>";
$TBMAIL_WWW     = "Testbed WWW <testbed-www@flux.cs.utah.edu>";
$TBMAIL_APPROVE = "Testbed Approval <testbed-approval@flux.cs.utah.edu>";

#$TBBASE         = "http://golden-gw.ballmoss.com:8080/src/testbed/www/";
#$TBMAIL_CONTROL = "Testbed Ops <stoller@fast.cs.utah.edu>";
#$TBMAIL_WWW     = "Testbed WWW <stoller@fast.cs.utah.edu>";
#$TBMAIL_APPROVE = "Testbed Approval <stoller@fast.cs.utah.edu>";

$TBDBNAME       = "tbdb";
$TBDIR          = "/usr/testbed/";
$TBWWW_DIR	= "$TBDIR"."www/";
$TBBIN_DIR	= "$TBDIR"."bin/";

$TBLIST_DIR     = "/usr/testbed/www/maillist";
$TBLIST_LEADERS = "$TBLIST_DIR"."/leaders.txt";
$TBLIST_USERS   = "$TBLIST_DIR"."/users.txt";

$TBUSER_DIR	= "/users/";
$TBNSSUBDIR     = "nsdir";

$TBAUTHCOOKIE   = "HashCookie";
$TBAUTHTIMEOUT  = 10800;
$TBAUTHDOMAIN   = ".emulab.net";
#$TBAUTHDOMAIN   = "golden-gw.ballmoss.com";

#
# Generate the KEY from a name
#
function GENKEY ($name) {
     return crypt("TB_"."$name"."_USR", strlen($name) + 13);
}

#
# Internal errors should be reported back to the user simply. The actual 
# error information should be emailed to the list for action. The script
# should then terminate if required to do so.
#
function TBERROR ($message, $death) {
    global $TBMAIL_WWW;

    if (1) {
    mail($TBMAIL_WWW,
         "TESTBED ERROR REPORT",
         "\n".
         "$message\n\n".
         "Thanks,\n".
         "Testbed WWW\n",
         "From: $TBMAIL_WWW\n".
         "Errors-To: $TBMAIL_WWW");
    }
    # Allow sendmail to run.
    sleep(2); 

    if ($death) {
        die("<br><br><h3>".
            "$message <p>".
            "Could not continue. Please contact ".
            "<a href=\"mailto:testbed-www@flux.cs.utah.edu\">".
                "Testbed WWW (testbed-www@flux.cs.utah.edu)</a>.".
            "</h3><p>");
    }
    return 0;
}

#
# General user errors should print something warm and fuzzy
#
function USERERROR($message, $death) {
    echo "<h3><br><br>
          $message
          <br>
          </h3>";

    echo "<p><p>
          Please contact <a href=\"mailto:testbed-ops@flux.cs.utah.edu\"> 
          Testbed Operations (testbed-ops@flux.cs.utah.edu)</a> 
          if you feel this message is an error.";

    if ($death) {
        echo "</body>
              </html>";
        die("");
    }
}

#
# Is this user an admin type?
#
function ISADMIN($uid) {
    global $TBDBNAME;

    $query_result = mysql_db_query($TBDBNAME,
	"SELECT admin FROM users WHERE uid='$uid'");

    if (! $query_result) {
        $err = mysql_error();
        TBERROR("Database Error getting admin status for $uid: $err\n", 1);
    }

    $row = mysql_fetch_row($query_result);
    $admin  = $row[0];

    return $admin;
}

#
# Beware empty spaces (cookies)!
# 
require("tbauth.php3");
?>

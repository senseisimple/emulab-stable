<?php
#
# Standard definitions!
#
$TBWWW          = "<https://www.emulab.net/tbdb.html>";
$TBMAIL_CONTROL = "Testbed Ops <testbed-ops@flux.cs.utah.edu>";
$TBMAIL_WWW     = "Testbed WWW <testbed-www@flux.cs.utah.edu>";
$TBMAIL_APPROVE = "Testbed Approval <testbed-approval@flux.cs.utah.edu>";
$TBDBNAME       = "tbdb";

$TBLIST_DIR     = "/usr/testbed/www/maillist";
$TBLIST_LEADERS = "$TBLIST_DIR"."/leaders.txt";
$TBLIST_USERS   = "$TBLIST_DIR"."/users.txt";

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
    mail($TBMAIL_WWW,
         "TESTBED ERROR REPORT",
         "\n".
         "$message\n\n".
         "Thanks,\n".
         "Testbed WWW\n",
         "From: $TBMAIL_WWW\n".
         "Errors-To: $TBMAIL_WWW");

    if ($death) {
        die("<br><br><h3>".
            "$message <p>".
            "Could not continue. Please contact $TBMAIL_WWW".
            "</h3><p>");
    }
    return 0;
}
?>

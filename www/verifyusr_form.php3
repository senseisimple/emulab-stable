<html>
<head>
<title>New User Verification</title>
<link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
<?php
include("defs.php3");

#
# Only known and logged in users can be verified.
#
$uid = "";
if (ereg("php3\?([[:alnum:]]+)", $REQUEST_URI, $Vals)) {
    $uid=$Vals[1];
    addslashes($uid);
}
else {
    unset($uid);
}
LOGGEDINORDIE($uid);

?>

<h1>New User Verification</h1>
<p>
The purpose of this page is to verify, for security purposes, that
information given in your application is correct. If you never
received a key at the email address given on your application, please
contact <a href="mailto:testbed-ops@flux.cs.utah.edu"> Testbed Ops
(testbed-ops@flux.cs.utah.edu)</a> for further assistance.
<p>

<?php
echo "<table align=\"center\" border=\"1\">
      <form action=\"verifyusr.php3\" method=\"post\">\n";

echo "<tr>
          <td>Username:</td>
          <td><input type=\"readonly\" name=\"uid\" value=\"$uid\"></td>
      </tr>\n";

echo "<tr>
          <td>Key:</td>
          <td><input type=\"text\" name=\"key\" size=20></td>
      </tr>\n";

echo "<tr>
         <td colspan=\"2\" align=\"center\">
             <b><input type=\"submit\" value=\"Submit\"></b></td>
      </tr>\n";

echo "</form>\n";
echo "</table>\n";
?>
</body>
</html>





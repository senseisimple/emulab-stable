<?php
chdir("..");
require("defs.php3");
chdir("doc");

#
# Standard Testbed Header
#
if (!$printable) {
    PAGEHEADER("Emulab Documentation");
}

#
# Need to sanity check the path! For now, just make sure the path
# does not start with a dot or a slash.
#
$first = substr($docname, 0, 1);
if (strcmp($first, ".") == 0 ||
    strcmp($first, "/") == 0) {
    USERERROR("Invalid document name: $docname!", 1);
}
#
# Nothing that looks like a ../ is allowed anywhere in the name
#
if (strstr($docname, "../")) {
    USERERROR("Invalid document name: $docname!", 1);
}

if ($printable) {
    #
    # Need to spit out some header stuff.
    #
    echo "<html>
          <head>
  	  <link rel='stylesheet' href='../tbstyle-plain.css' type='text/css'>
          </head>
          <body>\n";
}
else {
	echo "<b><a href=$REQUEST_URI&printable=1>
                 Printable version of this document</a></b><br>";
}

readfile("$docname");

#
# Standard Testbed Footer
# 
if ($printable) {
    echo "</body>
          </html>\n";
}
else {
    PAGEFOOTER();
}
?>


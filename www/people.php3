<?php
require("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("People Power");

function FLUXPERSON($person, $name) {
    echo "<li> <a href=\"http://www.cs.utah.edu/~$person\">$name</a>\n";
}

echo "<h3>Faculty:</h3>\n";
echo "<ul>\n";
FLUXPERSON("lepreau", "Jay Lepreau");
echo "</ul>\n";

echo "<h3>Students and Staff:</h3>\n";
echo "<ul>\n";
FLUXPERSON("danderse", "Dave Andersen");
FLUXPERSON("calfeld", "Chris Alfeld");
FLUXPERSON("barb", "Chad Barb");
FLUXPERSON("mike", "Mike Hibler");
FLUXPERSON("newbold", "Mac Newbold");
FLUXPERSON("ricci", "Robert Ricci");
FLUXPERSON("stoller", "Leigh Stoller");
echo "</ul>\n";

echo "<h3>Alumni:</h3>\n";
echo "<ul>\n";
FLUXPERSON("sclawson", "Steve Clawson");
FLUXPERSON("vanmaren", "Kevin Van Maren");
FLUXPERSON("kwright", "Kristin Wright");
echo "</ul>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>


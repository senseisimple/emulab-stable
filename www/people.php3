<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("People Power");

function FLUXPERSON($person, $name) {
    echo "<li> <a href=\"http://www.cs.utah.edu/~$person\">$name</a>\n";
}

function PARTFLUXPERSON($person, $name, $other) {
    echo "<li> <a href=\"http://www.cs.utah.edu/~$person\">$name</a> ($other)\n";
}

function MAILPERSON($person, $name) {
    echo "<li> <a href=\"mailto:$person\">$name</a>\n";
}

echo "<h3>Faculty:</h3>\n";
echo "<ul>\n";
FLUXPERSON("lepreau", "Jay Lepreau");
echo "</ul>\n";

echo "<h3>Students and Staff:</h3>\n";
echo "<ul>\n";
PARTFLUXPERSON("calfeld", "Chris Alfeld", "Univ. Wisconsin");
PARTFLUXPERSON("danderse", "Dave Andersen", "MIT");
FLUXPERSON("davidand", "David Anderson");
FLUXPERSON("rchriste", "Russ Christensen");
FLUXPERSON("aclement", "Austin Clements");
FLUXPERSON("shash", "Shashi Guruprasad");
FLUXPERSON("mike", "Mike Hibler");
FLUXPERSON("abhijeet", "Abhijeet Joglekar");
MAILPERSON("rolke@gmx.net", "Roland Kempter");
FLUXPERSON("newbold", "Mac Newbold");
FLUXPERSON("ricci", "Robert Ricci");
FLUXPERSON("stoller", "Leigh Stoller");
FLUXPERSON("kwebb", "Kirk Webb");
echo "</ul>\n";

echo "<h3>Alumni:</h3>\n";
echo "<ul>\n";
PARTFLUXPERSON("barb", "Chad Barb", "Sensory Sweep");
PARTFLUXPERSON("sclawson", "Steve Clawson", "Alcatel");
PARTFLUXPERSON("ikumar", "Indrajeet Kumar", "Qualcomm");
PARTFLUXPERSON("vanmaren", "Kevin Van Maren", "Unisys");
PARTFLUXPERSON("imurdock", "Ian Murdock", "Progeny");
PARTFLUXPERSON("bwhite", "Brian White", "Cornell");
# Some consulting for UCB; don't know how much.
FLUXPERSON("kwright", "Kristin Wright");
echo "</ul>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("People Power");

function FLUXPERSON($login, $name) {
    echo "<li> <a href=\"http://www.cs.utah.edu/~$login\">$name</a>\n";
}

function PARTFLUXPERSON($login, $name, $where) {
    echo "<li> <a href=\"http://www.cs.utah.edu/~$login\">$name</a> ($where)\n";
}

function EXFLUXPERSON($url, $name, $where) {
    echo "<li> <a href=\"$url\">$name</a> ($where)\n";
}

function MAILPERSON($login, $name) {
    echo "<li> <a href=\"mailto:$login\">$name</a>\n";
}

echo "<h3>Faculty:</h3>\n";
echo "<ul>\n";
FLUXPERSON("lepreau", "Jay Lepreau");
echo "</ul>\n";

echo "<h3>Students and Staff:</h3>\n";
echo "<ul>\n";
PARTFLUXPERSON("calfeld", "Chris Alfeld", "Univ. Wisconsin");
PARTFLUXPERSON("danderse", "David G. Andersen", "MIT");
FLUXPERSON("davidand", "David S. Anderson");
FLUXPERSON("rchriste", "Russ Christensen");
FLUXPERSON("aclement", "Austin Clements");
FLUXPERSON("duerig", "Jonathon Duerig");
FLUXPERSON("shash", "Shashi Guruprasad");
FLUXPERSON("mike", "Mike Hibler");
MAILPERSON("rolke@gmx.net", "Roland Kempter");
FLUXPERSON("newbold", "Mac Newbold");
FLUXPERSON("ricci", "Robert Ricci");
FLUXPERSON("stoller", "Leigh Stoller");
FLUXPERSON("kwebb", "Kirk Webb");
FLUXPERSON("fish", "Russ Fish");
FLUXPERSON("saggarwa", "Siddharth Aggarwal");
FLUXPERSON("vaibhave", "Vaibhave Agarwal");
echo "</ul>\n";

echo "<h3>Alumni:</h3>\n";
echo "<ul>\n";
PARTFLUXPERSON("abhijeet", "Abhijeet Joglekar", "Intel");
PARTFLUXPERSON("barb", "Chad Barb", "Sensory Sweep");
PARTFLUXPERSON("sclawson", "Steve Clawson", "Alcatel");
PARTFLUXPERSON("ikumar", "Indrajeet Kumar", "Qualcomm");
PARTFLUXPERSON("vanmaren", "Kevin Van Maren", "Unisys");
EXFLUXPERSON("http://ianmurdock.com/", "Ian Murdock", "Progeny");
EXFLUXPERSON("http://www.csl.cornell.edu/~bwhite/", "Brian White", "Cornell");
# Some consulting for UCB; don't know how much.
FLUXPERSON("kwright", "Kristin Wright");
echo "</ul>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

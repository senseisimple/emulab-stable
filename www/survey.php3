<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Only known and logged in users can do this.
#
$effuid = "unknown";
$uidnotes = "No UID Provided.";
$uid = GETUID(); 
# GETLOGIN();

if ($uid) {
  $effuid = $uid; 
  $uidnotes = "Checklogin returned " . CHECKLOGIN($uid) . ".";
}

$intro = "How would adding the following to Emulab affect you, now or in future?";

$answers = array(
  'Choose one',
  'Not necessary',
  'Might help me a little, but not that much',
  'Would definitely help',
  'Would let me do experiments I want to do, but can\'t'
);

$questions = array(
  'More nodes',
    '"There are not enough free when I need them."',
  'More nodes',
    '"I want to run experiments with more nodes than Emulab has (170)."',
  'More powerful nodes',
    'Faster, more RAM, bigger disks, etc (add specifics in text box below)',
  'Gigabit Ethernet links',
    'If you want faster than Gbit, say so in text box below.',
  'Nodes with more physical network interfaces',
    '',
  'Virtual links/nodes <i>instead of</i> more physical resources',
    'Approx 10 virtual per each physical link/node, without performance guarantees',
  '"Real" commercial routers instead of just PCs',
    '',
  'Better user support',
    '',
  'Better/more organized documentation',
    ''
);

$followups = array(
'Any elaborations, comments, or suggestions?',

'If we were to expand as you suggested above, what kinds
 of qualitatively new experiments could you perform, if any?',

'Besides scale, are there any Emulab characteristics you were forced to
 "work around," or that prevented you from performing an experiment you
 wanted to?',

'What are we doing right?<br />Are there things we do well that you
 would like us to continue to focus on?'
);

PAGEHEADER("Emulab Survey");  

# echo "<h1>I think you are $effuid</h1>";

if ($submit) {

    $mesg = "";
    if ($anonymous) {
	$mesg .= "\nSurvey Responder: *Anonymous* (uid hash ='" . md5($effuid) . "' )\n";
	$mesg .= "Auth notes: $uidnotes\n";
    } else {
        $mesg .= "\nSurvey Responder: $effuid\n";
	$mesg .= "Auth notes: $uidnotes\n";
	$mesg .= "Remote IP: $REMOTE_ADDR\n";
    }
    if (strcmp($name,"")) {
    	$mesg .= "User-supplied name: $name\n";
    }
    if (strcmp($email,"")) {
    	$mesg .= "User-supplied email: $email\n";
    }
    $mesg .= "> Multiple Choice:\n";
    $foo = 0;
    for( $n = 0; $n < count($questions) - 1; $n += 2) {
	$foo++;
	$questionWord =  $questions[$n];

	$answerVar = "opt$n";
	$answerNum = $$answerVar;
	$answerWord = $answers[$answerNum];

	$questionWord = wordwrap( preg_replace( "/[\r\n\t ]+/", " ", $questionWord ), 70, "\n>    ");

	$mesg .= "> ($foo) " . $questionWord . "\n" .
		 "$answerNum. " . $answerWord   . "\n\n";
    }

    $mesg .= "\n\n> Free Response:\n";
    $foo = 0;
    for( $n = 0; $n < count($followups); $n++) {
	$foo++;
	$questionWord = $followups[$n];
	$answerVar = "fu$n";
	$answerWord = $$answerVar;

	$questionWord = wordwrap( preg_replace( "/[\r\n\t ]+/", " ", $questionWord ), 70, "\n>    ");
	$answerWord   = wordwrap( preg_replace( "/[\r\n\t ]+/", " ", $answerWord ),   70, "\n");

	$mesg .= "> (FR$foo) " . $questionWord . "\n\n" .
		 $answerWord . "\n\n\n";
    }

    if ($uid && ! $anonymous) {
	TBUserInfo($uid,$name,$mail);
	if ($name != "" && $mail != "") {
	    $from = "\"$name\" <$mail>";
	} else {
	    $from = "$uid@emulab.net";
	}
    } else {
	$from = "$TBMAIL_OPS";
    }
    
#   testbed-survey@emulab.net
#    TBMAIL("barb@flux.utah.edu",
    TBMAIL("testbed-survey@emulab.net",
	   "Survey Answers",
	   $mesg,
	   "From: $from\n".
	   "Reply-To: $TBMAIL_OPS");

    if ($anonymous) {
	echo "<i>Answers reported anonymously.</i><br />\n";
    }

    echo "<h2>Thank you for filling out the survey!</h2>".
         "<h4>Please feel free to ".
	 "<a href='mailto:testbed-ops@flux.utah.edu'>".
         "Email testbed-ops</a> with any further comments.</h4>";

    PAGEFOOTER();
    return;
}

echo "<form action='survey.php3' method='post'>";

if (0 == strcmp($effuid, "unknown")) {
    echo "<h3>If you have an account, please ";
    echo "<a href=\"$TBBASE/login.php3?refer=1\">log in</a>. ";
    echo "If you don't have an account or don't remember your password, ";
    echo "don't worry-- fill out the form anyway!</h3>\n";
    echo "<p>If you don't have an account, you can\n";
    echo "provide the following infomation: (<i>Not Required)</i></p>\n";
    echo "<center><table>";
    echo "<tr><th>Name</th><td><input name=\"name\"\></td></tr>\n";
    echo "<tr><th>Email</th><td><input name=\"email\"\></td>\n";
    echo "</table></center>";

}

echo "<p>";
echo "We are planning to grow the Emulab cluster and need you to tell us what's
    important to you-- or why it still won't be much good for your research or
    teaching.  Please answer at least the multiple choice section; it's quick!
    If you've never used Emulab, be sure to tell us why, in a text box below.";
  
echo "</p>";

echo "<center>";
echo "<input type='checkbox' name='anonymous' value='1'><b>Report my answers anonymously</b></input>";
echo "<h2>$intro</h2>";
echo "<table>";


for( $n = 0; $n < count($questions) - 1; $n += 2) {
    $q =  $questions[$n];
    $q2 = $questions[$n + 1];
    echo "<tr>";
    echo "<td>";
    echo "<font size='-1'><b>$q</b></font><br />\n";
    echo "<font size='-2'>$q2</font></td>";

    echo "<td>
          <select name='opt$n'>";
    $an = 0;
    $foo = "selected";
    foreach ($answers as $a) {
	echo "<option value='$an' $foo>$a</option>\n";
	$foo = "";
	$an++;
    }
    echo "</select></td>";
    echo "</tr>";
}

echo "</table>";
echo "<br><br>";
echo "<table class='stealth' cellpadding=0 cellspacing=0 border=0 width=50%>";
for( $n = 0; $n < count($followups); $n++) {
    $fu = $followups[$n];
    echo "<tr><td align='center' class='stealth'>";
    echo "<b>$fu</b>";
    echo "</td></tr><tr><td align='center' class='stealth'>";
    echo "<textarea name='fu$n' cols='60' rows='8'></textarea>";
    echo "</td></tr>";
    echo "<tr><td class='stealth'><br />&nbsp;</tr></tr>";
}
echo "</table>";

echo "<div style='border: 1px solid #000000; padding: 8px; background-color: #FFFFE7;'>";
echo "<font size='+1'><b>Thanks for your feedback!</b></font><br /><br />\n";

echo "<input type='submit' name='submit' value='   Submit   '>";
echo "</div>";
echo "</form>";
echo "</center>";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>


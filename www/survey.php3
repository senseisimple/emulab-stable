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

#LOGGEDINORDIE($uid, CHECKLOGIN_USERSTATUS|CHECKLOGIN_WEBONLY);
#$isadmin = ISADMIN($uid);
#$checklogin = CHECKLOGIN($uid);

$intro = "How would adding the following to Emulab/Netbed affect you?";

$answers = array(
  'Don\'t Care',
  'Not necessary / Already good enough',
  'I wouldn\'t use it',
  'Would be nice, but wouldn\'t affect my Netbed use much',
  'Would have a definite positive effect on my Netbed use',
  'Would let me do experiments I want to do, but can\'t'
);

$questions = array(
  'More Emulab nodes',
    '"There are not enough free when I need them."',
  'More Emulab nodes',
    '"I want to run experiments with more nodes than Emulab has."',
  'More powerful Emulab nodes',
    'Faster, more RAM, bigger disks, etc.',
  'Faster network links',
    'For instance, Gigabit Ethernet',
  'Nodes with more network interfaces',
    '',
  'More diverse network hardware',
    'For instance, "real" routers',
  'More wide-area nodes',
    '',
  'More diversity in wide-area nodes',
    'Such as more consumer-type links',
  'Better user support',
    '',
  'Better documentation',
    '',
  'Fixed-location wireless nodes',
    '',
  'Mobile wireless nodes',
    '',
  'Network instrumentation',
    'Automatic tools to monitor network performance',
  'Better experiment control',
    'More tools for changing a running experiment or controlling your software',
  'Better monitoring',
     'Such things as built-in support for capturing and replaying packet and network characteristic traces',    
  'More extensive traffic generation capabilities',
     '',
  'More powerful or flexible traffic shaping',
     '',
  'More persistent state between swap-out and swap-in',
     '(especially node disk contents)',
  'Programmatic API to creating and controlling experiments',
     '',
  'Group collaboration tools',
     'Including source control and bug tracking (e.g., SourceForge)'
);

$followups = array(
'Any comments about your ratings above?<br />Anything we left out?',

'If we were to expand in the directions you indicated above, would this enable
 you to perform qualitatively new experiments? What types?',

'Are there any characteristics of Netbed you were forced to "work around", or
 that prevented you from being able to perform an experiment you wanted to?',

'What are we doing right?<br />Has Netbed helped you to perform experiments that you
 wouldn\'t have otherwise been able to do? Are there things we do well that you
 would like us to continue to focus on?',

'Any other comments?'
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

#   testbed-survey@emulab.net
#    TBMAIL("barb@flux.utah.edu",
    TBMAIL("testbed-survey@emulab.net",
	   "Survey Answers",
	   $mesg,
	   "From: $TBMAIL_OPS");	      

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

if (0 == strcmp($effuid, "unknown")) {
    echo "<h3>If you have an account, please ";
    echo "<a href=\"$TBBASE/login.php3?refer=1\">log in</a>. ";
    echo "If you don't have an account, or can't log in, ";
    echo "please fill out this form anyway!</h3>\n";
}

echo "<p>";
echo "We're exploring our options for the future development and growth of Netbed.
      And, we'd like to make sure we focus on the things that are important
      to researchers and educators. So, we'd appreciate it if you would take
      a moment of your time to give us some feedback.";
  
echo "</p>";

echo "<center>";
echo "<form action='survey.php3' method='post'>";
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


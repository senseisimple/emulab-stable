<?php
#
# Wrapper script for cvsweb.cgi
#
chdir("../");
require("defs.php3");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

if (! TBCvswebAllowed($uid)) {
        USERERROR("You do not have permission to use cvsweb!", 1);
}

$script = "cvsweb.cgi";

#
# Sine PHP helpfully scrubs out environment variables that we _want_, we
# have to pass them to env.....
#
$query = escapeshellcmd($QUERY_STRING);
$path = escapeshellcmd($PATH_INFO);
$name = escapeshellcmd($SCRIPT_NAME);
$agent = escapeshellcmd($HTTP_USER_AGENT);
$encoding = escapeshellcmd($HTTP_ACCEPT_ENCODING);

#
# Helpfully enough, escapeshellcmd doesn't escape spaces. Sigh.
#
$script = preg_replace("/ /","\\ ",$script);
$query = preg_replace("/ /","\\ ",$query);
$name = preg_replace("/ /","\\ ",$name);
$agent = preg_replace("/ /","\\ ",$agent);
$encoding = preg_replace("/ /","\\ ",$encoding);

$output = `env PATH=./cvsweb/ QUERY_STRING=$query PATH_INFO=$path SCRIPT_NAME=$name HTTP_USER_AGENT=$agent HTTP_ACCEPT_ENCODING=$encoding $script`;

#
# Yuck. Since we can't tell php to shut up and not print headers, we have to
# 'merge' headers from cvsweb with PHP's. And, since preg_match returns
# totally unhelpful results, we have to split it up into lines and iterate
# through them. Again, yuck!
#
$array = split("\n",$output);
$headers_done = 0;
for ($i = 0; $i < count($array); $i++) {
	#
	# A blank line signifies the end of headers
	#
	if (!preg_match("/\w+/",$array[$i])) {
		$headers_done = 1;
	} else {
		#
		# If it's a header, we use the PHP header() function to add it
		# to the list of headers that PHP maintains. Otherwise, we
		# just print it out.
		#
		if (!$headers_done) {
			header($array[$i]);
		} else {
			echo "$array[$i]\n";
		}
	}
}

?>

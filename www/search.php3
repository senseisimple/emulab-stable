<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2008 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("submit",      PAGEARG_STRING,
				 "query",       PAGEARG_STRING);
#
# Utah now stores its documentation in a Trac wiki. The Utah web site
# and web sites that link to Utahs documentation will use the Trac
# search engine at Utah. Optionally, if you have the docs locally
# in html format, continue to use the swish search tools on it. Note that
# the knowledge base in the DB is also deprecated; all of that content
# was pushed into the wiki.
#

#
# We no longer support an advanced search option. We might bring it back
# someday.
#
function SPITSEARCHFORM($query)
{
    echo "<table align=center border=1>
          <form action=search.php3 method=get>\n";

    $query = htmlspecialchars($query);

    #
    # Just the query please.
    #
    echo "<tr>
             <td class=left>
                 <input type=text name=query value=\"$query\"
                        size=25 maxlength=100>
               </td>
           </tr>\n";
    
    echo "<tr>
              <td align=center>
                 <b><input type=submit name=submit value='Submit Query'></b>
              </td>
          </tr>\n";

    echo "</form>
          </table><br>\n";
}

if (!isset($query) || $query == "") {
    PAGEHEADER("Search Emulab Documentation");
    SPITSEARCHFORM("");
    PAGEFOOTER();
    return;
}

if ($TBMAINSITE || $REMOTEWIKIDOCS) {
    $query = htmlspecialchars($query);
    header("Location: search_cse.php?query=$query");
    return;
}

#
# Standard Testbed Header after possible redirect above.
#
PAGEHEADER("Search Emulab Documentation");

# Sanitize for the shell. Be fancy later.
if (!preg_match("/^[-\w\ \"]+$/", $query)) {
    SPITSEARCHFORM("");
    PAGEFOOTER();
    return;
}

#
# Run the query. We get back html we can just spit out.
#
#
# A cleanup function to keep the child from becoming a zombie, since
# the script is terminated, but the children are left to roam.
#
$fp = 0;

function CLEANUP()
{
    global $fp;

    if (!$fp || !connection_aborted()) {
	exit();
    }
    pclose($fp);
    exit();
}
ignore_user_abort(1);
register_shutdown_function("CLEANUP");

SPITSEARCHFORM($query);
flush();

$safe_query  = escapeshellarg($query);

echo "<br>\n";
echo "<font size=+2>Documentation search results</font><br>\n";

if ($fp = popen("$TBSUEXEC_PATH nobody nobody websearch $safe_query", "r")) {
    while (!feof($fp)) {
	$string = fgets($fp, 1024);
	echo "$string";
	flush();
    }
    pclose($fp);
    $fp = 0;
}
else {
    TBERROR("Query failed: $query", 0);
}

#
# Add search to the browser's toolbar
#
echo "
<script type='text/javascript' language='javascript'>
function addSearch() {
    if (window.external &&
        ('AddSearchProvider' in window.external)) {
            window.external.AddSearchProvider('$TBBASE/emusearch.xml');
    } else {
        alert('Sorry, your web browser does not support Opensearch');
    }
}
</script>

<p>
<a onclick='addSearch();'>Add the Emulab search engine to your browser's
    toolbar</a>
</p>
";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>


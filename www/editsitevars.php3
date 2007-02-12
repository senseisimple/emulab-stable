<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Edit Site Variables");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

if (! $isadmin) {
    USERERROR("You do not have admin privileges to edit site variables!", 1);
}

#
# Verify Page agruments.
#
$optargs = OptionalPageArguments("edit",       PAGEARG_STRING,
				 "name",       PAGEARG_STRING,
				 "value",      PAGEARG_ANYTHING,
				 "defaulted",  PAGEARG_STRING,
				 "edited",     PAGEARG_STRING,
				 "canceled",   PAGEARG_STRING);

if (isset($edit)) {

    $result = DBQueryFatal("SELECT name, value, defaultvalue, description ".
			   "FROM sitevariables ".
			   "WHERE name='$edit'");

    if ($row = mysql_fetch_row($result)) {
	$name         = $row[0];
	$value        = $row[1];
	$defaultvalue = $row[2];
	$description  = $row[3];

	echo "<center>";
	echo "<form action='editsitevars.php3' method='post'>";
	echo "<input type='hidden' name='name' value='$name'></input>";
	echo "<table><tr><th colspan=2>";
	echo "Editing site variable '<b>$name</b>':";
	echo "</th></tr><tr><td>";
	echo "<b>Description:</b></td><td>$description</font>";
	echo "</td></tr><tr><td>";
	echo "<b>Default value:</b></td><td>";
	if (0 != strcmp($defaultvalue,"")) {
	    echo "<code>$defaultvalue</code>";
	} else {
	    echo "<font color='#00B040'><i>Empty String</i></font>";	    
	}
	echo "</td></tr><tr><td>&nbsp;</td><td>";
	echo "<input type='submit' name='defaulted' value='Reset to Default Value'></input>";
	echo "</td></tr><tr><td>";
	echo "<b>New value:</b></td><td>";
	echo "<input size='60' type='text' name='value' value='$value'></input>";
	echo "</td></tr><tr><td>&nbsp;</td><td>";
	echo "<input type='submit' name='edited' value='Change to New Value'></input>";
	echo "&nbsp;";
	echo "<input type='submit' name='canceled' value='Cancel'></input>";
	echo "</td></tr></table>";
	echo "</form>";
	echo "</center>";

    } else {
	TBERROR("Couldn't find variable '$edit'!");
    }

    PAGEFOOTER();
    return;
}

if (isset($edited)) {
    $value = addslashes("$value");
    
    DBQueryFatal("UPDATE sitevariables ".
		 "SET value='$value' ".
		 "WHERE name='$name'");

    echo "<h3>'$name' changed.</h3>";
    echo "<form action='editsitevars.php3' method='get'>";
    echo "<input type='submit' name='yadda' value='Return to list'></input>";
    echo "</form>";
    PAGEFOOTER();
    return;
}

if (isset($defaulted)) {
    DBQueryFatal("UPDATE sitevariables ".
		 "SET value=NULL ".
		 "WHERE name='$name'");

    echo "<h3>'$name' reset to default.</h3>";
    echo "<form action='editsitevars.php3' method='get'>";
    echo "<input type='submit' name='yadda' value='Return to list'></input>";
    echo "</form>";
    PAGEFOOTER();
    return;
}

if (isset($canceled)) {
    echo "<h3>Operation canceled.</h3>";
    echo "<form action='editsitevars.php3' method='get'>";
    echo "<input type='submit' name='yadda' value='Return to list'></input>";
    echo "</form>";
    PAGEFOOTER();
    return;
}

$result = DBQueryFatal("SELECT name, value, defaultvalue, description ".
		       "FROM sitevariables ".
		       "ORDER BY name");

echo "<table>
      <tr>
        <th>&nbsp;Name&nbsp;</th>
        <th>&nbsp;Value&nbsp;</th>
        <th><font size='-1'>Edit</font></th>
        <th>&nbsp;Default Value&nbsp;</th>
        <th>&nbsp;Description&nbsp;</th> 
      </tr>";
 
while ($row = mysql_fetch_row($result)) {
    $name         = $row[0];
    $value        = $row[1];
    $defaultvalue = $row[2];
    $description  = $row[3];
    $cginame      = urlencode($name);

    echo "<tr><td>&nbsp;<b>$name</b>&nbsp;</td>\n";

    echo "<td>&nbsp;";
    if (isset($value)) {
	if (0 != strcmp($value, "")) {
	    $wrapped_value = wordwrap($value, 30, "<br />", 1);
	    echo "<code>$wrapped_value</code>&nbsp;</td>";
	} else {
	    echo "<font color='#00B040'><i>empty string</i></font>&nbsp;</td>";	
	}
    } else {
	echo "<font color='#00B040'><i>default</i></font>&nbsp;</td>";
    }
    echo "<td align=center>";
    echo "&nbsp;<a href='editsitevars.php3?edit=$cginame'>";
    echo "<img border='0' src='greenball.gif' /></a>";
    echo "&nbsp;</td>";

    echo "<td nowrap='1'>&nbsp;";
    if (0 != strcmp($defaultvalue, "")) {
	echo "<code>$defaultvalue</code>&nbsp;</td>";
    } else {
	echo "<font color='#00B040'><i>empty string</i></font>&nbsp;</td>";	
    }

    echo "<td>&nbsp;$description&nbsp;</td></tr>\n";
}

echo "</table>";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
			

<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

$NSGEN         = "webnsgen";
$TMPDIR        = "/tmp/";

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify Page Arguments.
#
$reqargs = RequiredPageArguments("template",        PAGEARG_STRING);
$optargs = OptionalPageArguments("submit",          PAGEARG_STRING,
				 "advanced",        PAGEARG_STRING,
				 "templatevalues",  PAGEARG_ARRAY);

if (!isset($advanced)) {
    $advanced = 0;
}

#
# Check to see if the template file actually exists
#
$templatefile = "$TBDIR/etc/nsgen/$template.xml";
if (!file_exists($templatefile)) {
    USERERROR("No such template",1);
}

#
# Grab the variable names from the template
#
$xml_parser = xml_parser_create();
xml_set_element_handler($xml_parser, "startElement", "endElement");
xml_set_character_data_handler($xml_parser,"cdata");

#
# Parse the XML template file. This is super-de-dooper lame, but it comes
# directly from the PHP manual
#
if (!($fp = fopen($templatefile, "r"))) {
   TBERROR("Could not open template file",1);
}

$templateorder = array();
$templatefields = array();

while ($data = fread($fp, 4096)) {
   if (!xml_parse($xml_parser, $data, feof($fp))) {
       TBERROR(sprintf("XML error: %s at line %d",
                   xml_error_string(xml_get_error_code($xml_parser)),
                   xml_get_current_line_number($xml_parser)),1);
   }
}
xml_parser_free($xml_parser);


#
# Body - Either make the NS file or ask the user for the form parameters 
#
if (isset($submit) &&
    ($submit == "Begin Experiment" || $submit == "Show NS File")) {
    global $TMPDIR;
    $nsref = MAKENS($template,$templatefields,$templatevalues);
    if ($submit == "Begin Experiment") {
        BEGINEXP($nsref);
    } else {
        $filename =  $TMPDIR . $uid . "-$nsref.nsfile";
        header("Content-type: text/plain");
        readfile($filename);
        unlink($filename);
    }
} else {
    #
    # Just spit out the form!
    #

    PAGEHEADER("Automatic NS file Generation");

    #
    # JavaScript for the Show NS option
    #
    echo "<script language=JavaScript>
          <!--
            function NormalSubmit() {
              document.form1.target='_self';
              document.form1.submit();
            }
            function ShowNS() {
                var oldtarget = document.form1.target;
                document.form1.target='nsgen';
                document.form1.submit();
                document.form1.target = oldtarget;
            }
          //-->
          </script>\n";

    SPITFORM($advanced,$templatefields,$templateorder);

    PAGEFOOTER();
}


function SPITFORM($advanced,$templatefields,$templateorder) {
    global $template;
    global $templatename;
    global $templateauthor;
    global $templatedescr;

    echo "<h3 align=center>$templatename";
    if ($templateauthor != "") {
        echo " - $templateauthor";
    }
    echo "</h3>\n";

    if (isset($templatedescr)) {
        echo "<p>$templatedescr</p>\n";
    }

    echo "<form name='form1' action='nsgen.php3' method='get'>\n";
    echo "<input type='hidden' name='template' value='$template'>\n";
    echo "<table align=center border=1>\n";
    echo "<tr><th colspan=2>Template Parameters</th></tr>";
    for ($i = 0; $i < sizeof($templateorder); $i++) {
        $param = $templateorder[$i];
        if (isset($templatefields[$param]['advanced']) &&
	    $templatefields[$param]['advanced'] && $advanced = 0) {
            continue;
        }
        
        $default = $templatefields[$param]['default'];
        $descr = $templatefields[$param]['descr'];

        echo "<tr>";
        echo "<td>$descr</td>\n";
        echo "<td><input type=text name=templatevalues[$param] ";
        echo "           value=\"$default\">";
        echo "</tr>\n";
    }

    echo "<tr><th colspan=2><center>";
    echo "<input type=submit name=submit value='Begin Experiment' onclick=\"NormalSubmit();\">&nbsp;";
    echo "<input type=submit id=showns name=submit value='Show NS File' onclick=\"ShowNS();\">&nbsp;";
    echo "<input type=reset name=reset value='Reset'>";
    echo "</center></th></tr>\n";

    echo "</form></table>\n";
}

function cdata($parser,$data)
{
    global $current_cdata;

    #
    # Just stash CDATA in a global so that we can pull it back out in the
    # endElement handler. This gets cleaned in the startElement handler
    #
    $current_cdata .= $data;
}

function endElement($parser,$name)
{
    global $current_cdata;
    global $templatedescr;
    if ($name == "DESCRIPTION") {
        $templatedescr = $current_cdata;
    }

}

function startElement($parser,$name,$attrs)
{
    global $templatefields;
    global $templateorder;
    global $templatename;
    global $templateauthor;
    global $template;
    global $current_cdata;
    $current_cdata = "";

    if ($name == "VARIABLE") {

        #
        # Lets assume the template file is well-formed. I should write a DTD
        # and validate it.
        #
        $varname = $attrs['NAME'];
        if (isset($attrs['DEFAULT'])) {
            $default = $attrs['DEFAULT'];
        } else {
            $default = "";
        }

        if (isset($attrs['DESCR'])) {
            $descr = $attrs['DESCR'];
        } else {
            $descr = $varname;
        }

        #
        # Stick all of this stuff into a structure so that we can easily
        # look it all up
        #
        $templatefields[$varname] = array('default' => $default,
                                          'descr' => $descr);
        
        #
        # Keep track of the order we saw them in
        #
        array_push($templateorder,$varname);
    } elseif ($name == "NSTEMPLATE") {
        #
        # Look for the template name
        #
        if (isset($attrs['NAME'])) {
            $templatename = $attrs['NAME'];
        } else {
            $templatename = $template;
        }

        #
        # Figure out author information
        #
        $templateauthor = "";
        if (isset($attrs['AUTHOR'])) {
            if (isset($attrs['AUTHORUID'])) {
                $templateauthor = "<a href='showuser.php3?user=$attrs[AUTHORUID]'>";
                $templateauthor .= $attrs['AUTHOR'] . "</a>";
            } else {
                $templateauthor = isset($attrs['AUTHOR']);
            }
        }

        if (isset($attrs['AUTHORMAIL'])) {
            $templateauthor .= " &lt;<a href='mailto:$attrs[AUTHORMAIL]'>";
            $templateauthor .= $attrs['AUTHORMAIL'] . "</a>&gt;";
        }
    }
}

#
# Make an NS file with the supplied data. Return the nsref for it
#
function MAKENS($template,$templatefields,$templatevalues) {
    global $NSGEN;
    global $TMPDIR;
    global $templatefile, $uid;
    
    #
    # Pick out some defaults for the exp. creation page
    #

    #
    # Run nsgen - make up a random nsref for use with it
    #
    list($usec, $sec) = explode(' ', microtime());
    srand((float) $sec + ((float) $usec * 100000));
    $nsref = rand();
    $outfile = $TMPDIR . $uid . "-$nsref.nsfile";

    #
    # Pick out the parameters for command-line arguments
    #
    $nsgen_args = "";
    foreach ($templatevalues as $name => $value) {
        if (!isset($templatefields[$name])) {
            TBERROR("Bad template parameter $name",1);
        }

        #
        # Do not include default values as command-line args
        #
	if (isset($templatefields[$name]['default']) &&
	    $templatefields[$name]['default'] == $value) {
            continue;
        }

        #
        # Make sure they don't try to break out of our quotes.
        #
        $value = escapeshellarg($value);

        $nsgen_args .= "-v $name=$value ";

    }

    #
    # Note: We run this as nobody on purpose - this is really dumb, but later
    # the web interface needs to be able to remove this file, and /tmp
    # usually has the sticky bit set, so only the owner can remove it.
    #
    SUEXEC("nobody","nobody","$NSGEN -o $outfile $nsgen_args $templatefile",
	SUEXEC_ACTION_DIE);

    return $nsref;
}

function BEGINEXP($nsref)
{
    #
    # Build up a URL to send the user to
    #
    $url = "beginexp_html.php3?";
    $url .= "&nsref=$nsref";
    header("Location: $url");
}

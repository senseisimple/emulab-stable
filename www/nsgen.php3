<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

$NSGEN         = "webnsgen";
$TMPDIR        = "/tmp/";

# Page arguments.
$template = $_GET['template'];
$advanced = $_GET['advanced'];

if (!isset($template)) {
    PAGEARGERROR();
}

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

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
if ($submit == "Begin Experiment" || $submit == "Show NS File") {
    global $TMPDIR;
    $nsref = MAKENS($template,$templatefields,$templatevalues);
    if ($submit == "Begin Experiment") {
        BEGINEXP($nsref);
    } else {
        $filename =  $TMPDIR . GETUID() . "-$nsref.nsfile";
        header("Content-type: text/plain");
        readfile($filename);
        unlink($filename);
    }
} else {
    #
    # Just spit out the form!
    #

    PAGEHEADER("Automatic NS file Generation");

    SPITFORM($advanced,$templatefields,$templateorder);

    PAGEFOOTER();
}


function SPITFORM($advanced,$templatefields,$templateorder) {
    global $template;

    echo "<form name='form1' action='nsgen.php3' method='get'>\n";
    echo "<input type='hidden' name='template' value='$template'>\n";
    echo "<table align=center border=1>\n";
    echo "<tr><th colspan=2>Template Parameters</th></tr>";
    for ($i = 0; $i < sizeof($templateorder); $i++) {
        $param = $templateorder[$i];
        if ($templatefields[$param]['advanced'] && $advanced = 0) {
            next;
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
    echo "<input type=submit name=submit value='Begin Experiment'>&nbsp;";
    echo "<input type=submit name=submit value='Show NS File'>&nbsp;";
    echo "<input type=reset name=reset value='Reset'>";
    echo "</center></th></tr>\n";

    echo "</form></table>\n";
}

function endElement($parser,$name)
{
    # Do nothing for now
}

function startElement($parser,$name,$attrs)
{
    global $templatefields;
    global $templateorder;
    if ($name == "VARIABLE") {

        #
        # Let's assume the template file is well-formed. I should write a DTD
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
    }
}

#
# Make an NS file with the supplied data. Return the nsref for it
#
function MAKENS($template,$templatefields,$templatevalues) {
    global $NSGEN;
    global $TMPDIR;
    global $templatefile;
    
    #
    # Pick out some defaults for the exp. creation page
    #

    #
    # Run nsgen - make up a random nsref for use with it
    #
    list($usec, $sec) = explode(' ', microtime());
    srand((float) $sec + ((float) $usec * 100000));
    $nsref = rand();
    $outfile = $TMPDIR . GETUID() . "-$nsref.nsfile";

    #
    # Pick out the parameters for command-line arguments
    #
    $nsgen_args = "";
    foreach ($templatevalues as $name => $value) {
        if (!isset($templatefields[$name])) {
            TBERROR("Bad template parameter $name",1);
        }

        #
        # Don't include default values as command-line args
        #
        if ($value == $templatefields[$name]['default']) {
            next;
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

<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("template_defs.php");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

# This will not return if its a sajax request.
include("showlogfile_sup.php3");

#
# Verify page arguments
#
$reqargs = RequiredPageArguments("template",   PAGEARG_TEMPLATE);
$optargs = OptionalPageArguments("modify",     PAGEARG_STRING,
				 "formfields", PAGEARG_ARRAY);

# Need these below.
$guid = $template->guid();
$vers = $template->vers();
$pid  = $template->pid();
$unix_gid = $template->UnixGID();

if (! $template->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to export in template ".
	      "$guid/$vers!", 1);
}

#
# Standard Testbed Header
#
PAGEHEADER("Modify Experiment Template");

#
# Spit the form out using the array of data.
#
function SPITFORM($template, $formfields, $errors)
{
    global $TBDB_PIDLEN, $TBDB_GIDLEN, $TBDB_EIDLEN, $TBDOCBASE;

    if ($errors) {
	echo "<table class=nogrid
                     align=center border=0 cellpadding=6 cellspacing=0>
              <tr>
                 <th align=center colspan=2>
                   <font size=+1 color=red>
                      &nbsp;Oops, please fix the following errors!&nbsp;
                   </font>
                 </td>
              </tr>\n";

	while (list ($name, $message) = each ($errors)) {
	    echo "<tr>
                     <td align=right>
                       <font color=red>$name:&nbsp;</font></td>
                     <td align=left>
                       <font color=red>$message</font></td>
                  </tr>\n";
	}
	echo "</table><br>\n";
    }

    echo $template->PageHeader();
    echo "<br><br>\n";

    $url = CreateURL("template_modify", $template);

    echo "<form action='$url' method='post'>";
    echo "<table align=center border=1>\n";

    #
    # TID:
    #
    echo "<tr>
              <td class='pad4'>Template ID:
              <br><font size='-1'>(alphanumeric, no blanks)</font></td>
              <td class='pad4' class=left>
                  <input type=text
                         name=\"formfields[tid]\"
                         value=\"" . $formfields["tid"] . "\"
	                 size=$TBDB_EIDLEN
                         maxlength=$TBDB_EIDLEN>
              &nbsp (optionally change this to something informative).
              </td>
          </tr>\n";

    echo "<tr>
              <td colspan=2>
               Use this text area to optionally describe your template:
              </td>
          </tr>
          <tr>
              <td colspan=2 align=center class=left>
                  <textarea name=\"formfields[description]\"
                    rows=4 cols=100>" .
	            ereg_replace("\r", "", $formfields["description"]) .
	           "</textarea>
              </td>
          </tr>\n";


    echo "<tr>
              <td colspan=2>
              <textarea name=\"formfields[nsdata]\"
                   cols=100 rows=40>" . $formfields["nsdata"] . "</textarea>
              </td>
          </tr>\n";

    echo "<tr>
              <td class='pad4' align=center colspan=2>
                <b><input type=submit name=modify value='Modify Template'></b>
              </td>
         </tr>
        </form>
        </table>\n";
}

#
# Put up the modify form on first load.
# 
if (! isset($modify)) {
    #
    # Grab NS file for the template.
    #
    $input_list  = $template->InputFiles();

    $defaults = array();
    $defaults["tid"] = $template->NextTID();
    $defaults["nsdata"] = $input_list[0];
    $defaults["description"] = $template->description();
    SPITFORM($template, $defaults, 0);
    PAGEFOOTER();
    exit();
}
elseif (! isset($formfields)) {
    PAGEARGERROR();
}

#
# Okay, validate form arguments.
#
$errors       = array();
$command_args = "";

#
# Generate a hopefully unique filename that is hard to guess. See below.
# 
list($usec, $sec) = explode(' ', microtime());
srand((float) $sec + ((float) $usec * 100000));
$foo = rand();

#
# TID
#
if (!isset($formfields["tid"]) || $formfields["tid"] == "") {
    #
    # Generate a new one.
    #
    $tid = $template->NextTID();
}
elseif (!TBvalid_eid($formfields["tid"])) {
    $errors["Template ID"] = TBFieldErrorString();
}
else {
    $tid = $formfields["tid"];
}

#
# Description:
# 
if (!isset($formfields["description"]) || $formfields["description"] == "") {
    $errors["Description"] = "Missing Field";
}
elseif (!TBvalid_template_description($formfields["description"])) {
    $errors["Description"] = TBFieldErrorString();
}
else {
    $command_args .= " -E " . escapeshellarg($formfields["description"]);
}

#
# NS File.
#
if (!isset($formfields["nsdata"]) || $formfields["nsdata"] == "") {
    $errors["NS File"] = "Missing Field";
}

if (count($errors)) {
    SPITFORM($template, $formfields, $errors);
    PAGEFOOTER();
    exit(1);
}

echo "<script type='text/javascript' language='javascript' ".
     "        src='template_sup.js'>\n";
echo "</script>\n";

#
# Generate a unique and hard to guess filename, and write NS to it.
#
$nsfile = "/tmp/$uid-$foo.nsfile";

if (! ($fp = fopen($nsfile, "w"))) {
    TBERROR("Could not create temporary file $nsfile", 1);
}
fwrite($fp, $formfields["nsdata"]);
fclose($fp);
chmod($nsfile, 0666);

STARTBUSY("Starting template modification!");

# And run that script!
$retval = SUEXEC($uid, "$pid,$unix_gid",
		 "webtemplate_create ".
		 "-m $guid/$vers $command_args $pid $tid $nsfile",
		 SUEXEC_ACTION_IGNORE);
unlink($nsfile);
/* Clear the various 'loading' indicators. */
HIDEBUSY();

if ($retval) {
    #
    # Fatal Error. Report to the user, even though there is not much he can
    # do with the error. Also reports to tbops.
    # 
    if ($retval < 0) {
	SUEXECERROR(SUEXEC_ACTION_CONTINUE);
    }
    # User error. Tell user and exit.
    SUEXECERROR(SUEXEC_ACTION_USERERROR);
    return;
}

#
# Parse the last line of output. Ick.
#
if (preg_match("/^Template\s+(\w+)\/(\w+)\s+is being/",
	       $suexec_output_array[count($suexec_output_array)-1],
	       $matches)) {
    $guid = $matches[1];
    $vers = $matches[2];

    $template = Template::Lookup($guid, $vers);
    if (! $template) {
	TBERROR("Could not lookup template object for $guid/$vers", 1);
	return;
    }
    echo $template->PageHeader();
    echo "<br><br>\n";
    STARTLOG($template);
}
else {
    SUEXECERROR(SUEXEC_ACTION_DIE);
}    

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

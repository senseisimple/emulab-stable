<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("node_defs.php");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Only admin users may modify node attributes.
#
if (! $isadmin) {
    USERERROR("You do not have permission to modify node atrributes!", 1);
}

#
# Verify form arguments.
#
$reqargs = RequiredPageArguments("node",       PAGEARG_NODE);
$optargs = OptionalPageArguments("refer",      PAGEARG_STRING,
				 "submit",     PAGEARG_STRING,
				 "_delattrs",  PAGEARG_ARRAY,
				 "_modattrs",  PAGEARG_ARRAY,
				 "_newattrs",  PAGEARG_ARRAY,
				 "_newvals",   PAGEARG_ARRAY);

# Need these below ...
$node_id = $node->node_id();
$type    = $node->type();

#
# Standard Testbed Header
#
PAGEHEADER("Modify Node Attributes Form");

#
# Get current set of attributes for node - used in comparison below
#
$cur_node_attrs = array();
$attr_result =
    DBQueryFatal("select attrkey,attrvalue from node_attributes ".
		 "where node_id='$node_id'");
while($row = mysql_fetch_array($attr_result)) {
    $cur_node_attrs[$row["attrkey"]] = $row["attrvalue"];
}

#
# Command strings are empty initially
#
$mod_command_string = "";
$add_command_string = "";
$del_command_string = "";

#
# Figure out which attributes are to be modified, added, or deleted
#

# Find attributes needing modification - make sure they are actually
# different than the current value before adding them to the command string.
if (isset($_modattrs)) {
  foreach ($_modattrs as $attrkey => $attrval) {
    if ($cur_node_attrs[$attrkey] != $attrval) {
      $mod_command_string .= "$attrkey='$attrval' ";
    }
  }
}

# Check for new attributes - make sure they are unique.
if (isset($_newattrs)) {
  for ($i = 0; $i < count($_newattrs); $i++) {
    if ($cur_node_attrs && 
        array_key_exists($_newattrs[$i], $cur_node_attrs)) {
      USERERROR("You cannot add a key that already exists!",1);
    }
    if ($_newattrs[$i]) {
      $add_command_string .= "$_newattrs[$i]='$_newvals[$i]' ";
    }
  }
}

# Finally, see if any attributes need to be deleted.
if (isset($_delattrs)) {
  foreach ($_delattrs as $attrkey => $attrval) {
    $del_command_string .= "$attrkey ";
  }
}

#
# Pass commands off to the script. It will check the arguments.
# NB: This is how nodcontrol.php3 does it, but it may not be safe;
#     Shell command injection may be possible.  This is an admin-only
#     command though.
#

# Fire off the modify operation first
if ($mod_command_string) {
  SUEXEC($uid, "nobody", "webnode_attributes -m ".
         "$mod_command_string $node_id",
         SUEXEC_ACTION_DIE);
}
# Next, add attributes
if ($add_command_string) {
  SUEXEC($uid, "nobody", "webnode_attributes -a ".
         "$add_command_string $node_id",
         SUEXEC_ACTION_DIE);
}
# Finally, delete attributes.
if ($del_command_string) {
  SUEXEC($uid, "nobody", "webnode_attributes -r ".
         "$del_command_string $node_id",
         SUEXEC_ACTION_DIE);
}

echo "<center>
      <br>
      <h3>Node attributes successfully modified!</h3><p>
      </center>\n";

$node->Show(SHOWNODE_NOFLAGS);

#
# Edit option.
#
$url = CreateURL("modnodeattributes_form", $node);
echo "<br><center>
           <A href='$url'>
           Edit this node's attributes again?</a>
         </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

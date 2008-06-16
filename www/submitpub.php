<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2008 University of Utah and the Flux Group.
# All rights reserved.
#

include("pub_defs.php");
include("form_defs.php");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$uid_idx   = $this_user->uid_idx();
$isadmin   = ISADMIN();

#
# Standard Testbed Header
#
PAGEHEADER("Submit Publication");

#
# Verify feature is enabled
#
if (!$PUBSUPPORT)
    USERERROR("Publication support not enabled.");

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("submit",      PAGEARG_STRING,
			  	 "idx", 	PAGEARG_INTEGER,
                                 "formfields",  PAGEARG_ARRAY);

$also_required = array();
$also_required['article'] = array('conf', 'month');

$typelist = array('article', 'phd thesis', 'masters thesis', 'bachelors thesis', 
                  'techreport', 'unpublished', 'talk', 'software', 'service');

$submit_url = isset($idx) ? CreateURL("submitpub", 'idx', $idx) : CreateURL("submitpub");

#
# Make Month List
#
$monthlist = array();
$query_result = DBQueryFatal("select * from emulab_pubs_month_map order by display_order");
while ($r = mysql_fetch_array($query_result)) {
  $monthlist[$r['month']] = $r['month_name'];
}

#
# Make Project List
#
$projectlist = array();
if ($isadmin) {
  $query_result = DBQueryFatal("select pid from projects order by pid");
} else {
  $query_result = DBQueryFatal("select distinct pid ".
			       "  from group_membership ".
			       "  where uid='$uid' order by pid");
}
while ($r = mysql_fetch_array($query_result)) {
    array_push($projectlist, $r[0]);   
}
array_push($projectlist, "N/A");

#
# Make Category List
#
$categorylist = array();
$query_result = DBQueryFatal("select distinct category ".
			   "  from emulab_pubs order by category");
while ($r = mysql_fetch_array($query_result)) {
    if ($r[0] != "") 
	array_push($categorylist, $r[0]);   
}
array_push($categorylist, "Other");

$form = array('#action' => $submit_url);
$default_textfield_size = 60;

#
# Create Fields List
#
function WrapOptionsList($options) {
    $res = array();
    foreach ($options as $o) {
        $res[$o] = $o;
    }
    return $res;
}
$fields = array();
if ($isadmin && isset($idx) && !isset($submit)) {
  $fields['created'] =
    array('#type' => 'display',
          '#label' => 'Created');
  $fields['owner_name'] =
    array('#type' => 'display',
          '#label' => 'Owner');
  $fields['submitted_by_name'] =
    array('#type' => 'display',
          '#label' => 'Submitted By');
  $fields['last_edit'] =
    array('#type' => 'display',
          '#label' => 'Last Edit');
  $fields['last_edit_by_name'] = 
   array('#type' => 'display',
         '#label' => 'Last Edit By');
}
$fields['type'] =
    array('#type' => 'select',
          '#label' => 'Type',
          '#required' => true,
	  '#checkslot' => 'default:tinytext',
          '#options' => WrapOptionsList($typelist));
$fields['authors'] = 
    array('#type' => 'textfield',
          '#label' => 'Authors',
	  '#size' => $default_textfield_size,
          '#required' => true,
	  '#prep' => 'trim',
	  '#checkslot' => 'default:tinytext_utf8',
          '#description' => 'comma seperated list');
$fields['affil'] = 
    array('#type' => 'textfield',
          '#label' => 'Author Affiliations',
	  '#size' => $default_textfield_size,
          '#required' => true,
	  '#prep' => 'trim',
	  '#checkslot' => 'default:tinytext_utf8',
          '#description' => 'comma separated list, only list each institution once');
$fields['title'] = 
    array('#type' => 'textfield',
          '#label' => 'Title',
	  '#size' => $default_textfield_size,
          '#required' => true,
	  '#prep' => 'trim',
	  '#checkslot' => 'default:tinytext_utf8');
$fields['conf'] = 
    array('#type' => 'textfield',
          '#label' => 'Conference/Journal',
	  '#size' => $default_textfield_size,
          '#required' => false,
	  '#prep' => 'trim',
	  '#checkslot' => 'default:tinytext_utf8');
$fields['where'] = 
    array('#type' => 'textfield',
          '#label' => 'Conference Location',
	  '#size' => $default_textfield_size,
          '#required' => false,
	  '#prep' => 'trim',
	  '#checkslot' => 'default:tinytext_utf8');
$fields['year'] = 
    array('#type' => 'textfield',
          '#label' => 'Year',
          '#size' => 4,
	  '#maxlength' => 4,
          '#required' => true,
	  '#prep' => 'trim',
	  '#checkslot' => '/^\d\d\d\d$/');
$fields['month'] = 
    array('#type' => 'select',
          '#label' => 'Month',
          '#required' => false,
	  '#no_default' => true,
          '#options' => $monthlist,
	  '#checkslot' => 'default:tinytext');
$fields['volume'] = 
    array('#type' => 'textfield',
          '#label' => 'Volume',
	  '#size' => $default_textfield_size,
          '#required' => false,
	  '#prep' => 'trim',
	  '#checkslot' => 'default:tinytext');
$fields['number'] = 
    array('#type' => 'textfield',
          '#label' => 'Number',
	  '#size' => $default_textfield_size,
          '#required' => false,
	  '#prep' => 'trim',
	  '#checkslot' => 'default:tinytext');
$fields['pages'] = 
    array('#type' => 'textfield',
          '#label' => 'Pages',
	  '#size' => $default_textfield_size,
          '#required' => false,
	  '#prep' => 'trim',
	  '#checkslot' => 'default:tinytext');
$fields['url'] = 
    array('#type' => 'textfield',
          '#label' => 'Publication URL',
	  '#size' => $default_textfield_size,
          '#required' => true,
	  '#prep' => 'trim',
	  '#checkslot' => 'default:tinytext');
$fields['conf_url'] = 
    array('#type' => 'textfield',
          '#label' => 'Conference URL',
	  '#size' => $default_textfield_size,
          '#required' => false,
	  '#prep' => 'trim',
	  '#checkslot' => 'default:tinytext');
$fields['category'] = 
    array('#type' => 'vlist',
          '#label' => 'Category',
          '#required' => true,
          '#elements' => array(
              'category' => 
                  array('#type' => 'select',
                        '#required' => true,
                        '#options' => WrapOptionsList($categorylist),
	                '#checkslot' => 'default:tinytext'),
              'category_other' =>
                  array('#type' => 'textfield',
                        '#label' => 'If other specify',
 	  		'#size' => 50,
	  		'#prep' => 'trim',
	                '#checkslot' => 'default:tinytext',
			'#nodump' => true)));
if ($isadmin) {
    $fields['evaluated_on_emulab'] = 
        array('#type' => 'textfield',
             '#label' => 'Evaluated on emulab, or just cite?',
             '#description' => '"Y" or "N"',
	     '#size' => $default_textfield_size,
             '#required' => false,
	     '#prep' => 'trim',
  	     '#checkslot' => 'default:tinytext');
}
$fields['project'] = 
    array('#type' => 'select',
          '#label' => 'Project',
          '#required' => 'true',
          '#size' => $default_textfield_size,
          '#options' => WrapOptionsList($projectlist),
          '#checkslot' => 'default:tinytext');
$fields['cite_osdi02'] = 
    array('#type' => 'vlist',
          '#label' => "Did you cite our OSDI'02 paper?",
	  '#link' => "docwrapper.php3?docname=policies.html#reporting",
          '#required' => true,
	  '#elements' => array(
              'cite_osdi02' =>
                  array('#type' => 'select',
                        '#required' => true,
			'#options' => array(1 => 'yes', 0 => 'no'),
                        '#checkslot' => 'default:boolean'),
              'no_cite_why' =>
                  array('#label' => 'If (no) why not',
			'#type' => 'textfield',
                        '#size' => 50,
			'#prep' => 'trim',
                        '#checkslot' => 'default:tinytext_utf8')));
if ($isadmin) {
    $fields['visible'] = 
        array('#type' => 'checkbox',
              '#label' => "Visible in Public Bibliography",
              '#checkslot' => 'default:boolean');
    $fields['editable_owner'] =
        array('#type' => 'checkbox',
              '#label' => "Editable by Owner",
              '#checkslot' => 'default:boolean');
    $fields['editable_proj'] =
        array('#type' => 'checkbox',
              '#label' => "Editable by Project Members",
              '#checkslot' => 'default:boolean');
    #TableRow("Take Ownership", "ownership", true,
    #         CheckBox("ownership") . "<br>".
    #         "<font size=-1><i>Show Publication in My Emulab Page?</i></font>");
}
$fields['notes'] = 
    array('#type' => 'textarea',
          '#label' => 'Additional Notes',
          '#required' => false,
	  '#checkslot' => 'default:fulltext_utf8',
          '#cols' => 78,
          '#rows' => 2);
if (isset($idx)) {
    $fields['deleted'] =
        array('#type' => 'checkbox',
              '#label' => 'Mark As Deleted');
}

$dbfields = array();
foreach ($fields as $k => $v) {
    if ($v['#type'] == 'display') continue;
    array_push($dbfields, $k);
}
array_push($dbfields, 'no_cite_why');

if ($isadmin) {
  $fields['allow_missing'] =
      array('#type' => 'checkbox',
	    '#label' => 'Allow Missing Fields');
}

#
# Spit the form out using the array of data and error strings (if any).
# 
function SPITFORM($formfields, $errors)
{
    global $form, $fields;

    echo "<center>";
    FormRender($form, $errors, $fields, $formfields);
    echo "</center>";
}

#
#
#

$defaults = array();
$errors  = array();

#
# Helper functions
#
function field_set($f) {
  global $formfields;
  return isset($formfields[$f]) && $formfields[$f] != "";
}

function GetName($what, $skip_if_same_as = null) {
  global $fields;
  global $defaults;
  $field = $what . "_name";
  if ($skip_if_same_as != null && $defaults[$what] == $defaults[$skip_if_same_as]) {
    unset($fields[$field]);
  } else if ($defaults[$what] == 0) {
    $fields[$field]['#value'] = 'Nobody';
  } else {
    $user = new User($defaults[$what]);
    $fields[$field]['#value'] = $user->name() . " (" . $user->uid() . ")";
  }
}


if (isset($idx)) {

  $query_result = DBQueryFatal("select * from emulab_pubs where idx=$idx");
  if (mysql_num_rows($query_result) < 1) {
    echo "Pub #$idx does not exist\n";
    PAGEFOOTER();
    return;
  }
  $r = mysql_fetch_assoc($query_result);
  if (!MayEditPub($this_user, $isadmin, $r)) {
    echo "You Do Not Have Permission to Edit this Publication Submission\n";
    PAGEFOOTER();
    return;
  }

  $defaults = $r;

  if ($isadmin && !isset($submit)) {
    GetName('submitted_by');
    GetName('owner', 'submitted_by');
    GetName('last_edit_by', 'submitted_by');
    if ($defaults['created'] == $defaults['last_edit'])
      unset($fields['last_edit']);
  }

  $fields['submit'] =
      array('#type' => 'submit',
	    '#value' => "Update");

} else {

  $defaults['project'] = $projectlist[0];
  $defaults['visible'] = true;
  $defaults['editable_owner'] = true;
  $defaults['editable_proj'] = true;
  #$defaults['ownership'] = false;

  $fields['submit'] =
      array('#type' => 'submit',
	    '#value' => "Submit");

}

if (!isset($submit)) {

  SPITFORM($defaults, $errors);

  PAGEFOOTER();
  return;

} 

#
# Verify Form
# 
if ((isset($formfields['deleted']) && $formfields['deleted'])
    || (isset($formfields['allow_missing']) && $formfields['allow_missing'])) {
    # Hack, modify fields to unset required flag
    foreach ($fields as $k => $v) {
	unset($fields[$k]['#required']);
    }
    unset($fields['category']['#elements']['category']['#required']);
    unset($fields['cite_osdi02']['#elements']['cite_osdi02']['#required']);
}

# hack since '' == 0 will evaluate to true and thus, "No" 
# will incorrectly be selected
if (strcmp($formfields['cite_osdi02'], '') == 0)
    unset($formfields['cite_osdi02']);

FormValidate($form, $errors, $fields, $formfields);

if (!(isset($formfields['allow_missing']) && $formfields['allow_missing'])) {

    $type = $formfields['type'];
    if (isset($also_required[$type])) {
      foreach ($also_required[$type] as $f) {
	if (!field_set($f)) {
	  $errors[$fields[$f]['#label']] = "Required for $type type.";
	}
      }
    }

    if (field_set('cite_osdi02') && !$formfields['cite_osdi02'] && !field_set('no_cite_why')) {
      $errors[$fields['cite_osdi02']['#label']] = "Must specify reason.";
    }
}

if ($formfields['category'] == 'Other') {
  if (field_set('category_other')) {
    $formfields['category'] = $formfields['category_other'];
  } else {
    $errors[$fields['category']['#label']] = "Must specify.";
  }
}

$formfields['notes'] = str_replace("\r", "", $formfields['notes']);

if ($errors) {
  SPITFORM($formfields, $errors);
  PAGEFOOTER();
  return;
}

#
# 
#
$formfields['month_name'] = $monthlist[$formfields['month']];

#
# Build DB Query and Send Mail to Testbed-ops
#

$formdump = FormTextDump($form, $fields, $formfields, 30);

function ConfirmationCommon($deleted = false) {
    global $form, $fields, $formfields, $formdump, $idx;

    echo "<pre>\n". htmlspecialchars($formdump). "</pre>\n";

    if (!$deleted) {
	echo '<p>It will appear in the public <a href="expubs.php">Bibliography</a> ';
	echo "under ". htmlspecialchars($formfields['category']) ." like this:</p>";

	echo "<ul>\n<li>\n";
	echo MakeBib(NULL, 0, $formfields);
	echo "</li></ul>\n";
    }

    echo "<a href=\"submitpub.php?idx=". $idx . "\" style=\"background:yellow\">Edit</a>";
}

if (!isset($idx)) {

    $cols = array('owner', 'submitted_by', 'last_edit_by', 'uuid', 'created', 'last_edit');
    #$owner = $isadmin && !$formfields['ownership'] ? 0 : $uid_idx;
    $owner = $uid_idx;
    $vals = array($owner, $uid_idx, $uid_idx, '"'.mysql_escape_string(NewUUID()).'"', "now()", "now()");
    foreach ($dbfields as $f) {
      $dbname = $f;
      $value  = $formfields[$f];
      array_push($cols, '`'. $dbname . '`');
      array_push($vals, '"'. mysql_escape_string($value) . '"');
    }

    DBQueryFatal("insert into emulab_pubs (".
		 implode(",", $cols). ") values (".
		 implode(",", $vals). ")");

    $idx = mysql_insert_id();

    echo "<p>The following  was Submitted: </p>";

    ConfirmationCommon();

    if (!$isadmin) {
        TBMAIL("$TBMAILADDR_OPS",
               "New Publication Submitted",
               $formdump,
               "From: ". $this_user->name() . "<" . $this_user->email() . ">");
    }

    PAGEFOOTER();

} else {

    $update_list = array("last_edit = now()", "`last_edit_by` = $uid_idx");
    $update_start = count($update_list);

    if (!isset($formfields['deleted'])) 
      $formfields['deleted'] = false;

    # determine what changed
    foreach ($dbfields as $f) {
      if ($defaults[$f] != $formfields[$f]) {
        $dbname = $f;
        $value  = @$formfields[$f];
        array_push($update_list,
	           "`".$dbname."` = \"".
                   mysql_escape_string($value) . '"');
      }
    }

    if (count($update_list) > $update_start) {

      DBQueryFatal("update emulab_pubs set ".
                   implode(",", $update_list).
                   " where idx = $idx");
      
      if (!$formfields['deleted']) {

        echo "<p>The following publication was Updated:</p>\n";

        ConfirmationCommon();

      } else {

        echo "<p>The following publication was deleted.  ";
        echo "To undelete simply edit it again and unclick \"Mark As Deleted\"</p>\n";

        ConfirmationCommon(true);

      }

    } else {

      echo "<p>Nothing has changed for the following pub:</p>";

      ConfirmationCommon();
 
    }

    PAGEFOOTER();
}

?>

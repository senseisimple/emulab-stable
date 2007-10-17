<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("template_defs.php");
include_once("form_defs.php");

#
# Only known and logged in users can look at experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$uid_idx   = $this_user->uid_idx();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("template",     PAGEARG_TEMPLATE);
$optargs = OptionalPageArguments("search",       PAGEARG_STRING,
				 "prevsearch",   PAGEARG_STRING,
				 "addclause",    PAGEARG_STRING,
				 "formfields",   PAGEARG_ARRAY); 

#
# Standard Testbed Header after argument checking.
#
PAGEHEADER("Template Search");

#
# Check permission.
#
if (! $template->AccessCheck($this_user, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view experiment template ".
	      "$guid/$version!", 1);
}

#
# We display the info for all versions of the template. 
#
$root = $template->LookupRoot($template->guid());
$guid = $root->guid();

echo $root->PageHeader();
echo "<br><br>\n";

# A list of match conditions we know about.
$clauseselection = array('equal'     => "==",
			 'like'      => "like",
			 'less'      => "<",
			 'lesseq'    => "<= ",
			 'greater'   => ">",
			 'greatereq' => ">=");

#
# Define the form.
#
$form = array('#id'	  => 'myform',
	      '#action'   => CreateURL("template_search", $root),
	      '#caption'  => "Search your template (records) ".
	                        "by parameter values");

$fields = array();

#
# Search either templates, instances, or runs.
#
$fields['searchwhich'] =
    array('#type'       => 'radio',
	  '#label'      => 'What do you want to search',
	  '#required'   => TRUE,
	  '#radios'     => array('Template' =>
				 array('#label'        => 'Templates',
				       '#return_value' => "template"),
				 'Instance' =>
				 array('#label'        => 'Instances',
				       '#return_value' => "instance"),
				 'Run' =>
				 array('#label'        => 'Runs',
				       '#return_value' => "run")),
	  '#checkslot'  => "/^(template|instance|run)$/");

$fields['matchif'] =
    array('#type'       => 'radio',
	  '#label'      => 'Match on',
	  '#description'=> 'all clauses match or any clause matches',
	  '#required'   => TRUE,
	  '#radios'     => array('Any' =>
				 array('#label'        => 'Any',
				       '#return_value' => "any"),
				 'All' =>
				 array('#label'        => 'All',
				       '#return_value' => "all")),
	  '#checkslot'  => "/^(any|all)$/");

#
# User is given the option to save this search for later.
#
$fields['save'] = array('#type'    => 'list',
			'#label'   => 'Save this search',
			'#elements'=> array('savesearch' =>
					    array('#type'       => 'checkbox',
						  '#label'      => 'Yes',
						  '#return_value'=> 1),
					    'savename'   =>
					    array('#type'       => 'textfield',
						  '#label'      => 'Save Name',
						  '#size'       => 20,
						  '#maxlength'  => 64)));

#
# Grab a list of all parameters across all the templates and create
# a list of input boxes.
#
$query_result =
    DBQueryFatal("select distinct name from experiment_template_parameters ".
		 "where parent_guid='$guid' ".
		 "order by name");
$index = 1;
$paramselection = array();
while ($row = mysql_fetch_array($query_result)) {
    $paramselection[$index++] = $row['name'];
}

#
# Start with a single clause. 
#
$fields['clause1'] =
    array('#type'     => 'list',
	  '#label'    => 'Clause 1',
	  '#elements' => array('parameter1' =>
			       array('#type'      => 'select',
				     '#default'   => 'Parameter',
				     '#which'     => 1,
				     '#checkslot' => 'CheckParameter',
				     '#options'   => $paramselection),
			       'condition1' =>
			       array('#type'      => 'select',
				     '#default'   => 'Cond',
				     '#which'     => 1,
				     '#checkslot' => 'CheckCondition',
				     '#options'   => $clauseselection),
			       'value1' =>
			       array('#type'      => 'textfield',
				     '#which'     => 1,
				     '#checkslot' => 'CheckValue',
				     '#size'      => 30)));
$fields['clausecount'] = array('#type'     => 'hidden',
			       '#required' => TRUE,
			       '#value'    => "1");

#
# Spit the form out using the array of data. 
# 
function SPITFORM($formfields, $errors)
{
    global $form, $fields, $root, $prevsearch, $this_user;

    if (($savedsearches = $root->SavedSearches($this_user))) {
	$action = CreateURL("template_search", $root);
    
	echo "<script language=JavaScript>
              function PreviousSearch() {
                  var index = document.prevsearch.prevselect.selectedIndex;
                  document.prevsearch.target='_self';
                  document.prevsearch.action='$action' + '&prevsearch=' +
                     document.prevsearch.prevselect.options[index].value;
                  document.prevsearch.submit();
              }
              </script>\n";
	echo "<form name=prevsearch onsubmit=\"return false;\"
                    action=foo method=post>\n";
	echo FormRenderSelect("savedsearches",
			      array('#type'      => 'select',
				    '#default'   => 'Choose a Previous Search',
				    '#options'   => $savedsearches,
				    '#name'      => 'prevselect',
				    '#javascript'=>
				      "onchange=\"PreviousSearch();\""));
	echo "</form>\n";
    }

    if (isset($prevsearch)) {
	#
	# Lets add a primitive mechanism to allow saved search deletion.
	# Add a delete button to the saved search row.
	#
	$fields['save']['#elements']['deletesearch'] =
	    array('#type'  => 'image',
		  '#value' => "$prevsearch",
		  '#image' => "trash.jpg");
    }
    
    $fields['submits'] =
	array('#type'     => 'list',
	      '#colspan'  => TRUE,
	      '#elements' => array('addclause' =>
				  array('#type'  => 'submit',
					'#value' => "Add Clause"),
				  'search' =>
				  array('#type'  => 'submit',
					'#value' => "Search")));

    echo "<center>";
    FormRender($form, $errors, $fields, $formfields);
    echo "</center>";
}

#
# On first load, display a virgin form and exit.
#
if (!isset($formfields)) {
    if (isset($prevsearch)) {
	$defaults = $root->SavedSearch(addslashes($prevsearch), $this_user);
    }
    else {
	$defaults = array();
	$defaults["searchwhich"] = "template";
	$defaults["matchif"]     = "any";
    }

    SPITFORM($defaults, null);
    PAGEFOOTER();
    return;
}
# Form submitted.
if (!isset($formfields['clausecount']) ||
    $formfields['clausecount'] <= 0 || $formfields['clausecount'] > 20) {
    PAGEARGERROR("Invalid form arguments.");
}

#
# Old searches. Process a deletion request before generating new list.
#
if (isset($formfields['deletesearch']) && $formfields['deletesearch'] != "" &&
    TBcheck_dbslot($formfields['deletesearch'],
		   "experiment_template_searches", "name",
		   TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR)) {
    $root->DeleteSearch(addslashes($formfields['deletesearch']), $this_user);
    #
    # Lets clear the saved search stuff.
    #
    unset($formfields['savesearch']);
    unset($formfields['savename']);
}

#
# Need to generate new clauses for any added previously, plus a new one
# if the Add Clause button was pressed. But, have to watch for deleted
# clauses since the formfield names encode the clause number. In other
# words, we end up with holes in the array.
#
$clausecount = $formfields['clausecount'];
if (isset($addclause)) {
     $clausecount++;
     $formfields['clausecount'] = $clausecount;
}
for ($i = 2; $i <= $clausecount; $i++) {
    #
    # If the user used the Trash button, then delete (skip) that clause.
    # It does not matter that there are values in $formfields; they will
    # be ignored.
    #
    if (isset($formfields["delete${i}"]))
	continue;

    # Watch for holes or end of list.
    if (!isset($formfields["parameter${i}"])) {
	# Added clause goes at the end of course (not in a hole).
	if (isset($addclause) && $i == $clausecount) {
	    unset($addclause);
	}
	else
	    continue;
    }
	      
    $fields["clause${i}"] =
	array('#type'     => 'list',
	      '#label'    => "Clause $i",
	      '#elements' => array("parameter${i}" =>
				   array('#type'      => 'select',
					 '#default'   => 'Parameter',
					 '#which'     => $i,
					 '#checkslot' => 'CheckParameter',
					 '#options'   => $paramselection),
				   "condition${i}" =>
				   array('#type'      => 'select',
					 '#default'   => 'Cond',
					 '#which'     => $i,
					 '#checkslot' => 'CheckCondition',
					 '#options'   => $clauseselection),
				   "value${i}" =>
				   array('#type'      => 'textfield',
					 '#which'     => $i,
					 '#checkslot' => 'CheckValue',
					 '#size'      => 30),
				   "delete${i}" =>
				   array('#type'  => 'image',
					 '#value' => 'Delete',
					 '#image' => "trash.jpg")));
}
if (!isset($search)) {
    SPITFORM($formfields, null);
    PAGEFOOTER();
    return;
}

#
# Otherwise, must validate and redisplay if errors
#
$errors  = array();

#
# This is the callback referenced above in the checkslot fields.
#
function CheckParameter($name, &$errors, $attributes, $value)
{
    global $paramselection;
    $which = $attributes['#which'];

    if (!(isset($value) && $value != "")) {
	$errors["Parameter $which"] = "Missing required value";
	return;
    }
    
    #
    # The param value is the index of a param. It must be a real param
    # index.
    #
    if ($value < 0 || $value > count($paramselection)) {
	$errors["Parameter $which"] = "Out of range";
    }
}
function CheckCondition($name, &$errors, $attributes, $value)
{
    global $clauseselection;

    $which = $attributes['#which'];
    if (!(isset($value) && $value != "")) {
	$errors["Condition $which"] = "Missing required value";
	return;
    }
   
    if (!array_key_exists($value, $clauseselection)) {
	$errors["Condition $which"] = "Invalid condition";
    }
}
function CheckValue($name, &$errors, $attributes, $value)
{
    $which = $attributes['#which'];

    # Allow a null value. User wants to compare against ""
    if (!isset($value) || $value == "") {
	return;
    }
    
    #
    # The value has to be plain text, but thats about it. We will escape
    # it before passing off to mysql in a query.
    #
    if (!TBvalid_userdata($value)) {
	$errors["Value $which"] = TBFieldErrorString();
    }
}
FormValidate($form, $errors, $fields, $formfields);

# Check the save search stuff; easier then trying to automate it since 
# I need to figure out how to automate correlated checks.
if (isset($formfields['savesearch']) && $formfields['savesearch']) {
    if (!isset($formfields['savename']) || $formfields['savename'] == "") {
	$errors['Save Name'] = "Must provide a search save name";
    }
    elseif (!TBcheck_dbslot($formfields['savename'],
			    "experiment_template_searches", "name",
			    TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR)) {
	$errors['Save Name'] = TBFieldErrorString();
    }
}

#
# If any errors, respit the form with the current values and the
# error messages displayed. Iterate until happy.
# 
if (count($errors)) {
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

#
# Build up the match clauses first.
#
$clauses = array();

for ($i = 1; $i <= $clausecount; $i++) {
    $name  = $paramselection[$formfields["parameter${i}"]];
    $cond  = $formfields["condition${i}"];
    $value = addslashes($formfields["value${i}"]);

    switch ($cond) {
    case 'equal':
	$clauses[] = "(name='$name' and value='$value')";
	break;
    case 'like':
	$clauses[] = "(name='$name' and value like '$value')";
	break;
    case 'less':
	$clauses[] = "(name='$name' and value regexp '^[0-9\.]+$' and ".
	    "CAST(value as signed)<'$value')";
	break;
    case 'lesseq':
	$clauses[] = "(name='$name' and value regexp '^[0-9\.]+$' and ".
	    "CAST(value as signed)<='$value')";
	break;
    case 'greater':
	$clauses[] = "(name='$name' and value regexp '^[0-9\.]+$' and ".
	    "CAST(value as signed)>'$value')";
	break;
    case 'greatereq':
	$clauses[] = "(name='$name' and value regexp '^[0-9\.]+$' and ".
	    "CAST(value as signed)>='$value')";
	break;
    }
}
# We want all rows that match.
$clausestring = join(" or ", $clauses);

# Any/All will match the number of rows returned by above clause.
if ($formfields["matchif"] == "any") {
    $matchif = ">=1";
}
else {
    $matchif = "=" . count($clauses);
}

if ($formfields["searchwhich"] == "template") {
    $query_string =
	"select t.* from experiment_template_parameters as p ".
	"left join experiment_templates as t on t.guid=p.parent_guid and ".
	"     t.vers=p.parent_vers ".
	"where p.parent_guid='$guid' and ($clausestring) ".
	"group by t.vers having count(t.vers) $matchif";
}
elseif ($formfields["searchwhich"] == "instance") {
    $query_string =
	"select i.* from experiment_template_instance_bindings as b ".
	"left join experiment_template_instances as i on ".
	"     i.parent_guid=b.parent_guid and ".
	"     i.parent_vers=b.parent_vers and ".
	"     i.idx=b.instance_idx ".
	"where b.parent_guid='$guid' and ($clausestring) ".
	"group by i.idx having count(i.idx) $matchif";
}
else {
    # This is complicated by the fact tha neither experiment_runs nor
    # experiment_run_bindings has a backlink to the template. 
    $query_string =
	"select r.*,b.runidx,i.parent_vers ".
	"     from experiment_template_instances as i ".
	"left join experiment_run_bindings as b on b.exptidx=i.exptidx ".
	"left join experiment_runs as r on ".
	"     r.exptidx=b.exptidx and r.idx=b.runidx ".
	"where i.parent_guid='$guid' and ($clausestring) ".
	"group by i.idx,b.runidx having count(b.runidx) $matchif";
}

#TBERROR($query_string, 0);

$query_result = DBQueryWarn($query_string);
if (!$query_result || !mysql_num_rows($query_result)) {
    $errors["Match Failure"] = "There were no matches";
    SPITFORM($formfields, $errors);
    return;
}

# Save this search to the DB if requested.
if (isset($formfields['savesearch']) && $formfields['savesearch']) {
    DBQueryWarn("replace into experiment_template_searches set ".
		" uid_idx='$uid_idx', created=now(), ".
		" parent_guid='$guid', ".
		" parent_vers='" . $template->vers() . "'," .
		" name='" . addslashes($formfields['savename']) . "'," .
		" expr='" . addslashes(serialize($formfields)) . "'");
    # Indicate that we are using a saved search, so that we get delete button.
    $prevsearch = $formfields['savename'];
}

# Spit the form again so the user can change the search criteria.
SPITFORM($formfields, $errors);
echo "<br>\n";

if ($formfields["searchwhich"] == "template") {
    AddSortedTable("mytable");
    echo "<table align=center id='mytable'
		 border=1 cellpadding=5 cellspacing=2>\n";

    echo "<thead class='sort'>\n";
    echo "<tr>
            <th>Vers</th>
            <th>Parent</th>
            <th>TID</th>
            <th>Created</th>
            <th>Description</th>
          </tr>\n";
    echo "</thead>\n";

    while ($row = mysql_fetch_array($query_result)) {
	$vers    = $row['vers'];
	$tid     = $row['tid'];
	$pvers   = $row['parent_vers'];
	$tid     = $row['tid'];
	$desc    = $row['description'];
	$created = $row['created'];

	$onmouseover = MakeMouseOver($desc);
	if (strlen($desc) > 30) {
	    $desc = substr($desc, 0, 30) . " <b>... </b>";
	}
	$parent_link = MakeLink("template",
				"guid=$guid&version=$pvers", "$pvers");

	$current_link = MakeLink("template",
				 "guid=$guid&version=$vers", "$vers");

	echo "<tr>".
	    "<td>$current_link</td>".
	    "<td>$parent_link</td>".
	    "<td>$tid</td>".
	    "<td>$created</td>".
	    "<td $onmouseover>$desc</td>";
    }
    echo "</table>\n";
}
elseif ($formfields["searchwhich"] == "instance") {
    AddSortedTable("mytable");
    echo "<table align=center id='mytable'
		 border=1 cellpadding=5 cellspacing=2>\n";

    echo "<thead class='sort'>\n";
    echo "<tr>
            <th>Instance</th>
            <th>Template</th>
            <th>Started</th>
            <th>Stopped</th>
            <th>Description</th>
          </tr>\n";
    echo "</thead>\n";

    while ($row = mysql_fetch_array($query_result)) {
	$pvers    = $row['parent_vers'];
	$exptidx  = $row['exptidx'];
	$desc     = $row['description'];
	$started  = $row['start_time'];
	$stopped  = $row['stop_time'];

	$onmouseover = MakeMouseOver($desc);
	if (strlen($desc) > 30) {
	    $desc = substr($desc, 0, 30) . " <b>... </b>";
	}
	$template_link = MakeLink("template",
				  "guid=$guid&version=$pvers", "$pvers");

	$instance_link = MakeLink("instance",
				  "instance=$exptidx", "$exptidx");

	echo "<tr>".
	    "<td>$instance_link</td>".
	    "<td>$template_link</td>".
	    "<td>$started</td>".
	    "<td>$stopped</td>".
	    "<td $onmouseover>$desc</td>";
    }
    echo "</table>\n";
}
else {
    AddSortedTable("mytable");
    echo "<table align=center id='mytable'
		 border=1 cellpadding=5 cellspacing=2>\n";

    echo "<thead class='sort'>\n";
    echo "<tr>
            <th>Run</th>
            <th>Instance</th>
            <th>Template</th>
            <th>ID</th>
            <th>Started</th>
            <th>Stopped</th>
            <th>Description</th>
          </tr>\n";
    echo "</thead>\n";

    while ($row = mysql_fetch_array($query_result)) {
	$pvers    = $row['parent_vers'];
	$runidx   = $row['runidx'];
	$runid    = $row['runid'];
	$exptidx  = $row['exptidx'];
	$desc     = $row['description'];
	$started  = $row['start_time'];
	$stopped  = $row['stop_time'];

	$onmouseover = MakeMouseOver($desc);
	if (strlen($desc) > 30) {
	    $desc = substr($desc, 0, 30) . " <b>... </b>";
	}
	$template_link = MakeLink("template",
				  "guid=$guid&version=$pvers", "$pvers");

	$instance_link = MakeLink("instance",
				  "instance=$exptidx", "$exptidx");

	$run_link = MakeLink("run",
			     "instance=$exptidx&runidx=$runidx", "$runidx");

	echo "<tr>".
	    "<td>$run_link</td>".
	    "<td>$instance_link</td>".
	    "<td>$template_link</td>".
	    "<td>$runid</td>".
	    "<td>$started</td>".
	    "<td>$stopped</td>".
	    "<td $onmouseover>$desc</td>";
    }
    echo "</table>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

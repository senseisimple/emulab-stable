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
PAGEHEADER("Archive Tags");

#
# Only known and logged in users can end experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("index",      PAGEARG_INTEGER,
				 "experiment", PAGEARG_EXPERIMENT,
				 "template",   PAGEARG_TEMPLATE,
				 "instance",   PAGEARG_INSTANCE,
				 "records",    PAGEARG_INTEGER,
				 "tag",        PAGEARG_STRING);

#
# An instance might be a current or historical. If its a template, use
# the underlying experiment of the template.
#
if (isset($instance)) {
    if (($foo = $instance->GetExperiment()))
	$experiment = $foo;
    else
	$index = $instance->exptidx();
    $template = $instance->GetTemplate();
}
elseif (isset($template)) {
    $experiment = $template->GetExperiment();
}
elseif (isset($index)) {
    $experiment = Experiment::Lookup($index);
}

#
# If we got a current experiment, great. Otherwise we have to lookup
# data for a historical experiment.
#
if (isset($experiment) && $experiment) {
    # Need these below.
    $pid = $experiment->pid();
    $eid = $experiment->eid();
    $exptidx = $experiment->idx();

    $stats = $experiment->GetStats();
    if (!$stats) {
	TBERROR("Could not load stats object for experiment $pid/$eid", 1);
    }
    $archive_idx = $stats->archive_idx();

    # Permission
    if (!$isadmin &&
	!$experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view tags for ".
		  "archive in $pid/$eid!", 1);
    }
}
elseif (isset($index)) {
    $stats = ExperimentStats::Lookup($index);
    if (!$stats) {
	PAGEARGERROR("Invalid experiment index: $index");
    }

    # Need these below.
    $pid = $stats->pid();
    $eid = $stats->eid();
    $exptidx = $index;
    $archive_idx = $stats->archive_idx();

    # Permission
    if (!$isadmin &&
	!$stats->AccessCheck($this_user, $TB_PROJECT_READINFO)) {
	USERERROR("You do not have permission to view tags for ".
		  "archive in $pid/$eid!", 1);
    }
}
else {
    PAGEARGERROR("Must provide a current or former experiment index");
}

# Show just the last N records unless request is different.
if (!isset($records)) {
    $records = 100;
}

echo "<center>\n";
if ($instance) {
    echo $instance->PageHeader();
    $url = CreateURL("archive_view", $instance);
}
elseif ($template) {
    echo $template->PageHeader();
    $url = CreateURL("archive_view", $template);
}
else {
    echo $stats->PageHeader();
    $url = CreateURL("archive_view", "index", $exptidx);
}
echo "</center>\n";
echo "<br>\n";

#
# Grab all the (commit/user) tags.
#
$query_result =
    DBQueryFatal("select *,FROM_UNIXTIME(date_created) as date_created ".
		 "  from archive_revisions ".
		 "where archive_idx='$archive_idx' and view='$exptidx' " .
		 "order by date_created desc");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("No tags for experiment $pid/$eid", 1);
}

echo "<table align=center border=1>
      <tr>";
echo "  <th>Run</th>";
echo "  <th>Tag (Click to visit archive)</th>";
echo "  <th>Date</th>";
echo "  <th>Description</th>";
echo "</tr>\n";

while ($row = mysql_fetch_assoc($query_result)) {
    $archive_tag  = $row["tag"];
    $date_tagged  = $row["date_created"];
    $description  = $row["description"];
    $archive_view = $url . "&tag=$archive_tag";

    echo "<tr>";
    echo "  <td align=center>
                <a href=beginexp_html.php3?copyid=$exptidx:$archive_tag>
                    <img border=0 alt=Run src=greenball.gif></a></td>";
    echo "  <td>".
	     "<a href='$archive_view'>$archive_tag</a>".
	 "  </td>";
    
    echo "  <td>$date_tagged</td>";
    echo "  <td>$description</td>";
    echo "</tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

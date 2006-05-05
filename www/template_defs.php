<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Grab a new GUID.
#
function TBNewGUID(&$newguid)
{
    DBQueryFatal("lock tables emulab_indicies write");

    $query_result = 
	DBQueryFatal("select idx from emulab_indicies ".
		     "where name='next_guid'");

    $row = mysql_fetch_array($query_result);
    $newguid = $row['idx'];
    $nextidx = $newguid + 1;
    
    DBQueryFatal("update emulab_indicies set idx='$nextidx' ".
		 "where name='next_guid'");

    DBQueryFatal("unlock tables");
    return 0;
}

#
# Confirm a valid experiment template
#
# usage TBValidExperimentTemplate($guid, $version)
#       returns 1 if valid
#       returns 0 if not valid
#
function TBValidExperimentTemplate($guid, $version)
{
    $guid    = addslashes($guid);
    $version = addslashes($version);

    $query_result =
	DBQueryFatal("select guid from experiment_templates ".
		     "where guid='$guid' and vers='$version'");

    if (mysql_num_rows($query_result) == 0) {
	return 0;
    }
    return 1;
}

#
# Check if a template is hidden.
#
# usage TBIsExperimentTemplateHidden($guid, $version)
#       returns 1 if hidden
#       returns 0 if visible
#
function TBIsExperimentTemplateHidden($guid, $version)
{
    $guid    = addslashes($guid);
    $version = addslashes($version);

    $query_result =
	DBQueryFatal("select hidden from experiment_templates ".
		     "where guid='$guid' and vers='$version'");

    if (mysql_num_rows($query_result) == 0) {
	return 0;
    }
    
    $row  = mysql_fetch_array($query_result);
    
    return $row[hidden];
}

#
# Experiment Template permission checks; using the experiment access checks.
#
# Usage: TBExptTemplateAccessCheck($uid, $guid, $access_type)
#	 returns 0 if not allowed.
#        returns 1 if allowed.
# 
function TBExptTemplateAccessCheck($uid, $guid, $access_type)
{
    global $TB_EXPT_READINFO;
    global $TB_EXPT_MODIFY;
    global $TB_EXPT_DESTROY;
    global $TB_EXPT_UPDATE;
    global $TB_EXPT_MIN;
    global $TB_EXPT_MAX;
    global $TBDB_TRUST_USER;
    global $TBDB_TRUST_LOCALROOT;
    global $TBDB_TRUST_GROUPROOT;
    global $TBDB_TRUST_PROJROOT;
    $mintrust;

    if ($access_type < $TB_EXPT_MIN ||
	$access_type > $TB_EXPT_MAX) {
	TBERROR("Invalid access type: $access_type!", 1);
    }

    #
    # Admins do whatever they want!
    # 
    if (ISADMIN()) {
	return 1;
    }
    $guid = addslashes($guid);

    $query_result =
	DBQueryFatal("select pid,gid,uid from experiment_templates where ".
		     "guid='$guid' limit 1");
    
    if (mysql_num_rows($query_result) == 0) {
	return 0;
    }
    $row  = mysql_fetch_array($query_result);
    $pid  = $row[pid];
    $gid  = $row[gid];
    $head = $row[uid];

    if ($access_type == $TB_EXPT_READINFO) {
	$mintrust = $TBDB_TRUST_USER;
    }
    else {
	$mintrust = $TBDB_TRUST_LOCALROOT;
    }

    #
    # Either proper permission in the group, or group_root in the project.
    # This lets group_roots muck with other peoples experiments, including
    # those in groups they do not belong to.
    #
    return TBMinTrust(TBGrpTrust($uid, $pid, $gid), $mintrust) ||
	TBMinTrust(TBGrpTrust($uid, $pid, $pid), $TBDB_TRUST_GROUPROOT);
}

# Helper function
function ShowItem($tag, $value, $default = "&nbsp")
{
    if (!isset($value)) {
	$value = $default;
    }
    echo "<tr><td>${tag}: </td><td class=left>$value</td></tr>\n";
}

function MakeLink($which, $args, $text)
{
    $page = "";
    
    if ($which == "project") {
	$page = "showproject.php3";
    }
    elseif ($which == "user") {
	$page = "showuser.php3";
    }
    elseif ($which == "template") {
	$page = "template_show.php";
    }
    elseif ($which == "metadata") {
	$page = "template_metadata.php";
    }
    elseif ($which == "instance") {
	$page = "instance_show.php";
    }
    elseif ($which == "run") {
	$page = "experimentrun_show.php";
    }
    return "<a href=${page}?${args}>$text</a>";
}

#
# Display a template in its own table.
#
function SHOWTEMPLATE($guid, $version)
{
    $query_result =
	DBQueryFatal("select * from experiment_templates ".
		     "where guid='$guid' and vers='$version'");

    if (!mysql_num_rows($query_result)) {
	USERERROR("Experiment Template $guid/$version is no longer a ".
		  "valid template!", 1);
    }

    $row = mysql_fetch_array($query_result);

    #
    # We need the metadata guid/version for the TID and description since
    # they are mungable metadata.
    #
    if (! TBTemplateMetadataLookupGUID($guid, $version, "TID",
				       &$tid_guid, &$tid_version)) {
	TBERROR("Could not find TID guid/vers for $guid/$version", 1);
    }
    if (! TBTemplateMetadataLookupGUID($guid, $version, "description",
				       &$desc_guid, &$desc_version)) {
	TBERROR("Could not find description guid/vers for $guid/$version", 1);
    }
    
    #
    # Generate the table.
    #
    echo "<center>
           <h3>Details</h3>
          </center>\n";

    echo "<table align=center cellpadding=2 cellspacing=2 border=1>\n";
    
    ShowItem("GUID",
	     MakeLink("template",
		      "guid=$guid&version=$version", "$guid/$version"));
    ShowItem("ID",
	     MakeLink("metadata",
		      "action=modify&guid=$guid&version=$version".
		      "&metadata_guid=$tid_guid&metadata_vers=$tid_version",
		      $row['tid']));
    ShowItem("Project",
	     MakeLink("project", "pid=" . $row['pid'], $row['pid']));
    ShowItem("Group",       $row['gid']);
    ShowItem("Creator",
	     MakeLink("user", "target_uid=" . $row['uid'], $row['uid']));
    ShowItem("Created",     $row['created']);

    if (strlen($row['description']) > 40) {
	$row['description'] = substr($row['description'], 0, 40) .
	    " <b>... </b>";
    }
    
    ShowItem("Description", 
	     MakeLink("metadata",
		      "action=modify&guid=$guid&version=$version".
		      "&metadata_guid=$desc_guid&metadata_vers=$desc_version",
		      $row['description']));
    
    if (isset($row['parent_guid'])) {
	$parent_guid = $row['parent_guid'];
	$parent_vers = $row['parent_vers'];
	ShowItem("Parent Template",
		 MakeLink("template",
			  "guid=$parent_guid&version=$parent_vers",
			  "$parent_guid/$parent_vers"));
    }
    echo "</table>\n";
}

#
# Display template parameters and default values in a table
#
function SHOWTEMPLATEPARAMETERS($guid, $version)
{
    if (!TBTemplatePidEid($guid, $version, &$pid, &$eid))
	return;
    
    $query_result =
	DBQueryFatal("select * from virt_parameters ".
		     "where pid='$pid' and eid='$eid'");

    if (mysql_num_rows($query_result)) {
	echo "<center>
               <h3>Parameters</h3>
             </center> 
             <table align=center border=1 cellpadding=5 cellspacing=2>\n";

 	echo "<tr>
                <th>Name</th>
                <th>Default Value</th>
              </tr>\n";

	while ($row = mysql_fetch_array($query_result)) {
	    $name	= $row['name'];
	    $value	= $row['value'];
	    if (!isset($value)) {
		$value = "&nbsp";
	    }

	    echo "<tr>
                   <td>$name</td>
                   <td>$value</td>
                  </tr>\n";
  	}
	echo "</table>\n";
	return 1;
    }
    return 0;
}

#
# Display template metadata and values in a table
#
function SHOWTEMPLATEMETADATA($guid, $version)
{
    $query_result =
	DBQueryFatal("select i.* from experiment_template_metadata as m ".
		     "left join experiment_template_metadata_items as i on ".
		     "     m.metadata_guid=i.guid and m.metadata_vers=i.vers ".
		     "where m.parent_guid='$guid' and ".
		     "      m.parent_vers='$version' and m.internal=0");

    if (mysql_num_rows($query_result)) {
	echo "<center>
               <h3>Metadata</h3>
             </center> 
             <table align=center border=1 cellpadding=5 cellspacing=2>\n";

 	echo "<tr>
                <th>Edit</th>
                <th>Name</th>
                <th>Value</th>
              </tr>\n";

	while ($row = mysql_fetch_array($query_result)) {
	    $name	   = $row['name'];
	    $value	   = $row['value'];
	    $metadata_guid = $row['guid'];
	    $metadata_vers = $row['vers'];
	    if (!isset($value)) {
		$value = "&nbsp";
	    }
	    elseif (strlen($value) > 40) {
		$value = substr($value, 0, 40) . " <b>... </b>";
	    }

	    echo "<tr>
   	           <td align=center>
                     <a href='template_metadata.php?action=modify".
		        "&guid=$guid&version=$version".
		        "&metadata_guid=$metadata_guid".
		        "&metadata_vers=$metadata_vers'>
                     <img border=0 alt='modify' src='greenball.gif'></A></td>
                   <td>$name</td>
                   <td>
                       <a href='template_metadata.php?action=modify".
		          "&guid=$guid&version=$version".
		          "&metadata_guid=$metadata_guid".
		          "&metadata_vers=$metadata_vers'>
                         $value</a></td>
                  </tr>\n";
  	}
	echo "</table>\n";
	return 1;
    }
    return 0;
}

#
# Display template instance binding values in a table
#
function SHOWTEMPLATEINSTANCEBINDINGS($guid, $version, $instance_idx)
{
    $query_result =
	DBQueryFatal("select * from experiment_template_instance_bindings ".
		     "where parent_guid='$guid' and parent_vers='$version' ".
		     "      and instance_idx='$instance_idx'");


    if (mysql_num_rows($query_result)) {
	echo "<center>
               <h3>Template Instance Bindings</h3>
             </center> 
             <table align=center border=1 cellpadding=5 cellspacing=2>\n";

 	echo "<tr>
                <th>Name</th>
                <th>Value</th>
              </tr>\n";

	while ($row = mysql_fetch_array($query_result)) {
	    $name	= $row['name'];
	    $value	= $row['value'];
	    if (!isset($value)) {
		$value = "&nbsp";
	    }

	    echo "<tr>
                   <td>$name</td>
                   <td>$value</td>
                  </tr>\n";
  	}
	echo "</table>\n";
	return 1;
    }
    return 0;
}

#
# Display a list of templates in its own table. Optional 
#
function SHOWTEMPLATELIST($which, $all, $myuid, $id, $gid = "")
{
    $extraclause = ($all ? "" : "and parent_guid is null");
	
    if ($which == "USER") {
	$where = "t.uid='$id'";
	$title = "Current";
    }
    elseif ($which == "PROJ") {
	$where = "t.pid='$id'";
	$title = "Project";
    }
    elseif ($which == "GROUP") {
	$where = "t.pid='$id' and t.gid='$gid'";
	$title = "Group";
    }
    elseif ($which == "TEMPLATE") {
	$where = "t.guid='$id' or t.parent_guid='$id'";
	$title = "Template";
    }
    else {
	$where = "1";
    }

    if (ISADMIN()) {
	$query_result =
	    DBQueryFatal("select t.* from experiment_templates as t ".
			 "where ($where) $extraclause ".
			 "order by t.pid,t.tid,t.created ");
    }
    else {
	$query_result =
	    DBQueryFatal("select t.* from experiment_templates as t ".
			 "left join group_membership as g on g.pid=t.pid and ".
			 "     g.gid=t.gid and g.uid='$myuid' ".
			 "where g.uid is not null and ($where) $extraclause ".
			 "order by t.pid,t.tid,t.created");
    }

    if (mysql_num_rows($query_result)) {
	echo "<center>
               <h3>Experiment Templates</h3>
             </center> 
             <table align=center border=1 cellpadding=5 cellspacing=2>\n";

 	echo "<tr>
                <th>GUID</th>
                <th>TID</th>
                <th>PID/GID</th>
              </tr>\n";

	while ($row = mysql_fetch_array($query_result)) {
	    $guid	= $row['guid'];
	    $pid	= $row['pid'];
	    $gid	= $row['gid'];
	    $tid	= $row['tid'];
	    $vers       = $row['vers'];

	    echo "<tr>
                   <td>" . MakeLink("template",
				    "guid=$guid&version=$vers", "$guid/$vers")
		      . "</td>
                   <td>$tid</td>
                   <td>" . MakeLink("project", "pid=$pid", "$pid/$gid") ."</td>
                  </tr>\n";
  	}
	echo "</table>\n";
    }
}

#
# Show the instance list for a template.
#
function SHOWTEMPLATEINSTANCES($guid, $version)
{
    $query_result =
	DBQueryFatal("select e.*,count(r.node_id) as nodes, ".
		     "    round(minimum_nodes+.1,0) as min_nodes ".
		     "from experiment_template_instances as i ".
		     "left join experiments as e on e.idx=i.exptidx ".
		     "left join reserved as r on e.pid=r.pid and ".
		     "     e.eid=r.eid ".
		     "where e.pid is not null and ".
		     "      (i.parent_guid='$guid' and ".
		     "       i.parent_vers='$version') ".
		     "group by e.pid,e.eid order by e.state,e.eid");

    if (mysql_num_rows($query_result)) {
	echo "<center>
               <h3>Template Instances</h3>
             </center> 
             <table align=center border=1 cellpadding=5 cellspacing=2>\n";

	echo "<tr>
               <th>EID</th>
               <th>State</th>
               <th align=center>Nodes</th>
              </tr>\n";

	$idlemark = "<b>*</b>";
	$stalemark = "<b>?</b>";
	
	while ($row = mysql_fetch_array($query_result)) {
	    $pid       = $row['pid'];
	    $eid       = $row['eid'];
	    $state     = $row['state'];
	    $nodes     = $row['nodes'];
	    $minnodes  = $row['min_nodes'];
	    $idlehours = TBGetExptIdleTime($pid, $eid);
	    $stale     = TBGetExptIdleStale($pid, $eid);
	    $ignore    = $row['idle_ignore'];
	    $name      = $row['expt_name'];
	    
	    if ($nodes == 0) {
		$nodes = "<font color=green>$minnodes</font>";
	    }
	    elseif ($row[swap_requests] > 0) {
		$nodes .= $idlemark;
	    }

	    $idlestr = $idlehours;
	    
	    if ($idlehours > 0) {
		if ($stale) {
		    $idlestr .= $stalemark;
		}
		if ($ignore) {
		    $idlestr = "($idlestr)";
		}
	    }
	    elseif ($idlehours == -1) {
		$idlestr = "&nbsp;";
	    }
	    
	    echo "<tr>
                   <td><A href='showexp.php3?pid=$pid&eid=$eid'>$eid</A></td>
  		   <td>$state</td>
                   <td align=center>$nodes</td>
                 </tr>\n";
	}
	echo "</table>\n";
    }
}

#
# Show the historical instance list for a template.
#
function SHOWTEMPLATEHISTORY($guid, $version, $expand)
{
    $query_result =
	DBQueryFatal("select * from experiment_template_instances as i ".
		     "where (i.parent_guid='$guid' and ".
		     "       i.parent_vers='$version') ".
		     "order by i.start_time");

    if (mysql_num_rows($query_result)) {
	echo "<center>
               <h3>Template History (Swapins)</h3>
             </center> 
             <table align=center border=1 cellpadding=5 cellspacing=2>\n";

	echo "<tr>
               <th align=center>Expand</th>
               <th>EID</th>
               <th>IDX</th>
               <th>UID</th>
               <th>Start Time</th>
               <th>Stop Time</th>
               <th align=center>Archive</th>
              </tr>\n";

	$idlemark = "<b>*</b>";
	$stalemark = "<b>?</b>";
	
	while ($row = mysql_fetch_array($query_result)) {
	    $pid       = $row['pid'];
	    $eid       = $row['eid'];
	    $uid       = $row['uid'];
	    $start     = $row['start_time'];
	    $stop      = $row['stop_time'];
	    $exptidx   = $row['exptidx'];
	    $idx       = $row['idx'];

	    if (! isset($stop)) {
		$stop = "&nbsp";
	    }

	    $expandit = ((isset($expand) && $expand == $idx) ? 1 : 0);
	    
	    echo "<tr>\n";
	    echo " <td align=center>";
	    if ($expandit) {
		echo "<a href=template_history.php".
                         "?guid=$guid&version=$version>".
		         "<img border=0 alt='c' src='/icons/down.png'></a>\n";
	    }
	    else {
 		echo "<a href=template_history.php".
                         "?guid=$guid&version=$version&expand=$idx#$idx>".
		         "<img border=0 alt='e' src='/icons/right.png'></a>\n";
	    }
	    echo " </td>
                   <td>$eid</td>\n";
	    
	    echo " <td align=center>".
 		    MakeLink("instance",
			     "guid=$guid&version=$version&exptidx=$exptidx",
			     "$idx");
	    echo " </td>
  		   <td>$uid</td>
                   <td>$start</td>
                   <td>$stop</td>
                   <td align=center>
                     <a href=cvsweb/cvswebwrap.php3/$exptidx/?exptidx=$exptidx>
                     <img border=0 alt='i' src='greenball.gif'></a></td>
                 </tr>\n";

	    if ($expandit) {
		echo "<tr>\n";
		echo " <a NAME=$idx></a>\n";
		echo "<td>&nbsp</td>\n";
		echo " <td colspan=6>\n";
		SHOWTEMPLATEINSTANCERUNS($guid, $version, $exptidx, 0);
		echo " </td>\n";
		echo "</tr>\n";
	    }
	}
	echo "</table>\n";
    }
}

#
# Show the historical instance list for a template.
#
function SHOWTEMPLATEINSTANCE($guid, $version, $exptidx, $withruns)
{
    $query_result =
	DBQueryFatal("select * from experiment_template_instances as i ".
		     "where (i.parent_guid='$guid' and ".
		     "       i.parent_vers='$version' and ".
		     "       i.exptidx='$exptidx') ".
		     "order by i.start_time");

    if (mysql_num_rows($query_result)) {
	$irow = mysql_fetch_array($query_result);
	
	echo "<center>
               <h3>Template Instance</h3>
             </center>\n";

	echo "<table align=center cellpadding=2 cellspacing=2 border=1>\n";
    
	ShowItem("Template",
		 MakeLink("template",
			  "guid=$guid&version=$version", "$guid/$version"));
	ShowItem("ID",          $irow['exptidx']);
	ShowItem("Project",
		 MakeLink("project", "pid=" . $irow['pid'], $irow['pid']));
	ShowItem("Creator",
		 MakeLink("user", "target_uid=" . $irow['uid'], $irow['uid']));
	ShowItem("Started",     $irow['start_time']);
	ShowItem("Stopped",     (isset($irow['stop_time']) ?
				 $irow['stop_time'] : "&nbsp"));
	ShowItem("Current Run", (isset($irow['runidx']) ?
				 $irow['runidx'] : "&nbsp"));
	echo "</table>\n";

	if ($withruns) {
	    $query_result =
		DBQueryFatal("select * from experiment_runs ".
			     "where exptidx='$exptidx'");

	    if (mysql_num_rows($query_result)) {
		echo "<center>
                        <h3>Instance History (Runs)</h3>
                      </center> 
                      <table align=center border=1
                             cellpadding=5 cellspacing=2>\n";

		echo "<tr>
                       <th align=center>Expand</th>
                       <th align=center>Archive</th>
                       <th>RunID</th>
                       <th>ID</th>
                       <th>Start Time</th>
                       <th>Stop Time</th>
                       <th>Description</th>
                      </tr>\n";

		while ($rrow = mysql_fetch_array($query_result)) {
		    $runidx    = $rrow['idx'];
		    $runid     = $rrow['runid'];
		    $start     = $rrow['start_time'];
		    $stop      = $rrow['stop_time'];
		    $exptidx   = $rrow['exptidx'];
		    $description = $rrow['description'];

		    if (! isset($stop)) {
			$stop = "&nbsp";
		    }
	    
		    echo "<tr>\n";
		    echo " <td align=center>".
			MakeLink("run",
				 "guid=$guid&version=$version".
				 "&exptidx=$exptidx&runidx=$runidx",
				 "<img border=0 alt='i' src='greenball.gif'>");
		    echo " </td>
                            <td align=center>
                                <a href=cvsweb/cvswebwrap.php3".
			           "/$exptidx/history/$runid/?exptidx=$exptidx>
                                <img border=0 alt='i'
                                     src='greenball.gif'></a></td>
                            <td>$runid</td>
  		            <td>$runidx</td>
                            <td>$start</td>
                            <td>$stop</td>
                            <td>$description</td>
                           </tr>\n";
		}
		echo "</table>\n";
	    }
	}
    }
}

#
# Show the run list for an instance.
#
function SHOWTEMPLATEINSTANCERUNS($guid, $version, $exptidx, $withheader)
{
    $query_result =
	DBQueryFatal("select * from experiment_runs ".
		     "where exptidx='$exptidx'");

    if (mysql_num_rows($query_result)) {
	if ($withheader) {
	    echo "<center>
                    <h3>Instance History (Runs)</h3>
                  </center> \n";
	}
	echo "<table align=center border=1 cellpadding=5 cellspacing=2>\n";

	echo "<tr>
               <th align=center>Expand</th>
               <th align=center>Archive</th>
               <th>RunID</th>
               <th>ID</th>
               <th>Start Time</th>
               <th>Stop Time</th>
               <th>Description</th>
              </tr>\n";

	while ($rrow = mysql_fetch_array($query_result)) {
	    $runidx    = $rrow['idx'];
	    $runid     = $rrow['runid'];
	    $start     = $rrow['start_time'];
	    $stop      = $rrow['stop_time'];
	    $exptidx   = $rrow['exptidx'];
	    $description = $rrow['description'];

	    if (! isset($stop)) {
		$stop = "&nbsp";
	    }
	    
	    echo "<tr>\n";
	    echo " <td align=center>".
		    MakeLink("run",
			     "guid=$guid&version=$version".
			     "&exptidx=$exptidx&runidx=$runidx",
			     "<img border=0 alt='i' src='greenball.gif'>");
	    echo " </td>
                    <td align=center>
                        <a href=cvsweb/cvswebwrap.php3".
			   "/$exptidx/history/$runid/?exptidx=$exptidx>
                        <img border=0 alt='i'
                             src='greenball.gif'></a></td>
                    <td>$runid</td>
  		    <td>$runidx</td>
                    <td>$start</td>
                    <td>$stop</td>
                    <td>$description</td>
                   </tr>\n";
	}
	echo "</table>\n";
    }
}

#
# Show the historical instance list for a template.
#
function SHOWEXPERIMENTRUN($exptidx, $runidx)
{
    $query_result =
	DBQueryFatal("select r.*,i.parent_guid,i.parent_vers ".
		     "  from experiment_runs as r ".
		     "left join experiment_template_instances as i on ".
		     "     i.exptidx=r.exptidx ".
		     "where r.exptidx='$exptidx' and r.idx='$runidx'");

    if (mysql_num_rows($query_result)) {
	$row = mysql_fetch_array($query_result);
	$guid     = $row['parent_guid'];
	$version  = $row['parent_vers'];
	$start    = $row['start_time'];
	$stop     = $row['stop_time'];

	if (!isset($stop))
	    $stop = "&nbsp";
	
	echo "<center>
               <h3>Experiment Run</h3>
             </center>\n";

	echo "<table align=center cellpadding=2 cellspacing=2 border=1>\n";
    
	ShowItem("Template",
		 MakeLink("template",
			  "guid=$guid&version=$version", "$guid/$version"));
	ShowItem("Experiment",
		 MakeLink("instance",
			  "guid=$guid&version=$version&exptidx$exptidx",
			  "$exptidx"));
	ShowItem("ID",          $runidx);
	ShowItem("Started",     $start);
	ShowItem("Stopped",     $stop);

	echo "</table>\n";

	$query_result =
	    DBQueryFatal("select * from experiment_run_bindings ".
			 "where exptidx='$exptidx' and runidx='$runidx'");

	if (mysql_num_rows($query_result)) {
	    echo "<center>
                   <h3>Experiment Run Bindings</h3>
                  </center> 
                  <table align=center border=1 cellpadding=5 cellspacing=2>\n";

	    echo "<tr>
                    <th>Name</th>
                    <th>Value</th>
                  </tr>\n";

	    while ($row = mysql_fetch_array($query_result)) {
		$name	= $row['name'];
		$value	= $row['value'];
		if (!isset($value)) {
		    $value = "&nbsp";
		}

		echo "<tr>
                       <td>$name</td>
                       <td>$value</td>
                      </tr>\n";
	    }
	    echo "</table>\n";
	}
    }
}

#
# Display a metadata item in its own table.
#
function SHOWMETADATAITEM($metadata_guid, $metadata_version)
{
    #
    # See if this item is a current template item.
    #
    unset($template_guid);
    unset($template_vers);
    
    $query_result =
	DBQueryFatal("select * from experiment_template_metadata ".
		     "where metadata_guid='$metadata_guid' and ".
		     "      metadata_vers='$metadata_version'");
    
    if (mysql_num_rows($query_result)) {
	$row = mysql_fetch_array($query_result);
	$template_guid = $row['parent_guid'];
	$template_vers = $row['parent_vers'];
    }

    # Now the metadata info.
    $query_result =
	DBQueryFatal("select * from experiment_template_metadata_items ".
		     "where guid='$metadata_guid' and ".
		     "      vers='$metadata_version'");

    if (!mysql_num_rows($query_result)) {
	USERERROR("Experiment Template Metadata $guid/$version is no longer ".
		  "a valid metadata item!", 1);
    }
    $row = mysql_fetch_array($query_result);

    #
    # Generate the table.
    #
    echo "<table align=center cellpadding=2 cellspacing=2 border=1>\n";
    
    ShowItem("GUID",       "$metadata_guid/$metadata_version");
    ShowItem("Name",        $row['name']);
    ShowItem("Created",     $row['created']);

    if (isset($template_guid)) {
	ShowItem("Template",
		 MakeLink("template",
			  "guid=$template_guid&version=$template_vers",
			  "$template_guid/$template_vers"));
    }
    
    if (isset($row['parent_guid'])) {
	$parent_guid = $row['parent_guid'];
	$parent_vers = $row['parent_vers'];
	ShowItem("Parent Version",
		 MakeLink("metadata",
			  "action=show&guid=$parent_guid&version=$parent_vers",
			  "$parent_guid/$parent_vers"));
    }

    echo "<tr>
              <td align=center colspan=2>
               Metadata Value
              </td>
          </tr>
          <tr>
              <td colspan=2 align=center class=left>
                  <textarea readonly
                    rows=10 cols=80>" .
	            ereg_replace("\r", "", $row['value']) .
	           "</textarea>
              </td>
          </tr>\n";


    
    echo "</table>\n";
}

#
# Dump the image map for a template to the output.
#
function SHOWTEMPLATEGRAPH($guid)
{
    $query_result =
	DBQueryFatal("select * from experiment_template_graphs ".
		     "where parent_guid='$guid'");

    if (!mysql_num_rows($query_result)) {
	USERERROR("Experiment Template graph for $guid is no longer ".
		  "in the DB!", 1);
    }
    $row = mysql_fetch_array($query_result);

    $imap = $row['imap'];

    echo "<center>";
#    echo "<h3>Template Graph</h3>\n";
    echo "<div id=fee style='display: block; overflow: hidden; position: relative; z-index:1010; height: 400px; width: 600px; border: 2px solid black;'>\n";
    echo "<div id=\"D$guid\" style='position:relative;'>\n";

    echo $imap;
    echo "<img id=\"G$guid\" border=0 usemap=\"#TemplateGraph\" ";
    echo "      src='template_graph.php?guid=$guid'>\n";
    echo "</div>\n";
    echo "</div>\n";

    echo "<script type='text/javascript'>
           <!--
             SET_DHTML(\"D$guid\");
           //-->
          </script>\n";
    
    echo "</center>\n";
}

#
# Map pid/tid to a template guid. This only makes sense after a new
# template is created. Needs more thought.
#
function TBPidTid2Template($pid, $tid, &$guid, &$version)
{
    $query_result =
	DBQueryFatal("select * from experiment_templates ".
		     "where pid='$pid' and tid='$tid'");

    if (mysql_num_rows($query_result) == 0) {
	return 0;
    }
    $row = mysql_fetch_array($query_result);
    $guid    = $row['guid'];
    $version = $row['vers'];
    return 1;
}

function TBTemplateInstanceIndex($guid, $version, $exptidx, &$instidx)
{
    $query_result =
	DBQueryFatal("select idx from experiment_template_instances ".
		     "where parent_guid='$guid' and parent_vers='$version' ".
		     "      and exptidx='$exptidx'");

    if (mysql_num_rows($query_result) == 0) {
	return 0;
    }
    $row = mysql_fetch_array($query_result);
    $instidx  = $row['idx'];
    return 1;
}

#
# Map guid to pid/gid.
#
function TBGuid2PidGid($guid, &$pid, &$gid)
{
    $query_result =
	DBQueryFatal("select pid,gid from experiment_templates ".
		     "where guid='$guid' limit 1");

    if (mysql_num_rows($query_result) == 0) {
	return 0;
    }
    $row = mysql_fetch_array($query_result);
    $pid = $row['pid'];
    $gid = $row['gid'];
    return 1;
}

#
# Map guid/version to its underlying experiment.
#
function TBTemplatePidEid($guid, $version, &$pid, &$eid)
{
    $query_result =
	DBQueryFatal("select pid from experiment_templates ".
		     "where guid='$guid' and vers='$version'");

    if (mysql_num_rows($query_result) == 0) {
	return 0;
    }
    $row = mysql_fetch_array($query_result);
    $pid = $row['pid'];
    $eid = "T${guid}-${version}";
    return 1;
}

#
# Map guid/version/exptidx to its run, if any.
#
function TBTemplateCurrentExperimentRun($guid, $version, $exptidx, &$runidx)
{
    $query_result =
	DBQueryFatal("select runidx from experiment_template_instances ".
		     "where parent_guid='$guid' and parent_vers='$version' ".
		     "      and exptidx='$exptidx'");

    if (mysql_num_rows($query_result) == 0) {
	return 0;
    }
    $row = mysql_fetch_array($query_result);
    $runidx = $row['runidx'];
    return 1;
}

#
# Find next candidate for an experiment run. 
#
function TBTemplateNextExperimentRun($guid, $version, $exptidx, &$runidx)
{
    $query_result =
	DBQueryFatal("select MAX(idx) from experiment_runs ".
		     "where exptidx='$exptidx'");

    if (mysql_num_rows($query_result) == 0) {
	return 0;
    }
    $row = mysql_fetch_array($query_result);
    $runidx = $row[0] + 1;
    return 1;
}

#
# Map guid/version to the template tid.
#
function TBTemplateTid($guid, $version, &$tid)
{
    $query_result =
	DBQueryFatal("select tid from experiment_templates ".
		     "where guid='$guid' and vers='$version'");

    if (mysql_num_rows($query_result) == 0) {
	return 0;
    }
    $row = mysql_fetch_array($query_result);
    $tid = $row['tid'];
    return 1;
}

#
# Map guid/version to the template description
#
function TBTemplateDescription($guid, $version, &$description)
{
    $query_result =
	DBQueryFatal("select description from experiment_templates ".
		     "where guid='$guid' and vers='$version'");

    if (mysql_num_rows($query_result) == 0) {
	return 0;
    }
    $row = mysql_fetch_array($query_result);
    $description = $row['description'];
    return 1;
}

#
# Grab array of input files for a template, indexed by input_idx.
#
function TBTemplateInputFiles($guid, $version)
{
    $input_list = array();

    $query_result =
	DBQueryFatal("select * from experiment_template_inputs ".
		     "where parent_guid='$guid' and parent_vers='$version'");

    while ($row = mysql_fetch_array($query_result)) {
	$input_idx = $row['input_idx'];

	$input_query =
	    DBQueryFatal("select input from experiment_template_input_data ".
			 "where idx='$input_idx'");

	$input_row = mysql_fetch_array($input_query);
	$input_list[] = $input_row['input'];
    }
    return $input_list;
}

#
# Find out if an experiment is a template instantiation; used by existing
# pages to alter what they do.
#
function TBIsTemplateInstanceExperiment($exptidx)
{
    $query_result =
	DBQueryFatal("select pid,eid from experiment_template_instances ".
		     "where exptidx='$exptidx'");

    return mysql_num_rows($query_result);
}

#
# Map pid/eid to a template guid.
#
function TBPidEid2Template($pid, $eid, &$guid, &$version, &$instidx)
{
    $query_result =
	DBQueryFatal("select * from experiment_template_instances ".
		     "where pid='$pid' and eid='$eid'");

    if (mysql_num_rows($query_result) == 0) {
	return 0;
    }
    $row = mysql_fetch_array($query_result);
    $guid    = $row['parent_guid'];
    $version = $row['parent_vers'];
    $instidx = $row['idx'];
    return 1;
}

#
# Map metadata to its template. This must be a current item associated with
# a template.
#
function TBValidTemplateMetadata($metadata_guid, $metadata_version,
				 $template_guid, $template_version)
{
    $query_result =
	DBQueryFatal("select internal from experiment_template_metadata ".
		     "where parent_guid='$template_guid' and ".
		     "      parent_vers='$template_version' and ".
		     "      metadata_guid='$metadata_guid' and ".
		     "      metadata_vers='$metadata_version'");

    return mysql_num_rows($query_result);
}

#
# Map metadata to its template GUID only, as for permission purposes.
#
function TBMetadataTemplate($metadata_guid, $metadata_version, &$template_guid)
{
    $query_result =
	DBQueryFatal("select template_guid ".
		     "    from experiment_template_metadata_items ".
		     "where guid='$metadata_guid' and ".
		     "      vers='$metadata_version'");

    if (mysql_num_rows($query_result) == 0) {
	return 0;
    }
    $row = mysql_fetch_array($query_result);
    $template_guid = $row['template_guid'];
    return 1;
}

#
# Return array of metadata data. 
#
function TBMetadataData($metadata_guid, $metadata_version, &$metadata_data)
{
    $query_result =
	DBQueryFatal("select * from experiment_template_metadata_items ".
		     "where guid='$metadata_guid' and ".
		     "      vers='$metadata_version'");

    if (mysql_num_rows($query_result) == 0) {
	return 0;
    }
    $metadata_data = mysql_fetch_array($query_result);
    return 1;
}

#
# Get a metadata value given a name and a template. 
#
function TBTemplateMetadataLookup($template_guid, $template_version,
				  $metadata_name, &$metadata_value)
{
    $query_result =
	DBQueryFatal("select i.value from experiment_template_metadata as m ".
		     "left join experiment_template_metadata_items as i on ".
		     "     i.guid=m.metadata_guid and i.vers=m.metadata_vers ".
		     "where m.parent_guid='$template_guid' and ".
		     "      m.parent_vers='$template_version' and ".
		     "      i.name='$metadata_name'");

    if (mysql_num_rows($query_result) == 0) {
	return 0;
    }
    $row = mysql_fetch_array($query_result);
    $metadata_value = $row['value'];
    return 1;
}

#
# Get a metadata guid/version given a name and a template. 
#
function TBTemplateMetadataLookupGUID($template_guid, $template_version,
				      $metadata_name,
				      &$metadata_guid, &$metadata_version)
{
    $query_result =
	DBQueryFatal("select i.guid,i.vers ".
		     "    from experiment_template_metadata as m ".
		     "left join experiment_template_metadata_items as i on ".
		     "     i.guid=m.metadata_guid and i.vers=m.metadata_vers ".
		     "where m.parent_guid='$template_guid' and ".
		     "      m.parent_vers='$template_version' and ".
		     "      i.name='$metadata_name'");

    if (mysql_num_rows($query_result) == 0) {
	return 0;
    }
    $row = mysql_fetch_array($query_result);
    $metadata_guid    = $row['guid'];
    $metadata_version = $row['vers'];
    return 1;
}

#
# Return an array of the formal parameters for a template.
#
function TBTemplateFormalParameters($guid, $version, &$parameters)
{
    $parameters = array();

    if (!TBTemplatePidEid($guid, $version, &$pid, &$eid))
	return -1;
    
    $query_result =
	DBQueryFatal("select * from virt_parameters ".
		     "where pid='$pid' and eid='$eid'");

    while ($row = mysql_fetch_array($query_result)) {
	$name	= $row['name'];
	$value	= $row['value'];

	$parameters[$name] = $value;
    }
    return 0;
}

#
# Return an array of the bindings for a template instance.
#
function TBTemplateInstanceBindings($guid, $version, $instance_idx, &$bindings)
{
    $bindings = array();

    $query_result =
	DBQueryFatal("select * from experiment_template_instance_bindings ".
		     "where parent_guid='$guid' and parent_vers='$version' ".
		     "      and instance_idx='$instance_idx'");

    while ($row = mysql_fetch_array($query_result)) {
	$name	= $row['name'];
	$value	= $row['value'];

	$bindings[$name] = $value;
    }
    return 0;
}

#
# Confirm a valid experiment template instance
#
# usage TBValidExperimentTemplateInstance($guid, $version, $exptidx)
#       returns 1 if valid
#       returns 0 if not valid
#
function TBValidExperimentTemplateInstance($guid, $version, $exptidx)
{
    $guid    = addslashes($guid);
    $version = addslashes($version);
    $exptidx = addslashes($exptidx);

    $query_result =
	DBQueryFatal("select parent_guid ".
		     "  from experiment_template_instances as i ".
		     "left join experiments as e on e.idx=i.exptidx ".
		     "where (i.parent_guid='$guid' and ".
		     "       i.parent_vers='$version' and ".
		     "       i.exptidx='$exptidx') and ".
		     "      (e.eid is not null and e.state='active')");

    if (mysql_num_rows($query_result) == 0) {
	return 0;
    }
    return 1;
}

#
# Check to see if a runidx is a valid run.
#
function TBValidExperimentRun($exptidx, $runidx)
{
    $query_result =
	DBQueryFatal("select * from experiment_runs ".
		     "where exptidx='$exptidx' and idx='$runidx'");

    return mysql_num_rows($query_result);
}

#
# Return number of parameters.
#
function TemplateParameterCount($guid, $version)
{
    if (!TBTemplatePidEid($guid, $version, &$pid, &$eid))
	return;
    
    $query_result =
	DBQueryFatal("select * from virt_parameters ".
		     "where pid='$pid' and eid='$eid'");

    return mysql_num_rows($query_result);
}

#
# Return number of metadata items
#
function TemplateMetadataCount($guid, $version)
{
    $query_result =
	DBQueryFatal("select internal from experiment_template_metadata as m ".
		     "where m.parent_guid='$guid' and ".
		     "      m.parent_vers='$version' and m.internal=0");

    return mysql_num_rows($query_result);
}

#
# Return number of instances
#
function TemplateInstanceCount($guid, $version)
{
    $query_result =
	DBQueryFatal("select * from experiment_template_instances as i ".
		     "where (i.parent_guid='$guid' and ".
		     "       i.parent_vers='$version')");

    return mysql_num_rows($query_result);
}

#
# Slot checking support
#
function TBvalid_template_description($token) {
    return TBcheck_dbslot($token, "experiment_templates", "description",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_guid($token) {
    return TBcheck_dbslot($token, "experiment_templates", "guid",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_template_metadata_name($token) {
    return TBcheck_dbslot($token, "experiment_template_metadata", "name",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}
function TBvalid_template_metadata_value($token) {
    return TBcheck_dbslot($token, "experiment_template_metadata", "value",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}

?>

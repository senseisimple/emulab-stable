<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
#
# The template class is really just a wrapper around the DB data, plus
# some access routines and printing functions. 
#
class Template
{
    var	$template;
    
    function Template($guid, $vers) {
	$guid = addslashes($guid);
	$vers = addslashes($vers);
	
	$query_result =
	    DBQueryWarn("select * from experiment_templates ".
			"where guid='$guid' and vers='$vers'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->template = NULL;
	    return;
	}
	$this->template = mysql_fetch_array($query_result);
    }

    # Hmm, how does one cause an error in a php constructor?
    function IsValid() {
	return !is_null($this->template);
    }

    # Do class level lookup.
    function Lookup($guid, $vers) {
	$foo = new Template($guid, $vers);

	if ($foo->IsValid())
	    return $foo;
	return null;
    }
    # Do class level lookup for the root template.
    function LookupRoot($guid) {
	$foo = new Template($guid, 1); 

	if ($foo->IsValid())
	    return $foo;
	return null;
    }

    #
    # Refresh a template instance by reloading from the DB.
    #
    function Refresh() {
	if (! $this->IsValidTemplate())
	    return -1;

	$guid = $this->guid();
	$vers = $this->vers();
    
	$query_result =
	    DBQueryWarn("select * from experiment_templates ".
			"where guid='$guid' and vers='$vers'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->template = NULL;
	    return -1;
	}
	$this->template = mysql_fetch_array($query_result);
	return 0;
    }

    # accessors
    function guid() {
	return (is_null($this->template) ? -1 : $this->template['guid']);
    }
    function vers() {
	return (is_null($this->template) ? -1 : $this->template['vers']);
    }
    function pid() {
	return (is_null($this->template) ? -1 : $this->template['pid']);
    }
    function gid() {
	return (is_null($this->template) ? -1 : $this->template['gid']);
    }
    function eid() {
	return (is_null($this->template) ? -1 : $this->template['eid']);
    }
    function tid() {
	return (is_null($this->template) ? -1 : $this->template['tid']);
    }
    function uid() {
	return (is_null($this->template) ? -1 : $this->template['uid']);
    }
    function IsHidden() {
	return (is_null($this->template) ? -1 : $this->template['hidden']);
    }
    function created() {
	return (is_null($this->template) ? -1 : $this->template['created']);
    }
    function description() {
	return (is_null($this->template) ? -1 :
		$this->template['description']);
    }
    function parent_guid() {
	return (is_null($this->template) ? -1 :$this->template['parent_guid']);
    }
    function parent_vers() {
	return (is_null($this->template) ? -1 :$this->template['parent_vers']);
    }

    # The root template has no parent guid.
    function IsRoot() {
	if (is_null($this->template))
	    return -1;

	return is_null($this->template['parent_guid']);
    }

    function AccessCheck($uid, $access_type) {
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
	$pid = $this->pid();
	$gid = $this->gid();

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

    #
    # Display a template in its own table.
    #
    function Show() {
	$guid        = $this->guid();
	$vers        = $this->vers();
	$pid         = $this->pid();
	$gid         = $this->gid();
	$uid         = $this->uid();
	$tid         = $this->tid();
	$created     = $this->created();
	$description = $this->description();

        #
        # We need the metadata guid/version for the TID and description since
        # they are mungable metadata.
        #
	$tid_metadata  = $this->LookupMetadataByName("TID");
	$desc_metadata = $this->LookupMetadataByName("description");
	
	if ($tid_metadata == NULL) {
	    TBERROR("Could not find Metadata 'TID' for $guid/$vers", 1);
	}
	if ($desc_metadata == NULL) {
	    TBERROR("Could not find Metadata 'description' for $guid/$vers",1);
	}
	$tid_guid  = $tid_metadata->guid();
	$tid_vers  = $tid_metadata->vers();
	$desc_guid = $desc_metadata->guid();
	$desc_vers = $desc_metadata->vers();
    
        #
        # Generate the table.
        #
	echo "<center>
               <h3>Details</h3>
              </center>\n";

	echo "<table align=center cellpadding=2 cellspacing=2 border=1>\n";
    
	ShowItem("GUID",
		 MakeLink("template",
			  "guid=$guid&version=$vers", "$guid/$vers"));
	
	ShowItem("ID",
		 MakeLink("metadata",
			  "action=modify&guid=$guid&version=$vers".
			  "&metadata_guid=$tid_guid&metadata_vers=$tid_vers",
			  $tid));
	
	ShowItem("Project", MakeLink("project", "pid=$pid", $pid));
	ShowItem("Group",   $gid);
	ShowItem("Creator", MakeLink("user", "target_uid=$uid", $uid));
	ShowItem("Created", $created);

	$onmouseover = MakeMouseOver($description);
	if (strlen($description) > 40) {
	    $description = substr($description, 0, 40) . " <b>... </b>";
	}
	ShowItem("Description", 
		 MakeLink("metadata",
			  "action=modify&guid=$guid&version=$vers".
			  "&metadata_guid=$desc_guid&metadata_vers=$desc_vers".
			  " $onmouseover",
			  $description));
    
	if (! $this->IsRoot()) {
	    $parent_guid = $this->parent_guid();
	    $parent_vers = $this->parent_vers();
	    
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
    function ShowParameters() {
	$guid = $this->guid();
	$vers = $this->vers();
	
	$query_result =
	    DBQueryFatal("select p.name,p.value, ".
			 "       p.metadata_guid,p.metadata_vers, ".
			 "       m.value as description ".
			 "   from experiment_template_parameters as p ".
			 "left join experiment_template_metadata_items as m ".
			 "  on m.guid=p.metadata_guid and ".
			 "     m.vers=p.metadata_vers ".
			 "where p.parent_guid='$guid' and ".
			 "      p.parent_vers='$vers'");
    
	if (!$query_result ||
	    !mysql_num_rows($query_result))
	    return 0;

	echo "<center>
               <h3>Parameters</h3>
             </center> 
             <table align=center border=1 cellpadding=5 cellspacing=2>\n";

 	echo "<tr>
                <th>Name</th>
                <th>Default Value</th>
                <th>Description</th>
              </tr>\n";

	while ($row = mysql_fetch_array($query_result)) {
	    $name	   = $row['name'];
	    $value	   = $row['value'];
	    $metadata_guid = $row['metadata_guid'];
	    $metadata_vers = $row['metadata_vers'];
	    $description   = $row['description'];
	    
	    if (!isset($value)) {
		$value = "&nbsp";
	    }

	    echo "<tr>
                   <td>$name</td>
                   <td>$value</td>";

	    if (is_null($description)) {
		echo "<td>".
		     "<a href=template_metadata.php?action=add&".
		     "guid=$guid&version=$vers&type=parameter_description&".
		     "formfields[name]=${name}>Click to add</a></td>\n";
	    }
	    else {
		$onmouseover = MakeMouseOver($description);
		if (strlen($description) > 30) {
		    $description =
			substr($description, 0, 30) . " <b>... </b>";
		}
		echo "<td><a href='template_metadata.php?action=modify".
		     "&guid=$guid&version=$vers".
		     "&metadata_guid=$metadata_guid".
		     "&metadata_vers=$metadata_vers' $onmouseover>".
  		     "$description</a></td>\n";
	    }
	    echo "</tr>\n";
	    }
	echo "</table>\n";
	return 1;
    }
	
    #
    # Display template metadata and values in a table
    #
    function ShowMetadata() {
	$guid = $this->guid();
	$vers = $this->vers();
	
	$query_result =
	    DBQueryFatal("select i.* from experiment_template_metadata as m ".
		     "left join experiment_template_metadata_items as i on ".
		     "     m.metadata_guid=i.guid and m.metadata_vers=i.vers ".
		     "where m.parent_guid='$guid' and ".
		     "      m.parent_vers='$vers' and ".
		     "      m.internal=0 and m.hidden=0");

	if (! mysql_num_rows($query_result))
	    return 0;

	echo "<center>
               <h3>Metadata</h3>
             </center> 
             <table align=center border=1 cellpadding=5 cellspacing=2>\n";

 	echo "<tr>
                <th>Edit</th>
                <th>Delete</th>
                <th>Name</th>
                <th>Value</th>
              </tr>\n";

	while ($row = mysql_fetch_array($query_result)) {
	    $name	   = $row['name'];
	    $value	   = $row['value'];
	    $metadata_guid = $row['guid'];
	    $metadata_vers = $row['vers'];
	    $onmouseover   = "";
	    
	    if (!isset($value) || $value == "") {
		$value = "&nbsp";
	    }
	    else {
		$onmouseover = MakeMouseOver($value);
		
		if (strlen($value) > 40) {
		    $value = substr($value, 0, 40) . "<b> ... </b></a>";
		}
	    }

	    echo "<tr>
   	           <td align=center>
                     <a href='template_metadata.php?action=modify".
		        "&guid=$guid&version=$vers".
		        "&metadata_guid=$metadata_guid".
		        "&metadata_vers=$metadata_vers'>
                     <img border=0 alt='modify' src='greenball.gif'></A></td>
   	           <td align=center>
                     <a href='template_metadata.php?action=delete".
		        "&guid=$guid&version=$vers".
		        "&metadata_guid=$metadata_guid".
		        "&metadata_vers=$metadata_vers'>
                     <img border=0 alt='delete' src='redball.gif'></A></td>
                   <td>$name</td>
                   <td $onmouseover>$value</td>
                  </tr>\n";
  	}
	echo "</table>\n";
	return 1;
    }

    #
    # Show the instance list for a template.
    #
    function ShowInstances() {
	$guid = $this->guid();
	$vers = $this->vers();

	$query_result =
	    DBQueryFatal("select e.*,count(r.node_id) as nodes, ".
			 "    round(minimum_nodes+.1,0) as min_nodes ".
			 "from experiment_template_instances as i ".
			 "left join experiments as e on e.idx=i.exptidx ".
			 "left join reserved as r on e.pid=r.pid and ".
			 "     e.eid=r.eid ".
			 "where e.pid is not null and ".
			 "      (i.parent_guid='$guid' and ".
			 "       i.parent_vers='$vers') ".
			 "group by e.pid,e.eid order by e.state,e.eid");

	if (! mysql_num_rows($query_result))
	    return;
	
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

    #
    # Show the historical instance list for a template.
    #
    function ShowHistory($expand) {
	$guid = $this->guid();
	$vers = $this->vers();

	$query_result =
	    DBQueryFatal("select i.*,r.archive_tag ".
			 "  from experiment_template_instances as i ".
			 "left join experiment_stats as s on ".
			 "     s.exptidx=i.exptidx ".
			 "left join experiment_resources as r on ".
			 "     r.idx=s.rsrcidx ".
			 "where (i.parent_guid='$guid' and ".
			 "       i.parent_vers='$vers') ".
			 "order by i.start_time");

	if (! mysql_num_rows($query_result))
	    return 0;
	
	echo "<center>
               <h3>Template History (Swapins)</h3>
             </center> 
             <table align=center border=1 cellpadding=5 cellspacing=2>\n";

	echo "<tr>
               <th align=center>Expand</th>
               <th>EID</th>
               <th>UID</th>
               <th>Start Time</th>
               <th>Stop Time</th>
               <th align=center>Show</th>
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
	    $tag       = $row['archive_tag'];

	    if (! isset($stop)) {
		$stop = "&nbsp";
	    }

	    $expandit = ((isset($expand) && $expand == $idx) ? 1 : 0);
	    
	    echo "<tr>\n";
	    echo " <td align=center>";
	    if ($expandit) {
		echo "<a href=template_history.php".
                         "?guid=$guid&version=$vers>".
		         "<img border=0 alt='c' src='/icons/down.png'></a>\n";
	    }
	    else {
 		echo "<a href=template_history.php".
                         "?guid=$guid&version=$vers&expand=$idx#$idx>".
		         "<img border=0 alt='e' src='/icons/right.png'></a>\n";
	    }
	    echo " </td>
                   <td>$eid</td>\n";
	    
	    echo " <td>$uid</td>
                   <td>$start</td>
                   <td>$stop</td>\n";

	    echo " <td align=center>".
 		    MakeLink("instance",
			     "guid=$guid&version=$vers&exptidx=$exptidx",
			     "<img border=0 alt='Show' src='greenball.gif'>");

	    echo " <td align=center>
                     <a href=archive_view.php3/$exptidx/history/$tag/".
		        "?exptidx=$exptidx>
                     <img border=0 alt='i' src='greenball.gif'></a></td>";
	    
	    echo " </td>
                 </tr>\n";

	    if ($expandit) {
		$instance = new TemplateInstance($exptidx);
		
		echo "<tr>\n";
		echo " <a NAME=$idx></a>\n";
		echo "<td>&nbsp</td>\n";
		echo " <td colspan=6>\n";
		$instance->ShowRunList(0);
		echo " </td>\n";
		echo "</tr>\n";
	    }
	}
	echo "</table>\n";
    }

    #
    # Dump the image map for a template to the output.
    #
    function ShowGraph() {
	global $bodyclosestring;
	
	$guid = $this->guid();
	$vers = $this->vers();

	$query_result =
	    DBQueryFatal("select imap from experiment_template_graphs ".
			 "where parent_guid='$guid'");

	if (!mysql_num_rows($query_result)) {
	    USERERROR("Experiment Template $guid is no longer in the DB!", 1);
	}
	$row  = mysql_fetch_array($query_result);
	$imap = $row['imap'];

	echo "<script type='text/javascript' ".
	            "src='js/wz_dragdrop.js'></script>";

	echo "<center>";
	echo "<div id=fee style='display: block; overflow: hidden; ".
	    "position: relative; z-index:1010; height: 400px; ".
	    "width: 700px; border: 2px solid black;'>\n";
	echo "<div id=\"D$guid\" style='position:relative;'>\n";

	echo "<div id='CurrentTemplate' style='display: block; opacity: 1; ".
	    "visibility: hidden; ".
	    "position: absolute; z-index:1020; left: 0px; top: 0px;".
	    "height: 0px; width: 0px; border: 2px solid red;'></div>\n";

	echo $imap;
	echo "<img id=\"G$guid\" border=0 usemap=\"#TemplateGraph\" ";
	echo "      src='template_graph.php?guid=$guid'>\n";
	echo "</div>\n";
	echo "</div>\n";

	#
	# This has to happen ...
	#
	$bodyclosestring =
	    "<script type='text/javascript'>
               SET_DHTML(\"D$guid\");
        
	       SetActiveTemplate(\"G$guid\", \"CurrentTemplate\", 
				 \"Tarea${vers}\");
              </script>\n";
    }

    #
    # Grab array of input files for a template, indexed by input_idx.
    #
    function InputFiles() {
	$guid = $this->guid();
	$vers = $this->vers();
	
	$input_list = array();

	$query_result =
	    DBQueryFatal("select * from experiment_template_inputs ".
			 "where parent_guid='$guid' and parent_vers='$vers'");

	while ($row = mysql_fetch_array($query_result)) {
	    $input_idx = $row['input_idx'];

	    $input_query =
		DBQueryFatal("select input ".
			     "  from experiment_template_input_data ".
			     "where idx='$input_idx'");

	    $input_row = mysql_fetch_array($input_query);
	    $input_list[] = $input_row['input'];
	}
	return $input_list;
    }

    #
    # Return number of parameters.
    #
    function ParameterCount() {
	$guid = $this->guid();
	$vers = $this->vers();
	
	$query_result =
	    DBQueryFatal("select name,value ".
			 "   from experiment_template_parameters ".
			 "where parent_guid='$guid' and ".
			 "      parent_vers='$vers'");

	return mysql_num_rows($query_result);
    }

    #
    # Return number of metadata items
    #
    function MetadataCount() {
	$guid = $this->guid();
	$vers = $this->vers();

	$query_result =
	    DBQueryFatal("select internal ".
			 "  from experiment_template_metadata as m ".
			 "where m.parent_guid='$guid' and ".
			 "      m.parent_vers='$vers' and ".
			 "      m.internal=0 and m.hidden=0");

	return mysql_num_rows($query_result);
    }

    #
    # Return number of instances
    #
    function InstanceCount() {
	$guid = $this->guid();
	$vers = $this->vers();

	$query_result =
	    DBQueryFatal("select * from experiment_template_instances as i ".
			 "where (i.parent_guid='$guid' and ".
			 "       i.parent_vers='$vers')");

	return mysql_num_rows($query_result);
    }

    #
    # Look for a metadata item in a template, by the guid/vers of the
    # metadata. Returns a class instance (see below).
    #
    function LookupMetadataByGUID($metadata_guid, $metadata_vers) {
	return TemplateMetadata::TemplateLookupByGUID($this,
						      $metadata_guid,
						      $metadata_vers);
    }
    # Ditto by name,
    function LookupMetadataByName($metadata_name) {
	return TemplateMetadata::TemplateLookupByName($this, $metadata_name);
    }

    # Grab the graph data.
    function GraphImage(&$image) {
	$guid = $this->guid();

	$query_result =
	    DBQueryWarn("select image from experiment_template_graphs ".
			"where parent_guid='$guid'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    return -1;
	}
	$row   = mysql_fetch_array($query_result);
	$image = $row['image'];
	return 0;
    }

    #
    # Return an array of the formal parameters for a template.
    #
    function FormalParameters(&$parameters) {
	$parameters = array();
	$guid = $this->guid();
	$vers = $this->vers();
	
	$query_result =
	    DBQueryFatal("select name,value ".
			 "   from experiment_template_parameters ".
			 "where parent_guid='$guid' and ".
			 "      parent_vers='$vers'");

	while ($row = mysql_fetch_array($query_result)) {
	    $name	= $row['name'];
	    $value	= $row['value'];

	    $parameters[$name] = $value;
	}
	return 0;
    }

    #
    # Find next candidate for a template (modify) TID
    #
    function NextTID() {
	$tid  = $this->tid();
	$guid = $this->guid();

	$query_result =
	    DBQueryFatal("select MAX(vers) from experiment_templates ".
			 "where guid='$guid'");

	if (mysql_num_rows($query_result) == 0) {
	    return "T" . substr(md5(uniqid($foo, true)), 0, 10);
	}
	$row = mysql_fetch_array($query_result);
	$foo = $row[0] + 1;
	return "${tid}-V${foo}"; 
    }
}

#
# This is the class for a template instance (swapin).
#
class TemplateInstance
{
    var	$template;
    var $instance;

    #
    # Instances are found by their experiment index. 
    #
    function TemplateInstance($exptidx) {
	$exptidx = addslashes($exptidx);

	$query_result =
	    DBQueryFatal("select * ".
			 "  from experiment_template_instances ".
			 "where exptidx='$exptidx'");
	
	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->template = NULL;
	    $this->instance = NULL;
	    return;
	}
	$this->instance = mysql_fetch_array($query_result);
	$this->template = new Template($this->instance['parent_guid'],
				       $this->instance['parent_vers']);
    }
    
    # Hmm, how does one cause an error in a php constructor?
    function IsValid() {
	return !is_null($this->instance);
    }

    # Do class level lookup.
    function LookupByExptidx($exptidx) {
	$foo = new TemplateInstance($exptidx);

	if ($foo->IsValid())
	    return $foo;
	return null;
    }

    # accessors
    function idx() {
	return (is_null($this->instance) ? -1 : $this->instance['idx']);
    }
    function exptidx() {
	return (is_null($this->instance) ? -1 : $this->instance['exptidx']);
    }
    function runidx() {
	return (is_null($this->instance) ? -1 : $this->instance['runidx']);
    }
    function pid() {
	return (is_null($this->instance) ? -1 : $this->instance['pid']);
    }
    function eid() {
	return (is_null($this->instance) ? -1 : $this->instance['eid']);
    }
    function uid() {
	return (is_null($this->instance) ? -1 : $this->instance['uid']);
    }
    function guid() {
	return (is_null($this->instance) ? -1 :
		$this->instance['parent_guid']);
    }
    function vers() {
	return (is_null($this->instance) ? -1 :
		$this->instance['parent_vers']);
    }
    function start_time() {
	return (is_null($this->instance) ? -1 :
		$this->instance['start_time']);
    }
    function stop_time() {
	return (is_null($this->instance) ? -1 :
		$this->instance['stop_time']);
    }
    function template() {
	return (is_null($this->instance) ? -1 : $this->template);
    }

    #
    # Show an instance.
    #
    function Show($detailed) {
	$exptidx = $this->exptidx();
	$runidx  = $this->runidx();
	$guid    = $this->guid();
	$vers    = $this->vers();
	$pid     = $this->pid();
	$uid     = $this->uid();
	$start   = $this->start_time();
	$stop    = $this->stop_time();
	$template= $this->template();
	$pcount  = $template->ParameterCount();

	if ($detailed && $pcount) {
	    echo "<table border=0 bgcolor=#000 color=#000 class=stealth ".
		 " cellpadding=0 cellspacing=0 align=center>\n";
            echo "<tr valign=top>";
	    echo "<td class=stealth align=center>\n";
	}
	echo "<center>
               <h3>Template Details</h3>
             </center>\n";

	echo "<table align=center cellpadding=2 cellspacing=2 border=1>\n";
    
	ShowItem("Template",
		 MakeLink("template",
			  "guid=$guid&version=$vers", "$guid/$vers"));
	ShowItem("ID",          $exptidx);
	ShowItem("Project",     MakeLink("project", "pid=$pid", $pid));
	ShowItem("Creator",     MakeLink("user", "target_uid=$uid", $uid));
	ShowItem("Started",     $start);
	ShowItem("Stopped",     (isset($stop) ? $stop : "&nbsp"));
	ShowItem("Current Run", (isset($runidx) ? $runidx : "&nbsp"));
	echo "</table>\n";

	if ($detailed && $pcount) {
	    echo "</td>";
	    echo "<td align=center class=stealth> &nbsp &nbsp &nbsp </td>\n";
	    echo "<td class=stealth align=center>\n";
	    $this->ShowBindings();
	    echo "</tr>";
	    echo "</table>\n";
	}

	if ($detailed) {
	    $this->ShowRunList(1);
	}
    }
    
    #
    # Display instance bindings in a table
    #
    function ShowBindings() {
	$instance_idx = $this->idx();

	$query_result =
	    DBQueryWarn("select * from experiment_template_instance_bindings ".
			"where instance_idx='$instance_idx'");

	if (!mysql_num_rows($query_result))
	    return 0;

	echo "<center>
               <h3>Instance Bindings</h3>
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

    #
    # Show the run list for an instance.
    #
    function ShowRunList($withheader) {
	$exptidx = $this->exptidx();
	$guid    = $this->guid();
	$vers    = $this->vers();
	
	$query_result =
	    DBQueryFatal("select * from experiment_runs ".
			 "where exptidx='$exptidx'");
	
	if (! mysql_num_rows($query_result))
	    return 0;
	
	if ($withheader) {
	    echo "<center>
                    <h3>Instance History (Runs)</h3>
                  </center> \n";
	}
	echo "<table align=center border=1 cellpadding=5 cellspacing=2>\n";

	echo "<tr>
               <th align=center>Show</th>
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
	    $tag       = $rrow['archive_tag'];
	    $description = $rrow['description'];
	    $onmouseover = "";

	    if (! isset($stop)) {
		$stop = "&nbsp";
	    }

	    if (isset($description) && $description != "") {
		$onmouseover = MakeMouseOver($description);
		if (strlen($description) > 30) {
		    $description = substr($description, 0, 30) . " <b>...</b>";
		}
	    }
	    else {
		$description = "&nbsp ";
	    }

	    if (isset($tag) && $tag != "") {
		$archive_link =
		    "<a href=archive_view.php3".
		    "/$exptidx/history/$tag/?exptidx=$exptidx>".
		    "<img border=0 alt='i' src='greenball.gif'></a>";
	    }
	    else {
		$archive_link = "&nbsp ";
	    }
	    
	    echo "<tr>\n";
	    echo " <td align=center>".
		    MakeLink("run",
			     "guid=$guid&version=$vers".
			     "&exptidx=$exptidx&runidx=$runidx",
			     "<img border=0 alt='i' src='greenball.gif'>");
	    echo " </td>
                    <td align=center>$archive_link</td>
                    <td>$runid</td>
  		    <td>$runidx</td>
                    <td>$start</td>
                    <td>$stop</td>
                    <td $onmouseover>$description</td>
                   </tr>\n";
	}
	echo "</table>\n";
    }

    #
    # Check if a valid run.
    #
    function ValidRun($runidx) {
	$exptidx = $this->exptidx();
	$runidx  = addslashes($runidx);
	
	$query_result =
	    DBQueryFatal("select * from experiment_runs ".
			 "where exptidx='$exptidx' and idx='$runidx'");

	return mysql_num_rows($query_result);
    }

    #
    # Show details for an experiment run
    #
    function ShowRun($runidx) {
	$runidx  = addslashes($runidx);
	$exptidx = $this->exptidx();
	$guid    = $this->guid();
	$vers    = $this->vers();
	
	$query_result =
	    DBQueryFatal("select r.* from experiment_runs as r ".
			 "left join experiment_template_instances as i on ".
			 "     i.exptidx=r.exptidx ".
			 "where r.exptidx='$exptidx' and r.idx='$runidx'");

	if (!mysql_num_rows($query_result))
	    return;
	
	$row   = mysql_fetch_array($query_result);
	$start = $row['start_time'];
	$stop  = $row['stop_time'];
	$description = $row['description'];

	if (!isset($stop))
	    $stop = "&nbsp";
	
	echo "<center>
               <h3>Experiment Run</h3>
             </center>\n";

	echo "<table align=center cellpadding=2 cellspacing=2 border=1>\n";
    
	ShowItem("Template",
		 MakeLink("template",
			  "guid=$guid&version=$vers", "$guid/$vers"));
	ShowItem("Experiment",
		 MakeLink("instance",
			  "guid=$guid&version=$vers&exptidx$exptidx",
			  "$exptidx"));
	ShowItem("ID",          $runidx);
	ShowItem("Started",     $start);
	ShowItem("Stopped",     $stop);

	if (isset($description) && $description != "") {
	    echo "<tr>
                     <td align=center colspan=2>
                      Description
                     </td>
                  </tr>
                  <tr>
                     <td colspan=2 align=center class=left>
                         <textarea readonly
                            rows=5 cols=50>" .
		            ereg_replace("\r", "", $description) .
		         "</textarea>
                  </td>
              </tr>\n";
	}
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

    #
    # Find next candidate for an experiment run. 
    #
    function NextRunID() {
	$exptidx = $this->exptidx();
	$eid     = $this->eid();

	$query_result =
	    DBQueryFatal("select MAX(idx) from experiment_runs ".
			 "where exptidx='$exptidx'");

	if (mysql_num_rows($query_result) == 0) {
	    return 0;
	}
	$row = mysql_fetch_array($query_result);
	$foo = $row[0] + 1;
	return "${eid}-R${foo}"; 
    }

    #
    # Return an array of the bindings for a template instance.
    #
    function Bindings(&$bindings) {
	$bindings = array();

	$guid = $this->guid();
	$vers = $this->vers();
	$instance_idx = $this->idx();
	
	$query_result =
	    DBQueryFatal("select * ".
			 "   from experiment_template_instance_bindings ".
			 "where parent_guid='$guid' and parent_vers='$vers' ".
			 "      and instance_idx='$instance_idx'");

	while ($row = mysql_fetch_array($query_result)) {
	    $name	= $row['name'];
	    $value	= $row['value'];

	    $bindings[$name] = $value;
	}
	return 0;
    }
}

#
# This is the class for metadata.
#
class TemplateMetadata
{
    var	$template;
    var $metadata;

    #
    # Instances are found by their experiment index. 
    #
    function TemplateMetadata($guid, $vers) {
	$guid = addslashes($guid);
	$vers = addslashes($vers);
	
	$query_result =
	    DBQueryFatal("select * from experiment_template_metadata_items ".
			 "where guid='$guid' and ".
			 "      vers='$vers'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->template = NULL;
	    $this->metadata = NULL;
	    return;
	}
	$this->metadata = mysql_fetch_array($query_result);
	$this->template = NULL;
    }
    
    # Hmm, how does one cause an error in a php constructor?
    function IsValid() {
	return !is_null($this->metadata);
    }

    # Do class level lookup.
    function Lookup($guid, $vers) {
	$foo = new TemplateMetadata($guid, $vers);

	if ($foo->IsValid())
	    return $foo;
	return null;
    }
    function TemplateLookupByGUID($template, $guid, $vers) {
	$metadata_guid = addslashes($guid);
	$metadata_vers = addslashes($vers);
	$template_guid = $template->guid();
	$template_vers = $template->vers();

	$query_result =
	    DBQueryFatal("select internal from experiment_template_metadata ".
			 "where parent_guid='$template_guid' and ".
			 "      parent_vers='$template_vers' and ".
			 "      metadata_guid='$metadata_guid' and ".
			 "      metadata_vers='$metadata_vers'");

	if (! mysql_num_rows($query_result))
	    return null;

	$foo = new TemplateMetadata($guid, $vers);

	if (! $foo->IsValid())
	    return null;

	$foo->template = $template;
	return $foo;
    }

    function TemplateLookupByName($template, $name) {
	$metadata_name = addslashes($name);
	$template_guid = $template->guid();
	$template_vers = $template->vers();

	$query_result =
	    DBQueryFatal("select i.guid,i.vers ".
		     "    from experiment_template_metadata as m ".
		     "left join experiment_template_metadata_items as i on ".
		     "     i.guid=m.metadata_guid and i.vers=m.metadata_vers ".
		     "where m.parent_guid='$template_guid' and ".
		     "      m.parent_vers='$template_vers' and ".
		     "      i.name='$metadata_name'");

	if (! mysql_num_rows($query_result))
	    return null;
	
	$row = mysql_fetch_array($query_result);
	$metadata_guid = $row['guid'];
	$metadata_vers = $row['vers'];
	
	$foo = new TemplateMetadata($metadata_guid, $metadata_vers);

	if (! $foo->IsValid())
	    return null;

	$foo->template = $template;
	return $foo;
    }

    # accessors
    function guid() {
	return (is_null($this->metadata) ? -1 : $this->metadata['guid']);
    }
    function vers() {
	return (is_null($this->metadata) ? -1 : $this->metadata['vers']);
    }
    function name() {
	return (is_null($this->metadata) ? -1 : $this->metadata['name']);
    }
    function value() {
	return (is_null($this->metadata) ? -1 : $this->metadata['value']);
    }
    function type() {
	return (is_null($this->metadata) ? -1 :
		$this->metadata['metadata_type']);
    }
    function parent_guid() {
	return (is_null($this->metadata) ? -1 :
		$this->metadata['parent_guid']);
    }
    function parent_vers() {
	return (is_null($this->metadata) ? -1 :
		$this->metadata['parent_vers']);
    }
    function template_guid() {
	return (is_null($this->metadata) ? -1 :
		$this->metadata['template_guid']);
    }
    function created() {
	return (is_null($this->metadata) ? -1 : $this->metadata['created']);
    }
    function uid() {
	return (is_null($this->metadata) ? -1 : $this->metadata['uid']);
    }

    #
    # Display a metadata item in its own table.
    #
    function Show() {
	$metadata_guid  = $this->guid();
	$metadata_vers  = $this->vers();
	$created        = $this->created();
	$metadata_name  = $this->name();
	$metadata_value = $this->value();
	$metadata_type  = $this->type();

	echo "<table align=center cellpadding=2 cellspacing=2 border=1>\n";
	
	ShowItem("GUID",     "$metadata_guid/$metadata_vers");
	ShowItem("Name",     $metadata_name);
	if (ISADMIN() && isset($metadata_type)) {
	    ShowItem("Type",     $metadata_type);
	}
	ShowItem("Created",  $created);

	if (! is_null($this->template)) {
	    $template_guid  = $template->guid();
	    $template_vers  = $template->vers();

	    ShowItem("Template",
		     MakeLink("template",
			      "guid=$template_guid&version=$template_vers",
			      "$template_guid/$template_vers"));
	}
    
	if ($this->parent_guid()) {
	    $parent_guid = $this->parent_guid();
	    $parent_vers = $this->parent_vers();
	    
	    ShowItem("Parent Version",
		     MakeLink("metadata",
			      "action=show&guid=$parent_guid".
			      "&version=$parent_vers",
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
	                ereg_replace("\r", "", $metadata_value) .
	             "</textarea>
                  </td>
              </tr>\n";
	echo "</table>\n";
    }
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
function TBvalid_template_parameter_description($token) {
    return TBcheck_dbslot($token, "experiment_templates", "description",
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
function TBvalid_template_metadata_type($token) {
    return TBcheck_dbslot($token, "experiment_template_metadata",
			  "metadata_type",
			  TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR);
}

function MakeMouseOver($string)
{
    $string = ereg_replace("\n", "<br>", $string);
    $string = ereg_replace("\r", "", $string);
    $string = htmlentities($string);
    $string = preg_replace("/\'/", "\&\#039;", $string);

    return "onmouseover=\"return escape('$string')\"";
}

?>

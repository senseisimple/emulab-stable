<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006-2011 University of Utah and the Flux Group.
# All rights reserved.
#
#
include_once("template_defs.php");
include_once("geni_defs.php");

# This class is really just a wrapper around the DB data ...
#
# A cache to avoid lookups. Indexed by node_id.
#
$experiment_cache = array();

class Experiment
{
    var	$experiment;
    var $group;
    var $project;
    var $isinstance = false;
    var $resources;

    #
    # Constructor by lookup on unique index.
    #
    function Experiment($exptidx) {
	$safe_exptidx = addslashes($exptidx);

	#
	# We do a join with the instances table simply to determine if this
	# experiment is a current instance. 
	#
	$query_result =
	    DBQueryWarn("select e.*,i.parent_guid from experiments as e ".
			"left join experiment_template_instances as i on ".
			"     i.exptidx=e.idx ".
			"where e.idx='$safe_exptidx'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->experiment = null;
	    return;
	}
	$this->experiment = mysql_fetch_array($query_result);

	# Instance flag.
	if ($this->experiment["parent_guid"]) {
	    $this->isinstance = true;
	}

	# Load lazily;
	$this->group      = null;
	$this->project    = null;
	$this->resources  = null;
    }

    # Hmm, how does one cause an error in a php constructor?
    function IsValid() {
	return !is_null($this->experiment);
    }

    # Lookup by exptidx, but allow for lookup by pid,eid with variable args.
    function Lookup($exptidx) {
	global $experiment_cache;
	
	$args = func_get_args();

        # Look in cache first
	if (array_key_exists("$exptidx", $experiment_cache))
	    return $experiment_cache["$exptidx"];
	
	$foo = new Experiment($exptidx);

	if ($foo->IsValid()) {
            # Insert into cache.
	    $experiment_cache["$exptidx"] =& $foo;
	    return $foo;
	}

        # Try lookup with pid,eid.
	if (count($args) == 2) {
	    $pid = array_shift($args);
	    $eid = array_shift($args);

	    $foo = Experiment::LookupByPidEid($pid, $eid);

	    # Already in the cache from LookupByPidEid() so just return it.
	    if ($foo && $foo->IsValid())
		return $foo;
	}
	return null;
    }

    # Backwards compatable lookup by pid,eid. Will eventually flush this.
    function LookupByPidEid($pid, $eid) {
	$safe_pid = addslashes($pid);
	$safe_eid = addslashes($eid);

	$query_result =
	    DBQueryWarn("select idx from experiments ".
			"where pid='$safe_pid' and eid='$safe_eid'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    return null;
	}
	$row = mysql_fetch_array($query_result);
	$idx = $row['idx'];

	return Experiment::Lookup($idx); 
    }
    function LookupByUUID($uuid) {
	$safe_uuid = addslashes($uuid);

	$query_result =
	    DBQueryWarn("select idx from experiments ".
			"where eid_uuid='$safe_uuid'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    return null;
	}
	$row = mysql_fetch_array($query_result);
	$idx = $row['idx'];

	return Experiment::Lookup($idx); 
    }
    
    #
    # Refresh an instance by reloading from the DB.
    #
    function Refresh() {
	if (! $this->IsValid())
	    return -1;

	$idx = $this->idx();

	$query_result =
	    DBQueryWarn("select * from experiments where idx='$idx'");
    
	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->experiment = NULL;
	    $this->group      = null;
	    $this->project    = null;
	    $this->resources  = null;
	    return -1;
	}
	$this->experiment = mysql_fetch_array($query_result);
	$this->group      = null;
	$this->project    = null;
	$this->resources  = null;
	return 0;
    }

    #
    # Class function to change experiment info via XML to a backend script.
    #
    function EditExp($experiment, $args, &$errors) {
	global $suexec_output, $suexec_output_array;

	if (!count($args)) {
	    $errors[] = "No changes to submit.";
	    return null;
	}	    

        #
        # Generate a temporary file and write in the XML goo.
        #
	$xmlname = tempnam("/tmp", "editexp");
	if (! $xmlname) {
	    TBERROR("Could not create temporary filename", 0);
	    $errors[] = "Transient error(1); please try again later.";
	    return null;
	}
	if (! ($fp = fopen($xmlname, "w"))) {
	    TBERROR("Could not open temp file $xmlname", 0);
	    $errors[] = "Transient error(2); please try again later.";
	    return null;
	}

	# Add these. Maybe caller should do this?
	$args["experiment"] = $experiment->idx();

	fwrite($fp, "<experiment>\n");
	foreach ($args as $name => $value) {
	    fwrite($fp, "<attribute name=\"$name\">");
	    fwrite($fp, "  <value>" . htmlspecialchars($value) . "</value>");
	    fwrite($fp, "</attribute>\n");
	}
	fwrite($fp, "</experiment>\n");
	fclose($fp);
	chmod($xmlname, 0666);

	$retval = SUEXEC("nobody", "nobody", "webeditexp $xmlname",
			 SUEXEC_ACTION_IGNORE);

	if ($retval) {
	    if ($retval < 0) {
		$errors[] = "Transient error(3, $retval); please try again later.";
		SUEXECERROR(SUEXEC_ACTION_CONTINUE);
	    }
	    else {
		# unlink($xmlname);
		if (count($suexec_output_array)) {
		    for ($i = 0; $i < count($suexec_output_array); $i++) {
			$line = $suexec_output_array[$i];
			if (preg_match("/^([-\w]+):\s*(.*)$/",
				       $line, $matches)) {
			    $errors[$matches[1]] = $matches[2];
			}
			else
			    $errors[] = $line;
		    }
		}
		else
		    $errors[] = "Transient error(4, $retval); ".
			"please try again later.";
	    }
	    return null;
	}

	# There are no return value(s) to parse at the end of the output.

	# Unlink this here, so that the file is left behind in case of error.
	# We can then edit the experiment by hand from the xmlfile, if desired.
	unlink($xmlname);
	return true;
    }

    #
    # Update fields. Array of "foo=bar" ...
    #
    function UpdateOldStyle($inserts) {
	$idx = $this->idx();

	DBQueryFatal("update experiments set ".
		     implode(",", $inserts) . " ".
		     "where idx='$idx'");

	return $this->Refresh();
    }
    
    #
    # Load the project object for an experiment.
    #
    function Project() {
	$pid = $this->pid();

	if ($this->project)
	    return $this->project;

	$this->project = Project::Lookup($pid);
	if (! $this->project) {
	    TBERROR("Could not lookup project $pid!", 1);
	}
	return $this->project;
    }
    #
    # Load the group object for an experiment.
    #
    function Group() {
	$pid = $this->pid();
	$gid = $this->gid();

	if ($this->group)
	    return $this->group;

	$this->group = Group::LookupByPidGid($pid, $gid);
	if (! $this->group) {
	    TBERROR("Could not lookup group $pid/$gid!", 1);
	}
	return $this->group;
    }

    #
    # Get the creator for a project.
    #
    function GetCreator() {
	return User::Lookup($this->creator());
    }
    function GetSwapper() {
	return User::Lookup($this->swapper());
    }
    function GetStats() {
	return ExperimentStats::Lookup($this->idx());
    }
    function GetResources() {
	$stats = $this->GetStats();
	return ExperimentResources::Lookup($stats->rsrcidx());
    }

    # accessors
    function field($name) {
	return (is_null($this->experiment) ? -1 : $this->experiment[$name]);
    }
    function pid()	    { return $this->field('pid'); }
    function gid()	    { return $this->field('gid'); }
    function eid()	    { return $this->field('eid'); }
    function idx()	    { return $this->field('idx'); }
    function uuid()	    { return $this->field('eid_uuid'); }
    function description()  { return $this->field('expt_name'); }
    function path()	    { return $this->field('path'); }
    function state()	    { return $this->field('state'); }
    function batchstate()   { return $this->field('batchstate'); }
    function batchmode()    { return $this->field('batchmode'); }
    function creator()      { return $this->field('expt_head_uid');}
    function canceled()     { return $this->field('canceled'); }
    function locked()       { return $this->field('expt_locked'); }
    function elabinelab()   { return $this->field('elab_in_elab');}
    function lockdown()     { return $this->field('lockdown'); }
    function skipvlans()    { return $this->field('skipvlans'); }
    function created()      { return $this->field('expt_created'); }
    function swapper()      { return $this->field('expt_swap_uid');}
    function swapper_idx()  { return $this->field('swapper_idx');}
    function swappable()    { return $this->field('swappable');}
    function idleswap()     { return $this->field('idleswap');}
    function autoswap()     { return $this->field('autoswap');}
    function noswap_reason(){ return $this->field('noswap_reason');}
    function noidleswap_reason(){ return $this->field('noidleswap_reason');}
    function idleswap_timeout() { return $this->field('idleswap_timeout');}
    function autoswap_timeout() { return $this->field('autoswap_timeout');}
    function prerender_pid()    { return $this->field('prerender_pid');}
    function dpdb()		{ return $this->field('dpdb');}
    function dpdbname()         { return $this->field('dpdbname');}
    function dpdbpassword()     { return $this->field('dpdbpassword');}
    function instance_idx()     { return $this->field('instance_idx'); }
    function swap_requests()    { return $this->field('swap_requests'); }
    function last_swap_req()    { return $this->field('last_swap_req'); }
    function idle_ignore()      { return $this->field('idle_ignore'); }
    function savedisk()         { return $this->field('savedisk'); }
    function mem_usage()        { return $this->field('mem_usage'); }
    function cpu_usage()        { return $this->field('cpu_usage'); }
    function linktest_level()   { return $this->field('linktest_level'); }
    function linktest_pid()     { return $this->field('linktest_pid'); }
    function logfile()          { return $this->field('logfile'); }
    function keyhash()          { return $this->field('keyhash'); }
    function paniced()          { return $this->field('paniced'); }
    function panic_date()       { return $this->field('panic_date'); }
    function geniflags()        { return $this->field('geniflags'); }

    #
    # Access Check. Project level check since this might not be a current
    # experiment.
    #
    function AccessCheck ($user, $access_type) {
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
	$mintrust = $TB_EXPT_READINFO;

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

	if ($access_type == $TB_EXPT_READINFO) {
	    $mintrust = $TBDB_TRUST_USER;
	}
	else {
	    $mintrust = $TBDB_TRUST_LOCALROOT;
	}

	$uid = $user->uid();
	$pid = $this->pid();
	$gid = $this->gid();
	$uid = $user->uid();
	
        #
        # Either proper permission in the group, or group_root in the project.
        # This lets group_roots muck with other peoples experiments, including
        # those in groups they do not belong to.
        #
	return TBMinTrust(TBGrpTrust($uid, $pid, $gid), $mintrust) ||
	    TBMinTrust(TBGrpTrust($uid, $pid, $pid), $TBDB_TRUST_GROUPROOT);
    }

    #
    # Page header; spit back some html for the typical page header.
    #
    function PageHeader() {
	$pid = $this->pid();
	$eid = $this->eid();
	$tag = ($this->isinstance ? "Instance" : "Experiment");
	
	$html = "<font size=+2>$tag <b>".
	    "<a href='showproject.php3?pid=$pid'>$pid</a>/".
	    "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a>".
	    "</b></font>\n";

	return $html;
    }

    #
    # Flip batch mode, return success to caller. 
    #
    function SetBatchMode($mode) {
	global $TB_EXPTSTATE_SWAPPED;
	
	$reqstate = $TB_EXPTSTATE_SWAPPED;
	$idx      = $this->idx();
	$mode     = ($mode ? 1 : 0);

	DBQueryFatal("lock tables experiments write");

	$query_result =
	    DBQueryFatal("update experiments set ".
			 "   batchmode=$mode ".
			 "where idx='$idx' and ".
			 "     expt_locked is NULL and state='$reqstate'");

	$success = DBAffectedRows();
	DBQueryFatal("unlock tables");

	return ($success ? 0 : -1);
    }

    #
    # Flip lockdown bit.
    #
    function SetLockDown($mode) {
	$idx      = $this->idx();
	$mode     = ($mode ? 1 : 0);

	$query_result =
	    DBQueryFatal("update experiments set lockdown='$mode' ".
			 "where idx='$idx'");

	return 0;
    }

    #
    # Flip lockdown bit.
    #
    function SetSkipVlans($mode) {
	$idx      = $this->idx();
	$mode     = ($mode ? 1 : 0);

	$query_result =
	    DBQueryFatal("update experiments set skipvlans='$mode' ".
			 "where idx='$idx'");

	return 0;
    }

    function GetLogfile() {
	$this->Refresh();
	
	if ($this->logfile()) 
	    return Logfile::Lookup($this->logfile());
	return null;
    }

    #
    # Return the Unix GID (numeric) for an experiments group.
    #
    function UnixGID() {
	$group = $this->Group();

	return $group->unix_gid();
    }

    # Return the stored NS file for an experiment.
    function NSFile() {
	$resources      = $this->GetResources();
	$input_data_idx = $resources->input_data_idx();

	$query_result =
	    DBQueryFatal("select input from experiment_input_data ".
			 "where idx='$input_data_idx'");

	if (! mysql_num_rows($query_result)) {
	    return null;
	}
	$row    = mysql_fetch_array($query_result);
	return $row["input"];
    }

    #
    # Return number of physical nodes (pcs) in use by experiment.
    #
    function PCCount() {
	$pid = $this->pid();
	$eid = $this->eid();

	$query_result =
	    DBQueryFatal("select count(*) as count from reserved as r ".
			 "left join nodes as n on r.node_id=n.node_id ".
			 "left join node_types as nt on n.type=nt.type ".
			 "where nt.class='pc' and pid='$pid' and eid='$eid'");

	if (!mysql_num_rows($query_result))
	    return 0;

	$row = mysql_fetch_array($query_result);
	return $row["count"];
    }

    #
    # Return number of events.
    #
    function EventCount() {
	$pid = $this->pid();
	$eid = $this->eid();

	$query_result =
	    DBQueryFatal("select count(*) from eventlist ".
			 "where pid='$pid' and eid='$eid'");

	if (mysql_num_rows($query_result) == 0) {
	    return 0;
	}
	$row = mysql_fetch_row($query_result);
	return $row[0];
    }

    #
    # Return time since swapin
    #
    function SwapSeconds() {
	$idx = $this->idx();

	$query_result =
	    DBQueryFatal("select (UNIX_TIMESTAMP(now()) - ".
			 "        UNIX_TIMESTAMP(expt_swapped)) ".
			 "  from experiments ".
			 "where idx='$idx'");

	$row = mysql_fetch_array($query_result);
	return $row[0];
    }

    var $lastact_query =
	"greatest(last_tty_act,last_net_act,last_cpu_act,last_ext_act)";

    #
    # Gets the time of last activity for an expt
    #
    function LastAct() {
	$pid = $this->pid();
	$eid = $this->eid();
	$clause = $this->lastact_query;
	
	$query_result =
	    DBQueryWarn("select max($clause) as last_act ".
			"  from node_activity as na ".
			"left join reserved as r on na.node_id=r.node_id ".
			"where pid='$pid' and eid='$eid' ".
			"group by pid,eid");

	if (mysql_num_rows($query_result) == 0) {
	    return -1;
	}
	$row   = mysql_fetch_array($query_result);
	return $row["last_act"];
    }

    #
    # Gets the time of idleness for an expt, in hours
    #
    function IdleTime($format = 0) {
	$pid = $this->pid();
	$eid = $this->eid();
	$clause = $this->lastact_query;

	$query_result =
	    DBQueryWarn("select (unix_timestamp(now()) - unix_timestamp( ".
			"        max($clause))) as idle_time ".
			"  from node_activity as na ".
			"left join reserved as r on na.node_id=r.node_id ".
			"where pid='$pid' and eid='$eid' ".
			"group by pid,eid");

	if (mysql_num_rows($query_result) == 0) {
	    return -1;
	}
	$row   = mysql_fetch_array($query_result);
	$t = $row["idle_time"];
        # if it is less than 5 minutes, it is not idle at all...
	$t = ($t < 300 ? 0 : $t);
	if (!$format) {
	    $t = round($t/3600,2);
	}
	else {
	    $t = date($format,mktime(0,0,$t,1,1));
	}
	return $t;
    }

    #
    # Find out if an expts idle report is stale
    #
    function IdleStale() {
	$pid = $this->pid();
	$eid = $this->eid();

        # We currently have a 5 minute interval for slothd between reports
        # So give some slack in case a node reboots without reporting for
	# a while
	#
	# in minutes
	$staletime = 10; 
	$stalesec  = 60 * $staletime;

	$query_result =
	    DBQueryWarn("select (unix_timestamp(now()) - ".
			"        unix_timestamp(min(last_report) )) as t ".
			"  from node_activity as na ".
			"left join reserved as r on na.node_id=r.node_id ".
			"where pid='$pid' and eid='$eid' ".
			"group by pid,eid");

	if (mysql_num_rows($query_result) == 0) {
	    return -1;
	}
	$row   = mysql_fetch_array($query_result);
	return ($row["t"]>$stalesec);
    }

    #
    # Is an experiment firewalled.
    # 
    function Firewalled() {
	$pid = $this->pid();
	$eid = $this->eid();

	$query_result =
	    DBQueryFatal("select eid from virt_firewalls ".
			 "where pid='$pid' and eid='$eid' and ".
			 "      type like '%-vlan'");
	
	if (!mysql_num_rows($query_result)) {
	    return 0;
	}
	return 1;
    }
    
    #
    # Show an experiment.
    #
    function Show($short = 0, $sortby = "") {
	global $TBDBNAME, $TBDOCBASE, $WIKIDOCURL;
	$pid = $this->pid();
	$eid = $this->eid();
	$nodecounts  = array();

        # Node counts, by class. 
	$query_result =
	    DBQueryFatal("select nt.class,count(*) from reserved as r ".
			 "left join nodes as n on n.node_id=r.node_id ".
			 "left join node_types as nt on n.type=nt.type ".
			 "where pid='$pid' and eid='$eid' group by nt.class");

	while ($row = mysql_fetch_array($query_result)) {
	    $class = $row[0];
	    $count = $row[1];
	
	    $nodecounts[$class] = $count;
	}
		
	$query_result =
	    DBQueryFatal("select e.*, s.archive_idx, pl.slicename, ". 
			 "round(e.minimum_nodes+.1,0) as min_nodes, ".
			 "round(e.maximum_nodes+.1,0) as max_nodes ".
			 " from experiments as e ".
			 "left join experiment_stats as s on s.exptidx=e.idx ".
			 "left join plab_slices as pl".
			 " on e.pid = pl.pid and e.eid = pl.eid ".
			 "where e.pid='$pid' and e.eid='$eid'");
    
	if (($exprow = mysql_fetch_array($query_result)) == 0) {
	    TBERROR("Experiment $eid in project $pid is gone!\n", 1);
	}

	$exp_gid     = $exprow["gid"];
	$exp_name    = $exprow["expt_name"];
	$exp_swapped = $exprow["expt_swapped"];
	$exp_swapuid = $exprow["expt_swap_uid"];
	$exp_end     = $exprow["expt_end"];
	$exp_created = $exprow["expt_created"];
	$exp_head    = $exprow["expt_head_uid"];
	$exp_swapper = $exprow["swapper_idx"];
	$exp_state   = $exprow["state"];
	$exp_shared  = $exprow["shared"];
	$exp_path    = $exprow["path"];
	$batchmode   = $exprow["batchmode"];
	$canceled    = $exprow["canceled"];
	$attempts    = $exprow["attempts"];
	$expt_locked = $exprow["expt_locked"];
	$priority    = $exprow["priority"];
	$swappable   = $exprow["swappable"];
	$noswap_reason = $exprow["noswap_reason"];
	$idleswap    = $exprow["idleswap"];
	$idleswap_timeout  = $exprow["idleswap_timeout"];
	$noidleswap_reason = $exprow["noidleswap_reason"];
	$autoswap    = $exprow["autoswap"];
	$autoswap_timeout  = $exprow["autoswap_timeout"];
	$idle_ignore = $exprow["idle_ignore"];
	$savedisk    = $exprow["savedisk"];
	$swapreqs    = $exprow["swap_requests"];
	$lastswapreq = $exprow["last_swap_req"];
	$minnodes    = $exprow["min_nodes"];
	$maxnodes    = $exprow["max_nodes"];
	$virtnodes   = $exprow["virtnode_count"];
	$syncserver  = $exprow["sync_server"];
	$mem_usage   = $exprow["mem_usage"];
	$cpu_usage   = $exprow["cpu_usage"];
	$exp_slice   = $exprow["slicename"];
	$linktest_level = $exprow["linktest_level"];
	$linktest_pid   = $exprow["linktest_pid"];
	$usemodelnet = $exprow["usemodelnet"];
	$mnet_cores  = $exprow["modelnet_cores"];
	$mnet_edges  = $exprow["modelnet_edges"];
	$lockdown    = $exprow["lockdown"];
	$skipvlans   = $exprow["skipvlans"];
	$exptidx     = $exprow["idx"];
	$archive_idx = $exprow["archive_idx"];
	$dpdb        = $exprow["dpdb"];
	$dpdbname    = $exprow["dpdbname"];
	$dpdbpassword= $exprow["dpdbpassword"];
	$uuid        = $exprow["eid_uuid"];

	$autoswap_hrs= ($autoswap_timeout/60.0);
	$idleswap_hrs= ($idleswap_timeout/60.0);
	$noswap = "($noswap_reason)";
	$noidleswap = "($noidleswap_reason)";
	$autoswap_str= $autoswap_hrs." hour".($autoswap_hrs==1 ? "" : "s");
	$idleswap_str= $idleswap_hrs." hour".($idleswap_hrs==1 ? "":"s");

	if (! ($head_user = User::Lookup($exp_head))) {
	    TBERROR("Error getting object for user $exp_head", 1);
	}
	$showuser_url = CreateURL("showuser", $head_user);
	if (! ($swapper = User::Lookup($exp_swapper))) {
	    TBERROR("Error getting object for user $exp_swapper", 1);
	}
	$swapper_uid = $swapper->uid();
	$swapper_url = CreateURL("showuser", $swapper);

	if ($swappable)
	    $swappable = "Yes";
	else
	    $swappable = "<b>No</b> $noswap";

	if ($idleswap)
	    $idleswap = "Yes (after $idleswap_str)";
	else
	    $idleswap = "<b>No</b> $noidleswap";

	if ($autoswap)
	    $autoswap = "<b>Yes</b> (after $autoswap_str)";
	else
	    $autoswap = "No";

	if ($idle_ignore)
	    $idle_ignore = "<b>Yes</b>";
	else
	    $idle_ignore = "No";

	if ($savedisk)
	    $savedisk = "<b>Yes</b>";
	else
	    $savedisk = "No";

	if ($expt_locked)
	    $expt_locked = "($expt_locked)";
	else
	    $expt_locked = "";

	$query_result =
	    DBQueryFatal("select cause_desc ".
			 "from experiment_stats as s,errors,causes ".
			 "where s.exptidx = $exptidx ".
			 "and errors.cause = causes.cause ".
			 "and s.last_error = errors.session");

	if ($row = mysql_fetch_array($query_result)) {
	    $err_cause = $row[0];
	} else {
	    $err_cause = '';
	}

        #
        # Generate the table.
        #
	echo "<table align=center border=1>\n";

	if (!$short) {
	    $thisurl = CreateURL("showexp", $this);
	    echo "<tr>
                <td>Name: </td>
                <td class=left><a href='$thisurl'>$eid</a></td>
              </tr>\n";

	    echo "<tr>
                <td>Description: </td>
                <td class=\"left\">$exp_name</td>
              </tr>\n";

	    echo "<tr>
                <td>Project: </td>
                <td class=\"left\">
                  <a href='showproject.php3?pid=$pid'>$pid</a></td>
              </tr>\n";

	    echo "<tr>
                <td>Group: </td>
                <td class=\"left\">
                  <a href='showgroup.php3?pid=$pid&gid=$exp_gid'>$exp_gid</a>
                </td>
              </tr>\n";

	    if (isset($exp_slice)) {
		echo "<tr>
                  <td>Planetlab Slice: </td>
                  <td class=\"left\">$exp_slice</td>
                </tr>\n";
	    }
	}

	echo "<tr>
            <td>Creator: </td>
            <td class=\"left\">
              <a href='$showuser_url'>$exp_head</a></td>
          </tr>\n";

	if (!$swapper->SameUser($head_user)) {
	    echo "<tr>
                      <td>Swapper: </td>
                      <td class=\"left\">
                          <a href='$swapper_url'>$swapper_uid</a></td>
                  </tr>\n";
	}

	if (!$short) {
	    $instance = TemplateInstance::LookupByExptidx($exptidx);

	    if (! is_null($instance)) {
		$guid   = $instance->guid();
		$vers   = $instance->vers();
	
		echo "<tr>
                    <td>Template: </td>
                    <td class=\"left\">
                       <a href='template_show.php?guid=$guid&version=$vers'>
                          $guid/$vers</a>";

		if ($instance->runidx()) {
		    $runidx = $instance->runidx();
		    $runid  = $instance->GetRunID($runidx);
		    $url    = CreateURL("experimentrun_show", $instance,
					"runidx", $runidx);
		    echo " (Current Run:
                       <a href='$url'>$runid</a>)</td>";
		} else {
		    $runidx = $instance->LastRunIdx();
		    $runid  = $instance->GetRunID($runidx);
		    $url    = CreateURL("experimentrun_show", $instance,
					"runidx", $runidx);
		    echo " (Last Run:
                       <a href='$url'>$runid</a>)</td>";
		}
		echo "</tr>\n";
	    }

	    echo "<tr>
                <td>Created: </td>
                <td class=\"left\">$exp_created</td>
              </tr>\n";

	    if ($exp_swapped) {
		echo "<tr>
                    <td>Last Swap/Modify: </td>
                    <td class=\"left\">$exp_swapped
                        (<a href='$swapper_url'>$swapper_uid</a>)</td>
                  </tr>\n";
	    }

	    if (ISADMIN()) {
		echo "<tr>
                    <td><a href='$WIKIDOCURL/Swapping#swapping'>Swappable:</a></td>
                    <td class=\"left\">$swappable</td>
                  </tr>\n";
	    }
    
	    echo "<tr>
                  <td><a href='$WIKIDOCURL/Swapping#idleswap'>Idle-Swap:</a></td>
                  <td class=\"left\">$idleswap</td>
              </tr>\n";

	    echo "<tr>
                <td><a href='$WIKIDOCURL/Swapping#autoswap'>Max. Duration:</a></td>
                <td class=\"left\">$autoswap</td>
              </tr>\n";

	    echo "<tr>
                <td><a href='$WIKIDOCURL/Swapping#swapstatesave'>Save State:</a></td>
                <td class=\"left\">$savedisk</td>
              </tr>\n";

	    if (ISADMIN()) {
		echo "<tr>
                    <td>Idle Ignore:</td>
                    <td class=\"left\">$idle_ignore</td>
                 </tr>\n";
	    }
    
	    echo "<tr>
                <td>Path: </td>
                <td class=left>$exp_path</td>
              </tr>\n";

	    echo "<tr>
                <td>Status: </td>
                <td id=exp_state class=\"left\">$exp_state $expt_locked</td>
              </tr>\n";

	    if ($err_cause) {
		echo "<tr>
                    <td>Last Error: </td>
                    <td class=\"left\"><a href=\"showlasterror.php3?pid=$pid&eid=$eid\">$err_cause</a></td>
                  </tr>\n";
	    }

	    if ($linktest_pid) {
		$linktest_running = "<b>(Linktest Running)</b>";
	    }
	    else {
		$linktest_running = "";
	    }

	    echo "<tr>
                <td><a href='$WIKIDOCURL/linktest'>".
		"Linktest Level</a>: </td>
                <td class=\"left\">$linktest_level $linktest_running</td>
              </tr>\n";
	}

	if (count($nodecounts)) {
	    echo "<tr>
                 <td>Reserved Nodes: </td>
                 <td class=\"left\">\n";
	    while (list ($class, $count) = each($nodecounts)) {
		echo "$count ($class) &nbsp; ";
	    }
	    echo "   </td>
              </tr>\n";
	}
	elseif (!$short) {
	    if ($minnodes!="") {
		echo "<tr>
                      <td>Min/Max Nodes: </td>
                      <td class=\"left\"><font color=green>
                          $minnodes/$maxnodes (estimates)</font></td>
                  </tr>\n";
	    }
	    else {
		echo "<tr>
                      <td>Minumum Nodes: </td>
                      <td class=\"left\"><font color=green>Unknown</font></td>
                  </tr>\n";
	    }
	    if ($virtnodes) {
		echo "<tr>
                      <td>Virtual Nodes: </td>
                      <td class=\"left\"><font>
                          $virtnodes</font></td>
                  </tr>\n";
	    }
	    else {
		echo "<tr>
                      <td>Virtual Nodes: </td>
                      <td class=\"left\"><font color=green>Unknown</font></td>
                  </tr>\n";
	    }
	}
	if (!$short) {
	    if ($mem_usage || $cpu_usage) {
		echo "<tr>
                      <td>Mem Usage Est: </td>
                      <td class=\"left\">$mem_usage</td>
                  </tr>\n";

		echo "<tr>
                      <td>CPU Usage Est: </td>
                      <td class=\"left\">$cpu_usage</td>
                  </tr>\n";
	    }

	    $lastact  = $this->LastAct();
	    $idletime = $this->IdleTime();
	    $stale    = $this->IdleStale();
    
	    if ($lastact != -1) {
		echo "<tr>
                      <td>Last Activity: </td>
                      <td class=\"left\">$lastact</td>
                  </tr>\n";

		if ($stale) { $str = "(stale)"; } else { $str = ""; }
	
		echo "<tr>
                      <td>Idle Time: </td>
                      <td class=\"left\">$idletime hours $str</td>
                  </tr>\n";
	    }

	    if (! ($swapreqs=="" || $swapreqs==0)) {
		echo "<tr>
                      <td>Swap Requests: </td>
                      <td class=\"left\">$swapreqs</td>
                  </tr>\n";

		echo "<tr>
                      <td>Last Swap Req.: </td>
                      <td class=\"left\">$lastswapreq</td>
                  </tr>\n";
	    }

	    $lockflip = ($lockdown ? 0 : 1);
	    $lockval  = ($lockdown ? "Yes" : "No");
	    echo "<tr>
                   <td>Locked Down:</td>
                   <td>$lockval (<a href=toggle.php?pid=$pid&eid=$eid".
		"&type=lockdown&value=$lockflip>Toggle</a>)
                   </td>
              </tr>\n";

	    if (ISADMIN() || STUDLY() || OPSGUY()) {
		$thisflip = ($skipvlans ? 0 : 1);
		$flipval  = ($skipvlans ? "Yes" : "No");
		echo "<tr>
                       <td>Skip Vlans:</td>
                       <td>$flipval (<a href=toggle.php?pid=$pid&eid=$eid".
		           "&type=skipvlans&value=$thisflip>Toggle</a>)
                       </td>
                      </tr>\n";
	    }
	}

	if ($batchmode) {
	    echo "<tr>
                    <td>Batch Mode: </td>
                    <td class=\"left\">Yes</td>
                  </tr>\n";

	    echo "<tr>
                    <td>Start Attempts: </td>
                    <td class=\"left\">$attempts</td>
                  </tr>\n";
	}

	if ($canceled) {
	    echo "<tr>
                 <td>Cancel Flag: </td>
                 <td class=\"left\">$canceled</td>
              </tr>\n";
	}

	if (!$short) {
	    if (ISADMIN()) {
		echo "<tr>
                        <td>UUID: </td>
                        <td class=left>$uuid</td>
                      </tr>\n";
	    }
	    if ($usemodelnet) {
		echo "<tr>
                      <td>Use Modelnet: </td>
                      <td class=\"left\">Yes</td>
                  </tr>\n";
		echo "<tr>
                      <td>Modelnet Phys Core Nodes: </td>
                      <td class=\"left\">$mnet_cores</td>
                  </tr>\n";
		echo "<tr>
                      <td>Modelnet Phys Edge Nodes: </td>
                      <td class=\"left\">$mnet_edges</td>
                  </tr>\n";

	    }
	    if (isset($syncserver)) {
		echo "<tr>
                      <td>Sync Server: </td>
                      <td class=\"left\">$syncserver</td>
                  </tr>\n";
	    }
	    if ($dpdb && $dpdbname && $dpdbpassword) {
		echo "<tr>
                      <td>DataBase Name: </td>
                      <td class=\"left\">$dpdbname</td>
                  </tr>\n";

		echo "<tr>
                      <td>DataBase User: </td>
                      <td class=\"left\">E${exptidx}</td>
                  </tr>\n";

		echo "<tr>
                      <td>DataBase Password: </td>
                      <td class=\"left\">$dpdbpassword</td>
                  </tr>\n";
	    }
	    echo "<tr>
                  <td>Index: </td>
                  <td class=\"left\">$exptidx";
	    if ($archive_idx)
		echo " ($archive_idx) ";
	    echo " </td>
              </tr>\n";
	}
	if (!$short) {
	    if ($this->geniflags()) {
		$slice = GeniSlice::Lookup("geni-cm", $uuid);

		if ($slice) {
		    $slice_hrn = $slice->hrn();
		    $slice_urn = $slice->urn();
		    if (ISADMIN()) {
			$url = CreateURL("showslice", "slice_idx",
					 $slice->idx(), "showtype", "cm");

			echo "<tr>
                                <td>Geni Slice (CM): </td>
                                <td class=\"left\">
                                     <a href='$url'>$slice_urn</a></td>
                              </tr>\n";
		    }
		    else {
			echo "<tr>
                                <td>Geni Slice (CM): </td>
                                <td class=\"left\">$slice_urn</td>
                              </tr>\n";
		    }
		}
	    }
	    else {
		$slice = GeniSlice::LookupByExperiment("geni-sa", $this);
		if ($slice) {
		    $slice_hrn = $slice->hrn();
		    $slice_urn = $slice->urn();
		    if (ISADMIN()) {
			$url = CreateURL("showslice", "slice_idx",
					 $slice->idx(), "showtype", "sa");

			echo "<tr>
                                 <td>Geni Slice (SA): </td>
                                 <td class=\"left\">
                                      <a href='$url'>$slice_urn</a></td>
                             </tr>\n";
		    }
		    else {
			echo "<tr>
                                <td>Geni Slice (SA): </td>
                                <td class=\"left\">$slice_urn</td>
                              </tr>\n";
		    }
		    $slice = GeniSlice::Lookup("geni-cm", $slice_hrn);
		    if ($slice) {
			if (ISADMIN()) {
			    $url = CreateURL("showslice", "slice_idx",
					     $slice->idx(), "showtype", "cm");

			    echo "<tr>
                                     <td>Geni Slice (CM): </td>
                                     <td class=\"left\">
                                           <a href='$url'>$slice_urn</a></td>
                                  </tr>\n";
			}
			else {
			    echo "<tr>
                                    <td>Geni Slice (SA): </td>
                                    <td class=\"left\">$slice_urn</td>
                                  </tr>\n";
			}
		    }
		}
	    }
	}
	echo "</table>\n";
    }

    function ShowStats() {
	$idx = $this->idx();

	$query_result =
	    DBQueryFatal("select s.*,r.* from experiments as e ".
			 "left join experiment_stats as s on s.exptidx=e.idx ".
			 "left join experiment_resources as r on ".
			 "     r.idx=s.rsrcidx ".
			 "where e.idx='$idx'");

	if (! mysql_num_rows($query_result)) {
	    return 0;
	}
	$row = mysql_fetch_assoc($query_result);

        #
        # Not pretty printed yet.
        #
	echo "<table align=center border=1>\n";
    
	foreach($row as $key => $value) {
	    if ($key == "thumbnail")
		continue;
	
	    echo "<tr>
                      <td>$key:</td>
                      <td>$value</td>
                  </tr>\n";
	}
	echo "</table>\n";
	return 0;
    }
}

class ExperimentStats
{
    var	$stats;

    #
    # Constructor by lookup on unique index.
    #
    function ExperimentStats($exptidx) {
	$safe_exptidx = addslashes($exptidx);

	$query_result =
	    DBQueryWarn("select * from experiment_stats ".
			"where exptidx='$safe_exptidx'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->stats = null;
	    return;
	}
	$this->stats = mysql_fetch_array($query_result);
    }

    # Hmm, how does one cause an error in a php constructor?
    function IsValid() {
	return !is_null($this->stats);
    }

    # Lookup by exptidx, but allow for lookup by pid,eid with variable args.
    function Lookup($exptidx) {
	$foo = new ExperimentStats($exptidx);

	if ($foo->IsValid())
	    return $foo;

	return null;
    }

    #
    # Refresh an instance by reloading from the DB.
    #
    function Refresh() {
	if (! $this->IsValid())
	    return -1;

	$idx = $this->exptidx();

	$query_result =
	    DBQueryWarn("select * from experiment_stats ".
			"where exptidx='$idx'");
    
	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->stats = NULL;
	    return -1;
	}
	$this->stats = mysql_fetch_array($query_result);
	return 0;
    }

    # accessors
    function field($name) {
	return (is_null($this->stats) ? -1 : $this->stats[$name]);
    }
    function pid()	    { return $this->field('pid'); }
    function gid()	    { return $this->field('gid'); }
    function eid()	    { return $this->field('eid'); }
    function idx()	    { return $this->field('exptidx'); }
    function exptidx()	    { return $this->field('exptidx'); }
    function rsrcidx()      { return $this->field('rsrcidx'); }
    function pid_idx()      { return $this->field('pid_idx'); }
    function gid_idx()      { return $this->field('gid_idx'); }
    function archive_idx()  { return $this->field('archive_idx'); }

    function Project() {
	return Project::Lookup($this->pid_idx());
    }

    #
    # Page header; spit back some html for the typical page header.
    #
    function PageHeader() {
	$idx = $this->idx();
	
	$html = "<font size=+2>Experiment <b>".
	    "<a href='showstats.php3?showby=expt&exptidx=$idx'>$idx</a> ".
	    "</b></font>\n";

	return $html;
    }

    #
    # Project level check.
    #
    function AccessCheck ($user, $access_type) {
	global $TBDB_TRUST_USER;
	$pid_idx = $this->pid_idx();
	
	if (! ($project = Project::Lookup($pid_idx))) {
	    TBERROR("ExperimentStats::AccessCheck: ".
		    "Cannot map project $pid_idx to its object", 1);
	}
	return $project->AccessCheck($user, $TBDB_TRUST_USER);
    }
}

class ExperimentResources
{
    var	$resources;

    #
    # Constructor by lookup on unique index for current resources
    #
    function ExperimentResources($rsrcidx) {
	$safe_rsrcidx = addslashes($rsrcidx);

	$query_result =
	    DBQueryWarn("select r.* from experiment_resources as r ".
			"where r.idx='$safe_rsrcidx'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->resources = null;
	    return;
	}
	$this->resources = mysql_fetch_array($query_result);
    }

    # Hmm, how does one cause an error in a php constructor?
    function IsValid() {
	return !is_null($this->resources);
    }

    # Lookup by resource record number
    function Lookup($rsrcidx) {
	$foo = new ExperimentResources($rsrcidx);

	if ($foo->IsValid())
	    return $foo;

	return null;
    }

    # accessors
    function field($name) {
	return (is_null($this->resources) ? -1 : $this->resources[$name]);
    }
    function idx()	      { return $this->field('idx'); }
    function exptidx()	      { return $this->field('exptidx'); }
    function lastidx()        { return $this->field('lastidx'); }
    function wirelesslans()   { return $this->field('wirelesslans'); }
    function input_data_idx() { return $this->field('input_data_idx'); }

    function GetStats() {
	return ExperimentStats::Lookup($this->exptidx());
    }

    #
    # Project level check via the stats record.
    #
    function AccessCheck ($user, $access_type) {
	$experiment_stats = $this->GetStats();

	return $experiment_stats->AccessCheck($user, $access_type);
    }
    
    # Return the stored NS file this resource record
    function NSFile() {
	$input_data_idx = $this->input_data_idx();

	if (! $input_data_idx) {
	    return null;
	}

	$query_result =
	    DBQueryFatal("select input from experiment_input_data ".
			 "where idx='$input_data_idx'");

	if (! mysql_num_rows($query_result)) {
	    return null;
	}
	$row    = mysql_fetch_array($query_result);
	return $row["input"];
    }
}

#
# Class function to show a listing of experiments by user/pid/gid
#
function ShowExperimentList($type, $this_user, $object) {
    global $EXPOSETEMPLATES;
    
    if ($EXPOSETEMPLATES) {
	ShowExperimentList_internal(1, $type, $this_user, $object);
    }
    ShowExperimentList_internal(0, $type, $this_user, $object);
}

function ShowExperimentList_internal($templates_only,
				     $type, $this_user, $object,
				     $tabledefs = null) {
    global $TB_EXPTSTATE_SWAPPED, $TB_EXPTSTATE_SWAPPING;
    
    $from_idx = $this_user->uid_idx();
    $nopid    = 0;
    $parens   = 0;
    $id       = "explist";
    $class    = "explist";
    $wanthtml = 0;
    $notitle  = FALSE;
    $html     = null;
    
    if ($tabledefs) {
	$id    = (isset($tabledefs['#id']) ? $tabledefs['#id'] : $id);
	$class = (isset($tabledefs['#class']) ? $tabledefs['#class'] : $class);
	$wanthtml = (isset($tabledefs['#html']) ? $tabledefs['#html'] : $html);
	$notitle = (isset($tabledefs['#notitle']) ?
		    $tabledefs['#notitle'] : $notitle);
    }

    if ($type == "USER") {
	$uid   = $object->uid();
	$where = "expt_head_uid='$uid'";
	$title = "Current";
    } elseif ($type == "PROJ") {
	$pid   = $object->pid();
	$where = "e.pid='$pid'";
	$title = "Project";
	$nopid = 1;
    } elseif ($type == "GROUP") {
	$pid   = $object->pid();
	$gid   = $object->gid();
	$where = "e.pid='$pid' and e.gid='$gid'";
	$title = "Group";
	$nopid = 1;
    } else {
	TBERROR("ShowExperimentList: Invalid arguments", 1);
    }

    $template_clause = "";
    if ($templates_only) {
	$template_clause = " and i.idx is not null ";
    }

    if (ISADMIN()) {
	$query_result =
	    DBQueryFatal("select e.*,count(r.node_id) as nodes, ".
			 "round(e.minimum_nodes+.1,0) as min_nodes ".
			 "from experiments as e ".
			 "left join experiment_templates as t on ".
			 "     t.pid=e.pid and t.eid=e.eid ".
			 "left join experiment_template_instances as i on ".
			 "     i.exptidx=e.idx " .
			 "left join reserved as r on e.pid=r.pid and ".
			 "     e.eid=r.eid ".
			 "where ($where) and t.guid is null $template_clause ".
			 "group by e.pid,e.eid order by e.state,e.eid");
    }
    else {
	$query_result =
	    DBQueryFatal("select e.*,count(r.node_id) as nodes, ".
	 		 "round(e.minimum_nodes+.1,0) as min_nodes ".
 			 "from experiments as e ".
			 "left join experiment_templates as t on ".
			 "     t.pid=e.pid and t.eid=e.eid ".
			 "left join experiment_template_instances as i on ".
			 "     i.exptidx=e.idx " .
			 "left join reserved as r on e.pid=r.pid and ".
			 "     e.eid=r.eid ".
 			 "left join group_membership as g on g.pid=e.pid and ".
	 		 "     g.gid=e.gid and g.uid_idx='$from_idx' ".
			 "where g.uid is not null and ($where) ".
			 "      and t.guid is null $template_clause " .
			 "group by e.pid,e.eid order by e.state,e.eid");
    }
    
    if (mysql_num_rows($query_result)) {
	if ($wanthtml)
	    ob_start();

	echo "<center>
               <h3>$title ".
	        ($templates_only ?
		 "Template Instances" : "Experiments") . "</h3>
              </center>
              <table align=center border=1 class=class id=$id>\n";

	if ($nopid) {
	    $pidrow="";
	} else {
	    $pidrow="\n<th>PID</th>";
	}
	
	echo "<thead class='sort'>";
	echo "<tr>$pidrow
              <th>EID</th>
              <th>State</th>
              <th align=center>Nodes [1]</th>
              <th align=center>Hours Idle [2]</th>
              <th>Description</th>
          </tr></thead>\n";

	$idlemark = "<b>*</b>";
	$stalemark = "<b>?</b>";
	
	while ($row = mysql_fetch_array($query_result)) {
	    $pid  = $row["pid"];
	    $eid  = $row["eid"];
	    $state= $row["state"];
	    $nodes= $row["nodes"];
	    $minnodes = $row["min_nodes"];
	    $ignore = $row["idle_ignore"];
	    $name = $row["expt_name"];

	    if (! ($experiment = Experiment::LookupByPidEid($pid, $eid))) {
		TBERROR("Could not map $pid/$eid to its object", 1);
	    }
	    $idlehours = $experiment->IdleTime();
	    $stale     = $experiment->IdleStale();
	    
	    if ($nodes==0) {
		$nodes = "<font color=green>$minnodes</font>";
	    } elseif ($row["swap_requests"] > 0) {
		$nodes .= $idlemark;
	    }

	    if ($nopid) {
		$pidrow="";
	    } else {
		$pidrow="\n<td>".
		    "<a href='showproject.php3?pid=$pid'>$pid</a></td>";
	    }

	    $idlestr = $idlehours;
	    if ($idlehours > 0) {
		if ($stale) { $idlestr .= $stalemark; }
		if ($ignore) { $idlestr = "($idlestr)"; $parens=1; }
	    } elseif ($idlehours == -1) { $idlestr = "&nbsp;"; }
	    
	    echo "<tr>$pidrow
                 <td><a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></td>
		 <td>$state</td>
                 <td align=center>$nodes</td>
                 <td align=center>$idlestr</td>
                 <td>$name</td>
             </tr>\n";
	}
	echo "</table>\n";
	echo "<table align=center cellpadding=0 class=stealth><tr>\n".
	    "<td class=stealth align=left><font size=-1><ol>\n".
	    "<li>Node counts in <font color=green><b>green</b></font>\n".
	    "show a rough estimate of the minimum number of \n".
	    "nodes required to swap in.\n".
	    "They account for delay nodes, but not for node types, etc.\n".
	    "<li>A $stalemark indicates that the data is stale, and ".
	    "at least one node in the experiment has not reported ".
	    "on its proper schedule.\n"; 
	if ($parens) {
            # do not show this unless we did it... most users should not ever
	    # need to know that some expts have their idleness ignored
	    echo "Values are in parenthesis for idle-ignore experiments.\n";
	}
	echo "</ol></font></td></tr></table>\n";

	# Sort initialized later when page fully loaded.
	AddSortedTable($id);

	if ($wanthtml) {
	    $html = ob_get_contents();
	    ob_end_clean();
	}
    }
    return $html;
}
?>

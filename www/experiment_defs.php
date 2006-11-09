<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
#
# This class is really just a wrapper around the DB data ...
#
class Experiment
{
    var	$experiment;
    var $group;

    #
    # Constructor by lookup on unique index.
    #
    function Experiment($exptidx) {
	$safe_exptidx = addslashes($exptidx);

	$query_result =
	    DBQueryWarn("select * from experiments ".
			"where idx='$safe_exptidx'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->experiment = null;
	    return;
	}
	$this->experiment = mysql_fetch_array($query_result);

	# Load lazily;
	$this->group      = null;
    }

    # Hmm, how does one cause an error in a php constructor?
    function IsValid() {
	return !is_null($this->experiment);
    }

    # Lookup by exptidx.
    function Lookup($exptidx) {
	$foo = new Experiment($exptidx);

	if ($foo->IsValid())
	    return $foo;
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
	    return -1;
	}
	$this->experiment = mysql_fetch_array($query_result);
	$this->group      = null;
	return 0;
    }

    #
    # Load the group object for an experiment.
    #
    function Group() {
	$pid = $this->pid();
	$gid = $this->gid();

	if ($this->group)
	    return $group;

	$this->group = Group::LookupByPidGid($pid, $gid);
	if (! $this->group) {
	    TBERROR("Could not lookup group $pid/$gid!", 1);
	}
	return $this->group;
    }

    # accessors
    function field($name) {
	return (is_null($this->experiment) ? -1 : $this->experiment[$name]);
    }
    function pid()	    { return $this->field('pid'); }
    function gid()	    { return $this->field('gid'); }
    function eid()	    { return $this->field('eid'); }
    function idx()	    { return $this->field('idx'); }
    function path()	    { return $this->field('path'); }
    function state()	    { return $this->field('state'); }
    function batchstate()   { return $this->field('batchstate'); }
    function batchmode()    { return $this->field('batchmode'); }
    function rsrcidx()      { return $this->stats('rsrcidx'); }
    function creator()      { return $this->field('expt_head_uid');}
    function canceled()     { return $this->field('canceled'); }
    function locked()       { return $this->field('expt_locked'); }
    function elabinelab()   { return $this->field('elab_in_elab');}
    function lockdown()     { return $this->field('lockdown'); }
    function created()      { return $this->field('expt_created'); }
    function swapper()      { return $this->field('expt_swap_uid');}
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

    #
    # Access Check. This is not code I want to duplicate, so hand off to
    # global routine until all code converted.
    #
    function AccessCheck ($uid, $access_type) {
	$pid = $this->pid();
	$eid = $this->eid();
	
	return TBExptAccessCheck($uid, $pid, $eid, $access_type);
    }

    #
    # Page header; spit back some html for the typical page header.
    #
    function PageHeader() {
	$pid = $this->pid();
	$eid = $this->eid();
	
	$html = "<font size=+2>Experiment <b>".
	    "<a href='showproject.php3?pid=$pid'>$pid</a>/".
	    "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a>".
	    "</b></font>\n";

	return $html;
    }

    #
    # Return the Unix GID (numeric) for an experiments group.
    #
    function UnixGID() {
	$group = $this->Group();

	return $group->unix_gid();
    }
}

#
# Notes about the template table
#
# * I am not storing the existing idle,maxdur,etc settings. These will be
#   required to be specified when the swapin is created.
#
# * There is duplication of the "settings" that result from the parse.
#   I am wondering about moving these to a "virt_settings" table as a
#   key/value pairing, but thats a big change and the sql queries get more
#   complicated. Or, it can be a virt_settings table with the values
#   specified (read: just moved from experiments table to another table).
#
# * My understanding is that a "modification" causes a new record with the
#   same GUID, but a new version number. These are permanent records.
#

CREATE TABLE experiment_templates (
  -- Globally Unique ID. Okay, how global is global? Site global?
  guid varchar(16) NOT NULL default '',
  -- Version number for tracking modifications
  vers smallint(5) unsigned NOT NULL default '0',
  -- Pointer to parent, for modification.
  parent_guid varchar(16) default NULL,
  parent_vers smallint(5) unsigned default NULL,
  -- Pointer to most recent child, for modification.
  child_guid varchar(16) default NULL,
  child_vers smallint(5) unsigned default NULL,
  -- Project ID
  pid varchar(12) NOT NULL default '',
  -- Group ID
  gid varchar(16) NOT NULL default '',
  -- Template ID (something unique that the user specifies)
  tid varchar(32) NOT NULL default '',
  -- Creator of the template
  uid_idx mediumint(8) unsigned NOT NULL default '0',
  uid varchar(8) NOT NULL default '',
  -- Eric says these are really metadata. Probably true.
  description mediumtext,
  -- EID of the underlying parsed experiment. 
  eid varchar(32) NOT NULL default '',
  -- The Archive for the template. This is shared with derived templates.
  archive_idx int(10) unsigned default NULL,
  -- These are all mirrors of what is in the existing experiments table
  created datetime default NULL,
  modified datetime default NULL,
  locked datetime default NULL,
  state varchar(16) NOT NULL default 'new',
  path tinytext,
  maximum_nodes int(6) unsigned default NULL,
  minimum_nodes int(6) unsigned default NULL,
  logfile tinytext,
  logfile_open tinyint(4) NOT NULL default '0',
  prerender_pid int(11) default '0',
  -- Hide this template in the graph.
  hidden tinyint(1) NOT NULL default '0',
  -- Make this template an active template within its graph
  active tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (guid, vers),
  KEY pidtid (pid,tid),
  KEY pideid (pid,eid)
) TYPE=MyISAM;

#
# Temporary storage for graphs generated with dot.
#
CREATE TABLE experiment_template_graphs (
  -- Backlink to the template the input belongs to.
  parent_guid varchar(16) NOT NULL default '',
  -- Current scale factor. 
  scale float(10,3) NOT NULL default '1.0',
  image mediumblob,
  imap mediumtext,
  PRIMARY KEY  (parent_guid)
) TYPE=MyISAM;

#
# I like to keep the nsfiles (or whatever the input will be) separate
# from the template record so that I do not have to see it when
# talking to mysql directly. The table below allows for multiple input
# files per template, although not sure if that is really needed.
#
# Note that table does not actually store the input files, but is a pointer
# to another table. Why? Cause I expect a lot of duplication of input files,
# if changes to the metadata cause a new template record. Instead, put the
# files in another table and md5 them to track uniqueness.
#
CREATE TABLE experiment_template_inputs (
  -- Auto generated unique index so there can be more then one input
  -- per template. Note sure this is actually needed.
  idx int(10) unsigned NOT NULL auto_increment,  
  -- Backlink to the template the input belongs to.
  parent_guid varchar(16) NOT NULL default '',
  parent_vers smallint(5) unsigned NOT NULL default '0',
  -- These are for debugging and clarity; not strictly needed.
  -- Project ID
  pid varchar(12) NOT NULL default '',
  -- Template ID (aka the eid)
  tid varchar(32) NOT NULL default '',
  -- The index into the inputs table.
  input_idx int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (parent_guid, parent_vers, idx),
  KEY pidtid (pid,tid)
) TYPE=MyISAM;

#
# And this is the table of inputs mentioned above.
#
CREATE TABLE experiment_template_input_data (
  -- Auto generated unique index.
  idx int(10) unsigned NOT NULL auto_increment,
  -- MD5 of the input file.
  md5 varchar(32) NOT NULL default '',
  -- The actual text of the input
  input mediumtext,
  PRIMARY KEY  (idx),
  UNIQUE KEY md5 (md5)
) TYPE=MyISAM;

#
# These are the various settings that result from an NS file being parsed
# and are passed along to the experiment instance later (swapin). They
# could go into the template structure above, but that is too messy.
#
CREATE TABLE experiment_template_settings (
  -- Backlink to the template the metadata belongs to.
  parent_guid varchar(16) NOT NULL default '',
  parent_vers smallint(5) unsigned NOT NULL default '0',
  -- Project ID
  pid varchar(12) NOT NULL default '',
  -- Template ID (aka the eid)
  tid varchar(32) NOT NULL default '',
  -- These are also mirrors of what is in the experiments table, and are
  -- needed to parse and hold state about the virtual topo. They will
  -- eventually be passed down to the experiment table entry when a swapin
  -- is created. All come from the NS file parse.
  uselinkdelays tinyint(4) NOT NULL default '0',
  forcelinkdelays tinyint(4) NOT NULL default '0',
  multiplex_factor smallint(5) default NULL,
  uselatestwadata tinyint(4) NOT NULL default '0',
  usewatunnels tinyint(4) NOT NULL default '1',
  wa_delay_solverweight float default '0',
  wa_bw_solverweight float default '0',
  wa_plr_solverweight float default '0',
  sync_server varchar(32) default NULL,
  cpu_usage tinyint(4) unsigned NOT NULL default '0',
  mem_usage tinyint(4) unsigned NOT NULL default '0',
  veth_encapsulate tinyint(4) NOT NULL default '1',
  allowfixnode tinyint(4) NOT NULL default '1',
  jail_osname varchar(20) default NULL,
  delay_osname varchar(20) default NULL,
  use_ipassign tinyint(4) NOT NULL default '0',
  ipassign_args varchar(255) default NULL,
  linktest_level tinyint(4) NOT NULL default '0',
  linktest_pid int(11) default '0',
  useprepass tinyint(1) NOT NULL default '0',
  elab_in_elab tinyint(1) NOT NULL default '0',
  elabinelab_eid varchar(32) default NULL,
  elabinelab_cvstag varchar(64) default NULL,
  elabinelab_nosetup tinyint(1) NOT NULL default '0',
  security_level tinyint(1) NOT NULL default '0',
  delay_capacity tinyint(3) unsigned default NULL,
  savedisk tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (parent_guid, parent_vers),
  KEY pidtid (pid,tid)
) TYPE=MyISAM;

#
# This is versioned metadata that goes with each template. We store the
# guid and version of the corresponding record in the table below.
#
CREATE TABLE experiment_template_metadata (
  -- Globally Unique ID of the ExperimentTemplate this record belongs to.
  parent_guid varchar(16) NOT NULL default '',
  parent_vers smallint(5) unsigned NOT NULL default '0',
  -- GUID of the metadata item.
  metadata_guid varchar(16) NOT NULL default '',
  metadata_vers smallint(5) unsigned NOT NULL default '0',
  -- Internal metadata items, handled specially.
  internal tinyint(1) NOT NULL default '0',
  -- Hidden metadata items
  hidden tinyint(1) NOT NULL default '0',
  -- A type descriptor for the metadata, when not user generated.
  metadata_type enum('tid','template_description','parameter_description') default NULL,
  PRIMARY KEY  (parent_guid, parent_vers, metadata_guid, metadata_vers)
) TYPE=MyISAM;

#
# The actual versioned metadata.
#
CREATE TABLE experiment_template_metadata_items (
  -- Globally Unique ID. 
  guid varchar(16) NOT NULL default '',
  -- Version number for tracking modifications
  vers smallint(5) unsigned NOT NULL default '0',
  -- Backlink to the previous version of this metadata item.
  parent_guid varchar(16) default NULL,
  parent_vers smallint(5) unsigned NOT NULL default '0',
  -- Record the template GUID this metadata associated with. This is for
  -- permission checks and for deletion.
  template_guid varchar(16) NOT NULL default '',
  -- Creator of this record
  uid_idx mediumint(8) unsigned NOT NULL default '0',
  uid varchar(8) NOT NULL default '',
  -- Key/Value pairs.
  name varchar(64) NOT NULL default '',
  value mediumtext,
  created datetime default NULL,
  PRIMARY KEY (guid, vers),
  KEY parent (parent_guid,parent_vers),
  KEY template (template_guid)
) TYPE=MyISAM;

#
# This is a table of formal parameters for the template, with optional
# values associated. They  are Key/Value pairs that are associated
# with a template. 
#
# * Need some new syntax for the NS file. Perhaps:
#
#	tb-define-formal-parameter name value
#
#   where the value is optional and causes an entry with a null value (in
#   the table below).
#
# * My understanding is that formal parameters which are bound (value
#   provided) when the template is created, have that value passed along
#   to all instances (swapins) of the template. Unbound values must be
#   provided when the instance is created, perhaps via a simple XML data
#   file that specifies key/value pairs. You can of course override the
#   already bound values (ones specified in the template).
#
# * I am still not clear on the versioning of metadata, and as discussed
#   in the meeting, I am glossing over it for now.
#
CREATE TABLE experiment_template_parameters (
  -- Globally Unique ID of the ExperimentTemplate this record belongs to.
  parent_guid varchar(16) NOT NULL default '',
  -- Version number for tracking modifications
  parent_vers smallint(5) unsigned NOT NULL default '0',
  -- Project ID
  pid varchar(12) NOT NULL default '',
  -- Template ID (aka the eid)
  tid varchar(32) NOT NULL default '',
  name varchar(64) NOT NULL default '',
  value tinytext,
  -- These point to the optional metadata description.
  metadata_guid varchar(16) default NULL,
  metadata_vers smallint(5) unsigned NOT NULL default '0',
  PRIMARY KEY  (parent_guid, parent_vers, name),
  KEY pidtid (pid,tid)
) TYPE=MyISAM;

#
# Hmm, the above table is a problem wrt experiment parsing. 
#
CREATE TABLE `virt_parameters` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `name` varchar(64) NOT NULL default '',
  `value` tinytext,
  `description` text,
  PRIMARY KEY  (`exptidx`,`name`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

#
# Events that are dynamically created by the user, as for analysis.
# Assumed to be just program agent events; generalize later, perhaps.
#
CREATE TABLE experiment_template_events (
  -- Globally Unique ID of the ExperimentTemplate this record belongs to.
  parent_guid varchar(16) NOT NULL default '',
  -- Version number for tracking modifications
  parent_vers smallint(5) unsigned NOT NULL default '0',
  -- stuff for the eventlist table
  vname varchar(64) NOT NULL default '',
  vnode varchar(32) NOT NULL default '',
  time float(10,3) NOT NULL default '0.000',
  objecttype smallint(5) unsigned NOT NULL default '0',
  eventtype smallint(5) unsigned NOT NULL default '0',
  arguments text,
  PRIMARY KEY  (parent_guid, parent_vers, vname)
) TYPE=MyISAM;

#
# This table is a wrapper around the current experiments table, which will
# continue to be the basic structure for the swapin. This includes all of
# virt tables which are uniquely associated with an experiment record. So,
# an instantiation of a template generates one of these, and does the equiv
# of what happens now when an experiment is created and swapped in.
#
# This table is also the historical record for template instantiations;
# records are permanent. 
#
# Note that there other things that will go in this table, just not sure
# what they are yet, as they relate to "Experiment" (in Eric jargon) and
# to "ExperimentRecord." 
#
CREATE TABLE experiment_template_instances (
  -- Auto generated unique index.
  idx int(10) unsigned NOT NULL auto_increment,  
  -- Backlink to the template.
  parent_guid varchar(16) NOT NULL default '',
  parent_vers smallint(5) unsigned NOT NULL default '0',
  -- The experiment index (into the current experiments table).
  exptidx int(10) unsigned NOT NULL default '0',
  -- Project ID
  pid varchar(12) NOT NULL default '',
  -- The actual eid (for the experiments table)
  eid varchar(32) NOT NULL default '',
  -- Creator of the instance.
  uid_idx mediumint(8) unsigned NOT NULL default '0',
  uid varchar(8) NOT NULL default '',
  -- A short description; not sure I really want this. 
  description tinytext,
  -- A little bit of duplication ...
  start_time datetime default NULL,
  stop_time datetime default NULL,
  pause_time datetime default NULL,
  continue_time datetime default NULL,
  runtime int(10) unsigned default 0,
  -- The current experiment that is running (see below). One at a time!
  runidx int(10) unsigned default NULL,
  -- The tag for the template at the time of instantiation.
  template_tag varchar(64) default NULL,
  -- Date of last export.
  export_time datetime default NULL,
  -- A lock to prevent mayhem.
  locked datetime default NULL,
  locker_pid int(11) default '0',
  PRIMARY KEY  (idx),
  KEY  (exptidx),
  KEY  (parent_guid,parent_vers),
  KEY  (pid,eid)
) TYPE=MyISAM;

#
# These are the bindings for the formal parameters above. Note that values
# fully specified in the template can still be overridden when the template
# is instantiated (swapped in), and again before each Experiment run. At
# present I am using two tables, but that is probably overkill.
#
# This table is also the historical record for template bindings; records
# are permanent. 
#
CREATE TABLE experiment_template_instance_bindings (
  -- Backlink to the instance above.
  instance_idx int(10) unsigned NOT NULL default '0',
  -- Backlink to the template.
  parent_guid varchar(16) NOT NULL default '',
  parent_vers smallint(5) unsigned NOT NULL default '0',
  -- The experiment index (into the current experiments table).
  exptidx int(10) unsigned NOT NULL default '0',
  -- Project ID
  pid varchar(12) NOT NULL default '',
  -- The actual eid (for the experiments table)
  eid varchar(32) NOT NULL default '',
  name varchar(64) NOT NULL default '',
  value tinytext NOT NULL,
  PRIMARY KEY  (instance_idx, name),
  KEY parent_guid (parent_guid,parent_vers),
  KEY pidtid (pid,eid)
) TYPE=MyISAM;

#
# This is the proto structure for the new Experiment object. These are
# just the things I can think of right now. Others?
#
CREATE TABLE experiment_runs (
  -- The experiment index (into the current experiments table).
  exptidx int(10) unsigned NOT NULL default '0',
  -- Auto increment to generate unique per-exptidx IDs
  idx int(10) unsigned NOT NULL auto_increment,  
  -- Run ID (a per-experiment unique identifier for this run)
  runid varchar(32) NOT NULL default '',
  -- A short description; not sure I really want this. 
  description tinytext,
  -- The archive tags for the start and end of the run.
  starting_archive_tag varchar(64) default NULL,
  ending_archive_tag varchar(64) default NULL,
  -- The tag for the commit at the end of the run.
  archive_tag varchar(64) default NULL,
  -- Timestamps
  start_time datetime default NULL,
  stop_time datetime default NULL,
  -- If the run specified swapmod, record that with this flag.
  -- The NS file is stored in the archive.
  swapmod tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (exptidx, idx)
) TYPE=MyISAM;

#
# As mentioned above, these are bindings for each Experiment run, which
# default to the experiment_instance_bindings above if no per-run value
# is provided.
#
CREATE TABLE experiment_run_bindings (
  -- The experiment index (into the current experiments table).
  exptidx int(10) unsigned NOT NULL default '0',
  runidx int(10) unsigned NOT NULL default '0',
  name varchar(64) NOT NULL default '',
  value tinytext NOT NULL,
  PRIMARY KEY  (exptidx, runidx, name)
) TYPE=MyISAM;

#
# This table holds the names of nodes that failed to respond during a start
# or stop run. This is mostly informational at this point.
#
CREATE TABLE experiment_template_instance_deadnodes (
  -- Backlink to the instance above.
  instance_idx int(10) unsigned NOT NULL default '0',
  -- The experiment index (into the current experiments table).
  exptidx int(10) unsigned NOT NULL default '0',
  -- The run index 
  runidx int(10) unsigned NOT NULL default '0',
  -- The node that failed.
  node_id varchar(32) NOT NULL default '',
  -- The vname of the node since that is typically more useful.
  vname varchar(32) NOT NULL default '',
  PRIMARY KEY  (instance_idx, runidx, node_id)
) TYPE=MyISAM;


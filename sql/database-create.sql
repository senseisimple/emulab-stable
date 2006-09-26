-- MySQL dump 8.23
--
-- Host: localhost    Database: tbdb
---------------------------------------------------------
-- Server version	3.23.59-nightly-20050301-log

--
-- Table structure for table `accessed_files`
--

CREATE TABLE accessed_files (
  fn text NOT NULL,
  idx int(11) unsigned NOT NULL auto_increment,
  PRIMARY KEY  (fn(255)),
  KEY idx (idx)
) TYPE=MyISAM;

--
-- Table structure for table `active_checkups`
--

CREATE TABLE active_checkups (
  object varchar(128) NOT NULL default '',
  object_type varchar(64) NOT NULL default '',
  type varchar(64) NOT NULL default '',
  state varchar(16) NOT NULL default 'new',
  start datetime default NULL,
  PRIMARY KEY  (object)
) TYPE=MyISAM;

--
-- Table structure for table `archive_tags`
--

CREATE TABLE archive_tags (
  idx int(10) unsigned NOT NULL auto_increment,
  tag varchar(64) NOT NULL default '',
  archive_idx int(10) unsigned NOT NULL default '0',
  view varchar(64) NOT NULL default '',
  date_created int(10) unsigned NOT NULL default '0',
  tagtype enum('user','commit','savepoint','internal') NOT NULL default 'internal',
  description text,
  PRIMARY KEY  (idx),
  UNIQUE KEY tag (tag,archive_idx,view)
) TYPE=MyISAM;

--
-- Table structure for table `archive_views`
--

CREATE TABLE archive_views (
  view varchar(64) NOT NULL default '',
  archive_idx int(10) unsigned NOT NULL default '0',
  current_tag varchar(64) NOT NULL default '',
  previous_tag varchar(64) default NULL,
  date_created int(10) unsigned NOT NULL default '0',
  branch_tag varchar(64) default NULL,
  parent_view varchar(64) default NULL,
  PRIMARY KEY  (view,archive_idx)
) TYPE=MyISAM;

--
-- Table structure for table `archives`
--

CREATE TABLE archives (
  idx int(10) unsigned NOT NULL auto_increment,
  directory tinytext,
  date_created int(10) unsigned NOT NULL default '0',
  archived tinyint(1) default '0',
  date_archived int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (idx)
) TYPE=MyISAM;

--
-- Table structure for table `buildings`
--

CREATE TABLE buildings (
  building varchar(32) NOT NULL default '',
  image_path tinytext,
  title tinytext NOT NULL,
  PRIMARY KEY  (building)
) TYPE=MyISAM;

--
-- Table structure for table `cameras`
--

CREATE TABLE cameras (
  name varchar(32) NOT NULL default '',
  building varchar(32) NOT NULL default '',
  floor varchar(32) NOT NULL default '',
  hostname varchar(255) default NULL,
  port smallint(5) unsigned NOT NULL default '6100',
  device varchar(64) NOT NULL default '',
  loc_x float NOT NULL default '0',
  loc_y float NOT NULL default '0',
  width float NOT NULL default '0',
  height float NOT NULL default '0',
  config tinytext,
  fixed_x float NOT NULL default '0',
  fixed_y float NOT NULL default '0',
  PRIMARY KEY  (name,building,floor)
) TYPE=MyISAM;

--
-- Table structure for table `causes`
--

CREATE TABLE causes (
  cause varchar(16) NOT NULL default '',
  cause_desc varchar(32) NOT NULL default '',
  PRIMARY KEY  (cause),
  UNIQUE KEY cause_desc (cause_desc)
) TYPE=MyISAM;

--
-- Table structure for table `cdroms`
--

CREATE TABLE cdroms (
  cdkey varchar(64) NOT NULL default '',
  user_name tinytext NOT NULL,
  user_email tinytext NOT NULL,
  ready tinyint(4) NOT NULL default '0',
  requested datetime NOT NULL default '0000-00-00 00:00:00',
  created datetime NOT NULL default '0000-00-00 00:00:00',
  version int(10) unsigned NOT NULL default '1',
  PRIMARY KEY  (cdkey)
) TYPE=MyISAM;

--
-- Table structure for table `checkup_types`
--

CREATE TABLE checkup_types (
  object_type varchar(64) NOT NULL default '',
  checkup_type varchar(64) NOT NULL default '',
  major_type varchar(64) NOT NULL default '',
  expiration int(10) NOT NULL default '86400',
  PRIMARY KEY  (object_type,checkup_type)
) TYPE=MyISAM;

--
-- Table structure for table `checkups`
--

CREATE TABLE checkups (
  object varchar(128) NOT NULL default '',
  object_type varchar(64) NOT NULL default '',
  type varchar(64) NOT NULL default '',
  next datetime default NULL,
  PRIMARY KEY  (object,type)
) TYPE=MyISAM;

--
-- Table structure for table `checkups_temp`
--

CREATE TABLE checkups_temp (
  object varchar(128) NOT NULL default '',
  object_type varchar(64) NOT NULL default '',
  type varchar(64) NOT NULL default '',
  next datetime default NULL,
  PRIMARY KEY  (object,type)
) TYPE=MyISAM;

--
-- Table structure for table `comments`
--

CREATE TABLE comments (
  table_name varchar(64) NOT NULL default '',
  column_name varchar(64) NOT NULL default '',
  description text NOT NULL,
  UNIQUE KEY table_name (table_name,column_name)
) TYPE=MyISAM;

--
-- Table structure for table `current_reloads`
--

CREATE TABLE current_reloads (
  node_id varchar(32) NOT NULL default '',
  image_id varchar(45) NOT NULL default '',
  mustwipe tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `datapository_databases`
--

CREATE TABLE datapository_databases (
  dbname varchar(64) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  gid varchar(16) NOT NULL default '',
  uid varchar(8) NOT NULL default '',
  created datetime default NULL,
  PRIMARY KEY  (dbname)
) TYPE=MyISAM;

--
-- Table structure for table `default_firewall_rules`
--

CREATE TABLE default_firewall_rules (
  type enum('ipfw','ipfw2','ipchains','ipfw2-vlan') NOT NULL default 'ipfw',
  style enum('open','closed','basic','emulab') NOT NULL default 'basic',
  enabled tinyint(4) NOT NULL default '0',
  ruleno int(10) unsigned NOT NULL default '0',
  rule text NOT NULL,
  PRIMARY KEY  (type,style,ruleno)
) TYPE=MyISAM;

--
-- Table structure for table `default_firewall_vars`
--

CREATE TABLE default_firewall_vars (
  name varchar(255) NOT NULL default '',
  value text,
  PRIMARY KEY  (name)
) TYPE=MyISAM;

--
-- Table structure for table `delays`
--

CREATE TABLE delays (
  node_id varchar(32) NOT NULL default '',
  pipe0 smallint(5) unsigned NOT NULL default '0',
  delay0 float(10,2) NOT NULL default '0.00',
  bandwidth0 int(10) unsigned NOT NULL default '100',
  lossrate0 float(10,3) NOT NULL default '0.000',
  q0_limit int(11) default '0',
  q0_maxthresh int(11) default '0',
  q0_minthresh int(11) default '0',
  q0_weight float default '0',
  q0_linterm int(11) default '0',
  q0_qinbytes tinyint(4) default '0',
  q0_bytes tinyint(4) default '0',
  q0_meanpsize int(11) default '0',
  q0_wait int(11) default '0',
  q0_setbit int(11) default '0',
  q0_droptail int(11) default '0',
  q0_red tinyint(4) default '0',
  q0_gentle tinyint(4) default '0',
  pipe1 smallint(5) unsigned NOT NULL default '0',
  delay1 float(10,2) NOT NULL default '0.00',
  bandwidth1 int(10) unsigned NOT NULL default '100',
  lossrate1 float(10,3) NOT NULL default '0.000',
  q1_limit int(11) default '0',
  q1_maxthresh int(11) default '0',
  q1_minthresh int(11) default '0',
  q1_weight float default '0',
  q1_linterm int(11) default '0',
  q1_qinbytes tinyint(4) default '0',
  q1_bytes tinyint(4) default '0',
  q1_meanpsize int(11) default '0',
  q1_wait int(11) default '0',
  q1_setbit int(11) default '0',
  q1_droptail int(11) default '0',
  q1_red tinyint(4) default '0',
  q1_gentle tinyint(4) default '0',
  iface0 varchar(8) NOT NULL default '',
  iface1 varchar(8) NOT NULL default '',
  eid varchar(32) default NULL,
  pid varchar(32) default NULL,
  vname varchar(32) default NULL,
  vnode0 varchar(32) default NULL,
  vnode1 varchar(32) default NULL,
  card0 tinyint(3) unsigned default NULL,
  card1 tinyint(3) unsigned default NULL,
  noshaping tinyint(1) default '0',
  PRIMARY KEY  (node_id,iface0,iface1),
  KEY pid (pid,eid)
) TYPE=MyISAM;

--
-- Table structure for table `deleted_users`
--

CREATE TABLE deleted_users (
  uid varchar(8) NOT NULL default '',
  uid_idx smallint(5) unsigned NOT NULL default '0',
  usr_created datetime default NULL,
  usr_deleted datetime default NULL,
  usr_name tinytext,
  usr_title tinytext,
  usr_affil tinytext,
  usr_email tinytext,
  usr_URL tinytext,
  usr_addr tinytext,
  usr_addr2 tinytext,
  usr_city tinytext,
  usr_state tinytext,
  usr_zip tinytext,
  usr_country tinytext,
  usr_phone tinytext,
  webonly tinyint(1) default '0',
  wikionly tinyint(1) default '0',
  notes text,
  PRIMARY KEY  (uid_idx)
) TYPE=MyISAM;

--
-- Table structure for table `delta_inst`
--

CREATE TABLE delta_inst (
  node_id varchar(32) NOT NULL default '',
  partition tinyint(4) NOT NULL default '0',
  delta_id varchar(10) NOT NULL default '',
  PRIMARY KEY  (node_id,partition,delta_id)
) TYPE=MyISAM;

--
-- Table structure for table `delta_proj`
--

CREATE TABLE delta_proj (
  delta_id varchar(10) NOT NULL default '',
  pid varchar(10) NOT NULL default '',
  PRIMARY KEY  (delta_id,pid)
) TYPE=MyISAM;

--
-- Table structure for table `deltas`
--

CREATE TABLE deltas (
  delta_id varchar(10) NOT NULL default '',
  delta_desc text,
  delta_path text NOT NULL,
  private enum('yes','no') NOT NULL default 'no',
  PRIMARY KEY  (delta_id)
) TYPE=MyISAM;

--
-- Table structure for table `elabinelab_vlans`
--

CREATE TABLE elabinelab_vlans (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  inner_id int(11) unsigned NOT NULL default '0',
  outer_id int(11) unsigned NOT NULL default '0',
  PRIMARY KEY  (pid,eid,inner_id)
) TYPE=MyISAM;

--
-- Table structure for table `emulab_indicies`
--

CREATE TABLE emulab_indicies (
  name varchar(64) NOT NULL default '',
  idx int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (name)
) TYPE=MyISAM;

--
-- Table structure for table `errors`
--

CREATE TABLE errors (
  session int(10) unsigned NOT NULL default '0',
  stamp int(10) unsigned NOT NULL default '0',
  exptidx int(11) NOT NULL default '0',
  script smallint(3) NOT NULL default '0',
  cause varchar(16) NOT NULL default '',
  confidence float NOT NULL default '0',
  mesg text NOT NULL,
  PRIMARY KEY  (session)
) TYPE=MyISAM;

--
-- Table structure for table `event_eventtypes`
--

CREATE TABLE event_eventtypes (
  idx smallint(5) unsigned NOT NULL default '0',
  type tinytext NOT NULL,
  PRIMARY KEY  (idx)
) TYPE=MyISAM;

--
-- Table structure for table `event_groups`
--

CREATE TABLE event_groups (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  idx int(10) unsigned NOT NULL auto_increment,
  group_name varchar(64) NOT NULL default '',
  agent_name varchar(64) NOT NULL default '',
  PRIMARY KEY  (pid,eid,idx),
  KEY group_name (group_name),
  KEY agent_name (agent_name)
) TYPE=MyISAM;

--
-- Table structure for table `event_objecttypes`
--

CREATE TABLE event_objecttypes (
  idx smallint(5) unsigned NOT NULL default '0',
  type tinytext NOT NULL,
  PRIMARY KEY  (idx)
) TYPE=MyISAM;

--
-- Table structure for table `eventlist`
--

CREATE TABLE eventlist (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  idx int(10) unsigned NOT NULL auto_increment,
  time float(10,3) NOT NULL default '0.000',
  vnode varchar(32) NOT NULL default '',
  vname varchar(64) NOT NULL default '',
  objecttype smallint(5) unsigned NOT NULL default '0',
  eventtype smallint(5) unsigned NOT NULL default '0',
  isgroup tinyint(1) unsigned default '0',
  arguments text,
  atstring text,
  parent varchar(64) NOT NULL default '',
  PRIMARY KEY  (pid,eid,idx),
  KEY vnode (vnode)
) TYPE=MyISAM;

--
-- Table structure for table `experiment_resources`
--

CREATE TABLE experiment_resources (
  idx int(10) unsigned NOT NULL auto_increment,
  exptidx int(10) unsigned NOT NULL default '0',
  lastidx int(10) unsigned default NULL,
  tstamp datetime default NULL,
  vnodes smallint(5) unsigned default '0',
  pnodes smallint(5) unsigned default '0',
  wanodes smallint(5) unsigned default '0',
  plabnodes smallint(5) unsigned default '0',
  simnodes smallint(5) unsigned default '0',
  jailnodes smallint(5) unsigned default '0',
  delaynodes smallint(5) unsigned default '0',
  linkdelays smallint(5) unsigned default '0',
  walinks smallint(5) unsigned default '0',
  links smallint(5) unsigned default '0',
  lans smallint(5) unsigned default '0',
  shapedlinks smallint(5) unsigned default '0',
  shapedlans smallint(5) unsigned default '0',
  wirelesslans smallint(5) unsigned default '0',
  minlinks tinyint(3) unsigned default '0',
  maxlinks tinyint(3) unsigned default '0',
  delay_capacity tinyint(3) unsigned default NULL,
  batchmode tinyint(1) unsigned default '0',
  archive_tag varchar(64) default NULL,
  thumbnail mediumblob,
  PRIMARY KEY  (idx),
  KEY exptidx (exptidx),
  KEY lastidx (lastidx)
) TYPE=MyISAM;

--
-- Table structure for table `experiment_run_bindings`
--

CREATE TABLE experiment_run_bindings (
  exptidx int(10) unsigned NOT NULL default '0',
  runidx int(10) unsigned NOT NULL default '0',
  name varchar(64) NOT NULL default '',
  value tinytext NOT NULL,
  PRIMARY KEY  (exptidx,runidx,name)
) TYPE=MyISAM;

--
-- Table structure for table `experiment_runs`
--

CREATE TABLE experiment_runs (
  exptidx int(10) unsigned NOT NULL default '0',
  idx int(10) unsigned NOT NULL auto_increment,
  runid varchar(32) NOT NULL default '',
  description tinytext,
  starting_archive_tag varchar(64) default NULL,
  ending_archive_tag varchar(64) default NULL,
  archive_tag varchar(64) default NULL,
  start_time datetime default NULL,
  stop_time datetime default NULL,
  swapmod tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (exptidx,idx)
) TYPE=MyISAM;

--
-- Table structure for table `experiment_stats`
--

CREATE TABLE experiment_stats (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  creator varchar(8) NOT NULL default '',
  exptidx int(10) unsigned NOT NULL default '0',
  rsrcidx int(10) unsigned NOT NULL default '0',
  lastrsrc int(10) unsigned default NULL,
  gid varchar(16) NOT NULL default '',
  created datetime default NULL,
  destroyed datetime default NULL,
  swapin_count smallint(5) unsigned default '0',
  swapin_last datetime default NULL,
  swapout_count smallint(5) unsigned default '0',
  swapout_last datetime default NULL,
  swapmod_count smallint(5) unsigned default '0',
  swapmod_last datetime default NULL,
  swap_errors smallint(5) unsigned default '0',
  swap_exitcode tinyint(3) unsigned default '0',
  idle_swaps smallint(5) unsigned default '0',
  swapin_duration int(10) unsigned default '0',
  batch tinyint(3) unsigned default '0',
  elabinelab tinyint(1) NOT NULL default '0',
  elabinelab_exptidx int(10) unsigned default NULL,
  security_level tinyint(1) NOT NULL default '0',
  archive_idx int(10) unsigned default NULL,
  last_error int(10) unsigned default NULL,
  dpdbname varchar(64) default NULL,
  PRIMARY KEY  (eid,pid,exptidx),
  KEY exptidx (exptidx),
  KEY rsrcidx (rsrcidx)
) TYPE=MyISAM;

--
-- Table structure for table `experiment_template_events`
--

CREATE TABLE experiment_template_events (
  parent_guid varchar(16) NOT NULL default '',
  parent_vers smallint(5) unsigned NOT NULL default '0',
  vname varchar(64) NOT NULL default '',
  vnode varchar(32) NOT NULL default '',
  time float(10,3) NOT NULL default '0.000',
  objecttype smallint(5) unsigned NOT NULL default '0',
  eventtype smallint(5) unsigned NOT NULL default '0',
  arguments text,
  PRIMARY KEY  (parent_guid, parent_vers, vname)
) TYPE=MyISAM;

--
-- Table structure for table `experiment_template_graphs`
--

CREATE TABLE experiment_template_graphs (
  parent_guid varchar(16) NOT NULL default '',
  scale float(10,3) NOT NULL default '1.000',
  image mediumblob,
  imap mediumtext,
  PRIMARY KEY  (parent_guid)
) TYPE=MyISAM;

--
-- Table structure for table `experiment_template_input_data`
--

CREATE TABLE experiment_template_input_data (
  idx int(10) unsigned NOT NULL auto_increment,
  md5 varchar(32) NOT NULL default '',
  input mediumtext,
  PRIMARY KEY  (idx),
  UNIQUE KEY md5 (md5)
) TYPE=MyISAM;

--
-- Table structure for table `experiment_template_inputs`
--

CREATE TABLE experiment_template_inputs (
  idx int(10) unsigned NOT NULL auto_increment,
  parent_guid varchar(16) NOT NULL default '',
  parent_vers smallint(5) unsigned NOT NULL default '0',
  pid varchar(12) NOT NULL default '',
  tid varchar(32) NOT NULL default '',
  input_idx int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (parent_guid,parent_vers,idx),
  KEY pidtid (pid,tid)
) TYPE=MyISAM;

--
-- Table structure for table `experiment_template_instance_bindings`
--

CREATE TABLE experiment_template_instance_bindings (
  instance_idx int(10) unsigned NOT NULL default '0',
  parent_guid varchar(16) NOT NULL default '',
  parent_vers smallint(5) unsigned NOT NULL default '0',
  exptidx int(10) unsigned NOT NULL default '0',
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  name varchar(64) NOT NULL default '',
  value tinytext NOT NULL,
  PRIMARY KEY  (instance_idx,name),
  KEY parent_guid (parent_guid,parent_vers),
  KEY pidtid (pid,eid)
) TYPE=MyISAM;

--
-- Table structure for table `experiment_template_instance_deadnodes`
--

CREATE TABLE experiment_template_instance_deadnodes (
  instance_idx int(10) unsigned NOT NULL default '0',
  exptidx int(10) unsigned NOT NULL default '0',
  runidx int(10) unsigned NOT NULL default '0',
  node_id varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  PRIMARY KEY  (instance_idx, runidx, node_id)
) TYPE=MyISAM;

--
-- Table structure for table `experiment_template_instances`
--

CREATE TABLE experiment_template_instances (
  idx int(10) unsigned NOT NULL auto_increment,
  parent_guid varchar(16) NOT NULL default '',
  parent_vers smallint(5) unsigned NOT NULL default '0',
  exptidx int(10) unsigned NOT NULL default '0',
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  uid varchar(8) NOT NULL default '',
  start_time datetime default NULL,
  stop_time datetime default NULL,
  runidx int(10) unsigned default NULL,
  template_tag varchar(64) default NULL,
  export_time datetime default NULL,
  PRIMARY KEY  (idx),
  KEY exptidx (exptidx),
  KEY parent_guid (parent_guid,parent_vers),
  KEY pid (pid,eid)
) TYPE=MyISAM;

--
-- Table structure for table `experiment_template_metadata`
--

CREATE TABLE experiment_template_metadata (
  parent_guid varchar(16) NOT NULL default '',
  parent_vers smallint(5) unsigned NOT NULL default '0',
  metadata_guid varchar(16) NOT NULL default '',
  metadata_vers smallint(5) unsigned NOT NULL default '0',
  internal tinyint(1) NOT NULL default '0',
  hidden tinyint(1) NOT NULL default '0',
  metadata_type enum('tid','template_description','parameter_description') default NULL,
  PRIMARY KEY  (parent_guid,parent_vers,metadata_guid,metadata_vers)
) TYPE=MyISAM;

--
-- Table structure for table `experiment_template_metadata_items`
--

CREATE TABLE experiment_template_metadata_items (
  guid varchar(16) NOT NULL default '',
  vers smallint(5) unsigned NOT NULL default '0',
  parent_guid varchar(16) default NULL,
  parent_vers smallint(5) unsigned NOT NULL default '0',
  template_guid varchar(16) NOT NULL default '',
  uid varchar(8) NOT NULL default '',
  name varchar(64) NOT NULL default '',
  value mediumtext,
  created datetime default NULL,
  PRIMARY KEY  (guid,vers),
  KEY parent (parent_guid,parent_vers),
  KEY template (template_guid)
) TYPE=MyISAM;

--
-- Table structure for table `experiment_template_parameters`
--

CREATE TABLE experiment_template_parameters (
  parent_guid varchar(16) NOT NULL default '',
  parent_vers smallint(5) unsigned NOT NULL default '0',
  pid varchar(12) NOT NULL default '',
  tid varchar(32) NOT NULL default '',
  name varchar(64) NOT NULL default '',
  value tinytext,
  metadata_guid varchar(16) default NULL,
  metadata_vers smallint(5) unsigned NOT NULL default '0',
  PRIMARY KEY  (parent_guid,parent_vers,name),
  KEY pidtid (pid,tid)
) TYPE=MyISAM;

--
-- Table structure for table `experiment_template_settings`
--

CREATE TABLE experiment_template_settings (
  parent_guid varchar(16) NOT NULL default '',
  parent_vers smallint(5) unsigned NOT NULL default '0',
  pid varchar(12) NOT NULL default '',
  tid varchar(32) NOT NULL default '',
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
  PRIMARY KEY  (parent_guid,parent_vers),
  KEY pidtid (pid,tid)
) TYPE=MyISAM;

--
-- Table structure for table `experiment_templates`
--

CREATE TABLE experiment_templates (
  guid varchar(16) NOT NULL default '',
  vers smallint(5) unsigned NOT NULL default '0',
  parent_guid varchar(16) default NULL,
  parent_vers smallint(5) unsigned default NULL,
  child_guid varchar(16) default NULL,
  child_vers smallint(5) unsigned default NULL,
  pid varchar(12) NOT NULL default '',
  gid varchar(16) NOT NULL default '',
  tid varchar(32) NOT NULL default '',
  uid varchar(8) NOT NULL default '',
  description mediumtext,
  eid varchar(32) NOT NULL default '',
  archive_idx int(10) unsigned default NULL,
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
  hidden tinyint(1) NOT NULL default '0',
  active tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (guid,vers),
  KEY pidtid (pid,tid),
  KEY pideid (pid,eid)
) TYPE=MyISAM;

--
-- Table structure for table `experiments`
--

CREATE TABLE experiments (
  eid varchar(32) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  gid varchar(16) NOT NULL default '',
  expt_created datetime default NULL,
  expt_expires datetime default NULL,
  expt_name tinytext,
  expt_head_uid varchar(8) NOT NULL default '',
  expt_start datetime default NULL,
  expt_end datetime default NULL,
  expt_terminating datetime default NULL,
  expt_locked datetime default NULL,
  expt_swapped datetime default NULL,
  expt_swap_uid varchar(8) NOT NULL default '',
  swappable tinyint(4) NOT NULL default '0',
  priority tinyint(4) NOT NULL default '0',
  noswap_reason tinytext,
  idleswap tinyint(4) NOT NULL default '0',
  idleswap_timeout int(4) NOT NULL default '0',
  noidleswap_reason tinytext,
  autoswap tinyint(4) NOT NULL default '0',
  autoswap_timeout int(4) NOT NULL default '0',
  batchmode tinyint(4) NOT NULL default '0',
  shared tinyint(4) NOT NULL default '0',
  state varchar(16) NOT NULL default 'new',
  maximum_nodes int(6) unsigned default NULL,
  minimum_nodes int(6) unsigned default NULL,
  testdb tinytext,
  path tinytext,
  logfile tinytext,
  logfile_open tinyint(4) NOT NULL default '0',
  attempts smallint(5) unsigned NOT NULL default '0',
  canceled tinyint(4) NOT NULL default '0',
  batchstate varchar(16) default NULL,
  event_sched_pid int(11) default '0',
  prerender_pid int(11) default '0',
  uselinkdelays tinyint(4) NOT NULL default '0',
  forcelinkdelays tinyint(4) NOT NULL default '0',
  multiplex_factor smallint(5) default NULL,
  uselatestwadata tinyint(4) NOT NULL default '0',
  usewatunnels tinyint(4) NOT NULL default '1',
  wa_delay_solverweight float default '0',
  wa_bw_solverweight float default '0',
  wa_plr_solverweight float default '0',
  swap_requests tinyint(4) NOT NULL default '0',
  last_swap_req datetime default NULL,
  idle_ignore tinyint(4) NOT NULL default '0',
  sync_server varchar(32) default NULL,
  cpu_usage tinyint(4) unsigned NOT NULL default '0',
  mem_usage tinyint(4) unsigned NOT NULL default '0',
  keyhash varchar(64) default NULL,
  eventkey varchar(64) default NULL,
  idx int(10) unsigned NOT NULL auto_increment,
  sim_reswap_count smallint(5) unsigned NOT NULL default '0',
  veth_encapsulate tinyint(4) NOT NULL default '1',
  encap_style enum('alias','veth','veth-ne','vlan','default') NOT NULL default 'default',
  allowfixnode tinyint(4) NOT NULL default '1',
  jail_osname varchar(20) default NULL,
  delay_osname varchar(20) default NULL,
  use_ipassign tinyint(4) NOT NULL default '0',
  ipassign_args varchar(255) default NULL,
  linktest_level tinyint(4) NOT NULL default '0',
  linktest_pid int(11) default '0',
  useprepass tinyint(1) NOT NULL default '0',
  usemodelnet tinyint(1) NOT NULL default '0',
  modelnet_cores tinyint(4) unsigned NOT NULL default '0',
  modelnet_edges tinyint(4) unsigned NOT NULL default '0',
  modelnetcore_osname varchar(20) default NULL,
  modelnetedge_osname varchar(20) default NULL,
  elab_in_elab tinyint(1) NOT NULL default '0',
  elabinelab_eid varchar(32) default NULL,
  elabinelab_cvstag varchar(64) default NULL,
  elabinelab_nosetup tinyint(1) NOT NULL default '0',
  security_level tinyint(1) NOT NULL default '0',
  lockdown tinyint(1) NOT NULL default '0',
  paniced tinyint(1) NOT NULL default '0',
  panic_date datetime default NULL,
  delay_capacity tinyint(3) unsigned default NULL,
  savedisk tinyint(1) NOT NULL default '0',
  locpiper_pid int(11) default '0',
  locpiper_port int(11) default '0',
  instance_idx int(10) unsigned NOT NULL default '0',
  dpdb tinyint(1) NOT NULL default '0',
  dpdbname varchar(64) default NULL,
  PRIMARY KEY  (eid,pid),
  KEY idx (idx),
  KEY batchmode (batchmode),
  KEY state (state)
) TYPE=MyISAM;

--
-- Table structure for table `exported_tables`
--

CREATE TABLE exported_tables (
  table_name varchar(64) NOT NULL default '',
  PRIMARY KEY  (table_name)
) TYPE=MyISAM;

--
-- Table structure for table `exppid_access`
--

CREATE TABLE exppid_access (
  exp_eid varchar(32) NOT NULL default '',
  exp_pid varchar(12) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  PRIMARY KEY  (exp_eid,exp_pid,pid)
) TYPE=MyISAM;

--
-- Table structure for table `firewall_rules`
--

CREATE TABLE firewall_rules (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  fwname varchar(32) NOT NULL default '',
  ruleno int(10) unsigned NOT NULL default '0',
  rule text NOT NULL,
  PRIMARY KEY  (pid,eid,fwname,ruleno),
  KEY fwname (fwname)
) TYPE=MyISAM;

--
-- Table structure for table `firewalls`
--

CREATE TABLE firewalls (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  fwname varchar(32) NOT NULL default '',
  vlan int(11) default NULL,
  vlanid int(11) default NULL,
  PRIMARY KEY  (pid,eid,fwname),
  KEY vlan (vlan)
) TYPE=MyISAM;

--
-- Table structure for table `floorimages`
--

CREATE TABLE floorimages (
  building varchar(32) NOT NULL default '',
  floor varchar(32) NOT NULL default '',
  image_path tinytext,
  thumb_path tinytext,
  x1 int(6) NOT NULL default '0',
  y1 int(6) NOT NULL default '0',
  x2 int(6) NOT NULL default '0',
  y2 int(6) NOT NULL default '0',
  scale tinyint(4) NOT NULL default '1',
  pixels_per_meter float(10,3) NOT NULL default '0.000',
  PRIMARY KEY  (building,floor,scale)
) TYPE=MyISAM;

--
-- Table structure for table `foreign_keys`
--

CREATE TABLE foreign_keys (
  table1 varchar(30) NOT NULL default '',
  column1 varchar(30) NOT NULL default '',
  table2 varchar(30) NOT NULL default '',
  column2 varchar(30) NOT NULL default '',
  PRIMARY KEY  (table1,column1)
) TYPE=MyISAM;

--
-- Table structure for table `fs_resources`
--

CREATE TABLE fs_resources (
  rsrcidx int(10) unsigned NOT NULL default '0',
  fileidx int(11) unsigned NOT NULL default '0',
  exptidx int(10) unsigned NOT NULL default '0',
  type enum('r','w','rw','l') default 'r',
  size int(11) unsigned default '0',
  PRIMARY KEY  (rsrcidx,fileidx),
  KEY rsrcidx (rsrcidx),
  KEY fileidx (fileidx)
) TYPE=MyISAM;

--
-- Table structure for table `global_policies`
--

CREATE TABLE global_policies (
  policy varchar(32) NOT NULL default '',
  auxdata varchar(128) NOT NULL default '',
  test varchar(32) NOT NULL default '',
  count int(10) NOT NULL default '0',
  PRIMARY KEY  (policy,auxdata)
) TYPE=MyISAM;

--
-- Table structure for table `global_vtypes`
--

CREATE TABLE global_vtypes (
  vtype varchar(30) NOT NULL default '',
  weight float NOT NULL default '0.5',
  types text NOT NULL,
  PRIMARY KEY  (vtype)
) TYPE=MyISAM;

--
-- Table structure for table `group_membership`
--

CREATE TABLE group_membership (
  uid varchar(8) NOT NULL default '',
  gid varchar(16) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  trust enum('none','user','local_root','group_root','project_root') default NULL,
  date_applied date default NULL,
  date_approved datetime default NULL,
  PRIMARY KEY  (uid,gid,pid),
  KEY pid (pid),
  KEY gid (gid)
) TYPE=MyISAM;

--
-- Table structure for table `group_policies`
--

CREATE TABLE group_policies (
  pid varchar(12) NOT NULL default '',
  gid varchar(12) NOT NULL default '',
  policy varchar(32) NOT NULL default '',
  auxdata varchar(64) NOT NULL default '',
  count int(10) NOT NULL default '0',
  PRIMARY KEY  (pid,gid,policy,auxdata)
) TYPE=MyISAM;

--
-- Table structure for table `group_stats`
--

CREATE TABLE group_stats (
  pid varchar(12) NOT NULL default '',
  gid varchar(12) NOT NULL default '',
  exptstart_count int(11) unsigned default '0',
  exptstart_last datetime default NULL,
  exptpreload_count int(11) unsigned default '0',
  exptpreload_last datetime default NULL,
  exptswapin_count int(11) unsigned default '0',
  exptswapin_last datetime default NULL,
  exptswapout_count int(11) unsigned default '0',
  exptswapout_last datetime default NULL,
  exptswapmod_count int(11) unsigned default '0',
  exptswapmod_last datetime default NULL,
  last_activity datetime default NULL,
  allexpt_duration int(11) unsigned default '0',
  allexpt_vnodes int(11) unsigned default '0',
  allexpt_vnode_duration int(11) unsigned default '0',
  allexpt_pnodes int(11) unsigned default '0',
  allexpt_pnode_duration int(11) unsigned default '0',
  PRIMARY KEY  (pid,gid)
) TYPE=MyISAM;

--
-- Table structure for table `groups`
--

CREATE TABLE groups (
  pid varchar(12) NOT NULL default '',
  gid varchar(12) NOT NULL default '',
  leader varchar(8) NOT NULL default '',
  created datetime default NULL,
  description tinytext,
  unix_gid smallint(5) unsigned NOT NULL auto_increment,
  unix_name varchar(16) NOT NULL default '',
  expt_count mediumint(8) unsigned default '0',
  expt_last date default NULL,
  wikiname tinytext,
  mailman_password tinytext,
  PRIMARY KEY  (pid,gid),
  KEY unix_gid (unix_gid),
  KEY gid (gid),
  KEY pid (pid)
) TYPE=MyISAM;

--
-- Table structure for table `iface_counters`
--

CREATE TABLE iface_counters (
  node_id varchar(32) NOT NULL default '',
  tstamp datetime NOT NULL default '0000-00-00 00:00:00',
  mac varchar(12) NOT NULL default '0',
  ipkts int(11) NOT NULL default '0',
  opkts int(11) NOT NULL default '0',
  PRIMARY KEY  (node_id,tstamp,mac),
  KEY macindex (mac),
  KEY node_idindex (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `images`
--

CREATE TABLE images (
  imagename varchar(30) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  gid varchar(12) NOT NULL default '',
  imageid varchar(45) NOT NULL default '',
  creator varchar(8) default NULL,
  created datetime default NULL,
  description tinytext NOT NULL,
  loadpart tinyint(4) NOT NULL default '0',
  loadlength tinyint(4) NOT NULL default '0',
  part1_osid varchar(35) default NULL,
  part2_osid varchar(35) default NULL,
  part3_osid varchar(35) default NULL,
  part4_osid varchar(35) default NULL,
  default_osid varchar(35) NOT NULL default '',
  path tinytext,
  magic tinytext,
  load_address text,
  frisbee_pid int(11) default '0',
  load_busy tinyint(4) NOT NULL default '0',
  ezid tinyint(4) NOT NULL default '0',
  shared tinyint(4) NOT NULL default '0',
  global tinyint(4) NOT NULL default '0',
  updated datetime default NULL,
  PRIMARY KEY  (imagename,pid),
  KEY imageid (imageid),
  KEY gid (gid)
) TYPE=MyISAM;

--
-- Table structure for table `interface_capabilities`
--

CREATE TABLE interface_capabilities (
  type varchar(30) NOT NULL default '',
  capkey varchar(64) NOT NULL default '',
  capval varchar(64) NOT NULL default '',
  PRIMARY KEY  (type,capkey)
) TYPE=MyISAM;

--
-- Table structure for table `interface_settings`
--

CREATE TABLE interface_settings (
  node_id varchar(32) NOT NULL default '',
  iface varchar(32) NOT NULL default '',
  capkey varchar(32) NOT NULL default '',
  capval varchar(64) NOT NULL default '',
  PRIMARY KEY  (node_id,iface,capkey),
  KEY node_id (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `interface_types`
--

CREATE TABLE interface_types (
  type varchar(30) NOT NULL default '',
  max_speed int(11) default NULL,
  full_duplex tinyint(1) default NULL,
  manufacturuer varchar(30) default NULL,
  model varchar(30) default NULL,
  ports tinyint(4) default NULL,
  connector varchar(30) default NULL,
  PRIMARY KEY  (type)
) TYPE=MyISAM;

--
-- Table structure for table `interfaces`
--

CREATE TABLE interfaces (
  node_id varchar(32) NOT NULL default '',
  card tinyint(3) unsigned NOT NULL default '0',
  port tinyint(3) unsigned NOT NULL default '0',
  mac varchar(12) NOT NULL default '000000000000',
  IP varchar(15) default NULL,
  IPaliases text,
  mask varchar(15) default NULL,
  interface_type varchar(30) default NULL,
  iface text NOT NULL,
  role enum('ctrl','expt','jail','fake','other','gw','outer_ctrl') default NULL,
  current_speed enum('0','10','100','1000') NOT NULL default '0',
  duplex enum('full','half') NOT NULL default 'full',
  rtabid smallint(5) unsigned NOT NULL default '0',
  vnode_id varchar(32) default NULL,
  whol tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (node_id,card,port),
  KEY mac (mac),
  KEY IP (IP)
) TYPE=MyISAM;

--
-- Table structure for table `ipport_ranges`
--

CREATE TABLE ipport_ranges (
  eid varchar(32) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  low int(11) NOT NULL default '0',
  high int(11) NOT NULL default '0',
  PRIMARY KEY  (eid,pid)
) TYPE=MyISAM;

--
-- Table structure for table `ipsubnets`
--

CREATE TABLE ipsubnets (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  idx smallint(5) unsigned NOT NULL auto_increment,
  PRIMARY KEY  (idx)
) TYPE=MyISAM;

--
-- Table structure for table `knowledge_base_entries`
--

CREATE TABLE knowledge_base_entries (
  idx int(11) NOT NULL auto_increment,
  creator_uid varchar(8) NOT NULL default '',
  date_created datetime default NULL,
  section tinytext,
  title tinytext,
  body text,
  xref_tag varchar(64) default NULL,
  faq_entry tinyint(1) NOT NULL default '0',
  date_modified datetime default NULL,
  modifier_uid varchar(8) default NULL,
  archived tinyint(1) NOT NULL default '0',
  date_archived datetime default NULL,
  archiver_uid varchar(8) default NULL,
  PRIMARY KEY  (idx)
) TYPE=MyISAM;

--
-- Table structure for table `last_reservation`
--

CREATE TABLE last_reservation (
  node_id varchar(32) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  PRIMARY KEY  (node_id,pid)
) TYPE=MyISAM;

--
-- Table structure for table `linkdelays`
--

CREATE TABLE linkdelays (
  node_id varchar(32) NOT NULL default '',
  iface varchar(8) NOT NULL default '',
  ip varchar(15) NOT NULL default '',
  netmask varchar(15) NOT NULL default '255.255.255.0',
  type enum('simplex','duplex') NOT NULL default 'duplex',
  eid varchar(32) default NULL,
  pid varchar(32) default NULL,
  vlan varchar(32) NOT NULL default '',
  vnode varchar(32) NOT NULL default '',
  pipe smallint(5) unsigned NOT NULL default '0',
  delay float(10,2) NOT NULL default '0.00',
  bandwidth int(10) unsigned NOT NULL default '100',
  lossrate float(10,3) NOT NULL default '0.000',
  rpipe smallint(5) unsigned NOT NULL default '0',
  rdelay float(10,2) NOT NULL default '0.00',
  rbandwidth int(10) unsigned NOT NULL default '100',
  rlossrate float(10,3) NOT NULL default '0.000',
  q_limit int(11) default '0',
  q_maxthresh int(11) default '0',
  q_minthresh int(11) default '0',
  q_weight float default '0',
  q_linterm int(11) default '0',
  q_qinbytes tinyint(4) default '0',
  q_bytes tinyint(4) default '0',
  q_meanpsize int(11) default '0',
  q_wait int(11) default '0',
  q_setbit int(11) default '0',
  q_droptail int(11) default '0',
  q_red tinyint(4) default '0',
  q_gentle tinyint(4) default '0',
  PRIMARY KEY  (node_id,vlan,vnode),
  KEY id (pid,eid)
) TYPE=MyISAM;

--
-- Table structure for table `location_info`
--

CREATE TABLE location_info (
  node_id varchar(32) NOT NULL default '',
  floor varchar(32) NOT NULL default '',
  building varchar(32) NOT NULL default '',
  loc_x int(10) unsigned NOT NULL default '0',
  loc_y int(10) unsigned NOT NULL default '0',
  loc_z float default NULL,
  orientation float default NULL,
  contact tinytext,
  email tinytext,
  phone tinytext,
  room varchar(32) default NULL,
  stamp int(10) unsigned default NULL,
  PRIMARY KEY  (node_id,building,floor)
) TYPE=MyISAM;

--
-- Table structure for table `log`
--

CREATE TABLE log (
  seq int(10) unsigned NOT NULL default '0',
  stamp int(10) unsigned NOT NULL default '0',
  session int(10) unsigned NOT NULL default '0',
  attempt tinyint(1) NOT NULL default '0',
  cleanup tinyint(1) NOT NULL default '0',
  invocation int(10) unsigned NOT NULL default '0',
  parent int(10) unsigned NOT NULL default '0',
  script smallint(3) NOT NULL default '0',
  level tinyint(2) NOT NULL default '0',
  sublevel tinyint(2) NOT NULL default '0',
  priority smallint(3) NOT NULL default '0',
  inferred tinyint(1) NOT NULL default '0',
  cause varchar(16) NOT NULL default '',
  type enum('normal','entering','exiting','thecause','extra','summary','primary','secondary') default 'normal',
  relevant tinyint(1) NOT NULL default '0',
  mesg text NOT NULL,
  PRIMARY KEY  (seq),
  KEY session (session),
  KEY stamp (stamp)
) TYPE=MyISAM;

--
-- Table structure for table `login`
--

CREATE TABLE login (
  uid varchar(10) NOT NULL default '',
  hashkey varchar(64) NOT NULL default '',
  hashhash varchar(64) NOT NULL default '',
  timeout varchar(10) NOT NULL default '',
  adminon tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (uid,hashkey),
  UNIQUE KEY hashhash (uid,hashhash)
) TYPE=MyISAM;

--
-- Table structure for table `login_failures`
--

CREATE TABLE login_failures (
  IP varchar(15) NOT NULL default '1.1.1.1',
  frozen tinyint(3) unsigned NOT NULL default '0',
  failcount smallint(5) unsigned NOT NULL default '0',
  failstamp int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (IP)
) TYPE=MyISAM;

--
-- Table structure for table `loginmessage`
--

CREATE TABLE loginmessage (
  valid tinyint(4) NOT NULL default '1',
  message tinytext NOT NULL,
  PRIMARY KEY  (valid)
) TYPE=MyISAM;

--
-- Table structure for table `mailman_listnames`
--

CREATE TABLE mailman_listnames (
  listname varchar(64) NOT NULL default '',
  owner_uid varchar(8) NOT NULL default '',
  created datetime default NULL,
  PRIMARY KEY  (listname)
) TYPE=MyISAM;

--
-- Table structure for table `mode_transitions`
--

CREATE TABLE mode_transitions (
  op_mode1 varchar(20) NOT NULL default '',
  state1 varchar(20) NOT NULL default '',
  op_mode2 varchar(20) NOT NULL default '',
  state2 varchar(20) NOT NULL default '',
  label varchar(255) NOT NULL default '',
  PRIMARY KEY  (op_mode1,state1,op_mode2,state2),
  KEY op_mode1 (op_mode1,state1),
  KEY op_mode2 (op_mode2,state2)
) TYPE=MyISAM;

--
-- Table structure for table `motelogfiles`
--

CREATE TABLE motelogfiles (
  logfileid varchar(45) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  gid varchar(12) default NULL,
  creator varchar(8) NOT NULL default '',
  created datetime NOT NULL default '0000-00-00 00:00:00',
  updated datetime default NULL,
  description tinytext NOT NULL,
  classfilepath tinytext NOT NULL,
  specfilepath tinytext,
  mote_type varchar(30) default NULL,
  PRIMARY KEY  (logfileid,pid)
) TYPE=MyISAM;

--
-- Table structure for table `new_interfaces`
--

CREATE TABLE new_interfaces (
  new_node_id int(11) NOT NULL default '0',
  card int(11) NOT NULL default '0',
  mac varchar(12) NOT NULL default '',
  interface_type varchar(15) default NULL,
  switch_id varchar(32) default NULL,
  switch_card tinyint(3) default NULL,
  switch_port tinyint(3) default NULL,
  cable smallint(6) default NULL,
  len tinyint(4) default NULL,
  role tinytext,
  PRIMARY KEY  (new_node_id,card)
) TYPE=MyISAM;

--
-- Table structure for table `new_nodes`
--

CREATE TABLE new_nodes (
  new_node_id int(11) NOT NULL auto_increment,
  node_id varchar(32) NOT NULL default '',
  type varchar(30) default NULL,
  IP varchar(15) default NULL,
  temporary_IP varchar(15) default NULL,
  dmesg text,
  created timestamp(14) NOT NULL,
  identifier varchar(255) default NULL,
  floor varchar(32) default NULL,
  building varchar(32) default NULL,
  loc_x int(10) unsigned NOT NULL default '0',
  loc_y int(10) unsigned NOT NULL default '0',
  contact tinytext,
  phone tinytext,
  room varchar(32) default NULL,
  role varchar(32) NOT NULL default 'testnode',
  PRIMARY KEY  (new_node_id)
) TYPE=MyISAM;

--
-- Table structure for table `newdelays`
--

CREATE TABLE newdelays (
  node_id varchar(32) NOT NULL default '',
  pipe0 smallint(5) unsigned NOT NULL default '0',
  delay0 int(10) unsigned NOT NULL default '0',
  bandwidth0 int(10) unsigned NOT NULL default '100',
  lossrate0 float(10,3) NOT NULL default '0.000',
  pipe1 smallint(5) unsigned NOT NULL default '0',
  delay1 int(10) unsigned NOT NULL default '0',
  bandwidth1 int(10) unsigned NOT NULL default '100',
  lossrate1 float(10,3) NOT NULL default '0.000',
  iface0 varchar(8) NOT NULL default '',
  iface1 varchar(8) NOT NULL default '',
  eid varchar(32) default NULL,
  pid varchar(32) default NULL,
  vname varchar(32) default NULL,
  card0 tinyint(3) unsigned default NULL,
  card1 tinyint(3) unsigned default NULL,
  PRIMARY KEY  (node_id,iface0,iface1)
) TYPE=MyISAM;

--
-- Table structure for table `next_reserve`
--

CREATE TABLE next_reserve (
  node_id varchar(32) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `nextfreenode`
--

CREATE TABLE nextfreenode (
  nodetype varchar(30) NOT NULL default '',
  nextid int(10) unsigned NOT NULL default '1',
  nextpri int(10) unsigned NOT NULL default '1',
  PRIMARY KEY  (nodetype)
) TYPE=MyISAM;

--
-- Table structure for table `node_activity`
--

CREATE TABLE node_activity (
  node_id varchar(32) NOT NULL default '',
  last_tty_act datetime NOT NULL default '0000-00-00 00:00:00',
  last_net_act datetime NOT NULL default '0000-00-00 00:00:00',
  last_cpu_act datetime NOT NULL default '0000-00-00 00:00:00',
  last_ext_act datetime NOT NULL default '0000-00-00 00:00:00',
  last_report datetime NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `node_attributes`
--

CREATE TABLE node_attributes (
  node_id varchar(32) NOT NULL default '',
  attrkey varchar(32) NOT NULL default '',
  attrvalue tinytext NOT NULL,
  PRIMARY KEY  (node_id,attrkey),
  KEY node_id (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `node_auxtypes`
--

CREATE TABLE node_auxtypes (
  node_id varchar(32) NOT NULL default '',
  type varchar(30) NOT NULL default '',
  count int(11) default '1',
  PRIMARY KEY  (node_id,type)
) TYPE=MyISAM;

--
-- Table structure for table `node_bootlogs`
--

CREATE TABLE node_bootlogs (
  node_id varchar(32) NOT NULL default '',
  bootlog text,
  bootlog_timestamp datetime default NULL,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `node_features`
--

CREATE TABLE node_features (
  node_id varchar(32) NOT NULL default '',
  feature varchar(30) NOT NULL default '',
  weight float NOT NULL default '0',
  PRIMARY KEY  (node_id,feature)
) TYPE=MyISAM;

--
-- Table structure for table `node_history`
--

CREATE TABLE node_history (
  history_id int(10) unsigned NOT NULL auto_increment,
  node_id varchar(32) NOT NULL default '',
  op enum('alloc','free','move') NOT NULL default 'alloc',
  uid varchar(8) NOT NULL default '',
  exptidx int(10) unsigned default NULL,
  stamp int(10) unsigned default NULL,
  PRIMARY KEY  (history_id),
  KEY node_id (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `node_hostkeys`
--

CREATE TABLE node_hostkeys (
  node_id varchar(32) NOT NULL default '',
  sshrsa_v1 mediumtext,
  sshrsa_v2 mediumtext,
  sshdsa_v2 mediumtext,
  sfshostid varchar(128) default NULL,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `node_idlestats`
--

CREATE TABLE node_idlestats (
  node_id varchar(32) NOT NULL default '',
  tstamp datetime NOT NULL default '0000-00-00 00:00:00',
  last_tty datetime NOT NULL default '0000-00-00 00:00:00',
  load_1min float NOT NULL default '0',
  load_5min float NOT NULL default '0',
  load_15min float NOT NULL default '0',
  PRIMARY KEY  (node_id,tstamp)
) TYPE=MyISAM;

--
-- Table structure for table `node_rusage`
--

CREATE TABLE node_rusage (
  node_id varchar(32) NOT NULL default '',
  load_1min float NOT NULL default '0',
  load_5min float NOT NULL default '0',
  load_15min float NOT NULL default '0',
  disk_used float NOT NULL default '0',
  status_timestamp datetime default NULL,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `node_startloc`
--

CREATE TABLE node_startloc (
  node_id varchar(32) NOT NULL default '',
  building varchar(32) NOT NULL default '',
  floor varchar(32) NOT NULL default '',
  loc_x float NOT NULL default '0',
  loc_y float NOT NULL default '0',
  orientation float NOT NULL default '0',
  PRIMARY KEY  (node_id,building,floor)
) TYPE=MyISAM;

--
-- Table structure for table `node_status`
--

CREATE TABLE node_status (
  node_id varchar(32) NOT NULL default '',
  status enum('up','possibly down','down','unpingable') default NULL,
  status_timestamp datetime default NULL,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `node_type_attributes`
--

CREATE TABLE node_type_attributes (
  type varchar(30) NOT NULL default '',
  attrkey varchar(32) NOT NULL default '',
  attrvalue tinytext NOT NULL,
  attrtype enum('integer','float','boolean','string') default 'string',
  PRIMARY KEY  (type,attrkey),
  KEY node_id (type)
) TYPE=MyISAM;

--
-- Table structure for table `node_type_features`
--

CREATE TABLE node_type_features (
  type varchar(30) NOT NULL default '',
  feature varchar(30) NOT NULL default '',
  weight float NOT NULL default '0',
  PRIMARY KEY  (type,feature)
) TYPE=MyISAM;

--
-- Table structure for table `node_types`
--

CREATE TABLE node_types (
  class varchar(30) default NULL,
  type varchar(30) NOT NULL default '',
  proc varchar(30) default NULL,
  speed smallint(5) unsigned default NULL,
  RAM smallint(5) unsigned default NULL,
  HD float(10,2) default NULL,
  max_interfaces tinyint(3) unsigned default '0',
  osid varchar(35) NOT NULL default '',
  control_net tinyint(3) unsigned default NULL,
  power_time smallint(5) unsigned NOT NULL default '60',
  imageid varchar(45) NOT NULL default '',
  imageable tinyint(4) default '0',
  delay_capacity tinyint(4) NOT NULL default '0',
  virtnode_capacity tinyint(4) NOT NULL default '0',
  control_iface text,
  disktype enum('ad','da','ar','amrd') default NULL,
  bootdisk_unit tinyint(3) unsigned NOT NULL default '0',
  delay_osid varchar(35) default NULL,
  jail_osid varchar(35) default NULL,
  modelnetcore_osid varchar(35) default NULL,
  modelnetedge_osid varchar(35) default NULL,
  pxe_boot_path text,
  isvirtnode tinyint(4) NOT NULL default '0',
  ismodelnet tinyint(1) NOT NULL default '0',
  isjailed tinyint(1) NOT NULL default '0',
  isdynamic tinyint(1) NOT NULL default '0',
  isremotenode tinyint(4) NOT NULL default '0',
  issubnode tinyint(4) NOT NULL default '0',
  isplabdslice tinyint(4) NOT NULL default '0',
  isplabphysnode tinyint(4) NOT NULL default '0',
  issimnode tinyint(4) NOT NULL default '0',
  simnode_capacity smallint(5) unsigned NOT NULL default '0',
  trivlink_maxspeed int(11) unsigned NOT NULL default '0',
  isrebootable tinyint(1) default '1',
  bios_waittime int(10) unsigned default NULL,
  adminmfs_osid varchar(35) default 'FREEBSD-MFS',
  diskloadmfs_osid varchar(35) default 'FRISBEE-MFS',
  PRIMARY KEY  (type)
) TYPE=MyISAM;

--
-- Table structure for table `node_types_auxtypes`
--

CREATE TABLE node_types_auxtypes (
  auxtype varchar(30) NOT NULL default '',
  type varchar(30) NOT NULL default '',
  PRIMARY KEY  (auxtype)
) TYPE=MyISAM;

--
-- Table structure for table `nodeipportnum`
--

CREATE TABLE nodeipportnum (
  node_id varchar(32) NOT NULL default '',
  port smallint(5) unsigned NOT NULL default '11000',
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `nodelog`
--

CREATE TABLE nodelog (
  node_id varchar(32) NOT NULL default '',
  log_id smallint(5) unsigned NOT NULL auto_increment,
  type enum('misc') NOT NULL default 'misc',
  reporting_uid varchar(8) NOT NULL default '',
  entry tinytext NOT NULL,
  reported datetime default NULL,
  PRIMARY KEY  (node_id,log_id)
) TYPE=MyISAM;

--
-- Table structure for table `nodes`
--

CREATE TABLE nodes (
  node_id varchar(32) NOT NULL default '',
  type varchar(30) NOT NULL default '',
  phys_nodeid varchar(32) default NULL,
  role enum('testnode','virtnode','ctrlnode','testswitch','ctrlswitch','powerctrl','unused') NOT NULL default 'unused',
  def_boot_osid varchar(35) NOT NULL default '',
  def_boot_path text,
  def_boot_cmd_line text,
  temp_boot_osid varchar(35) NOT NULL default '',
  next_boot_osid varchar(35) NOT NULL default '',
  next_boot_path text,
  next_boot_cmd_line text,
  pxe_boot_path text,
  rpms text,
  deltas text,
  tarballs text,
  startupcmd tinytext,
  startstatus tinytext,
  ready tinyint(4) unsigned NOT NULL default '0',
  priority int(11) NOT NULL default '-1',
  bootstatus enum('okay','failed','unknown') default 'unknown',
  status enum('up','possibly down','down','unpingable') default NULL,
  status_timestamp datetime default NULL,
  failureaction enum('fatal','nonfatal','ignore') NOT NULL default 'fatal',
  routertype enum('none','ospf','static','manual','static-ddijk','static-old') NOT NULL default 'none',
  eventstate varchar(20) default NULL,
  state_timestamp int(10) unsigned default NULL,
  op_mode varchar(20) default NULL,
  op_mode_timestamp int(10) unsigned default NULL,
  allocstate varchar(20) default NULL,
  allocstate_timestamp int(10) unsigned default NULL,
  update_accounts smallint(6) default '0',
  next_op_mode varchar(20) NOT NULL default '',
  ipodhash varchar(64) default NULL,
  osid varchar(35) NOT NULL default '',
  ntpdrift float default NULL,
  ipport_low int(11) NOT NULL default '11000',
  ipport_next int(11) NOT NULL default '11000',
  ipport_high int(11) NOT NULL default '20000',
  sshdport int(11) NOT NULL default '11000',
  jailflag tinyint(3) unsigned NOT NULL default '0',
  jailip varchar(15) default NULL,
  sfshostid varchar(128) default NULL,
  stated_tag varchar(32) default NULL,
  rtabid smallint(5) unsigned NOT NULL default '0',
  cd_version varchar(32) default NULL,
  battery_voltage float default NULL,
  battery_percentage float default NULL,
  battery_timestamp int(10) unsigned default NULL,
  boot_errno int(11) NOT NULL default '0',
  destination_x float default NULL,
  destination_y float default NULL,
  destination_orientation float default NULL,
  reserved_pid varchar(12) default NULL,
  PRIMARY KEY  (node_id),
  KEY phys_nodeid (phys_nodeid),
  KEY node_id (node_id,phys_nodeid),
  KEY role (role)
) TYPE=MyISAM;

--
-- Table structure for table `nodetypeXpid_permissions`
--

CREATE TABLE nodetypeXpid_permissions (
  type varchar(30) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  PRIMARY KEY  (type,pid),
  KEY pid (pid)
) TYPE=MyISAM;

--
-- Table structure for table `nodeuidlastlogin`
--

CREATE TABLE nodeuidlastlogin (
  node_id varchar(32) NOT NULL default '',
  uid varchar(10) NOT NULL default '',
  date date default NULL,
  time time default NULL,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `nologins`
--

CREATE TABLE nologins (
  nologins tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (nologins)
) TYPE=MyISAM;

--
-- Table structure for table `nseconfigs`
--

CREATE TABLE nseconfigs (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  nseconfig mediumtext,
  PRIMARY KEY  (pid,eid,vname)
) TYPE=MyISAM;

--
-- Table structure for table `nsfiles`
--

CREATE TABLE nsfiles (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  nsfile mediumtext,
  PRIMARY KEY  (eid,pid)
) TYPE=MyISAM;

--
-- Table structure for table `ntpinfo`
--

CREATE TABLE ntpinfo (
  node_id varchar(32) NOT NULL default '',
  IP varchar(64) NOT NULL default '',
  type enum('server','peer') NOT NULL default 'peer',
  PRIMARY KEY  (node_id,IP,type)
) TYPE=MyISAM;

--
-- Table structure for table `obstacles`
--

CREATE TABLE obstacles (
  obstacle_id int(11) unsigned NOT NULL auto_increment,
  floor varchar(32) default NULL,
  building varchar(32) default NULL,
  x1 int(10) unsigned NOT NULL default '0',
  y1 int(10) unsigned NOT NULL default '0',
  z1 int(10) unsigned NOT NULL default '0',
  x2 int(10) unsigned NOT NULL default '0',
  y2 int(10) unsigned NOT NULL default '0',
  z2 int(10) unsigned NOT NULL default '0',
  description tinytext,
  label tinytext,
  draw tinyint(1) NOT NULL default '0',
  no_exclusion tinyint(1) NOT NULL default '0',
  no_tooltip tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (obstacle_id)
) TYPE=MyISAM;

--
-- Table structure for table `os_boot_cmd`
--

CREATE TABLE os_boot_cmd (
  OS enum('Unknown','Linux','Fedora','FreeBSD','NetBSD','OSKit','Windows','TinyOS','Other') NOT NULL default 'Unknown',
  version varchar(12) NOT NULL default '',
  role enum('default','delay','linkdelay','vnodehost') NOT NULL default 'default',
  boot_cmd_line text,
  PRIMARY KEY  (OS,version,role)
) TYPE=MyISAM;

--
-- Table structure for table `os_info`
--

CREATE TABLE os_info (
  osname varchar(20) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  osid varchar(35) NOT NULL default '',
  creator varchar(8) default NULL,
  created datetime default NULL,
  description tinytext NOT NULL,
  OS enum('Unknown','Linux','Fedora','FreeBSD','NetBSD','OSKit','Windows','TinyOS','Other') default 'Unknown',
  version varchar(12) default '',
  path tinytext,
  magic tinytext,
  machinetype varchar(30) NOT NULL default '',
  osfeatures set('ping','ssh','ipod','isup','veths','mlinks','linktest','linkdelays') default NULL,
  ezid tinyint(4) NOT NULL default '0',
  shared tinyint(4) NOT NULL default '0',
  mustclean tinyint(4) NOT NULL default '1',
  op_mode varchar(20) NOT NULL default 'MINIMAL',
  nextosid varchar(35) default NULL,
  max_concurrent int(11) default NULL,
  mfs tinyint(4) NOT NULL default '0',
  reboot_waittime int(10) unsigned default NULL,
  PRIMARY KEY  (osname,pid),
  KEY osid (osid),
  KEY OS (OS),
  KEY path (path(255))
) TYPE=MyISAM;

--
-- Table structure for table `osid_map`
--

CREATE TABLE osid_map (
  osid varchar(35) NOT NULL default '',
  btime datetime NOT NULL default '1000-01-01 00:00:00',
  etime datetime NOT NULL default '9999-12-31 23:59:59',
  nextosid varchar(35) default NULL,
  PRIMARY KEY  (osid,btime,etime)
) TYPE=MyISAM;

--
-- Table structure for table `osidtoimageid`
--

CREATE TABLE osidtoimageid (
  osid varchar(35) NOT NULL default '',
  type varchar(30) NOT NULL default '',
  imageid varchar(45) NOT NULL default '',
  PRIMARY KEY  (osid,type)
) TYPE=MyISAM;

--
-- Table structure for table `outlets`
--

CREATE TABLE outlets (
  node_id varchar(32) NOT NULL default '',
  power_id varchar(32) NOT NULL default '',
  outlet tinyint(1) unsigned NOT NULL default '0',
  last_power timestamp(14) NOT NULL,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `partitions`
--

CREATE TABLE partitions (
  node_id varchar(32) NOT NULL default '',
  partition tinyint(4) NOT NULL default '0',
  osid varchar(35) default NULL,
  imageid varchar(45) default NULL,
  imagepid varchar(12) NOT NULL default '',
  PRIMARY KEY  (node_id,partition),
  KEY osid (osid)
) TYPE=MyISAM;

--
-- Table structure for table `plab_mapping`
--

CREATE TABLE plab_mapping (
  node_id varchar(32) NOT NULL default '',
  plab_id varchar(32) NOT NULL default '',
  hostname varchar(255) NOT NULL default '',
  IP varchar(15) NOT NULL default '',
  mac varchar(17) NOT NULL default '',
  create_time datetime default NULL,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `plab_site_mapping`
--

CREATE TABLE plab_site_mapping (
  site_name varchar(255) NOT NULL default '',
  site_idx smallint(5) unsigned NOT NULL auto_increment,
  node_id varchar(32) NOT NULL default '',
  node_idx tinyint(3) unsigned NOT NULL default '0',
  PRIMARY KEY  (site_name,site_idx,node_idx)
) TYPE=MyISAM;

--
-- Table structure for table `plab_slice_nodes`
--

CREATE TABLE plab_slice_nodes (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  slicename varchar(64) NOT NULL default '',
  node_id varchar(32) NOT NULL default '',
  leaseend datetime default NULL,
  nodemeta text,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `plab_slices`
--

CREATE TABLE plab_slices (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  slicename varchar(64) NOT NULL default '',
  slicemeta text,
  leaseend datetime default NULL,
  admin tinyint(1) default '0',
  PRIMARY KEY  (pid,eid)
) TYPE=MyISAM;

--
-- Table structure for table `port_counters`
--

CREATE TABLE port_counters (
  node_id char(32) NOT NULL default '',
  card tinyint(3) unsigned NOT NULL default '0',
  port tinyint(3) unsigned NOT NULL default '0',
  ifInOctets int(10) unsigned NOT NULL default '0',
  ifInUcastPkts int(10) unsigned NOT NULL default '0',
  ifInNUcastPkts int(10) unsigned NOT NULL default '0',
  ifInDiscards int(10) unsigned NOT NULL default '0',
  ifInErrors int(10) unsigned NOT NULL default '0',
  ifInUnknownProtos int(10) unsigned NOT NULL default '0',
  ifOutOctets int(10) unsigned NOT NULL default '0',
  ifOutUcastPkts int(10) unsigned NOT NULL default '0',
  ifOutNUcastPkts int(10) unsigned NOT NULL default '0',
  ifOutDiscards int(10) unsigned NOT NULL default '0',
  ifOutErrors int(10) unsigned NOT NULL default '0',
  ifOutQLen int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (node_id,card,port)
) TYPE=MyISAM;

--
-- Table structure for table `port_registration`
--

CREATE TABLE port_registration (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  service varchar(64) NOT NULL default '',
  node_id varchar(32) NOT NULL default '',
  port int(11) unsigned NOT NULL default '0',
  PRIMARY KEY  (pid,eid,service)
) TYPE=MyISAM;

--
-- Table structure for table `portmap`
--

CREATE TABLE portmap (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vnode varchar(32) NOT NULL default '',
  vport tinyint(4) NOT NULL default '0',
  pport varchar(32) NOT NULL default ''
) TYPE=MyISAM;

--
-- Table structure for table `priorities`
--

CREATE TABLE priorities (
  priority smallint(3) NOT NULL default '0',
  priority_name varchar(8) NOT NULL default '',
  PRIMARY KEY  (priority),
  UNIQUE KEY name (priority_name)
) TYPE=MyISAM;

--
-- Table structure for table `proj_memb`
--

CREATE TABLE proj_memb (
  uid varchar(8) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  trust enum('none','user','local_root','group_root') default NULL,
  date_applied date default NULL,
  date_approved date default NULL,
  PRIMARY KEY  (uid,pid)
) TYPE=MyISAM;

--
-- Table structure for table `project_stats`
--

CREATE TABLE project_stats (
  pid varchar(12) NOT NULL default '',
  exptstart_count int(11) unsigned default '0',
  exptstart_last datetime default NULL,
  exptpreload_count int(11) unsigned default '0',
  exptpreload_last datetime default NULL,
  exptswapin_count int(11) unsigned default '0',
  exptswapin_last datetime default NULL,
  exptswapout_count int(11) unsigned default '0',
  exptswapout_last datetime default NULL,
  exptswapmod_count int(11) unsigned default '0',
  exptswapmod_last datetime default NULL,
  last_activity datetime default NULL,
  allexpt_duration int(11) unsigned default '0',
  allexpt_vnodes int(11) unsigned default '0',
  allexpt_vnode_duration int(11) unsigned default '0',
  allexpt_pnodes int(11) unsigned default '0',
  allexpt_pnode_duration int(11) unsigned default '0',
  PRIMARY KEY  (pid)
) TYPE=MyISAM;

--
-- Table structure for table `projects`
--

CREATE TABLE projects (
  pid varchar(12) NOT NULL default '',
  created datetime default NULL,
  expires date default NULL,
  name tinytext,
  URL tinytext,
  funders tinytext,
  addr tinytext,
  head_uid varchar(8) NOT NULL default '',
  num_members int(11) default '0',
  num_pcs int(11) default '0',
  num_sharks int(11) default '0',
  num_pcplab int(11) default '0',
  num_ron int(11) default '0',
  why text,
  control_node varchar(10) default NULL,
  unix_gid smallint(5) unsigned NOT NULL auto_increment,
  approved tinyint(4) default '0',
  inactive tinyint(4) default '0',
  date_inactive datetime default NULL,
  public tinyint(4) NOT NULL default '0',
  public_whynot tinytext,
  expt_count mediumint(8) unsigned default '0',
  expt_last date default NULL,
  pcremote_ok set('pcplabphys','pcron','pcwa') default NULL,
  default_user_interface enum('emulab','plab') NOT NULL default 'emulab',
  linked_to_us tinyint(4) NOT NULL default '0',
  cvsrepo_public tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (pid),
  KEY unix_gid (unix_gid),
  KEY approved (approved),
  KEY approved_2 (approved),
  KEY pcremote_ok (pcremote_ok)
) TYPE=MyISAM;

--
-- Table structure for table `report_assign_violation`
--

CREATE TABLE `report_assign_violation` (
  `seq` int(10) unsigned NOT NULL,
  `unassigned` int(11) default NULL,
  `pnode_load` int(11) default NULL,
  `no_connect` int(11) default NULL,
  `link_users` int(11) default NULL,
  `bandwidth` int(11) default NULL,
  `desires` int(11) default NULL,
  `vclass` int(11) default NULL,
  `delay` int(11) default NULL,
  `trivial_mix` int(11) default NULL,
  `subnodes` int(11) default NULL,
  `max_types` int(11) default NULL,
  `endpoints` int(11) default NULL,
  PRIMARY KEY  (`seq`)
) TYPE=MyISAM;

--
-- Table structure for table `report_context`
--

CREATE TABLE `report_context` (
  `seq` int(10) unsigned NOT NULL,
  `i0` int(11) default NULL,
  `i1` int(11) default NULL,
  `i2` int(11) default NULL,
  `vc0` varchar(255) default NULL,
  `vc1` varchar(255) default NULL,
  `vc2` varchar(255) default NULL,
  PRIMARY KEY  (`seq`)
) TYPE=MyISAM;

--
-- Table structure for table `report_error`
--

CREATE TABLE `report_error` (
  `seq` int(10) unsigned NOT NULL,
  `stamp` int(10) unsigned NOT NULL,
  `session` int(10) unsigned NOT NULL,
  `invocation` int(10) unsigned NOT NULL,
  `attempt` tinyint(1) NOT NULL,
  `severity` smallint(3) NOT NULL,
  `script` smallint(3) NOT NULL,
  `error_type` varchar(255) NOT NULL,
  PRIMARY KEY  (`seq`),
  KEY `session` (`session`)
) TYPE=MyISAM;

--
-- Table structure for table `reposition_status`
--

CREATE TABLE reposition_status (
  node_id varchar(32) NOT NULL default '',
  attempts tinyint(4) NOT NULL default '0',
  distance_remaining float default NULL,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `reserved`
--

CREATE TABLE reserved (
  node_id varchar(32) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  rsrv_time timestamp(14) NOT NULL,
  vname varchar(32) default NULL,
  erole enum('node','virthost','delaynode','simhost','modelnet-core','modelnet-edge') NOT NULL default 'node',
  simhost_violation tinyint(3) unsigned NOT NULL default '0',
  old_pid varchar(12) NOT NULL default '',
  old_eid varchar(32) NOT NULL default '',
  cnet_vlan int(11) default NULL,
  inner_elab_role enum('boss','boss+router','router','ops','ops+fs','fs','node') default NULL,
  inner_elab_boot tinyint(1) default '0',
  plab_role enum('plc','node','none') NOT NULL default 'none',
  plab_boot tinyint(1) default '0',
  mustwipe tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (node_id),
  UNIQUE KEY vname (pid,eid,vname),
  KEY old_pid (old_pid,old_eid)
) TYPE=MyISAM;

--
-- Table structure for table `scheduled_reloads`
--

CREATE TABLE scheduled_reloads (
  node_id varchar(32) NOT NULL default '',
  image_id varchar(45) NOT NULL default '',
  reload_type enum('netdisk','frisbee') default NULL,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `scripts`
--

CREATE TABLE scripts (
  script smallint(3) NOT NULL auto_increment,
  script_name varchar(24) NOT NULL default '',
  PRIMARY KEY  (script),
  UNIQUE KEY id (script_name)
) TYPE=MyISAM;

--
-- Table structure for table `session_info`
--

CREATE TABLE session_info (
  session int(11) NOT NULL default '0',
  uid int(11) NOT NULL default '0',
  exptidx int(11) NOT NULL default '0',
  PRIMARY KEY  (session)
) TYPE=MyISAM;

--
-- Table structure for table `sitevariables`
--

CREATE TABLE sitevariables (
  name varchar(255) NOT NULL default '',
  value text,
  defaultvalue text NOT NULL,
  description text,
  PRIMARY KEY  (name)
) TYPE=MyISAM;

--
-- Table structure for table `state_timeouts`
--

CREATE TABLE state_timeouts (
  op_mode varchar(20) NOT NULL default '',
  state varchar(20) NOT NULL default '',
  timeout int(11) NOT NULL default '0',
  action mediumtext NOT NULL,
  PRIMARY KEY  (op_mode,state)
) TYPE=MyISAM;

--
-- Table structure for table `state_transitions`
--

CREATE TABLE state_transitions (
  op_mode varchar(20) NOT NULL default '',
  state1 varchar(20) NOT NULL default '',
  state2 varchar(20) NOT NULL default '',
  label varchar(255) NOT NULL default '',
  PRIMARY KEY  (op_mode,state1,state2)
) TYPE=MyISAM;

--
-- Table structure for table `state_triggers`
--

CREATE TABLE state_triggers (
  node_id varchar(32) NOT NULL default '',
  op_mode varchar(20) NOT NULL default '',
  state varchar(20) NOT NULL default '',
  `trigger` tinytext NOT NULL,
  PRIMARY KEY  (node_id,op_mode,state)
) TYPE=MyISAM;

--
-- Table structure for table `switch_paths`
--

CREATE TABLE switch_paths (
  pid varchar(12) default NULL,
  eid varchar(32) default NULL,
  vname varchar(32) default NULL,
  node_id1 varchar(32) default NULL,
  node_id2 varchar(32) default NULL
) TYPE=MyISAM;

--
-- Table structure for table `switch_stack_types`
--

CREATE TABLE switch_stack_types (
  stack_id varchar(32) NOT NULL default '',
  stack_type varchar(10) default NULL,
  supports_private tinyint(1) NOT NULL default '0',
  single_domain tinyint(1) NOT NULL default '1',
  snmp_community varchar(32) default NULL,
  min_vlan int(11) default NULL,
  max_vlan int(11) default NULL,
  leader varchar(32) default NULL,
  PRIMARY KEY  (stack_id)
) TYPE=MyISAM;

--
-- Table structure for table `switch_stacks`
--

CREATE TABLE switch_stacks (
  node_id varchar(32) NOT NULL default '',
  stack_id varchar(32) NOT NULL default '',
  is_primary tinyint(1) NOT NULL default '1',
  KEY node_id (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `table_regex`
--

CREATE TABLE table_regex (
  table_name varchar(64) NOT NULL default '',
  column_name varchar(64) NOT NULL default '',
  column_type enum('text','int','float') default NULL,
  check_type enum('regex','function','redirect') default NULL,
  `check` tinytext NOT NULL,
  min int(11) NOT NULL default '0',
  max int(11) NOT NULL default '0',
  comment tinytext,
  UNIQUE KEY table_name (table_name,column_name)
) TYPE=MyISAM;

--
-- Table structure for table `testbed_stats`
--

CREATE TABLE testbed_stats (
  idx int(10) unsigned NOT NULL auto_increment,
  start_time datetime default NULL,
  end_time datetime default NULL,
  exptidx int(10) unsigned NOT NULL default '0',
  rsrcidx int(10) unsigned NOT NULL default '0',
  action varchar(16) NOT NULL default '',
  exitcode tinyint(3) default '0',
  uid varchar(8) NOT NULL default '',
  log_session int(10) unsigned default NULL,
  PRIMARY KEY  (idx),
  KEY rsrcidx (rsrcidx),
  KEY exptidx (exptidx)
) TYPE=MyISAM;

--
-- Table structure for table `testsuite_preentables`
--

CREATE TABLE testsuite_preentables (
  table_name varchar(128) NOT NULL default '',
  action enum('drop','clean','prune') default 'drop',
  PRIMARY KEY  (table_name)
) TYPE=MyISAM;

--
-- Table structure for table `tiplines`
--

CREATE TABLE tiplines (
  tipname varchar(32) NOT NULL default '',
  node_id varchar(32) NOT NULL default '',
  server varchar(64) NOT NULL default '',
  portnum int(11) NOT NULL default '0',
  keylen smallint(6) NOT NULL default '0',
  keydata text,
  PRIMARY KEY  (tipname),
  KEY node_id (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `tipservers`
--

CREATE TABLE tipservers (
  server varchar(64) NOT NULL default '',
  PRIMARY KEY  (server)
) TYPE=MyISAM;

--
-- Table structure for table `tmcd_redirect`
--

CREATE TABLE tmcd_redirect (
  node_id varchar(32) NOT NULL default '',
  dbname tinytext NOT NULL,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `traces`
--

CREATE TABLE traces (
  node_id varchar(32) NOT NULL default '',
  idx int(10) unsigned NOT NULL auto_increment,
  iface0 varchar(8) NOT NULL default '',
  iface1 varchar(8) NOT NULL default '',
  pid varchar(32) default NULL,
  eid varchar(32) default NULL,
  linkvname varchar(32) default NULL,
  vnode varchar(32) default NULL,
  trace_type tinytext,
  trace_expr tinytext,
  trace_snaplen int(11) NOT NULL default '0',
  trace_db tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (node_id,idx),
  KEY pid (pid,eid)
) TYPE=MyISAM;

--
-- Table structure for table `tunnels`
--

CREATE TABLE tunnels (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  node_id varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  isserver tinyint(3) unsigned NOT NULL default '0',
  port int(11) NOT NULL default '0',
  peer_ip varchar(32) NOT NULL default '',
  mask varchar(15) default NULL,
  password varchar(32) NOT NULL default '',
  proto varchar(12) NOT NULL default 'udp',
  encrypt tinyint(3) unsigned NOT NULL default '0',
  compress tinyint(3) unsigned NOT NULL default '0',
  assigned_ip varchar(32) NOT NULL default '',
  PRIMARY KEY  (pid,eid,node_id,vname),
  KEY node_id (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `uidnodelastlogin`
--

CREATE TABLE uidnodelastlogin (
  uid varchar(10) NOT NULL default '',
  node_id varchar(32) NOT NULL default '',
  date date default NULL,
  time time default NULL,
  PRIMARY KEY  (uid)
) TYPE=MyISAM;

--
-- Table structure for table `unixgroup_membership`
--

CREATE TABLE unixgroup_membership (
  uid varchar(8) NOT NULL default '',
  gid varchar(16) NOT NULL default '',
  PRIMARY KEY  (uid,gid)
) TYPE=MyISAM;

--
-- Table structure for table `user_policies`
--

CREATE TABLE user_policies (
  uid varchar(8) NOT NULL default '',
  policy varchar(32) NOT NULL default '',
  auxdata varchar(64) NOT NULL default '',
  count int(10) NOT NULL default '0',
  PRIMARY KEY  (uid,policy,auxdata)
) TYPE=MyISAM;

--
-- Table structure for table `user_pubkeys`
--

CREATE TABLE user_pubkeys (
  uid varchar(8) NOT NULL default '',
  idx int(10) unsigned NOT NULL auto_increment,
  pubkey text,
  stamp datetime default NULL,
  comment varchar(128) NOT NULL default '',
  PRIMARY KEY  (uid,idx)
) TYPE=MyISAM;

--
-- Table structure for table `user_sfskeys`
--

CREATE TABLE user_sfskeys (
  uid varchar(8) NOT NULL default '',
  comment varchar(128) NOT NULL default '',
  pubkey text,
  stamp datetime default NULL,
  PRIMARY KEY  (uid,comment)
) TYPE=MyISAM;

--
-- Table structure for table `user_sslcerts`
--

CREATE TABLE user_sslcerts (
  uid varchar(8) NOT NULL default '',
  idx int(10) unsigned NOT NULL default '0',
  cert text,
  privkey text,
  created datetime default NULL,
  encrypted tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (idx)
) TYPE=MyISAM;

--
-- Table structure for table `user_stats`
--

CREATE TABLE user_stats (
  uid varchar(8) NOT NULL default '',
  uid_idx smallint(5) unsigned NOT NULL default '0',
  weblogin_count int(11) unsigned default '0',
  weblogin_last datetime default NULL,
  exptstart_count int(11) unsigned default '0',
  exptstart_last datetime default NULL,
  exptpreload_count int(11) unsigned default '0',
  exptpreload_last datetime default NULL,
  exptswapin_count int(11) unsigned default '0',
  exptswapin_last datetime default NULL,
  exptswapout_count int(11) unsigned default '0',
  exptswapout_last datetime default NULL,
  exptswapmod_count int(11) unsigned default '0',
  exptswapmod_last datetime default NULL,
  last_activity datetime default NULL,
  allexpt_duration int(11) unsigned default '0',
  allexpt_vnodes int(11) unsigned default '0',
  allexpt_vnode_duration int(11) unsigned default '0',
  allexpt_pnodes int(11) unsigned default '0',
  allexpt_pnode_duration int(11) unsigned default '0',
  PRIMARY KEY  (uid_idx)
) TYPE=MyISAM;

--
-- Table structure for table `users`
--

CREATE TABLE users (
  uid varchar(8) NOT NULL default '',
  usr_created datetime default NULL,
  usr_expires datetime default NULL,
  usr_modified datetime default NULL,
  usr_name tinytext,
  usr_title tinytext,
  usr_affil tinytext,
  usr_email tinytext,
  usr_URL tinytext,
  usr_addr tinytext,
  usr_addr2 tinytext,
  usr_city tinytext,
  usr_state tinytext,
  usr_zip tinytext,
  usr_country tinytext,
  usr_phone tinytext,
  usr_shell tinytext,
  usr_pswd tinytext NOT NULL,
  usr_w_pswd tinytext,
  unix_uid smallint(5) unsigned NOT NULL default '0',
  status enum('newuser','unapproved','unverified','active','frozen','other') NOT NULL default 'newuser',
  admin tinyint(4) default '0',
  foreign_admin tinyint(4) default '0',
  dbedit tinyint(4) default '0',
  stud tinyint(4) default '0',
  webonly tinyint(4) default '0',
  pswd_expires date default NULL,
  cvsweb tinyint(4) NOT NULL default '0',
  emulab_pubkey text,
  home_pubkey text,
  adminoff tinyint(4) default '0',
  verify_key varchar(32) default NULL,
  widearearoot tinyint(4) default '0',
  wideareajailroot tinyint(4) default '0',
  notes text,
  weblogin_frozen tinyint(3) unsigned NOT NULL default '0',
  weblogin_failcount smallint(5) unsigned NOT NULL default '0',
  weblogin_failstamp int(10) unsigned NOT NULL default '0',
  plab_user tinyint(1) NOT NULL default '0',
  user_interface enum('emulab','plab') NOT NULL default 'emulab',
  chpasswd_key varchar(32) default NULL,
  chpasswd_expires int(10) unsigned NOT NULL default '0',
  wikiname tinytext,
  wikionly tinyint(1) default '0',
  mailman_password tinytext,
  PRIMARY KEY  (uid),
  KEY unix_uid (unix_uid),
  KEY status (status)
) TYPE=MyISAM;

--
-- Table structure for table `userslastlogin`
--

CREATE TABLE userslastlogin (
  uid varchar(10) NOT NULL default '',
  date date default NULL,
  time time default NULL,
  PRIMARY KEY  (uid)
) TYPE=MyISAM;

--
-- Table structure for table `usrp_orders`
--

CREATE TABLE usrp_orders (
  order_id varchar(32) NOT NULL default '',
  email tinytext,
  name tinytext,
  phone tinytext,
  affiliation tinytext,
  num_mobos int(11) default '0',
  num_dboards int(11) default '0',
  intended_use tinytext,
  comments tinytext,
  order_date datetime default NULL,
  modify_date datetime default NULL,
  PRIMARY KEY  (order_id)
) TYPE=MyISAM;

--
-- Table structure for table `v2pmap`
--

CREATE TABLE v2pmap (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  node_id varchar(32) NOT NULL default '',
  PRIMARY KEY  (pid,eid,vname)
) TYPE=MyISAM;

--
-- Table structure for table `veth_interfaces`
--

CREATE TABLE veth_interfaces (
  node_id varchar(32) NOT NULL default '',
  veth_id int(10) unsigned NOT NULL auto_increment,
  mac varchar(12) NOT NULL default '000000000000',
  IP varchar(15) default NULL,
  mask varchar(15) default NULL,
  iface varchar(10) default NULL,
  vnode_id varchar(32) default NULL,
  rtabid smallint(5) unsigned NOT NULL default '0',
  PRIMARY KEY  (node_id,veth_id),
  KEY IP (IP)
) TYPE=MyISAM;

--
-- Table structure for table `vinterfaces`
--

CREATE TABLE vinterfaces (
  node_id varchar(32) NOT NULL default '',
  unit int(10) unsigned NOT NULL auto_increment,
  mac varchar(12) NOT NULL default '000000000000',
  IP varchar(15) default NULL,
  mask varchar(15) default NULL,
  type enum('alias','veth','veth-ne','vlan') NOT NULL default 'veth',
  iface varchar(10) default NULL,
  rtabid smallint(5) unsigned NOT NULL default '0',
  vnode_id varchar(32) default NULL,
  PRIMARY KEY  (node_id,unit),
  KEY bynode (node_id,iface),
  KEY type (type)
) TYPE=MyISAM;

--
-- Table structure for table `virt_agents`
--

CREATE TABLE virt_agents (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vname varchar(64) NOT NULL default '',
  vnode varchar(32) NOT NULL default '',
  objecttype smallint(5) unsigned NOT NULL default '0',
  PRIMARY KEY  (pid,eid,vname,vnode)
) TYPE=MyISAM;

--
-- Table structure for table `virt_firewalls`
--

CREATE TABLE virt_firewalls (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  fwname varchar(32) NOT NULL default '',
  type enum('ipfw','ipfw2','ipchains','ipfw2-vlan') NOT NULL default 'ipfw',
  style enum('open','closed','basic','emulab') NOT NULL default 'basic',
  log tinytext NOT NULL,
  PRIMARY KEY  (pid,eid,fwname)
) TYPE=MyISAM;

--
-- Table structure for table `virt_lan_lans`
--

CREATE TABLE virt_lan_lans (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  idx int(11) NOT NULL auto_increment,
  vname varchar(32) NOT NULL default '',
  PRIMARY KEY  (pid,eid,idx),
  UNIQUE KEY vname (pid,eid,vname)
) TYPE=MyISAM;

--
-- Table structure for table `virt_lan_member_settings`
--

CREATE TABLE virt_lan_member_settings (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  member varchar(32) NOT NULL default '',
  capkey varchar(32) NOT NULL default '',
  capval varchar(64) NOT NULL default '',
  PRIMARY KEY  (pid,eid,vname,member,capkey)
) TYPE=MyISAM;

--
-- Table structure for table `virt_lan_settings`
--

CREATE TABLE virt_lan_settings (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  capkey varchar(32) NOT NULL default '',
  capval varchar(64) NOT NULL default '',
  PRIMARY KEY  (pid,eid,vname,capkey)
) TYPE=MyISAM;

--
-- Table structure for table `virt_lans`
--

CREATE TABLE virt_lans (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  vnode varchar(32) NOT NULL default '',
  vport tinyint(3) NOT NULL default '0',
  ip varchar(15) NOT NULL default '',
  delay float(10,2) default '0.00',
  bandwidth int(10) unsigned default NULL,
  est_bandwidth int(10) unsigned default NULL,
  lossrate float(10,5) default NULL,
  q_limit int(11) default '0',
  q_maxthresh int(11) default '0',
  q_minthresh int(11) default '0',
  q_weight float default '0',
  q_linterm int(11) default '0',
  q_qinbytes tinyint(4) default '0',
  q_bytes tinyint(4) default '0',
  q_meanpsize int(11) default '0',
  q_wait int(11) default '0',
  q_setbit int(11) default '0',
  q_droptail int(11) default '0',
  q_red tinyint(4) default '0',
  q_gentle tinyint(4) default '0',
  member text,
  mask varchar(15) default '255.255.255.0',
  rdelay float(10,2) default NULL,
  rbandwidth int(10) unsigned default NULL,
  rest_bandwidth int(10) unsigned default NULL,
  rlossrate float(10,5) default NULL,
  cost float NOT NULL default '1',
  widearea tinyint(4) default '0',
  emulated tinyint(4) default '0',
  uselinkdelay tinyint(4) default '0',
  nobwshaping tinyint(4) default '0',
  mustdelay tinyint(1) default '0',
  usevethiface tinyint(4) default '0',
  encap_style enum('alias','veth','veth-ne','vlan','default') NOT NULL default 'default',
  trivial_ok tinyint(4) default '1',
  protocol varchar(30) NOT NULL default 'ethernet',
  is_accesspoint tinyint(4) default '0',
  traced tinyint(1) default '0',
  trace_type enum('header','packet','monitor') NOT NULL default 'header',
  trace_expr tinytext,
  trace_snaplen int(11) NOT NULL default '0',
  trace_endnode tinyint(1) NOT NULL default '0',
  trace_db tinyint(1) NOT NULL default '0',
  KEY pid (pid,eid,vname),
  KEY vnode (pid,eid,vnode)
) TYPE=MyISAM;

--
-- Table structure for table `virt_node_desires`
--

CREATE TABLE virt_node_desires (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  desire varchar(30) NOT NULL default '',
  weight float default NULL,
  PRIMARY KEY  (pid,eid,vname,desire)
) TYPE=MyISAM;

--
-- Table structure for table `virt_node_motelog`
--

CREATE TABLE virt_node_motelog (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  logfileid varchar(45) NOT NULL default '',
  PRIMARY KEY  (pid,eid,vname,logfileid)
) TYPE=MyISAM;

--
-- Table structure for table `virt_node_startloc`
--

CREATE TABLE virt_node_startloc (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  building varchar(32) NOT NULL default '',
  floor varchar(32) NOT NULL default '',
  loc_x float NOT NULL default '0',
  loc_y float NOT NULL default '0',
  orientation float NOT NULL default '0',
  PRIMARY KEY  (pid,eid,vname)
) TYPE=MyISAM;

--
-- Table structure for table `virt_nodes`
--

CREATE TABLE virt_nodes (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  ips text,
  osname varchar(20) default NULL,
  cmd_line text,
  rpms text,
  deltas text,
  startupcmd tinytext,
  tarfiles text,
  vname varchar(32) NOT NULL default '',
  type varchar(30) default NULL,
  failureaction enum('fatal','nonfatal','ignore') NOT NULL default 'fatal',
  routertype enum('none','ospf','static','manual','static-ddijk','static-old') NOT NULL default 'none',
  fixed text NOT NULL,
  inner_elab_role enum('boss','boss+router','router','ops','ops+fs','fs','node') default NULL,
  plab_role enum('plc','node','none') NOT NULL default 'none',
  numeric_id int(11) default NULL,
  KEY pid (pid,eid,vname)
) TYPE=MyISAM;

--
-- Table structure for table `virt_parameters`
--

CREATE TABLE virt_parameters (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  name varchar(64) NOT NULL default '',
  value tinytext,
  description text,
  PRIMARY KEY  (pid,eid,name)
) TYPE=MyISAM;

--
-- Table structure for table `virt_programs`
--

CREATE TABLE virt_programs (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vnode varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  command tinytext,
  dir tinytext,
  timeout int(10) unsigned default NULL,
  expected_exit_code tinyint(4) unsigned default NULL,
  PRIMARY KEY  (pid,eid,vnode,vname),
  KEY vnode (vnode)
) TYPE=MyISAM;

--
-- Table structure for table `virt_routes`
--

CREATE TABLE virt_routes (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  src varchar(32) NOT NULL default '',
  dst varchar(32) NOT NULL default '',
  dst_type enum('host','net') NOT NULL default 'host',
  dst_mask varchar(15) default '255.255.255.0',
  nexthop varchar(32) NOT NULL default '',
  cost int(11) NOT NULL default '0',
  PRIMARY KEY  (pid,eid,vname,src,dst),
  KEY pid (pid,eid,vname)
) TYPE=MyISAM;

--
-- Table structure for table `virt_simnode_attributes`
--

CREATE TABLE virt_simnode_attributes (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  nodeweight smallint(5) unsigned NOT NULL default '1',
  eventrate int(11) unsigned NOT NULL default '0',
  PRIMARY KEY  (pid,eid,vname)
) TYPE=MyISAM;

--
-- Table structure for table `virt_tiptunnels`
--

CREATE TABLE virt_tiptunnels (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  host varchar(32) NOT NULL default '',
  vnode varchar(32) NOT NULL default '',
  PRIMARY KEY  (pid,eid,host,vnode)
) TYPE=MyISAM;

--
-- Table structure for table `virt_trafgens`
--

CREATE TABLE virt_trafgens (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vnode varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  role tinytext NOT NULL,
  proto tinytext NOT NULL,
  port int(11) NOT NULL default '0',
  ip varchar(15) NOT NULL default '',
  target_vnode varchar(32) NOT NULL default '',
  target_vname varchar(32) NOT NULL default '',
  target_port int(11) NOT NULL default '0',
  target_ip varchar(15) NOT NULL default '',
  generator tinytext NOT NULL,
  PRIMARY KEY  (pid,eid,vnode,vname),
  KEY vnode (vnode)
) TYPE=MyISAM;

--
-- Table structure for table `virt_user_environment`
--

CREATE TABLE virt_user_environment (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  idx int(10) unsigned NOT NULL auto_increment,
  name varchar(255) NOT NULL default '',
  value text,
  PRIMARY KEY  (pid,eid,idx)
) TYPE=MyISAM;

--
-- Table structure for table `virt_vtypes`
--

CREATE TABLE virt_vtypes (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  name varchar(12) NOT NULL default '',
  weight float(7,5) NOT NULL default '0.00000',
  members text
) TYPE=MyISAM;

--
-- Table structure for table `vis_graphs`
--

CREATE TABLE vis_graphs (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  zoom decimal(8,3) NOT NULL default '0.000',
  detail tinyint(2) NOT NULL default '0',
  image mediumblob,
  PRIMARY KEY  (pid,eid)
) TYPE=MyISAM;

--
-- Table structure for table `vis_nodes`
--

CREATE TABLE vis_nodes (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  vis_type varchar(10) NOT NULL default '',
  x float NOT NULL default '0',
  y float NOT NULL default '0',
  PRIMARY KEY  (pid,eid,vname)
) TYPE=MyISAM;

--
-- Table structure for table `vlans`
--

CREATE TABLE vlans (
  eid varchar(32) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  virtual varchar(64) default NULL,
  members text NOT NULL,
  id int(11) NOT NULL auto_increment,
  tag smallint(5) NOT NULL default '0',
  PRIMARY KEY  (id),
  KEY pid (pid,eid,virtual)
) TYPE=MyISAM;

--
-- Table structure for table `webcams`
--

CREATE TABLE webcams (
  id int(11) unsigned NOT NULL default '0',
  server varchar(64) NOT NULL default '',
  last_update datetime default NULL,
  URL tinytext,
  stillimage_URL tinytext,
  PRIMARY KEY  (id)
) TYPE=MyISAM;

--
-- Table structure for table `webdb_table_permissions`
--

CREATE TABLE webdb_table_permissions (
  table_name varchar(64) NOT NULL default '',
  allow_read tinyint(1) default '1',
  allow_row_add_edit tinyint(1) default '0',
  allow_row_delete tinyint(1) default '0',
  PRIMARY KEY  (table_name)
) TYPE=MyISAM;

--
-- Table structure for table `webnews`
--

CREATE TABLE webnews (
  msgid int(11) NOT NULL auto_increment,
  subject tinytext,
  date datetime default NULL,
  author varchar(32) default NULL,
  body text,
  archived tinyint(1) NOT NULL default '0',
  archived_date datetime default NULL,
  PRIMARY KEY  (msgid),
  KEY date (date)
) TYPE=MyISAM;

--
-- Table structure for table `widearea_accounts`
--

CREATE TABLE widearea_accounts (
  uid varchar(8) NOT NULL default '',
  node_id varchar(32) NOT NULL default '',
  trust enum('none','user','local_root') default NULL,
  date_applied date default NULL,
  date_approved datetime default NULL,
  PRIMARY KEY  (uid,node_id)
) TYPE=MyISAM;

--
-- Table structure for table `widearea_delays`
--

CREATE TABLE widearea_delays (
  time double default NULL,
  node_id1 varchar(32) NOT NULL default '',
  iface1 varchar(10) NOT NULL default '',
  node_id2 varchar(32) NOT NULL default '',
  iface2 varchar(10) NOT NULL default '',
  bandwidth double default NULL,
  time_stddev float NOT NULL default '0',
  lossrate float NOT NULL default '0',
  start_time int(10) unsigned default NULL,
  end_time int(10) unsigned default NULL,
  PRIMARY KEY  (node_id1,iface1,node_id2,iface2)
) TYPE=MyISAM;

--
-- Table structure for table `widearea_nodeinfo`
--

CREATE TABLE widearea_nodeinfo (
  node_id varchar(32) NOT NULL default '',
  machine_type varchar(40) default NULL,
  contact_uid varchar(8) NOT NULL default '',
  connect_type varchar(20) default NULL,
  city tinytext,
  state tinytext,
  country tinytext,
  zip tinytext,
  external_node_id tinytext,
  hostname varchar(255) default NULL,
  site varchar(255) default NULL,
  latitude float default NULL,
  longitude float default NULL,
  bwlimit varchar(32) default NULL,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table `widearea_privkeys`
--

CREATE TABLE widearea_privkeys (
  privkey varchar(64) NOT NULL default '',
  IP varchar(15) NOT NULL default '1.1.1.1',
  user_name tinytext NOT NULL,
  user_email tinytext NOT NULL,
  cdkey varchar(64) default NULL,
  nextprivkey varchar(64) default NULL,
  rootkey varchar(64) default NULL,
  lockkey varchar(64) default NULL,
  requested datetime NOT NULL default '0000-00-00 00:00:00',
  updated datetime NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (privkey,IP),
  KEY IP (IP)
) TYPE=MyISAM;

--
-- Table structure for table `widearea_recent`
--

CREATE TABLE widearea_recent (
  time double default NULL,
  node_id1 varchar(32) NOT NULL default '',
  iface1 varchar(10) NOT NULL default '',
  node_id2 varchar(32) NOT NULL default '',
  iface2 varchar(10) NOT NULL default '',
  bandwidth double default NULL,
  time_stddev float NOT NULL default '0',
  lossrate float NOT NULL default '0',
  start_time int(10) unsigned default NULL,
  end_time int(10) unsigned default NULL,
  PRIMARY KEY  (node_id1,iface1,node_id2,iface2)
) TYPE=MyISAM;

--
-- Table structure for table `widearea_updates`
--

CREATE TABLE widearea_updates (
  IP varchar(15) NOT NULL default '1.1.1.1',
  roottag tinytext NOT NULL,
  update_requested datetime NOT NULL default '0000-00-00 00:00:00',
  update_started datetime default NULL,
  `force` enum('yes','no') NOT NULL default 'no',
  PRIMARY KEY  (IP)
) TYPE=MyISAM;

--
-- Table structure for table `wireless_stats`
--

CREATE TABLE wireless_stats (
  name varchar(32) NOT NULL default '',
  floor varchar(32) NOT NULL default '',
  building varchar(32) NOT NULL default '',
  data_eid varchar(32) default NULL,
  data_pid varchar(32) default NULL,
  type varchar(32) default NULL,
  altsrc tinytext,
  PRIMARY KEY  (name)
) TYPE=MyISAM;

--
-- Table structure for table `wires`
--

CREATE TABLE wires (
  cable smallint(3) unsigned default NULL,
  len tinyint(3) unsigned NOT NULL default '0',
  type enum('Node','Serial','Power','Dnard','Control','Trunk','OuterControl') NOT NULL default 'Node',
  node_id1 char(32) NOT NULL default '',
  card1 tinyint(3) unsigned NOT NULL default '0',
  port1 tinyint(3) unsigned NOT NULL default '0',
  node_id2 char(32) NOT NULL default '',
  card2 tinyint(3) unsigned NOT NULL default '0',
  port2 tinyint(3) unsigned NOT NULL default '0',
  PRIMARY KEY  (node_id1,card1,port1),
  KEY node_id2 (node_id2,card2),
  KEY dest (node_id2,card2,port2),
  KEY src (node_id1,card1,port1)
) TYPE=MyISAM;


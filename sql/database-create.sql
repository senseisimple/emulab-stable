-- MySQL dump 8.22
--
-- Host: localhost    Database: tbdb
---------------------------------------------------------
-- Server version	3.23.54-log

--
-- Table structure for table 'cdroms'
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
-- Table structure for table 'comments'
--

CREATE TABLE comments (
  table_name varchar(64) NOT NULL default '',
  column_name varchar(64) default NULL,
  description text,
  UNIQUE KEY table_name (table_name,column_name)
) TYPE=MyISAM;

--
-- Table structure for table 'current_reloads'
--

CREATE TABLE current_reloads (
  node_id varchar(10) NOT NULL default '',
  image_id varchar(45) NOT NULL default '',
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table 'delays'
--

CREATE TABLE delays (
  node_id varchar(10) NOT NULL default '',
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
  PRIMARY KEY  (node_id,iface0,iface1),
  KEY pid (pid,eid)
) TYPE=MyISAM;

--
-- Table structure for table 'delta_inst'
--

CREATE TABLE delta_inst (
  node_id varchar(10) NOT NULL default '',
  partition tinyint(4) NOT NULL default '0',
  delta_id varchar(10) NOT NULL default '',
  PRIMARY KEY  (node_id,partition,delta_id)
) TYPE=MyISAM;

--
-- Table structure for table 'delta_proj'
--

CREATE TABLE delta_proj (
  delta_id varchar(10) NOT NULL default '',
  pid varchar(10) NOT NULL default '',
  PRIMARY KEY  (delta_id,pid)
) TYPE=MyISAM;

--
-- Table structure for table 'deltas'
--

CREATE TABLE deltas (
  delta_id varchar(10) NOT NULL default '',
  delta_desc text,
  delta_path text NOT NULL,
  private enum('yes','no') NOT NULL default 'no',
  PRIMARY KEY  (delta_id)
) TYPE=MyISAM;

--
-- Table structure for table 'event_eventtypes'
--

CREATE TABLE event_eventtypes (
  idx smallint(5) unsigned NOT NULL default '0',
  type tinytext NOT NULL,
  PRIMARY KEY  (idx)
) TYPE=MyISAM;

--
-- Table structure for table 'event_objecttypes'
--

CREATE TABLE event_objecttypes (
  idx smallint(5) unsigned NOT NULL default '0',
  type tinytext NOT NULL,
  PRIMARY KEY  (idx)
) TYPE=MyISAM;

--
-- Table structure for table 'eventlist'
--

CREATE TABLE eventlist (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  idx int(10) unsigned NOT NULL auto_increment,
  time float(10,3) NOT NULL default '0.000',
  vnode varchar(32) NOT NULL default '',
  vname varchar(20) NOT NULL default '',
  objecttype smallint(5) unsigned NOT NULL default '0',
  eventtype smallint(5) unsigned NOT NULL default '0',
  arguments tinytext,
  atstring tinytext,
  PRIMARY KEY  (pid,eid,idx),
  KEY vnode (vnode)
) TYPE=MyISAM;

--
-- Table structure for table 'experiments'
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
  swappable tinyint(4) NOT NULL default '0',
  priority tinyint(4) NOT NULL default '0',
  batchmode tinyint(4) NOT NULL default '0',
  shared tinyint(4) NOT NULL default '0',
  state varchar(12) NOT NULL default 'new',
  maximum_nodes tinyint(4) default NULL,
  minimum_nodes tinyint(4) default NULL,
  testdb tinytext,
  path tinytext,
  logfile tinytext,
  logfile_open tinyint(4) NOT NULL default '0',
  attempts smallint(5) unsigned NOT NULL default '0',
  canceled tinyint(4) NOT NULL default '0',
  batchstate varchar(12) default NULL,
  event_sched_pid int(11) default '0',
  uselinkdelays tinyint(4) NOT NULL default '0',
  forcelinkdelays tinyint(4) NOT NULL default '0',
  uselatestwadata tinyint(4) NOT NULL default '0',
  usewatunnels tinyint(4) NOT NULL default '1',
  wa_delay_solverweight float default '0',
  wa_bw_solverweight float default '0',
  wa_plr_solverweight float default '0',
  swap_requests tinyint(4) NOT NULL default '0',
  last_swap_req datetime default NULL,
  idle_ignore tinyint(4) NOT NULL default '0',
  idx int(10) unsigned NOT NULL auto_increment,
  PRIMARY KEY  (eid,pid),
  KEY idx (idx),
  KEY batchmode (batchmode)
) TYPE=MyISAM;

--
-- Table structure for table 'exported_tables'
--

CREATE TABLE exported_tables (
  table_name varchar(64) NOT NULL default '',
  PRIMARY KEY  (table_name)
) TYPE=MyISAM;

--
-- Table structure for table 'exppid_access'
--

CREATE TABLE exppid_access (
  exp_eid varchar(32) NOT NULL default '',
  exp_pid varchar(12) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  PRIMARY KEY  (exp_eid,exp_pid,pid)
) TYPE=MyISAM;

--
-- Table structure for table 'foreign_keys'
--

CREATE TABLE foreign_keys (
  table1 varchar(30) NOT NULL default '',
  column1 varchar(30) NOT NULL default '',
  table2 varchar(30) NOT NULL default '',
  column2 varchar(30) NOT NULL default '',
  PRIMARY KEY  (table1,column1)
) TYPE=MyISAM;

--
-- Table structure for table 'group_membership'
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
-- Table structure for table 'groups'
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
  PRIMARY KEY  (pid,gid),
  KEY unix_gid (unix_gid),
  KEY gid (gid),
  KEY pid (pid)
) TYPE=MyISAM;

--
-- Table structure for table 'iface_counters'
--

CREATE TABLE iface_counters (
  node_id varchar(10) NOT NULL default '',
  tstamp datetime NOT NULL default '0000-00-00 00:00:00',
  mac varchar(12) NOT NULL default '0',
  ipkts int(11) NOT NULL default '0',
  opkts int(11) NOT NULL default '0',
  PRIMARY KEY  (node_id,tstamp,mac),
  KEY macindex (mac),
  KEY node_idindex (node_id)
) TYPE=MyISAM;

--
-- Table structure for table 'images'
--

CREATE TABLE images (
  imagename varchar(30) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
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
  load_busy tinyint(4) NOT NULL default '0',
  ezid tinyint(4) NOT NULL default '0',
  shared tinyint(4) NOT NULL default '0',
  updated datetime default NULL,
  max_concurrent int(11) default NULL,
  PRIMARY KEY  (imagename,pid),
  KEY imageid (imageid)
) TYPE=MyISAM;

--
-- Table structure for table 'interface_types'
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
-- Table structure for table 'interfaces'
--

CREATE TABLE interfaces (
  node_id varchar(10) NOT NULL default '',
  card tinyint(3) unsigned NOT NULL default '0',
  port tinyint(3) unsigned NOT NULL default '0',
  mac varchar(12) NOT NULL default '000000000000',
  IP varchar(15) default NULL,
  IPalias varchar(15) default NULL,
  IPaliases text,
  interface_type varchar(30) default NULL,
  iface text NOT NULL,
  current_speed enum('100','10','1000') NOT NULL default '100',
  duplex enum('full','half') NOT NULL default 'full',
  PRIMARY KEY  (node_id,card,port),
  KEY mac (mac),
  KEY IP (IP)
) TYPE=MyISAM;

--
-- Table structure for table 'ipport_ranges'
--

CREATE TABLE ipport_ranges (
  eid varchar(32) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  low int(11) NOT NULL default '0',
  high int(11) NOT NULL default '0',
  PRIMARY KEY  (eid,pid)
) TYPE=MyISAM;

--
-- Table structure for table 'ipsubnets'
--

CREATE TABLE ipsubnets (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  idx smallint(5) unsigned NOT NULL auto_increment,
  PRIMARY KEY  (idx)
) TYPE=MyISAM;

--
-- Table structure for table 'last_reservation'
--

CREATE TABLE last_reservation (
  node_id varchar(10) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  PRIMARY KEY  (node_id,pid)
) TYPE=MyISAM;

--
-- Table structure for table 'lastlogin'
--

CREATE TABLE lastlogin (
  uid varchar(10) NOT NULL default '',
  time datetime default NULL,
  PRIMARY KEY  (uid)
) TYPE=MyISAM;

--
-- Table structure for table 'linkdelays'
--

CREATE TABLE linkdelays (
  node_id varchar(10) NOT NULL default '',
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
  PRIMARY KEY  (node_id,vlan,vnode)
) TYPE=MyISAM;

--
-- Table structure for table 'login'
--

CREATE TABLE login (
  uid varchar(10) NOT NULL default '',
  hashkey tinytext,
  timeout varchar(10) NOT NULL default '',
  PRIMARY KEY  (uid)
) TYPE=MyISAM;

--
-- Table structure for table 'loginmessage'
--

CREATE TABLE loginmessage (
  valid tinyint(4) NOT NULL default '1',
  message tinytext NOT NULL,
  PRIMARY KEY  (valid)
) TYPE=MyISAM;

--
-- Table structure for table 'mode_transitions'
--

CREATE TABLE mode_transitions (
  op_mode1 varchar(20) NOT NULL default '',
  state1 varchar(20) NOT NULL default '',
  op_mode2 varchar(20) NOT NULL default '',
  state2 varchar(20) NOT NULL default '',
  PRIMARY KEY  (op_mode1,state1,op_mode2,state2),
  KEY op_mode1 (op_mode1,state1),
  KEY op_mode2 (op_mode2,state2)
) TYPE=MyISAM;

--
-- Table structure for table 'newdelays'
--

CREATE TABLE newdelays (
  node_id varchar(10) NOT NULL default '',
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
-- Table structure for table 'next_reserve'
--

CREATE TABLE next_reserve (
  node_id varchar(10) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table 'nextfreenode'
--

CREATE TABLE nextfreenode (
  nodetype varchar(30) NOT NULL default '',
  nextid int(10) unsigned NOT NULL default '1',
  nextpri int(10) unsigned NOT NULL default '1',
  PRIMARY KEY  (nodetype)
) TYPE=MyISAM;

--
-- Table structure for table 'node_idlestats'
--

CREATE TABLE node_idlestats (
  node_id varchar(10) NOT NULL default '',
  tstamp datetime NOT NULL default '0000-00-00 00:00:00',
  last_tty datetime NOT NULL default '0000-00-00 00:00:00',
  load_1min float NOT NULL default '0',
  load_5min float NOT NULL default '0',
  load_15min float NOT NULL default '0',
  PRIMARY KEY  (node_id,tstamp)
) TYPE=MyISAM;

--
-- Table structure for table 'node_status'
--

CREATE TABLE node_status (
  node_id varchar(10) NOT NULL default '',
  status enum('up','possibly down','down','unpingable') default NULL,
  status_timestamp datetime default NULL,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table 'node_types'
--

CREATE TABLE node_types (
  class varchar(10) default NULL,
  type varchar(30) NOT NULL default '',
  proc varchar(30) default NULL,
  speed smallint(5) unsigned default NULL,
  RAM smallint(5) unsigned default NULL,
  HD float(10,2) default NULL,
  max_cards tinyint(3) unsigned default NULL,
  max_ports tinyint(3) unsigned default NULL,
  osid varchar(35) NOT NULL default '',
  control_net tinyint(3) unsigned default NULL,
  power_time smallint(5) unsigned NOT NULL default '60',
  imageid varchar(45) NOT NULL default '',
  imageable tinyint(4) default '0',
  delay_capacity tinyint(4) NOT NULL default '0',
  virtnode_capacity tinyint(4) NOT NULL default '0',
  control_iface text,
  disktype enum('ad','da','ar') default NULL,
  delay_osid varchar(35) default NULL,
  pxe_boot_path text,
  isvirtnode tinyint(4) NOT NULL default '0',
  isremotenode tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (type)
) TYPE=MyISAM;

--
-- Table structure for table 'nodeipportnum'
--

CREATE TABLE nodeipportnum (
  node_id varchar(10) NOT NULL default '',
  port smallint(5) unsigned NOT NULL default '11000',
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table 'nodelog'
--

CREATE TABLE nodelog (
  node_id varchar(10) NOT NULL default '',
  log_id smallint(5) unsigned NOT NULL auto_increment,
  type enum('misc') NOT NULL default 'misc',
  reporting_uid varchar(8) NOT NULL default '',
  entry tinytext NOT NULL,
  reported datetime default NULL,
  PRIMARY KEY  (node_id,log_id)
) TYPE=MyISAM;

--
-- Table structure for table 'nodes'
--

CREATE TABLE nodes (
  node_id varchar(10) NOT NULL default '',
  type varchar(30) NOT NULL default '',
  phys_nodeid varchar(10) default NULL,
  role enum('testnode','virtnode','ctrlnode','testswitch','ctrlswitch','powerctrl','unused') NOT NULL default 'unused',
  def_boot_osid varchar(35) NOT NULL default '',
  def_boot_path text,
  def_boot_cmd_line text,
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
  routertype enum('none','ospf','static','manual') NOT NULL default 'none',
  next_pxe_boot_path text,
  bios_version varchar(64) default NULL,
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
  PRIMARY KEY  (node_id),
  KEY phys_nodeid (phys_nodeid),
  KEY node_id (node_id,phys_nodeid),
  KEY role (role)
) TYPE=MyISAM;

--
-- Table structure for table 'nodetypeXpid_permissions'
--

CREATE TABLE nodetypeXpid_permissions (
  type varchar(30) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  PRIMARY KEY  (type,pid)
) TYPE=MyISAM;

--
-- Table structure for table 'nodeuidlastlogin'
--

CREATE TABLE nodeuidlastlogin (
  node_id varchar(10) NOT NULL default '',
  uid varchar(10) NOT NULL default '',
  date date default NULL,
  time time default NULL,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table 'nologins'
--

CREATE TABLE nologins (
  nologins tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (nologins)
) TYPE=MyISAM;

--
-- Table structure for table 'nseconfigs'
--

CREATE TABLE nseconfigs (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  nseconfig text,
  PRIMARY KEY  (pid,eid,vname)
) TYPE=MyISAM;

--
-- Table structure for table 'nsfiles'
--

CREATE TABLE nsfiles (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  nsfile text,
  PRIMARY KEY  (eid,pid)
) TYPE=MyISAM;

--
-- Table structure for table 'ntpinfo'
--

CREATE TABLE ntpinfo (
  node_id varchar(10) NOT NULL default '',
  IP varchar(64) NOT NULL default '',
  type enum('server','peer') NOT NULL default 'peer',
  PRIMARY KEY  (node_id,IP,type)
) TYPE=MyISAM;

--
-- Table structure for table 'os_info'
--

CREATE TABLE os_info (
  osname varchar(20) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  osid varchar(35) NOT NULL default '',
  creator varchar(8) default NULL,
  created datetime default NULL,
  description tinytext NOT NULL,
  OS enum('Unknown','Linux','FreeBSD','NetBSD','OSKit','Other') NOT NULL default 'Unknown',
  version varchar(12) default '',
  path tinytext,
  magic tinytext,
  machinetype varchar(30) NOT NULL default '',
  osfeatures set('ping','ssh','ipod','isup') default NULL,
  ezid tinyint(4) NOT NULL default '0',
  shared tinyint(4) NOT NULL default '0',
  mustclean tinyint(4) NOT NULL default '1',
  op_mode varchar(20) NOT NULL default 'MINIMAL',
  nextosid varchar(35) default NULL,
  PRIMARY KEY  (osname,pid),
  KEY osid (osid),
  KEY OS (OS)
) TYPE=MyISAM;

--
-- Table structure for table 'osidtoimageid'
--

CREATE TABLE osidtoimageid (
  osid varchar(35) NOT NULL default '',
  type varchar(30) NOT NULL default '',
  imageid varchar(45) NOT NULL default '',
  PRIMARY KEY  (osid,type)
) TYPE=MyISAM;

--
-- Table structure for table 'outlets'
--

CREATE TABLE outlets (
  node_id varchar(10) NOT NULL default '',
  power_id varchar(10) NOT NULL default '',
  outlet tinyint(1) unsigned NOT NULL default '0',
  last_power timestamp(14) NOT NULL,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table 'partitions'
--

CREATE TABLE partitions (
  node_id varchar(10) NOT NULL default '',
  partition tinyint(4) NOT NULL default '0',
  osid varchar(35) default NULL,
  PRIMARY KEY  (node_id,partition)
) TYPE=MyISAM;

--
-- Table structure for table 'port_counters'
--

CREATE TABLE port_counters (
  node_id char(10) NOT NULL default '',
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
-- Table structure for table 'portmap'
--

CREATE TABLE portmap (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vnode varchar(32) NOT NULL default '',
  vport tinyint(4) NOT NULL default '0',
  pport varchar(32) NOT NULL default ''
) TYPE=MyISAM;

--
-- Table structure for table 'proj_memb'
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
-- Table structure for table 'projects'
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
  public tinyint(4) NOT NULL default '0',
  public_whynot tinytext,
  expt_count mediumint(8) unsigned default '0',
  expt_last date default NULL,
  pcremote_ok set('pcplab','pcron','pcwa') default NULL,
  PRIMARY KEY  (pid),
  KEY unix_gid (unix_gid),
  KEY approved (approved),
  KEY approved_2 (approved),
  KEY pcremote_ok (pcremote_ok)
) TYPE=MyISAM;

--
-- Table structure for table 'reserved'
--

CREATE TABLE reserved (
  node_id varchar(10) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  rsrv_time timestamp(14) NOT NULL,
  vname varchar(32) default NULL,
  PRIMARY KEY  (node_id),
  KEY pid (pid,eid)
) TYPE=MyISAM;

--
-- Table structure for table 'scheduled_reloads'
--

CREATE TABLE scheduled_reloads (
  node_id varchar(10) NOT NULL default '',
  image_id varchar(45) NOT NULL default '',
  reload_type enum('netdisk','frisbee') default NULL,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table 'state_timeouts'
--

CREATE TABLE state_timeouts (
  op_mode varchar(20) NOT NULL default '',
  state varchar(20) NOT NULL default '',
  timeout int(11) default NULL,
  action mediumtext,
  PRIMARY KEY  (op_mode,state)
) TYPE=MyISAM;

--
-- Table structure for table 'state_transitions'
--

CREATE TABLE state_transitions (
  op_mode varchar(20) NOT NULL default '',
  state1 varchar(20) NOT NULL default '',
  state2 varchar(20) NOT NULL default '',
  PRIMARY KEY  (op_mode,state1,state2)
) TYPE=MyISAM;

--
-- Table structure for table 'state_triggers'
--

CREATE TABLE state_triggers (
  op_mode varchar(20) NOT NULL default '',
  state varchar(20) NOT NULL default '',
  trigger tinytext NOT NULL,
  PRIMARY KEY  (op_mode,state)
) TYPE=MyISAM;

--
-- Table structure for table 'switch_paths'
--

CREATE TABLE switch_paths (
  pid varchar(12) default NULL,
  eid varchar(32) default NULL,
  vname varchar(32) default NULL,
  node_id1 varchar(10) default NULL,
  node_id2 varchar(10) default NULL
) TYPE=MyISAM;

--
-- Table structure for table 'switch_stack_types'
--

CREATE TABLE switch_stack_types (
  stack_id varchar(10) NOT NULL default '',
  stack_type varchar(10) default NULL,
  supports_private tinyint(1) NOT NULL default '0',
  single_domain tinyint(1) NOT NULL default '1',
  snmp_community varchar(32) default NULL,
  PRIMARY KEY  (stack_id)
) TYPE=MyISAM;

--
-- Table structure for table 'switch_stacks'
--

CREATE TABLE switch_stacks (
  node_id varchar(10) NOT NULL default '',
  stack_id varchar(10) NOT NULL default '',
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table 'testsuite_preentables'
--

CREATE TABLE testsuite_preentables (
  table_name varchar(128) NOT NULL default '',
  action enum('drop','clean','prune') default 'drop',
  PRIMARY KEY  (table_name)
) TYPE=MyISAM;

--
-- Table structure for table 'tiplines'
--

CREATE TABLE tiplines (
  tipname varchar(32) NOT NULL default '',
  node_id varchar(10) NOT NULL default '',
  server varchar(64) NOT NULL default '',
  portnum int(11) NOT NULL default '0',
  keylen smallint(6) NOT NULL default '0',
  keydata text,
  PRIMARY KEY  (tipname),
  KEY node_id (node_id)
) TYPE=MyISAM;

--
-- Table structure for table 'tipservers'
--

CREATE TABLE tipservers (
  server varchar(64) NOT NULL default '',
  PRIMARY KEY  (server)
) TYPE=MyISAM;

--
-- Table structure for table 'tmcd_redirect'
--

CREATE TABLE tmcd_redirect (
  node_id varchar(10) NOT NULL default '',
  dbname tinytext NOT NULL,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table 'tunnels'
--

CREATE TABLE tunnels (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  node_id varchar(10) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  isserver tinyint(3) unsigned NOT NULL default '0',
  port int(11) NOT NULL default '0',
  peer_ip varchar(32) NOT NULL default '',
  password varchar(32) NOT NULL default '',
  proto varchar(12) NOT NULL default 'udp',
  encrypt tinyint(3) unsigned NOT NULL default '0',
  compress tinyint(3) unsigned NOT NULL default '0',
  assigned_ip varchar(32) NOT NULL default '',
  PRIMARY KEY  (pid,eid,node_id,vname),
  KEY node_id (node_id)
) TYPE=MyISAM;

--
-- Table structure for table 'uidnodelastlogin'
--

CREATE TABLE uidnodelastlogin (
  uid varchar(10) NOT NULL default '',
  node_id varchar(10) NOT NULL default '',
  date date default NULL,
  time time default NULL,
  PRIMARY KEY  (uid)
) TYPE=MyISAM;

--
-- Table structure for table 'unixgroup_membership'
--

CREATE TABLE unixgroup_membership (
  uid varchar(8) NOT NULL default '',
  gid varchar(16) NOT NULL default '',
  PRIMARY KEY  (uid,gid)
) TYPE=MyISAM;

--
-- Table structure for table 'user_pubkeys'
--

CREATE TABLE user_pubkeys (
  uid varchar(8) NOT NULL default '',
  comment varchar(128) NOT NULL default '',
  pubkey text,
  stamp datetime default NULL,
  PRIMARY KEY  (uid,comment)
) TYPE=MyISAM;

--
-- Table structure for table 'user_sfskeys'
--

CREATE TABLE user_sfskeys (
  uid varchar(8) NOT NULL default '',
  comment varchar(128) NOT NULL default '',
  pubkey text,
  stamp datetime default NULL,
  PRIMARY KEY  (uid,comment)
) TYPE=MyISAM;

--
-- Table structure for table 'users'
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
  usr_zip int(6) unsigned default NULL,
  usr_phone tinytext,
  usr_pswd tinytext NOT NULL,
  unix_uid smallint(5) unsigned NOT NULL auto_increment,
  status enum('newuser','unapproved','unverified','active','frozen','other') NOT NULL default 'newuser',
  admin tinyint(4) default '0',
  dbedit tinyint(4) default '0',
  stud tinyint(4) default '0',
  webonly tinyint(4) default '0',
  pswd_expires date default NULL,
  cvsweb tinyint(4) NOT NULL default '0',
  emulab_pubkey text,
  home_pubkey text,
  adminoff tinyint(4) default '0',
  verify_key varchar(32) default NULL,
  PRIMARY KEY  (uid),
  KEY unix_uid (unix_uid),
  KEY status (status)
) TYPE=MyISAM;

--
-- Table structure for table 'userslastlogin'
--

CREATE TABLE userslastlogin (
  uid varchar(10) NOT NULL default '',
  date date default NULL,
  time time default NULL,
  PRIMARY KEY  (uid)
) TYPE=MyISAM;

--
-- Table structure for table 'v2pmap'
--

CREATE TABLE v2pmap (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  node_id varchar(10) NOT NULL default '',
  PRIMARY KEY  (pid,eid,vname)
) TYPE=MyISAM;

--
-- Table structure for table 'virt_agents'
--

CREATE TABLE virt_agents (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  vnode varchar(32) NOT NULL default '',
  objecttype smallint(5) unsigned NOT NULL default '0',
  PRIMARY KEY  (pid,eid,vname,vnode)
) TYPE=MyISAM;

--
-- Table structure for table 'virt_lans'
--

CREATE TABLE virt_lans (
  pid varchar(12) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  vname varchar(32) NOT NULL default '',
  delay float(10,2) default '0.00',
  bandwidth int(10) unsigned default NULL,
  lossrate float(10,3) default NULL,
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
  rdelay float(10,2) default NULL,
  rbandwidth int(10) unsigned default NULL,
  rlossrate float(10,3) default NULL,
  cost float NOT NULL default '1',
  widearea tinyint(4) default '0',
  KEY pid (pid,eid,vname)
) TYPE=MyISAM;

--
-- Table structure for table 'virt_nodes'
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
  type varchar(12) default NULL,
  failureaction enum('fatal','nonfatal','ignore') NOT NULL default 'fatal',
  routertype enum('none','ospf','static','manual') NOT NULL default 'none',
  fixed text NOT NULL,
  KEY pid (pid,eid,vname)
) TYPE=MyISAM;

--
-- Table structure for table 'virt_routes'
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
-- Table structure for table 'virt_trafgens'
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
-- Table structure for table 'virt_vtypes'
--

CREATE TABLE virt_vtypes (
  pid varchar(12) NOT NULL default '',
  eid varchar(12) NOT NULL default '',
  name varchar(12) NOT NULL default '',
  weight float(7,5) NOT NULL default '0.00000',
  members text
) TYPE=MyISAM;

--
-- Table structure for table 'vis_experiments'
--

CREATE TABLE vis_experiments (
  eid varchar(32) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  thumb_hash varchar(64) NOT NULL default '',
  PRIMARY KEY  (eid,pid)
) TYPE=MyISAM;

--
-- Table structure for table 'vis_nodes'
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
-- Table structure for table 'vlans'
--

CREATE TABLE vlans (
  eid varchar(32) NOT NULL default '',
  pid varchar(12) NOT NULL default '',
  virtual varchar(64) default NULL,
  members text NOT NULL,
  id int(11) NOT NULL auto_increment,
  PRIMARY KEY  (id),
  KEY pid (pid,eid)
) TYPE=MyISAM;

--
-- Table structure for table 'webdb_table_permissions'
--

CREATE TABLE webdb_table_permissions (
  table_name varchar(64) NOT NULL default '',
  allow_read tinyint(1) default '1',
  allow_row_add_edit tinyint(1) default '0',
  allow_row_delete tinyint(1) default '0',
  PRIMARY KEY  (table_name)
) TYPE=MyISAM;

--
-- Table structure for table 'widearea_accounts'
--

CREATE TABLE widearea_accounts (
  uid varchar(8) NOT NULL default '',
  node_id varchar(10) NOT NULL default '',
  trust enum('none','user','local_root') default NULL,
  date_applied date default NULL,
  date_approved datetime default NULL,
  PRIMARY KEY  (uid,node_id)
) TYPE=MyISAM;

--
-- Table structure for table 'widearea_delays'
--

CREATE TABLE widearea_delays (
  time double default NULL,
  node_id1 varchar(10) NOT NULL default '',
  iface1 varchar(10) NOT NULL default '',
  node_id2 varchar(10) NOT NULL default '',
  iface2 varchar(10) NOT NULL default '',
  bandwidth double default NULL,
  time_stddev float NOT NULL default '0',
  lossrate float NOT NULL default '0',
  start_time int(10) unsigned default NULL,
  end_time int(10) unsigned default NULL,
  PRIMARY KEY  (node_id1,iface1,node_id2,iface2)
) TYPE=MyISAM;

--
-- Table structure for table 'widearea_nodeinfo'
--

CREATE TABLE widearea_nodeinfo (
  node_id varchar(10) NOT NULL default '',
  machine_type varchar(40) default NULL,
  contact_uid varchar(8) NOT NULL default '',
  connect_type varchar(20) default NULL,
  city varchar(40) default NULL,
  state varchar(40) default NULL,
  country varchar(10) default NULL,
  zip varchar(10) default NULL,
  PRIMARY KEY  (node_id)
) TYPE=MyISAM;

--
-- Table structure for table 'widearea_privkeys'
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
-- Table structure for table 'widearea_recent'
--

CREATE TABLE widearea_recent (
  time double default NULL,
  node_id1 varchar(10) NOT NULL default '',
  iface1 varchar(10) NOT NULL default '',
  node_id2 varchar(10) NOT NULL default '',
  iface2 varchar(10) NOT NULL default '',
  bandwidth double default NULL,
  time_stddev float NOT NULL default '0',
  lossrate float NOT NULL default '0',
  start_time int(10) unsigned default NULL,
  end_time int(10) unsigned default NULL,
  PRIMARY KEY  (node_id1,iface1,node_id2,iface2)
) TYPE=MyISAM;

--
-- Table structure for table 'wires'
--

CREATE TABLE wires (
  cable smallint(3) unsigned default NULL,
  len tinyint(3) unsigned NOT NULL default '0',
  type enum('Node','Serial','Power','Dnard','Control','Trunk') NOT NULL default 'Node',
  node_id1 char(10) NOT NULL default '',
  card1 tinyint(3) unsigned NOT NULL default '0',
  port1 tinyint(3) unsigned NOT NULL default '0',
  node_id2 char(10) NOT NULL default '',
  card2 tinyint(3) unsigned NOT NULL default '0',
  port2 tinyint(3) unsigned NOT NULL default '0',
  PRIMARY KEY  (node_id1,card1,port1),
  KEY node_id2 (node_id2,card2),
  KEY dest (node_id2,card2,port2),
  KEY src (node_id1,card1,port1)
) TYPE=MyISAM;


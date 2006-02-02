-- MySQL dump 8.23
--
-- Host: localhost    Database: nfsdb
---------------------------------------------------------
-- Server version	3.23.58-log

--
-- Table structure for table `create_replies`
--

CREATE TABLE create_replies (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  status int(3) NOT NULL default '0',
  fh varchar(96) NOT NULL default '',
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip)
) TYPE=MyISAM;

--
-- Table structure for table `creates`
--

CREATE TABLE creates (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  fn text,
  mode int(5) default NULL,
  euid int(5) default NULL,
  egid int(5) default NULL,
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip)
) TYPE=MyISAM;

--
-- Table structure for table `file_access`
--

CREATE TABLE file_access (
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  last_access double NOT NULL default '0',
  PRIMARY KEY  (node_ip,fh),
  KEY last_access (last_access),
  KEY fh (fh)
) TYPE=MyISAM;

--
-- Table structure for table `file_checkpoint`
--

CREATE TABLE file_checkpoint (
  timestamp double NOT NULL default '0',
  fh varchar(96) NOT NULL default '',
  ftype int(3) default NULL,
  mode int(5) default NULL,
  nlink int(5) default NULL,
  uid int(5) default NULL,
  gid int(5) default NULL,
  size int(10) default NULL,
  rdev int(10) default NULL,
  fsid int(10) default NULL,
  fileid int(10) default NULL,
  atime double default NULL,
  mtime double default NULL,
  ctime double default NULL,
  PRIMARY KEY  (fh)
) TYPE=MyISAM;

--
-- Table structure for table `file_dropped`
--

CREATE TABLE file_dropped (
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  last_remove double NOT NULL default '0',
  PRIMARY KEY  (node_ip,fh),
  KEY last_remove (last_remove),
  KEY fh (fh)
) TYPE=MyISAM;

--
-- Table structure for table `file_writes`
--

CREATE TABLE file_writes (
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  last_access double NOT NULL default '0',
  PRIMARY KEY  (node_ip,fh),
  KEY fh (fh),
  KEY last_access (last_access)
) TYPE=MyISAM;

--
-- Table structure for table `handle_map`
--

CREATE TABLE handle_map (
  fh varchar(96) NOT NULL default '',
  parent varchar(96) NOT NULL default '',
  fn text NOT NULL,
  timestamp double NOT NULL default '0',
  valid int(1) NOT NULL default '0',
  PRIMARY KEY  (fh,parent,fn(255)),
  KEY fh (fh),
  KEY parent (parent,fn(255)),
  KEY parent_2 (parent),
  KEY valid (valid)
) TYPE=MyISAM;

--
-- Table structure for table `link_access`
--

CREATE TABLE link_access (
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  fn text NOT NULL,
  last_access double NOT NULL default '0',
  PRIMARY KEY  (node_ip,fh),
  KEY last_access (last_access),
  KEY fh (fh)
) TYPE=MyISAM;

--
-- Table structure for table `lookup_replies`
--

CREATE TABLE lookup_replies (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  status int(3) NOT NULL default '0',
  fh varchar(96) NOT NULL default '',
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip),
  KEY timestamp (timestamp)
) TYPE=MyISAM;

--
-- Table structure for table `lookup_replies_tmp`
--

CREATE TABLE lookup_replies_tmp (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  status int(3) NOT NULL default '0',
  fh varchar(96) NOT NULL default '',
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip),
  KEY timestamp (timestamp)
) TYPE=MyISAM;

--
-- Table structure for table `lookups`
--

CREATE TABLE lookups (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  fn text,
  euid int(5) default NULL,
  egid int(5) default NULL,
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip),
  KEY timestamp (timestamp)
) TYPE=MyISAM;

--
-- Table structure for table `lookups_tmp`
--

CREATE TABLE lookups_tmp (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  fn text,
  euid int(5) default NULL,
  egid int(5) default NULL,
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip),
  KEY timestamp (timestamp)
) TYPE=MyISAM;

--
-- Table structure for table `mkdir_replies`
--

CREATE TABLE mkdir_replies (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  status int(3) NOT NULL default '0',
  fh varchar(96) NOT NULL default '',
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip)
) TYPE=MyISAM;

--
-- Table structure for table `mkdirs`
--

CREATE TABLE mkdirs (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  fn text,
  mode int(5) default NULL,
  euid int(5) default NULL,
  egid int(5) default NULL,
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip)
) TYPE=MyISAM;

--
-- Table structure for table `mknods`
--

CREATE TABLE mknods (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  fn text,
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip)
) TYPE=MyISAM;

--
-- Table structure for table `mount_replies`
--

CREATE TABLE mount_replies (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  status int(3) NOT NULL default '0',
  fh text,
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip)
) TYPE=MyISAM;

--
-- Table structure for table `mounts`
--

CREATE TABLE mounts (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  fn text,
  euid int(5) default NULL,
  egid int(5) default NULL,
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip)
) TYPE=MyISAM;

--
-- Table structure for table `node_ids`
--

CREATE TABLE node_ids (
  node_id varchar(32) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  pid varchar(32) NOT NULL default '',
  eid varchar(32) NOT NULL default '',
  PRIMARY KEY  (node_ip)
) TYPE=MyISAM;

--
-- Table structure for table `packet_stats`
--

CREATE TABLE packet_stats (
  table_name varchar(32) NOT NULL default '',
  stamp datetime NOT NULL default '0000-00-00 00:00:00',
  packet_count int(10) NOT NULL default '0',
  PRIMARY KEY  (table_name,stamp)
) TYPE=MyISAM;

--
-- Table structure for table `readlink_replies`
--

CREATE TABLE readlink_replies (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  status int(3) NOT NULL default '0',
  fn text,
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip)
) TYPE=MyISAM;

--
-- Table structure for table `readlinks`
--

CREATE TABLE readlinks (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  euid int(5) default NULL,
  egid int(5) default NULL,
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip)
) TYPE=MyISAM;

--
-- Table structure for table `reads`
--

CREATE TABLE reads (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  amount int(10) default NULL,
  euid int(5) default NULL,
  egid int(5) default NULL,
  total int(10) default NULL,
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip),
  KEY timestamp (timestamp)
) TYPE=MyISAM;

--
-- Table structure for table `remove_replies`
--

CREATE TABLE remove_replies (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  status int(3) NOT NULL default '0',
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip),
  KEY timestamp (timestamp,status)
) TYPE=MyISAM;

--
-- Table structure for table `removes`
--

CREATE TABLE removes (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  fn text,
  euid int(5) default NULL,
  egid int(5) default NULL,
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip),
  KEY fn (fn(255))
) TYPE=MyISAM;

--
-- Table structure for table `rename_handle_map`
--

CREATE TABLE rename_handle_map (
  fh varchar(96) NOT NULL default '',
  parent varchar(96) NOT NULL default '',
  fn text NOT NULL,
  timestamp double NOT NULL default '0',
  PRIMARY KEY  (parent,fn(255))
) TYPE=MyISAM;

--
-- Table structure for table `rename_replies`
--

CREATE TABLE rename_replies (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  status int(3) NOT NULL default '0',
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip)
) TYPE=MyISAM;

--
-- Table structure for table `renames`
--

CREATE TABLE renames (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  from_fh varchar(96) NOT NULL default '',
  from_fn text,
  to_fh varchar(96) NOT NULL default '',
  to_fn text,
  euid int(5) default NULL,
  egid int(5) default NULL,
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip)
) TYPE=MyISAM;

--
-- Table structure for table `rmdir_replies`
--

CREATE TABLE rmdir_replies (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  status int(3) NOT NULL default '0',
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip),
  KEY timestamp (timestamp,status)
) TYPE=MyISAM;

--
-- Table structure for table `rmdirs`
--

CREATE TABLE rmdirs (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  fn text,
  euid int(5) default NULL,
  egid int(5) default NULL,
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip),
  KEY fn (fn(255))
) TYPE=MyISAM;

--
-- Table structure for table `temp_handle_map`
--

CREATE TABLE temp_handle_map (
  fh varchar(96) NOT NULL default '',
  parent varchar(96) NOT NULL default '',
  fn text NOT NULL,
  timestamp double NOT NULL default '0',
  valid int(1) NOT NULL default '0',
  PRIMARY KEY  (fh,parent,fn(255)),
  KEY fh (fh),
  KEY parent (parent,fn(255))
) TYPE=MyISAM;

--
-- Table structure for table `writes`
--

CREATE TABLE writes (
  timestamp double NOT NULL default '0',
  id varchar(16) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  amount int(10) default NULL,
  euid int(5) default NULL,
  egid int(5) default NULL,
  total int(10) default NULL,
  PRIMARY KEY  (timestamp,id,node_ip),
  KEY id (id,node_ip),
  KEY timestamp (timestamp)
) TYPE=MyISAM;



-- Packets

CREATE TABLE node_ids (
  node_id varchar(32) NOT NULL default '',
  node_ip varchar(64) NOT NULL default '',
  PRIMARY KEY (node_id, node_ip)
) TYPE=MyISAM;

CREATE TABLE mounts (
  timestamp int(10) NOT NULL,
  id varchar(16) NOT NULL,
  node_ip varchar(64) NOT NULL default '',
  fn text,
  euid int(5),
  egid int(5),
  PRIMARY KEY (timestamp, id, node_ip)
) TYPE=MyISAM;

CREATE TABLE mount_replies (
  timestamp int(10) NOT NULL,
  id varchar(16) NOT NULL,
  node_ip varchar(64) NOT NULL default '',
  status int(3) NOT NULL default 0,
  fh text,
  PRIMARY KEY (timestamp, id, node_ip)
) TYPE=MyISAM;

CREATE TABLE lookups (
  timestamp int(10) NOT NULL,
  id varchar(16) NOT NULL,
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  fn text,
  euid int(5),
  egid int(5),
  PRIMARY KEY (timestamp, id, node_ip)
) TYPE=MyISAM;

CREATE TABLE lookup_replies (
  timestamp int(10) NOT NULL,
  id varchar(16) NOT NULL,
  node_ip varchar(64) NOT NULL default '',
  status int(3) NOT NULL default 0,
  fh varchar(96) NOT NULL default '',
  PRIMARY KEY (timestamp, id, node_ip)
) TYPE=MyISAM;

CREATE TABLE file_checkpoint (
  timestamp int(10) NOT NULL,
  fh varchar(96) NOT NULL default '',
  ftype int(3),
  mode int(5),
  nlink int(5),
  uid int(5),
  gid int(5),
  size int(10),
  blksize int(10),
  rdev int(10),
  blocks int(10),
  fsid int(10),
  fileid int(10),
  atime int(10),
  mtime int(10),
  ctime int(10),
  PRIMARY KEY (timestamp, fh)
) TYPE=MyISAM;

CREATE TABLE reads (
  timestamp int(10) NOT NULL,
  id varchar(16) NOT NULL,
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  amount int(10),
  euid int(5),
  egid int(5),
  PRIMARY KEY (timestamp, id, node_ip),
) TYPE=MyISAM;

CREATE TABLE writes (
  timestamp int(10) NOT NULL,
  id varchar(16) NOT NULL,
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  amount int(10),
  euid int(5),
  egid int(5),
  PRIMARY KEY (timestamp, id, node_ip),
) TYPE=MyISAM;

CREATE TABLE creates (
  timestamp int(10) NOT NULL,
  id varchar(16) NOT NULL,
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  fn text,
  mode int(5),
  euid int(5),
  egid int(5),
  PRIMARY KEY (timestamp, id, node_ip)
) TYPE=MyISAM;

CREATE TABLE create_replies (
  timestamp int(10) NOT NULL,
  id varchar(16) NOT NULL,
  node_ip varchar(64) NOT NULL default '',
  status int(3) NOT NULL default 0,
  fh varchar(96) NOT NULL default '',
  PRIMARY KEY (timestamp, id, node_ip)
) TYPE=MyISAM;

CREATE TABLE mkdirs (
  timestamp int(10) NOT NULL,
  id varchar(16) NOT NULL,
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  fn text,
  PRIMARY KEY (timestamp, id, node_ip)
) TYPE=MyISAM;

CREATE TABLE mknods (
  timestamp int(10) NOT NULL,
  id varchar(16) NOT NULL,
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  fn text,
  PRIMARY KEY (timestamp, id, node_ip)
) TYPE=MyISAM;

CREATE TABLE removes (
  timestamp int(10) NOT NULL,
  id varchar(16) NOT NULL,
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  fn text,
  euid int(5),
  egid int(5),
  PRIMARY KEY (timestamp, id, node_ip)
) TYPE=MyISAM;

CREATE TABLE rmdirs (
  timestamp int(10) NOT NULL,
  id varchar(16) NOT NULL,
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  fn text,
  PRIMARY KEY (timestamp, id, node_ip)
) TYPE=MyISAM;

CREATE TABLE renames (
  timestamp int(10) NOT NULL,
  id varchar(16) NOT NULL,
  node_ip varchar(64) NOT NULL default '',
  from_fh varchar(96) NOT NULL default '',
  from_fn text,
  to_fh varchar(96) NOT NULL default '',
  to_fn text,
  PRIMARY KEY (timestamp, id, node_ip)
) TYPE=MyISAM;

CREATE TABLE handle_map (
  fh varchar(96) NOT NULL default '',
  complete int(1),
  fn text,
  PRIMARY KEY (fh),
  KEY (fn(255))
) TYPE=MyISAM;

CREATE TABLE file_access (
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  last_access int(10) NOT NULL,
  PRIMARY KEY (node_ip, fh)
) TYPE=MyISAM;

CREATE TABLE file_dropped (
  node_ip varchar(64) NOT NULL default '',
  fh varchar(96) NOT NULL default '',
  last_remove int(10) NOT NULL,
  PRIMARY KEY (node_ip, fh)
) TYPE=MyISAM;

--
-- SQL extra defs for the DB on ops. See ops-install ...
--

CREATE TABLE IF NOT EXISTS emulab_dbs (
  dbname varchar(64) NOT NULL default '',
  PRIMARY KEY  (dbname)
) TYPE=MyISAM;

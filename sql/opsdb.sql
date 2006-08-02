--
-- SQL extra defs for the DB on ops. See ops-install ...
--

CREATE TABLE IF NOT EXISTS emulab_dbs (
  dbname varchar(64) NOT NULL default '',
  PRIMARY KEY  (dbname)
) TYPE=MyISAM;

alter table emulab_dbs add date_created datetime default NULL;
update emulab_dbs set date_created=now();
alter table emulab_dbs add temporary tinyint(1) NOT NULL default '0';

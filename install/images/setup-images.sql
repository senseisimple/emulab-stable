-- MySQL dump 8.23
--
-- Host: localhost    Database: tbdb
---------------------------------------------------------
-- Server version	3.23.58-log

--
-- Table structure for table `images`
--

DROP TABLE IF EXISTS `images`;
CREATE TABLE `images` (
  `imagename` varchar(30) NOT NULL default '',
  `pid_idx` mediumint(8) unsigned NOT NULL default '0',
  `gid_idx` mediumint(8) unsigned NOT NULL default '0',
  `pid` varchar(12) NOT NULL default '',
  `gid` varchar(12) NOT NULL default '',
  `imageid` varchar(45) NOT NULL default '',
  `creator` varchar(8) default NULL,
  `creator_idx` mediumint(8) unsigned NOT NULL default '0',
  `created` datetime default NULL,
  `description` tinytext NOT NULL,
  `loadpart` tinyint(4) NOT NULL default '0',
  `loadlength` tinyint(4) NOT NULL default '0',
  `part1_osid` varchar(35) default NULL,
  `part2_osid` varchar(35) default NULL,
  `part3_osid` varchar(35) default NULL,
  `part4_osid` varchar(35) default NULL,
  `default_osid` varchar(35) NOT NULL default '',
  `path` tinytext,
  `magic` tinytext,
  `load_address` text,
  `frisbee_pid` int(11) default '0',
  `load_busy` tinyint(4) NOT NULL default '0',
  `ezid` tinyint(4) NOT NULL default '0',
  `shared` tinyint(4) NOT NULL default '0',
  `global` tinyint(4) NOT NULL default '0',
  `updated` datetime default NULL,
  PRIMARY KEY  (`imageid`),
  UNIQUE KEY `pid` (`pid`,`imagename`),
  KEY `gid` (`gid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `images`
--
-- WHERE:  imagename like '%-STD%'


INSERT INTO images VALUES ('FBSD410+RHL90-STD',1,1,'emulab-ops','emulab-ops','emulab-ops-FBSD410+RHL90-STD','hibler','2004-11-09 00:00:00','Latest full disk image with FreeBSD 4.10 and RedHat 9.0',0,4,'emulab-ops-FBSD410-STD','emulab-ops-RHL90-STD',NULL,NULL,'emulab-ops-FBSD410-STD','/usr/testbed/images/FBSD410+RHL90-STD.ndz',NULL,'',0,1,0,0,1,NULL);
INSERT INTO images VALUES ('FBSD410-STD',1,1,'emulab-ops','emulab-ops','emulab-ops-FBSD410-STD','hibler','2004-11-09 00:00:00','Testbed version of FreeBSD 4.10',1,1,'emulab-ops-FBSD410-STD',NULL,NULL,NULL,'emulab-ops-FBSD410-STD','/usr/testbed/images/FBSD410-STD.ndz',NULL,'',0,0,0,0,1,NULL);
INSERT INTO images VALUES ('RHL90-STD',1,1,'emulab-ops','emulab-ops','emulab-ops-RHL90-STD','hibler','2004-11-09 00:00:00','Testbed version of RedHat Linux 9.0',2,1,NULL,'emulab-ops-RHL90-STD',NULL,NULL,'emulab-ops-RHL90-STD','/usr/testbed/images/RHL90-STD.ndz',NULL,'',0,0,0,0,1,NULL);


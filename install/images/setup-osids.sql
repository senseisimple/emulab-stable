-- MySQL dump 8.23
--
-- Host: localhost    Database: tbdb
---------------------------------------------------------
-- Server version	3.23.58-log

--
-- Table structure for table `os_info`
--

DROP TABLE IF EXISTS `os_info`;
CREATE TABLE `os_info` (
  `osname` varchar(20) NOT NULL default '',
  `pid` varchar(12) NOT NULL default '',
  `pid_idx` mediumint(8) unsigned NOT NULL default '0',
  `osid` varchar(35) NOT NULL default '',
  `creator` varchar(8) default NULL,
  `creator_idx` mediumint(8) unsigned NOT NULL default '0',
  `created` datetime default NULL,
  `description` tinytext NOT NULL,
  `OS` enum('Unknown','Linux','Fedora','FreeBSD','NetBSD','OSKit','Windows','TinyOS','Other') default 'Unknown',
  `version` varchar(12) default '',
  `path` tinytext,
  `magic` tinytext,
  `machinetype` varchar(30) NOT NULL default '',
  `osfeatures` set('ping','ssh','ipod','isup','veths','mlinks','linktest','linkdelays') default NULL,
  `ezid` tinyint(4) NOT NULL default '0',
  `shared` tinyint(4) NOT NULL default '0',
  `mustclean` tinyint(4) NOT NULL default '1',
  `op_mode` varchar(20) NOT NULL default 'MINIMAL',
  `nextosid` varchar(35) default NULL,
  `max_concurrent` int(11) default NULL,
  `mfs` tinyint(4) NOT NULL default '0',
  `reboot_waittime` int(10) unsigned default NULL,
  PRIMARY KEY  (`osid`),
  UNIQUE KEY `pid` (`pid`,`osname`),
  KEY `OS` (`OS`),
  KEY `path` (`path`(255))
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `os_info`
--
-- WHERE:  osid like '%-STD%' or osid like '%-JAIL%'


INSERT INTO os_info VALUES ('RHL-STD','emulab-ops',1,'emulab-ops-RHL-STD','root',NULL,'Default version of RedHat Linux','Linux','',NULL,'','','ping,ssh,ipod,isup',0,1,1,'NORMALv2','emulab-ops-RHL90-STD',NULL,0,150);
INSERT INTO os_info VALUES ('FBSD-STD','emulab-ops',1,'emulab-ops-FBSD-STD','root',NULL,'Default version of FreeBSD','FreeBSD','',NULL,'','','ping,ssh,ipod,isup,veths',0,1,1,'NORMALv2','emulab-ops-FBSD410-STD',NULL,0,150);
INSERT INTO os_info VALUES ('FBSD-JAIL','emulab-ops',1,'emulab-ops-FBSD-JAIL','root',NULL,'Generic OSID for jailed nodes','FreeBSD','',NULL,'','','ping,ssh,isup',0,1,0,'PCVM','emulab-ops-FBSD-STD',NULL,0,150);
INSERT INTO os_info VALUES ('FBSD410-STD','emulab-ops',1,'emulab-ops-FBSD410-STD','root','2004-11-09 00:00:00','Testbed version of FreeBSD 4.10','FreeBSD','4.10',NULL,'','','ping,ssh,ipod,isup,veths',0,1,1,'NORMALv2',NULL,NULL,0,150);
INSERT INTO os_info VALUES ('RHL90-STD','emulab-ops',1,'emulab-ops-RHL90-STD','root','2004-11-09 00:00:00','Testbed version of RedHat Linux 9.0','Linux','9.0',NULL,'','','ping,ssh,ipod,isup',0,1,1,'NORMALv2',NULL,NULL,0,150);


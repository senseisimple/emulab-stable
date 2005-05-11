-- MySQL dump 8.23
--
-- Host: localhost    Database: tbdb
---------------------------------------------------------
-- Server version       3.23.58-log

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
  OS enum('Unknown','Linux','FreeBSD','NetBSD','OSKit','Windows','TinyOS','Other') default 'Unknown',
  version varchar(12) default '',
  path tinytext,
  magic tinytext,
  machinetype varchar(30) NOT NULL default '',
  osfeatures set('ping','ssh','ipod','isup','veths','mlinks') default NULL,
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
-- Dumping data for table `os_info`
--
-- WHERE:  osid like "%MFS%"


INSERT INTO os_info VALUES ('FREEBSD-MFS','emulab-ops','FREEBSD-MFS','root',NULL,'FreeBSD in an MFS','FreeBSD','4.10','boss:/tftpboot/freebsd',NULL,'','ping,ssh,ipod,isup',0,1,0,'PXEFBSD',NULL,NULL,1,150);
INSERT INTO os_info VALUES ('FRISBEE-MFS','emulab-ops','FRISBEE-MFS','root',NULL,'Frisbee (FreeBSD) in an MFS','FreeBSD','4.10','boss:/tftpboot/frisbee',NULL,'','ping,ssh,ipod,isup',0,1,0,'RELOAD',NULL,NULL,1,150);
INSERT INTO os_info VALUES ('NEWNODE-MFS','emulab-ops','NEWNODE-MFS','root',NULL,'NewNode (FreeBSD) in an MFS','FreeBSD','4.10','boss:/tftpboot/freebsd.newnode',NULL,'','ping,ssh,ipod,isup',0,1,0,'PXEFBSD',NULL,NULL,1,150);

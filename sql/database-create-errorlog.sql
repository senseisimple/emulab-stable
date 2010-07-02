-- MySQL dump 10.10
--
-- Host: localhost    Database: errorlog
-- ------------------------------------------------------
-- Server version	5.0.20-log

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `causes`
--

DROP TABLE IF EXISTS `causes`;
CREATE TABLE `causes` (
  `cause` varchar(16) NOT NULL default '',
  `cause_desc` varchar(32) NOT NULL default '',
  PRIMARY KEY  (`cause`),
  UNIQUE KEY `cause_desc` (`cause_desc`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `errors`
--

DROP TABLE IF EXISTS `errors`;
CREATE TABLE `errors` (
  `session` int(10) unsigned NOT NULL default '0',
  `rank` tinyint(1) NOT NULL default '0',
  `stamp` int(10) unsigned NOT NULL default '0',
  `exptidx` int(11) NOT NULL default '0',
  `script` smallint(3) NOT NULL default '0',
  `cause` varchar(16) NOT NULL default '',
  `confidence` float NOT NULL default '0',
  `inferred` tinyint(1) default NULL,
  `need_more_info` tinyint(1) default NULL,
  `mesg` text NOT NULL,
  `tblog_revision` varchar(8) NOT NULL,
  PRIMARY KEY  (`session`,`rank`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiments`
--

DROP TABLE IF EXISTS `experiments`;
CREATE TABLE `experiments` (
  `idx` int(10) unsigned NOT NULL default '0',
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  PRIMARY KEY  (`idx`),
  KEY `eid` (`eid`,`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `log`
--

DROP TABLE IF EXISTS `log`;
CREATE TABLE `log` (
  `seq` int(10) unsigned NOT NULL default '0',
  `stamp` int(10) unsigned NOT NULL default '0',
  `session` int(10) unsigned NOT NULL default '0',
  `attempt` tinyint(1) NOT NULL default '0',
  `cleanup` tinyint(1) NOT NULL default '0',
  `invocation` int(10) unsigned NOT NULL default '0',
  `parent` int(10) unsigned NOT NULL default '0',
  `script` smallint(3) NOT NULL default '0',
  `level` tinyint(2) NOT NULL default '0',
  `sublevel` tinyint(2) NOT NULL default '0',
  `priority` smallint(3) NOT NULL default '0',
  `inferred` tinyint(1) NOT NULL default '0',
  `cause` varchar(16) NOT NULL default '',
  `type` enum('normal','entering','exiting','thecause','extra','summary','primary','secondary') default 'normal',
  `relevant` tinyint(1) NOT NULL default '0',
  `mesg` text NOT NULL,
  PRIMARY KEY  (`seq`),
  KEY `session` (`session`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `priorities`
--

DROP TABLE IF EXISTS `priorities`;
CREATE TABLE `priorities` (
  `priority` smallint(3) NOT NULL default '0',
  `priority_name` varchar(8) NOT NULL default '',
  PRIMARY KEY  (`priority`),
  UNIQUE KEY `name` (`priority_name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `report_assign_violation`
--

DROP TABLE IF EXISTS `report_assign_violation`;
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
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `report_context`
--

DROP TABLE IF EXISTS `report_context`;
CREATE TABLE `report_context` (
  `seq` int(10) unsigned NOT NULL,
  `i0` int(11) default NULL,
  `i1` int(11) default NULL,
  `i2` int(11) default NULL,
  `vc0` varchar(255) default NULL,
  `vc1` varchar(255) default NULL,
  `vc2` varchar(255) default NULL,
  PRIMARY KEY  (`seq`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `report_error`
--

DROP TABLE IF EXISTS `report_error`;
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
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `scripts`
--

DROP TABLE IF EXISTS `scripts`;
CREATE TABLE `scripts` (
  `script` smallint(3) NOT NULL auto_increment,
  `script_name` varchar(24) NOT NULL default '',
  PRIMARY KEY  (`script`),
  UNIQUE KEY `id` (`script_name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `session_info`
--

DROP TABLE IF EXISTS `session_info`;
CREATE TABLE `session_info` (
  `session` int(11) NOT NULL default '0',
  `uid` int(11) NOT NULL default '0',
  `exptidx` int(11) NOT NULL default '0',
  PRIMARY KEY  (`session`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `users`
--

DROP TABLE IF EXISTS `users`;
CREATE TABLE `users` (
  `uid` varchar(8) NOT NULL default '',
  `unix_uid` smallint(5) unsigned NOT NULL default '0',
  PRIMARY KEY  (`uid`),
  KEY `unix_uid` (`unix_uid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;


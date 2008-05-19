-- MySQL dump 10.10
--
-- Host: localhost    Database: tbdb
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
-- Table structure for table `accessed_files`
--

DROP TABLE IF EXISTS `accessed_files`;
CREATE TABLE `accessed_files` (
  `fn` text NOT NULL,
  `idx` int(11) unsigned NOT NULL auto_increment,
  PRIMARY KEY  (`fn`(255)),
  KEY `idx` (`idx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `active_checkups`
--

DROP TABLE IF EXISTS `active_checkups`;
CREATE TABLE `active_checkups` (
  `object` varchar(128) NOT NULL default '',
  `object_type` varchar(64) NOT NULL default '',
  `type` varchar(64) NOT NULL default '',
  `state` varchar(16) NOT NULL default 'new',
  `start` datetime default NULL,
  PRIMARY KEY  (`object`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `archive_revisions`
--

DROP TABLE IF EXISTS `archive_revisions`;
CREATE TABLE `archive_revisions` (
  `archive_idx` int(10) unsigned NOT NULL default '0',
  `revision` int(10) unsigned NOT NULL auto_increment,
  `parent_revision` int(10) unsigned default NULL,
  `tag` varchar(64) NOT NULL default '',
  `view` varchar(64) NOT NULL default '',
  `date_created` int(10) unsigned NOT NULL default '0',
  `converted` tinyint(1) default '0',
  `description` text,
  PRIMARY KEY  (`archive_idx`,`revision`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `archive_tags`
--

DROP TABLE IF EXISTS `archive_tags`;
CREATE TABLE `archive_tags` (
  `idx` int(10) unsigned NOT NULL auto_increment,
  `tag` varchar(64) NOT NULL default '',
  `archive_idx` int(10) unsigned NOT NULL default '0',
  `view` varchar(64) NOT NULL default '',
  `date_created` int(10) unsigned NOT NULL default '0',
  `tagtype` enum('user','commit','savepoint','internal') NOT NULL default 'internal',
  `version` tinyint(1) default '0',
  `description` text,
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `tag` (`tag`,`archive_idx`,`view`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `archive_views`
--

DROP TABLE IF EXISTS `archive_views`;
CREATE TABLE `archive_views` (
  `view` varchar(64) NOT NULL default '',
  `archive_idx` int(10) unsigned NOT NULL default '0',
  `revision` int(10) unsigned default NULL,
  `current_tag` varchar(64) default NULL,
  `previous_tag` varchar(64) default NULL,
  `date_created` int(10) unsigned NOT NULL default '0',
  `branch_tag` varchar(64) default NULL,
  `parent_view` varchar(64) default NULL,
  `parent_revision` int(10) unsigned default NULL,
  PRIMARY KEY  (`view`,`archive_idx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `archives`
--

DROP TABLE IF EXISTS `archives`;
CREATE TABLE `archives` (
  `idx` int(10) unsigned NOT NULL auto_increment,
  `directory` tinytext,
  `date_created` int(10) unsigned NOT NULL default '0',
  `archived` tinyint(1) default '0',
  `date_archived` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`idx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `buildings`
--

DROP TABLE IF EXISTS `buildings`;
CREATE TABLE `buildings` (
  `building` varchar(32) NOT NULL default '',
  `image_path` tinytext,
  `title` tinytext NOT NULL,
  PRIMARY KEY  (`building`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `cameras`
--

DROP TABLE IF EXISTS `cameras`;
CREATE TABLE `cameras` (
  `name` varchar(32) NOT NULL default '',
  `building` varchar(32) NOT NULL default '',
  `floor` varchar(32) NOT NULL default '',
  `hostname` varchar(255) default NULL,
  `port` smallint(5) unsigned NOT NULL default '6100',
  `device` varchar(64) NOT NULL default '',
  `loc_x` float NOT NULL default '0',
  `loc_y` float NOT NULL default '0',
  `width` float NOT NULL default '0',
  `height` float NOT NULL default '0',
  `config` tinytext,
  `fixed_x` float NOT NULL default '0',
  `fixed_y` float NOT NULL default '0',
  PRIMARY KEY  (`name`,`building`,`floor`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

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
-- Table structure for table `cdroms`
--

DROP TABLE IF EXISTS `cdroms`;
CREATE TABLE `cdroms` (
  `cdkey` varchar(64) NOT NULL default '',
  `user_name` tinytext NOT NULL,
  `user_email` tinytext NOT NULL,
  `ready` tinyint(4) NOT NULL default '0',
  `requested` datetime NOT NULL default '0000-00-00 00:00:00',
  `created` datetime NOT NULL default '0000-00-00 00:00:00',
  `version` int(10) unsigned NOT NULL default '1',
  PRIMARY KEY  (`cdkey`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `checkup_types`
--

DROP TABLE IF EXISTS `checkup_types`;
CREATE TABLE `checkup_types` (
  `object_type` varchar(64) NOT NULL default '',
  `checkup_type` varchar(64) NOT NULL default '',
  `major_type` varchar(64) NOT NULL default '',
  `expiration` int(10) NOT NULL default '86400',
  PRIMARY KEY  (`object_type`,`checkup_type`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `checkups`
--

DROP TABLE IF EXISTS `checkups`;
CREATE TABLE `checkups` (
  `object` varchar(128) NOT NULL default '',
  `object_type` varchar(64) NOT NULL default '',
  `type` varchar(64) NOT NULL default '',
  `next` datetime default NULL,
  PRIMARY KEY  (`object`,`type`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `checkups_temp`
--

DROP TABLE IF EXISTS `checkups_temp`;
CREATE TABLE `checkups_temp` (
  `object` varchar(128) NOT NULL default '',
  `object_type` varchar(64) NOT NULL default '',
  `type` varchar(64) NOT NULL default '',
  `next` datetime default NULL,
  PRIMARY KEY  (`object`,`type`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `comments`
--

DROP TABLE IF EXISTS `comments`;
CREATE TABLE `comments` (
  `table_name` varchar(64) NOT NULL default '',
  `column_name` varchar(64) NOT NULL default '',
  `description` text NOT NULL,
  UNIQUE KEY `table_name` (`table_name`,`column_name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `current_reloads`
--

DROP TABLE IF EXISTS `current_reloads`;
CREATE TABLE `current_reloads` (
  `node_id` varchar(32) NOT NULL default '',
  `image_id` int(8) unsigned NOT NULL default '0',
  `mustwipe` tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `daily_stats`
--

DROP TABLE IF EXISTS `daily_stats`;
CREATE TABLE `daily_stats` (
  `theday` date NOT NULL default '0000-00-00',
  `exptstart_count` int(11) unsigned default '0',
  `exptpreload_count` int(11) unsigned default '0',
  `exptswapin_count` int(11) unsigned default '0',
  `exptswapout_count` int(11) unsigned default '0',
  `exptswapmod_count` int(11) unsigned default '0',
  `allexpt_duration` int(11) unsigned default '0',
  `allexpt_vnodes` int(11) unsigned default '0',
  `allexpt_vnode_duration` int(11) unsigned default '0',
  `allexpt_pnodes` int(11) unsigned default '0',
  `allexpt_pnode_duration` int(11) unsigned default '0',
  PRIMARY KEY  (`theday`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `datapository_databases`
--

DROP TABLE IF EXISTS `datapository_databases`;
CREATE TABLE `datapository_databases` (
  `dbname` varchar(64) NOT NULL default '',
  `pid` varchar(12) NOT NULL default '',
  `gid` varchar(16) NOT NULL default '',
  `uid` varchar(8) NOT NULL default '',
  `pid_idx` mediumint(8) unsigned NOT NULL default '0',
  `gid_idx` mediumint(8) unsigned NOT NULL default '0',
  `uid_idx` mediumint(8) unsigned NOT NULL default '0',
  `created` datetime default NULL,
  PRIMARY KEY  (`dbname`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `default_firewall_rules`
--

DROP TABLE IF EXISTS `default_firewall_rules`;
CREATE TABLE `default_firewall_rules` (
  `type` enum('ipfw','ipfw2','ipchains','ipfw2-vlan') NOT NULL default 'ipfw',
  `style` enum('open','closed','basic','emulab') NOT NULL default 'basic',
  `enabled` tinyint(4) NOT NULL default '0',
  `ruleno` int(10) unsigned NOT NULL default '0',
  `rule` text NOT NULL,
  PRIMARY KEY  (`type`,`style`,`ruleno`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `default_firewall_vars`
--

DROP TABLE IF EXISTS `default_firewall_vars`;
CREATE TABLE `default_firewall_vars` (
  `name` varchar(255) NOT NULL default '',
  `value` text,
  PRIMARY KEY  (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `delays`
--

DROP TABLE IF EXISTS `delays`;
CREATE TABLE `delays` (
  `node_id` varchar(32) NOT NULL default '',
  `pipe0` smallint(5) unsigned NOT NULL default '0',
  `delay0` float(10,2) NOT NULL default '0.00',
  `bandwidth0` int(10) unsigned NOT NULL default '100',
  `lossrate0` float(10,3) NOT NULL default '0.000',
  `q0_limit` int(11) default '0',
  `q0_maxthresh` int(11) default '0',
  `q0_minthresh` int(11) default '0',
  `q0_weight` float default '0',
  `q0_linterm` int(11) default '0',
  `q0_qinbytes` tinyint(4) default '0',
  `q0_bytes` tinyint(4) default '0',
  `q0_meanpsize` int(11) default '0',
  `q0_wait` int(11) default '0',
  `q0_setbit` int(11) default '0',
  `q0_droptail` int(11) default '0',
  `q0_red` tinyint(4) default '0',
  `q0_gentle` tinyint(4) default '0',
  `pipe1` smallint(5) unsigned NOT NULL default '0',
  `delay1` float(10,2) NOT NULL default '0.00',
  `bandwidth1` int(10) unsigned NOT NULL default '100',
  `lossrate1` float(10,3) NOT NULL default '0.000',
  `q1_limit` int(11) default '0',
  `q1_maxthresh` int(11) default '0',
  `q1_minthresh` int(11) default '0',
  `q1_weight` float default '0',
  `q1_linterm` int(11) default '0',
  `q1_qinbytes` tinyint(4) default '0',
  `q1_bytes` tinyint(4) default '0',
  `q1_meanpsize` int(11) default '0',
  `q1_wait` int(11) default '0',
  `q1_setbit` int(11) default '0',
  `q1_droptail` int(11) default '0',
  `q1_red` tinyint(4) default '0',
  `q1_gentle` tinyint(4) default '0',
  `iface0` varchar(8) NOT NULL default '',
  `iface1` varchar(8) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `eid` varchar(32) default NULL,
  `pid` varchar(32) default NULL,
  `vname` varchar(32) default NULL,
  `vnode0` varchar(32) default NULL,
  `vnode1` varchar(32) default NULL,
  `card0` tinyint(3) unsigned default NULL,
  `card1` tinyint(3) unsigned default NULL,
  `noshaping` tinyint(1) default '0',
  PRIMARY KEY  (`node_id`,`iface0`,`iface1`),
  KEY `pid` (`pid`,`eid`),
  KEY `exptidx` (`exptidx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `deleted_users`
--

DROP TABLE IF EXISTS `deleted_users`;
CREATE TABLE `deleted_users` (
  `uid` varchar(8) NOT NULL default '',
  `uid_idx` mediumint(8) unsigned NOT NULL default '0',
  `usr_created` datetime default NULL,
  `usr_deleted` datetime default NULL,
  `usr_name` tinytext,
  `usr_title` tinytext,
  `usr_affil` tinytext,
  `usr_affil_abbrev` varchar(16) default NULL,
  `usr_email` tinytext,
  `usr_URL` tinytext,
  `usr_addr` tinytext,
  `usr_addr2` tinytext,
  `usr_city` tinytext,
  `usr_state` tinytext,
  `usr_zip` tinytext,
  `usr_country` tinytext,
  `usr_phone` tinytext,
  `webonly` tinyint(1) default '0',
  `wikionly` tinyint(1) default '0',
  `notes` text,
  PRIMARY KEY  (`uid_idx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `delta_inst`
--

DROP TABLE IF EXISTS `delta_inst`;
CREATE TABLE `delta_inst` (
  `node_id` varchar(32) NOT NULL default '',
  `partition` tinyint(4) NOT NULL default '0',
  `delta_id` varchar(10) NOT NULL default '',
  PRIMARY KEY  (`node_id`,`partition`,`delta_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `delta_proj`
--

DROP TABLE IF EXISTS `delta_proj`;
CREATE TABLE `delta_proj` (
  `delta_id` varchar(10) NOT NULL default '',
  `pid` varchar(10) NOT NULL default '',
  PRIMARY KEY  (`delta_id`,`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `deltas`
--

DROP TABLE IF EXISTS `deltas`;
CREATE TABLE `deltas` (
  `delta_id` varchar(10) NOT NULL default '',
  `delta_desc` text,
  `delta_path` text NOT NULL,
  `private` enum('yes','no') NOT NULL default 'no',
  PRIMARY KEY  (`delta_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `elabinelab_vlans`
--

DROP TABLE IF EXISTS `elabinelab_vlans`;
CREATE TABLE `elabinelab_vlans` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `inner_id` int(11) unsigned NOT NULL default '0',
  `outer_id` int(11) unsigned NOT NULL default '0',
  PRIMARY KEY  (`exptidx`,`inner_id`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`inner_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `emulab_indicies`
--

DROP TABLE IF EXISTS `emulab_indicies`;
CREATE TABLE `emulab_indicies` (
  `name` varchar(64) NOT NULL default '',
  `idx` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`name`)
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
  `tblog_revision` varchar(8) NOT NULL default '',
  PRIMARY KEY  (`session`,`rank`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `event_eventtypes`
--

DROP TABLE IF EXISTS `event_eventtypes`;
CREATE TABLE `event_eventtypes` (
  `idx` smallint(5) unsigned NOT NULL default '0',
  `type` tinytext NOT NULL,
  PRIMARY KEY  (`idx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `event_groups`
--

DROP TABLE IF EXISTS `event_groups`;
CREATE TABLE `event_groups` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `idx` int(10) unsigned NOT NULL auto_increment,
  `group_name` varchar(64) NOT NULL default '',
  `agent_name` varchar(64) NOT NULL default '',
  PRIMARY KEY  (`exptidx`,`idx`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`idx`),
  KEY `group_name` (`group_name`),
  KEY `agent_name` (`agent_name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `event_objecttypes`
--

DROP TABLE IF EXISTS `event_objecttypes`;
CREATE TABLE `event_objecttypes` (
  `idx` smallint(5) unsigned NOT NULL default '0',
  `type` tinytext NOT NULL,
  PRIMARY KEY  (`idx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `eventlist`
--

DROP TABLE IF EXISTS `eventlist`;
CREATE TABLE `eventlist` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `idx` int(10) unsigned NOT NULL auto_increment,
  `time` float(10,3) NOT NULL default '0.000',
  `vnode` varchar(32) NOT NULL default '',
  `vname` varchar(64) NOT NULL default '',
  `objecttype` smallint(5) unsigned NOT NULL default '0',
  `eventtype` smallint(5) unsigned NOT NULL default '0',
  `isgroup` tinyint(1) unsigned default '0',
  `arguments` text,
  `atstring` text,
  `parent` varchar(64) NOT NULL default '',
  PRIMARY KEY  (`exptidx`,`idx`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`idx`),
  KEY `vnode` (`vnode`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_input_data`
--

DROP TABLE IF EXISTS `experiment_input_data`;
CREATE TABLE `experiment_input_data` (
  `idx` int(10) unsigned NOT NULL auto_increment,
  `md5` varchar(32) NOT NULL default '',
  `compressed` tinyint(1) unsigned default '0',
  `input` mediumblob,
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `md5` (`md5`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_inputs`
--

DROP TABLE IF EXISTS `experiment_inputs`;
CREATE TABLE `experiment_inputs` (
  `rsrcidx` int(10) unsigned NOT NULL default '0',
  `exptidx` int(10) unsigned NOT NULL default '0',
  `input_data_idx` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`rsrcidx`,`input_data_idx`),
  KEY `rsrcidx` (`rsrcidx`),
  KEY `exptidx` (`exptidx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_pmapping`
--

DROP TABLE IF EXISTS `experiment_pmapping`;
CREATE TABLE `experiment_pmapping` (
  `rsrcidx` int(10) unsigned NOT NULL default '0',
  `vname` varchar(32) NOT NULL default '',
  `node_id` varchar(32) NOT NULL default '',
  `node_type` varchar(30) NOT NULL default '',
  `node_erole` varchar(30) NOT NULL default '',
  PRIMARY KEY  (`rsrcidx`,`vname`,`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_resources`
--

DROP TABLE IF EXISTS `experiment_resources`;
CREATE TABLE `experiment_resources` (
  `idx` int(10) unsigned NOT NULL auto_increment,
  `exptidx` int(10) unsigned NOT NULL default '0',
  `lastidx` int(10) unsigned default NULL,
  `tstamp` datetime default NULL,
  `uid_idx` mediumint(8) unsigned NOT NULL default '0',
  `swapin_time` int(10) unsigned NOT NULL default '0',
  `swapout_time` int(10) unsigned NOT NULL default '0',
  `swapmod_time` int(10) unsigned NOT NULL default '0',
  `byswapmod` tinyint(1) unsigned default '0',
  `byswapin` tinyint(1) unsigned default '0',
  `vnodes` smallint(5) unsigned default '0',
  `pnodes` smallint(5) unsigned default '0',
  `wanodes` smallint(5) unsigned default '0',
  `plabnodes` smallint(5) unsigned default '0',
  `simnodes` smallint(5) unsigned default '0',
  `jailnodes` smallint(5) unsigned default '0',
  `delaynodes` smallint(5) unsigned default '0',
  `linkdelays` smallint(5) unsigned default '0',
  `walinks` smallint(5) unsigned default '0',
  `links` smallint(5) unsigned default '0',
  `lans` smallint(5) unsigned default '0',
  `shapedlinks` smallint(5) unsigned default '0',
  `shapedlans` smallint(5) unsigned default '0',
  `wirelesslans` smallint(5) unsigned default '0',
  `minlinks` tinyint(3) unsigned default '0',
  `maxlinks` tinyint(3) unsigned default '0',
  `delay_capacity` tinyint(3) unsigned default NULL,
  `batchmode` tinyint(1) unsigned default '0',
  `archive_tag` varchar(64) default NULL,
  `input_data_idx` int(10) unsigned default NULL,
  `thumbnail` mediumblob,
  PRIMARY KEY  (`idx`),
  KEY `exptidx` (`exptidx`),
  KEY `lastidx` (`lastidx`),
  KEY `inputdata` (`input_data_idx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_run_bindings`
--

DROP TABLE IF EXISTS `experiment_run_bindings`;
CREATE TABLE `experiment_run_bindings` (
  `exptidx` int(10) unsigned NOT NULL default '0',
  `runidx` int(10) unsigned NOT NULL default '0',
  `name` varchar(64) NOT NULL default '',
  `value` tinytext NOT NULL,
  PRIMARY KEY  (`exptidx`,`runidx`,`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_runs`
--

DROP TABLE IF EXISTS `experiment_runs`;
CREATE TABLE `experiment_runs` (
  `exptidx` int(10) unsigned NOT NULL default '0',
  `idx` int(10) unsigned NOT NULL auto_increment,
  `runid` varchar(32) NOT NULL default '',
  `description` tinytext,
  `starting_archive_tag` varchar(64) default NULL,
  `ending_archive_tag` varchar(64) default NULL,
  `archive_tag` varchar(64) default NULL,
  `start_time` datetime default NULL,
  `stop_time` datetime default NULL,
  `swapmod` tinyint(1) NOT NULL default '0',
  `hidden` tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (`exptidx`,`idx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_stats`
--

DROP TABLE IF EXISTS `experiment_stats`;
CREATE TABLE `experiment_stats` (
  `pid` varchar(12) NOT NULL default '',
  `pid_idx` mediumint(8) unsigned NOT NULL default '0',
  `eid` varchar(32) NOT NULL default '',
  `eid_uuid` varchar(40) NOT NULL default '',
  `creator` varchar(8) NOT NULL default '',
  `creator_idx` mediumint(8) unsigned NOT NULL default '0',
  `exptidx` int(10) unsigned NOT NULL default '0',
  `rsrcidx` int(10) unsigned NOT NULL default '0',
  `lastrsrc` int(10) unsigned default NULL,
  `gid` varchar(16) NOT NULL default '',
  `gid_idx` mediumint(8) unsigned NOT NULL default '0',
  `created` datetime default NULL,
  `destroyed` datetime default NULL,
  `last_activity` datetime default NULL,
  `swapin_count` smallint(5) unsigned default '0',
  `swapin_last` datetime default NULL,
  `swapout_count` smallint(5) unsigned default '0',
  `swapout_last` datetime default NULL,
  `swapmod_count` smallint(5) unsigned default '0',
  `swapmod_last` datetime default NULL,
  `swap_errors` smallint(5) unsigned default '0',
  `swap_exitcode` tinyint(3) unsigned default '0',
  `idle_swaps` smallint(5) unsigned default '0',
  `swapin_duration` int(10) unsigned default '0',
  `batch` tinyint(3) unsigned default '0',
  `elabinelab` tinyint(1) NOT NULL default '0',
  `elabinelab_exptidx` int(10) unsigned default NULL,
  `security_level` tinyint(1) NOT NULL default '0',
  `archive_idx` int(10) unsigned default NULL,
  `last_error` int(10) unsigned default NULL,
  `dpdbname` varchar(64) default NULL,
  PRIMARY KEY  (`exptidx`),
  KEY `rsrcidx` (`rsrcidx`),
  KEY `pideid` (`pid`,`eid`),
  KEY `eid_uuid` (`eid_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_template_events`
--

DROP TABLE IF EXISTS `experiment_template_events`;
CREATE TABLE `experiment_template_events` (
  `parent_guid` varchar(16) NOT NULL default '',
  `parent_vers` smallint(5) unsigned NOT NULL default '0',
  `vname` varchar(64) NOT NULL default '',
  `vnode` varchar(32) NOT NULL default '',
  `time` float(10,3) NOT NULL default '0.000',
  `objecttype` smallint(5) unsigned NOT NULL default '0',
  `eventtype` smallint(5) unsigned NOT NULL default '0',
  `arguments` text,
  PRIMARY KEY  (`parent_guid`,`parent_vers`,`vname`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_template_graphs`
--

DROP TABLE IF EXISTS `experiment_template_graphs`;
CREATE TABLE `experiment_template_graphs` (
  `parent_guid` varchar(16) NOT NULL default '',
  `scale` float(10,3) NOT NULL default '1.000',
  `image` mediumblob,
  `imap` mediumtext,
  PRIMARY KEY  (`parent_guid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_template_input_data`
--

DROP TABLE IF EXISTS `experiment_template_input_data`;
CREATE TABLE `experiment_template_input_data` (
  `idx` int(10) unsigned NOT NULL auto_increment,
  `md5` varchar(32) NOT NULL default '',
  `input` mediumtext,
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `md5` (`md5`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_template_inputs`
--

DROP TABLE IF EXISTS `experiment_template_inputs`;
CREATE TABLE `experiment_template_inputs` (
  `idx` int(10) unsigned NOT NULL auto_increment,
  `parent_guid` varchar(16) NOT NULL default '',
  `parent_vers` smallint(5) unsigned NOT NULL default '0',
  `pid` varchar(12) NOT NULL default '',
  `tid` varchar(32) NOT NULL default '',
  `input_idx` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`parent_guid`,`parent_vers`,`idx`),
  KEY `pidtid` (`pid`,`tid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_template_instance_bindings`
--

DROP TABLE IF EXISTS `experiment_template_instance_bindings`;
CREATE TABLE `experiment_template_instance_bindings` (
  `instance_idx` int(10) unsigned NOT NULL default '0',
  `parent_guid` varchar(16) NOT NULL default '',
  `parent_vers` smallint(5) unsigned NOT NULL default '0',
  `exptidx` int(10) unsigned NOT NULL default '0',
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `name` varchar(64) NOT NULL default '',
  `value` tinytext NOT NULL,
  PRIMARY KEY  (`instance_idx`,`name`),
  KEY `parent_guid` (`parent_guid`,`parent_vers`),
  KEY `pidtid` (`pid`,`eid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_template_instance_deadnodes`
--

DROP TABLE IF EXISTS `experiment_template_instance_deadnodes`;
CREATE TABLE `experiment_template_instance_deadnodes` (
  `instance_idx` int(10) unsigned NOT NULL default '0',
  `exptidx` int(10) unsigned NOT NULL default '0',
  `runidx` int(10) unsigned NOT NULL default '0',
  `node_id` varchar(32) NOT NULL default '',
  `vname` varchar(32) NOT NULL default '',
  PRIMARY KEY  (`instance_idx`,`runidx`,`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_template_instances`
--

DROP TABLE IF EXISTS `experiment_template_instances`;
CREATE TABLE `experiment_template_instances` (
  `idx` int(10) unsigned NOT NULL auto_increment,
  `parent_guid` varchar(16) NOT NULL default '',
  `parent_vers` smallint(5) unsigned NOT NULL default '0',
  `exptidx` int(10) unsigned NOT NULL default '0',
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `uid` varchar(8) NOT NULL default '',
  `uid_idx` mediumint(8) unsigned NOT NULL default '0',
  `logfileid` varchar(40) default NULL,
  `description` tinytext,
  `start_time` datetime default NULL,
  `stop_time` datetime default NULL,
  `continue_time` datetime default NULL,
  `runtime` int(10) unsigned default '0',
  `pause_time` datetime default NULL,
  `runidx` int(10) unsigned default NULL,
  `template_tag` varchar(64) default NULL,
  `export_time` datetime default NULL,
  `locked` datetime default NULL,
  `locker_pid` int(11) default '0',
  PRIMARY KEY  (`idx`),
  KEY `exptidx` (`exptidx`),
  KEY `parent_guid` (`parent_guid`,`parent_vers`),
  KEY `pid` (`pid`,`eid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_template_metadata`
--

DROP TABLE IF EXISTS `experiment_template_metadata`;
CREATE TABLE `experiment_template_metadata` (
  `parent_guid` varchar(16) NOT NULL default '',
  `parent_vers` smallint(5) unsigned NOT NULL default '0',
  `metadata_guid` varchar(16) NOT NULL default '',
  `metadata_vers` smallint(5) unsigned NOT NULL default '0',
  `internal` tinyint(1) NOT NULL default '0',
  `hidden` tinyint(1) NOT NULL default '0',
  `metadata_type` enum('tid','template_description','parameter_description','annotation','instance_description','run_description') default NULL,
  PRIMARY KEY  (`parent_guid`,`parent_vers`,`metadata_guid`,`metadata_vers`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_template_metadata_items`
--

DROP TABLE IF EXISTS `experiment_template_metadata_items`;
CREATE TABLE `experiment_template_metadata_items` (
  `guid` varchar(16) NOT NULL default '',
  `vers` smallint(5) unsigned NOT NULL default '0',
  `parent_guid` varchar(16) default NULL,
  `parent_vers` smallint(5) unsigned NOT NULL default '0',
  `template_guid` varchar(16) NOT NULL default '',
  `uid` varchar(8) NOT NULL default '',
  `uid_idx` mediumint(8) unsigned NOT NULL default '0',
  `name` varchar(64) NOT NULL default '',
  `value` mediumtext,
  `created` datetime default NULL,
  PRIMARY KEY  (`guid`,`vers`),
  KEY `parent` (`parent_guid`,`parent_vers`),
  KEY `template` (`template_guid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_template_parameters`
--

DROP TABLE IF EXISTS `experiment_template_parameters`;
CREATE TABLE `experiment_template_parameters` (
  `parent_guid` varchar(16) NOT NULL default '',
  `parent_vers` smallint(5) unsigned NOT NULL default '0',
  `pid` varchar(12) NOT NULL default '',
  `tid` varchar(32) NOT NULL default '',
  `name` varchar(64) NOT NULL default '',
  `value` tinytext,
  `metadata_guid` varchar(16) default NULL,
  `metadata_vers` smallint(5) unsigned NOT NULL default '0',
  PRIMARY KEY  (`parent_guid`,`parent_vers`,`name`),
  KEY `pidtid` (`pid`,`tid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_template_searches`
--

DROP TABLE IF EXISTS `experiment_template_searches`;
CREATE TABLE `experiment_template_searches` (
  `parent_guid` varchar(16) NOT NULL default '',
  `parent_vers` smallint(5) unsigned NOT NULL default '0',
  `uid_idx` mediumint(8) unsigned NOT NULL default '0',
  `name` varchar(64) NOT NULL default '',
  `expr` mediumtext,
  `created` datetime default NULL,
  PRIMARY KEY  (`parent_guid`,`parent_vers`,`uid_idx`,`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_template_settings`
--

DROP TABLE IF EXISTS `experiment_template_settings`;
CREATE TABLE `experiment_template_settings` (
  `parent_guid` varchar(16) NOT NULL default '',
  `parent_vers` smallint(5) unsigned NOT NULL default '0',
  `pid` varchar(12) NOT NULL default '',
  `tid` varchar(32) NOT NULL default '',
  `uselinkdelays` tinyint(4) NOT NULL default '0',
  `forcelinkdelays` tinyint(4) NOT NULL default '0',
  `multiplex_factor` smallint(5) default NULL,
  `uselatestwadata` tinyint(4) NOT NULL default '0',
  `usewatunnels` tinyint(4) NOT NULL default '1',
  `wa_delay_solverweight` float default '0',
  `wa_bw_solverweight` float default '0',
  `wa_plr_solverweight` float default '0',
  `sync_server` varchar(32) default NULL,
  `cpu_usage` tinyint(4) unsigned NOT NULL default '0',
  `mem_usage` tinyint(4) unsigned NOT NULL default '0',
  `veth_encapsulate` tinyint(4) NOT NULL default '1',
  `allowfixnode` tinyint(4) NOT NULL default '1',
  `jail_osname` varchar(20) default NULL,
  `delay_osname` varchar(20) default NULL,
  `use_ipassign` tinyint(4) NOT NULL default '0',
  `ipassign_args` varchar(255) default NULL,
  `linktest_level` tinyint(4) NOT NULL default '0',
  `linktest_pid` int(11) default '0',
  `useprepass` tinyint(1) NOT NULL default '0',
  `elab_in_elab` tinyint(1) NOT NULL default '0',
  `elabinelab_eid` varchar(32) default NULL,
  `elabinelab_cvstag` varchar(64) default NULL,
  `elabinelab_nosetup` tinyint(1) NOT NULL default '0',
  `security_level` tinyint(1) NOT NULL default '0',
  `delay_capacity` tinyint(3) unsigned default NULL,
  `savedisk` tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (`parent_guid`,`parent_vers`),
  KEY `pidtid` (`pid`,`tid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiment_templates`
--

DROP TABLE IF EXISTS `experiment_templates`;
CREATE TABLE `experiment_templates` (
  `guid` varchar(16) NOT NULL default '',
  `vers` smallint(5) unsigned NOT NULL default '0',
  `parent_guid` varchar(16) default NULL,
  `parent_vers` smallint(5) unsigned default NULL,
  `child_guid` varchar(16) default NULL,
  `child_vers` smallint(5) unsigned default NULL,
  `pid` varchar(12) NOT NULL default '',
  `gid` varchar(16) NOT NULL default '',
  `tid` varchar(32) NOT NULL default '',
  `uid` varchar(8) NOT NULL default '',
  `uid_idx` mediumint(8) unsigned NOT NULL default '0',
  `pid_idx` mediumint(8) unsigned NOT NULL default '0',
  `gid_idx` mediumint(8) unsigned NOT NULL default '0',
  `description` mediumtext,
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `archive_idx` int(10) unsigned default NULL,
  `created` datetime default NULL,
  `modified` datetime default NULL,
  `locked` datetime default NULL,
  `state` varchar(16) NOT NULL default 'new',
  `path` tinytext,
  `maximum_nodes` int(6) unsigned default NULL,
  `minimum_nodes` int(6) unsigned default NULL,
  `logfile` tinytext,
  `logfile_open` tinyint(4) NOT NULL default '0',
  `prerender_pid` int(11) default '0',
  `hidden` tinyint(1) NOT NULL default '0',
  `active` tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (`guid`,`vers`),
  KEY `pidtid` (`pid`,`tid`),
  KEY `pideid` (`pid`,`eid`),
  KEY `exptidx` (`exptidx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `experiments`
--

DROP TABLE IF EXISTS `experiments`;
CREATE TABLE `experiments` (
  `eid` varchar(32) NOT NULL default '',
  `eid_uuid` varchar(40) NOT NULL default '',
  `pid_idx` mediumint(8) unsigned NOT NULL default '0',
  `gid_idx` mediumint(8) unsigned NOT NULL default '0',
  `pid` varchar(12) NOT NULL default '',
  `gid` varchar(16) NOT NULL default '',
  `creator_idx` mediumint(8) unsigned NOT NULL default '0',
  `swapper_idx` mediumint(8) unsigned default NULL,
  `expt_created` datetime default NULL,
  `expt_expires` datetime default NULL,
  `expt_name` tinytext,
  `expt_head_uid` varchar(8) NOT NULL default '',
  `expt_start` datetime default NULL,
  `expt_end` datetime default NULL,
  `expt_terminating` datetime default NULL,
  `expt_locked` datetime default NULL,
  `expt_swapped` datetime default NULL,
  `expt_swap_uid` varchar(8) NOT NULL default '',
  `swappable` tinyint(4) NOT NULL default '0',
  `priority` tinyint(4) NOT NULL default '0',
  `noswap_reason` tinytext,
  `idleswap` tinyint(4) NOT NULL default '0',
  `idleswap_timeout` int(4) NOT NULL default '0',
  `noidleswap_reason` tinytext,
  `autoswap` tinyint(4) NOT NULL default '0',
  `autoswap_timeout` int(4) NOT NULL default '0',
  `batchmode` tinyint(4) NOT NULL default '0',
  `shared` tinyint(4) NOT NULL default '0',
  `state` varchar(16) NOT NULL default 'new',
  `maximum_nodes` int(6) unsigned default NULL,
  `minimum_nodes` int(6) unsigned default NULL,
  `testdb` tinytext,
  `path` tinytext,
  `logfile` tinytext,
  `logfile_open` tinyint(4) NOT NULL default '0',
  `attempts` smallint(5) unsigned NOT NULL default '0',
  `canceled` tinyint(4) NOT NULL default '0',
  `batchstate` varchar(16) default NULL,
  `event_sched_pid` int(11) default '0',
  `prerender_pid` int(11) default '0',
  `uselinkdelays` tinyint(4) NOT NULL default '0',
  `forcelinkdelays` tinyint(4) NOT NULL default '0',
  `multiplex_factor` smallint(5) default NULL,
  `uselatestwadata` tinyint(4) NOT NULL default '0',
  `usewatunnels` tinyint(4) NOT NULL default '1',
  `wa_delay_solverweight` float default '0',
  `wa_bw_solverweight` float default '0',
  `wa_plr_solverweight` float default '0',
  `swap_requests` tinyint(4) NOT NULL default '0',
  `last_swap_req` datetime default NULL,
  `idle_ignore` tinyint(4) NOT NULL default '0',
  `sync_server` varchar(32) default NULL,
  `cpu_usage` tinyint(4) unsigned NOT NULL default '0',
  `mem_usage` tinyint(4) unsigned NOT NULL default '0',
  `keyhash` varchar(64) default NULL,
  `eventkey` varchar(64) default NULL,
  `idx` int(10) unsigned NOT NULL auto_increment,
  `sim_reswap_count` smallint(5) unsigned NOT NULL default '0',
  `veth_encapsulate` tinyint(4) NOT NULL default '1',
  `encap_style` enum('alias','veth','veth-ne','vlan','vtun','egre','gre','default') NOT NULL default 'default',
  `allowfixnode` tinyint(4) NOT NULL default '1',
  `jail_osname` varchar(20) default NULL,
  `delay_osname` varchar(20) default NULL,
  `use_ipassign` tinyint(4) NOT NULL default '0',
  `ipassign_args` varchar(255) default NULL,
  `linktest_level` tinyint(4) NOT NULL default '0',
  `linktest_pid` int(11) default '0',
  `useprepass` tinyint(1) NOT NULL default '0',
  `usemodelnet` tinyint(1) NOT NULL default '0',
  `modelnet_cores` tinyint(4) unsigned NOT NULL default '0',
  `modelnet_edges` tinyint(4) unsigned NOT NULL default '0',
  `modelnetcore_osname` varchar(20) default NULL,
  `modelnetedge_osname` varchar(20) default NULL,
  `elab_in_elab` tinyint(1) NOT NULL default '0',
  `elabinelab_eid` varchar(32) default NULL,
  `elabinelab_exptidx` int(11) default NULL,
  `elabinelab_cvstag` varchar(64) default NULL,
  `elabinelab_nosetup` tinyint(1) NOT NULL default '0',
  `elabinelab_singlenet` tinyint(1) NOT NULL default '0',
  `security_level` tinyint(1) NOT NULL default '0',
  `lockdown` tinyint(1) NOT NULL default '0',
  `paniced` tinyint(1) NOT NULL default '0',
  `panic_date` datetime default NULL,
  `delay_capacity` tinyint(3) unsigned default NULL,
  `savedisk` tinyint(1) NOT NULL default '0',
  `locpiper_pid` int(11) default '0',
  `locpiper_port` int(11) default '0',
  `instance_idx` int(10) unsigned NOT NULL default '0',
  `dpdb` tinyint(1) NOT NULL default '0',
  `dpdbname` varchar(64) default NULL,
  `dpdbpassword` varchar(64) default NULL,
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `pideid` (`pid`,`eid`),
  UNIQUE KEY `pididxeid` (`pid_idx`,`eid`),
  KEY `batchmode` (`batchmode`),
  KEY `state` (`state`),
  KEY `eid_uuid` (`eid_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `exported_tables`
--

DROP TABLE IF EXISTS `exported_tables`;
CREATE TABLE `exported_tables` (
  `table_name` varchar(64) NOT NULL default '',
  PRIMARY KEY  (`table_name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `exppid_access`
--

DROP TABLE IF EXISTS `exppid_access`;
CREATE TABLE `exppid_access` (
  `exp_eid` varchar(32) NOT NULL default '',
  `exp_pid` varchar(12) NOT NULL default '',
  `pid` varchar(12) NOT NULL default '',
  PRIMARY KEY  (`exp_eid`,`exp_pid`,`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `firewall_rules`
--

DROP TABLE IF EXISTS `firewall_rules`;
CREATE TABLE `firewall_rules` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `fwname` varchar(32) NOT NULL default '',
  `ruleno` int(10) unsigned NOT NULL default '0',
  `rule` text NOT NULL,
  PRIMARY KEY  (`exptidx`,`fwname`,`ruleno`),
  KEY `fwname` (`fwname`),
  KEY `pideid` (`pid`,`eid`,`fwname`,`ruleno`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `firewalls`
--

DROP TABLE IF EXISTS `firewalls`;
CREATE TABLE `firewalls` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `fwname` varchar(32) NOT NULL default '',
  `vlan` int(11) default NULL,
  `vlanid` int(11) default NULL,
  PRIMARY KEY  (`exptidx`,`fwname`),
  KEY `vlan` (`vlan`),
  KEY `pideid` (`pid`,`eid`,`fwname`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `floorimages`
--

DROP TABLE IF EXISTS `floorimages`;
CREATE TABLE `floorimages` (
  `building` varchar(32) NOT NULL default '',
  `floor` varchar(32) NOT NULL default '',
  `image_path` tinytext,
  `thumb_path` tinytext,
  `x1` int(6) NOT NULL default '0',
  `y1` int(6) NOT NULL default '0',
  `x2` int(6) NOT NULL default '0',
  `y2` int(6) NOT NULL default '0',
  `scale` tinyint(4) NOT NULL default '1',
  `pixels_per_meter` float(10,3) NOT NULL default '0.000',
  PRIMARY KEY  (`building`,`floor`,`scale`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `foreign_keys`
--

DROP TABLE IF EXISTS `foreign_keys`;
CREATE TABLE `foreign_keys` (
  `table1` varchar(30) NOT NULL default '',
  `column1` varchar(30) NOT NULL default '',
  `table2` varchar(30) NOT NULL default '',
  `column2` varchar(30) NOT NULL default '',
  PRIMARY KEY  (`table1`,`column1`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `fs_resources`
--

DROP TABLE IF EXISTS `fs_resources`;
CREATE TABLE `fs_resources` (
  `rsrcidx` int(10) unsigned NOT NULL default '0',
  `fileidx` int(11) unsigned NOT NULL default '0',
  `exptidx` int(10) unsigned NOT NULL default '0',
  `type` enum('r','w','rw','l') default 'r',
  `size` int(11) unsigned default '0',
  PRIMARY KEY  (`rsrcidx`,`fileidx`),
  KEY `rsrcidx` (`rsrcidx`),
  KEY `fileidx` (`fileidx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `global_policies`
--

DROP TABLE IF EXISTS `global_policies`;
CREATE TABLE `global_policies` (
  `policy` varchar(32) NOT NULL default '',
  `auxdata` varchar(128) NOT NULL default '',
  `test` varchar(32) NOT NULL default '',
  `count` int(10) NOT NULL default '0',
  PRIMARY KEY  (`policy`,`auxdata`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `global_vtypes`
--

DROP TABLE IF EXISTS `global_vtypes`;
CREATE TABLE `global_vtypes` (
  `vtype` varchar(30) NOT NULL default '',
  `weight` float NOT NULL default '0.5',
  `types` text NOT NULL,
  PRIMARY KEY  (`vtype`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `group_membership`
--

DROP TABLE IF EXISTS `group_membership`;
CREATE TABLE `group_membership` (
  `uid` varchar(8) NOT NULL default '',
  `gid` varchar(16) NOT NULL default '',
  `pid` varchar(12) NOT NULL default '',
  `uid_idx` mediumint(8) unsigned NOT NULL default '0',
  `pid_idx` mediumint(8) unsigned NOT NULL default '0',
  `gid_idx` mediumint(8) unsigned NOT NULL default '0',
  `trust` enum('none','user','local_root','group_root','project_root') default NULL,
  `date_applied` date default NULL,
  `date_approved` datetime default NULL,
  PRIMARY KEY  (`uid_idx`,`gid_idx`),
  UNIQUE KEY `uid` (`uid`,`pid`,`gid`),
  KEY `pid` (`pid`),
  KEY `gid` (`gid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `group_policies`
--

DROP TABLE IF EXISTS `group_policies`;
CREATE TABLE `group_policies` (
  `pid` varchar(12) NOT NULL default '',
  `gid` varchar(12) NOT NULL default '',
  `pid_idx` mediumint(8) unsigned NOT NULL default '0',
  `gid_idx` mediumint(8) unsigned NOT NULL default '0',
  `policy` varchar(32) NOT NULL default '',
  `auxdata` varchar(64) NOT NULL default '',
  `count` int(10) NOT NULL default '0',
  PRIMARY KEY  (`gid_idx`,`policy`,`auxdata`),
  UNIQUE KEY `pid` (`pid`,`gid`,`policy`,`auxdata`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `group_stats`
--

DROP TABLE IF EXISTS `group_stats`;
CREATE TABLE `group_stats` (
  `pid` varchar(12) NOT NULL default '',
  `gid` varchar(12) NOT NULL default '',
  `pid_idx` mediumint(8) unsigned NOT NULL default '0',
  `gid_idx` mediumint(8) unsigned NOT NULL default '0',
  `gid_uuid` varchar(40) NOT NULL default '',
  `exptstart_count` int(11) unsigned default '0',
  `exptstart_last` datetime default NULL,
  `exptpreload_count` int(11) unsigned default '0',
  `exptpreload_last` datetime default NULL,
  `exptswapin_count` int(11) unsigned default '0',
  `exptswapin_last` datetime default NULL,
  `exptswapout_count` int(11) unsigned default '0',
  `exptswapout_last` datetime default NULL,
  `exptswapmod_count` int(11) unsigned default '0',
  `exptswapmod_last` datetime default NULL,
  `last_activity` datetime default NULL,
  `allexpt_duration` int(11) unsigned default '0',
  `allexpt_vnodes` int(11) unsigned default '0',
  `allexpt_vnode_duration` double(14,0) unsigned default '0',
  `allexpt_pnodes` int(11) unsigned default '0',
  `allexpt_pnode_duration` double(14,0) unsigned default '0',
  PRIMARY KEY  (`gid_idx`),
  UNIQUE KEY `pidgid` (`pid`,`gid`),
  KEY `gid_uuid` (`gid_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `groups`
--

DROP TABLE IF EXISTS `groups`;
CREATE TABLE `groups` (
  `pid` varchar(12) NOT NULL default '',
  `gid` varchar(12) NOT NULL default '',
  `pid_idx` mediumint(8) unsigned NOT NULL default '0',
  `gid_idx` mediumint(8) unsigned NOT NULL default '0',
  `gid_uuid` varchar(40) NOT NULL default '',
  `leader` varchar(8) NOT NULL default '',
  `leader_idx` mediumint(8) unsigned NOT NULL default '0',
  `created` datetime default NULL,
  `description` tinytext,
  `unix_gid` smallint(5) unsigned NOT NULL auto_increment,
  `unix_name` varchar(16) NOT NULL default '',
  `expt_count` mediumint(8) unsigned default '0',
  `expt_last` date default NULL,
  `wikiname` tinytext,
  `mailman_password` tinytext,
  PRIMARY KEY  (`gid_idx`),
  UNIQUE KEY `pidgid` (`pid`,`gid`),
  KEY `unix_gid` (`unix_gid`),
  KEY `gid` (`gid`),
  KEY `pid` (`pid`),
  KEY `pididx` (`pid_idx`,`gid_idx`),
  KEY `gid_uuid` (`gid_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `iface_counters`
--

DROP TABLE IF EXISTS `iface_counters`;
CREATE TABLE `iface_counters` (
  `node_id` varchar(32) NOT NULL default '',
  `tstamp` datetime NOT NULL default '0000-00-00 00:00:00',
  `mac` varchar(12) NOT NULL default '0',
  `ipkts` int(11) NOT NULL default '0',
  `opkts` int(11) NOT NULL default '0',
  PRIMARY KEY  (`node_id`,`tstamp`,`mac`),
  KEY `macindex` (`mac`),
  KEY `node_idindex` (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `ifaces`
--

DROP TABLE IF EXISTS `ifaces`;
CREATE TABLE `ifaces` (
  `lanid` int(11) NOT NULL default '0',
  `ifaceid` int(11) NOT NULL default '0',
  `exptidx` int(11) NOT NULL default '0',
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `node_id` varchar(32) NOT NULL default '',
  `vnode` varchar(32) NOT NULL default '',
  `vname` varchar(32) NOT NULL default '',
  `vidx` int(11) NOT NULL default '0',
  `vport` tinyint(3) NOT NULL default '0',
  PRIMARY KEY  (`lanid`,`ifaceid`),
  KEY `pideid` (`pid`,`eid`),
  KEY `exptidx` (`exptidx`),
  KEY `lanid` (`lanid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

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
  `imageid` int(8) unsigned NOT NULL default '0',
  `uuid` varchar(40) NOT NULL default '',
  `old_imageid` varchar(45) NOT NULL default '',
  `creator` varchar(8) default NULL,
  `creator_idx` mediumint(8) unsigned NOT NULL default '0',
  `created` datetime default NULL,
  `description` tinytext NOT NULL,
  `loadpart` tinyint(4) NOT NULL default '0',
  `loadlength` tinyint(4) NOT NULL default '0',
  `part1_osid` int(8) unsigned default NULL,
  `part2_osid` int(8) unsigned default NULL,
  `part3_osid` int(8) unsigned default NULL,
  `part4_osid` int(8) unsigned default NULL,
  `default_osid` int(8) unsigned NOT NULL default '0',
  `path` tinytext,
  `magic` tinytext,
  `load_address` text,
  `frisbee_pid` int(11) default '0',
  `load_busy` tinyint(4) NOT NULL default '0',
  `ezid` tinyint(4) NOT NULL default '0',
  `shared` tinyint(4) NOT NULL default '0',
  `global` tinyint(4) NOT NULL default '0',
  `mbr_version` tinyint(4) NOT NULL default '1',
  `updated` datetime default NULL,
  `access_key` varchar(64) default NULL,
  PRIMARY KEY  (`imageid`),
  UNIQUE KEY `pid` (`pid`,`imagename`),
  KEY `gid` (`gid`),
  KEY `old_imageid` (`old_imageid`),
  KEY `uuid` (`uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `interface_capabilities`
--

DROP TABLE IF EXISTS `interface_capabilities`;
CREATE TABLE `interface_capabilities` (
  `type` varchar(30) NOT NULL default '',
  `capkey` varchar(64) NOT NULL default '',
  `capval` varchar(64) NOT NULL default '',
  PRIMARY KEY  (`type`,`capkey`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `interface_settings`
--

DROP TABLE IF EXISTS `interface_settings`;
CREATE TABLE `interface_settings` (
  `node_id` varchar(32) NOT NULL default '',
  `iface` varchar(32) NOT NULL default '',
  `capkey` varchar(32) NOT NULL default '',
  `capval` varchar(64) NOT NULL default '',
  PRIMARY KEY  (`node_id`,`iface`,`capkey`),
  KEY `node_id` (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `interface_types`
--

DROP TABLE IF EXISTS `interface_types`;
CREATE TABLE `interface_types` (
  `type` varchar(30) NOT NULL default '',
  `max_speed` int(11) default NULL,
  `full_duplex` tinyint(1) default NULL,
  `manufacturuer` varchar(30) default NULL,
  `model` varchar(30) default NULL,
  `ports` tinyint(4) default NULL,
  `connector` varchar(30) default NULL,
  PRIMARY KEY  (`type`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `interfaces`
--

DROP TABLE IF EXISTS `interfaces`;
CREATE TABLE `interfaces` (
  `node_id` varchar(32) NOT NULL default '',
  `card` tinyint(3) unsigned NOT NULL default '0',
  `port` tinyint(3) unsigned NOT NULL default '0',
  `mac` varchar(12) NOT NULL default '000000000000',
  `IP` varchar(15) default NULL,
  `IPaliases` text,
  `mask` varchar(15) default NULL,
  `interface_type` varchar(30) default NULL,
  `iface` text NOT NULL,
  `role` enum('ctrl','expt','jail','fake','other','gw','outer_ctrl') default NULL,
  `current_speed` enum('0','10','100','1000') NOT NULL default '0',
  `duplex` enum('full','half') NOT NULL default 'full',
  `rtabid` smallint(5) unsigned NOT NULL default '0',
  `vnode_id` varchar(32) default NULL,
  `whol` tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (`node_id`,`card`,`port`),
  KEY `mac` (`mac`),
  KEY `IP` (`IP`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `ipport_ranges`
--

DROP TABLE IF EXISTS `ipport_ranges`;
CREATE TABLE `ipport_ranges` (
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `pid` varchar(12) NOT NULL default '',
  `low` int(11) NOT NULL default '0',
  `high` int(11) NOT NULL default '0',
  PRIMARY KEY  (`exptidx`),
  UNIQUE KEY `pideid` (`pid`,`eid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `ipsubnets`
--

DROP TABLE IF EXISTS `ipsubnets`;
CREATE TABLE `ipsubnets` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `idx` smallint(5) unsigned NOT NULL auto_increment,
  PRIMARY KEY  (`idx`),
  KEY `pideid` (`pid`,`eid`),
  KEY `exptidx` (`exptidx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `knowledge_base_entries`
--

DROP TABLE IF EXISTS `knowledge_base_entries`;
CREATE TABLE `knowledge_base_entries` (
  `idx` int(11) NOT NULL auto_increment,
  `creator_uid` varchar(8) NOT NULL default '',
  `creator_idx` mediumint(8) unsigned NOT NULL default '0',
  `date_created` datetime default NULL,
  `section` tinytext,
  `title` tinytext,
  `body` text,
  `xref_tag` varchar(64) default NULL,
  `faq_entry` tinyint(1) NOT NULL default '0',
  `date_modified` datetime default NULL,
  `modifier_uid` varchar(8) default NULL,
  `modifier_idx` mediumint(8) unsigned NOT NULL default '0',
  `archived` tinyint(1) NOT NULL default '0',
  `date_archived` datetime default NULL,
  `archiver_uid` varchar(8) default NULL,
  `archiver_idx` mediumint(8) unsigned NOT NULL default '0',
  PRIMARY KEY  (`idx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `lan_attributes`
--

DROP TABLE IF EXISTS `lan_attributes`;
CREATE TABLE `lan_attributes` (
  `lanid` int(11) NOT NULL default '0',
  `attrkey` varchar(32) NOT NULL default '',
  `attrvalue` tinytext NOT NULL,
  `attrtype` enum('integer','float','boolean','string') default 'string',
  PRIMARY KEY  (`lanid`,`attrkey`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `lan_member_attributes`
--

DROP TABLE IF EXISTS `lan_member_attributes`;
CREATE TABLE `lan_member_attributes` (
  `lanid` int(11) NOT NULL default '0',
  `memberid` int(11) NOT NULL default '0',
  `attrkey` varchar(32) NOT NULL default '',
  `attrvalue` tinytext NOT NULL,
  `attrtype` enum('integer','float','boolean','string') default 'string',
  PRIMARY KEY  (`lanid`,`memberid`,`attrkey`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `lan_members`
--

DROP TABLE IF EXISTS `lan_members`;
CREATE TABLE `lan_members` (
  `lanid` int(11) NOT NULL default '0',
  `memberid` int(11) NOT NULL auto_increment,
  `node_id` varchar(32) NOT NULL default '',
  PRIMARY KEY  (`lanid`,`memberid`),
  KEY `node_id` (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `lans`
--

DROP TABLE IF EXISTS `lans`;
CREATE TABLE `lans` (
  `lanid` int(11) NOT NULL auto_increment,
  `exptidx` int(11) NOT NULL default '0',
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `vname` varchar(64) NOT NULL default '',
  `vidx` int(11) NOT NULL default '0',
  `type` varchar(32) NOT NULL default '',
  `link` int(11) default NULL,
  `ready` tinyint(1) default '0',
  PRIMARY KEY  (`lanid`),
  KEY `pideid` (`pid`,`eid`),
  KEY `exptidx` (`exptidx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `last_reservation`
--

DROP TABLE IF EXISTS `last_reservation`;
CREATE TABLE `last_reservation` (
  `node_id` varchar(32) NOT NULL default '',
  `pid` varchar(12) NOT NULL default '',
  `pid_idx` mediumint(8) unsigned NOT NULL default '0',
  PRIMARY KEY  (`node_id`,`pid_idx`),
  UNIQUE KEY `pid` (`node_id`,`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `linkdelays`
--

DROP TABLE IF EXISTS `linkdelays`;
CREATE TABLE `linkdelays` (
  `node_id` varchar(32) NOT NULL default '',
  `iface` varchar(8) NOT NULL default '',
  `ip` varchar(15) NOT NULL default '',
  `netmask` varchar(15) NOT NULL default '255.255.255.0',
  `type` enum('simplex','duplex') NOT NULL default 'duplex',
  `exptidx` int(11) NOT NULL default '0',
  `eid` varchar(32) default NULL,
  `pid` varchar(32) default NULL,
  `vlan` varchar(32) NOT NULL default '',
  `vnode` varchar(32) NOT NULL default '',
  `pipe` smallint(5) unsigned NOT NULL default '0',
  `delay` float(10,2) NOT NULL default '0.00',
  `bandwidth` int(10) unsigned NOT NULL default '100',
  `lossrate` float(10,3) NOT NULL default '0.000',
  `rpipe` smallint(5) unsigned NOT NULL default '0',
  `rdelay` float(10,2) NOT NULL default '0.00',
  `rbandwidth` int(10) unsigned NOT NULL default '100',
  `rlossrate` float(10,3) NOT NULL default '0.000',
  `q_limit` int(11) default '0',
  `q_maxthresh` int(11) default '0',
  `q_minthresh` int(11) default '0',
  `q_weight` float default '0',
  `q_linterm` int(11) default '0',
  `q_qinbytes` tinyint(4) default '0',
  `q_bytes` tinyint(4) default '0',
  `q_meanpsize` int(11) default '0',
  `q_wait` int(11) default '0',
  `q_setbit` int(11) default '0',
  `q_droptail` int(11) default '0',
  `q_red` tinyint(4) default '0',
  `q_gentle` tinyint(4) default '0',
  PRIMARY KEY  (`node_id`,`vlan`,`vnode`),
  KEY `id` (`pid`,`eid`),
  KEY `exptidx` (`exptidx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `location_info`
--

DROP TABLE IF EXISTS `location_info`;
CREATE TABLE `location_info` (
  `node_id` varchar(32) NOT NULL default '',
  `floor` varchar(32) NOT NULL default '',
  `building` varchar(32) NOT NULL default '',
  `loc_x` int(10) unsigned NOT NULL default '0',
  `loc_y` int(10) unsigned NOT NULL default '0',
  `loc_z` float default NULL,
  `orientation` float default NULL,
  `contact` tinytext,
  `email` tinytext,
  `phone` tinytext,
  `room` varchar(32) default NULL,
  `stamp` int(10) unsigned default NULL,
  PRIMARY KEY  (`node_id`,`building`,`floor`)
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
  KEY `session` (`session`),
  KEY `stamp` (`stamp`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `logfiles`
--

DROP TABLE IF EXISTS `logfiles`;
CREATE TABLE `logfiles` (
  `logid` varchar(40) NOT NULL default '',
  `filename` tinytext,
  `isopen` tinyint(4) NOT NULL default '0',
  `gid_idx` mediumint(8) unsigned NOT NULL default '0',
  `date_created` datetime default NULL,
  PRIMARY KEY  (`logid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `login`
--

DROP TABLE IF EXISTS `login`;
CREATE TABLE `login` (
  `uid_idx` mediumint(8) unsigned NOT NULL default '0',
  `uid` varchar(10) NOT NULL default '',
  `hashkey` varchar(64) NOT NULL default '',
  `hashhash` varchar(64) NOT NULL default '',
  `timeout` varchar(10) NOT NULL default '',
  `adminon` tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (`uid_idx`,`hashkey`),
  UNIQUE KEY `hashhash` (`uid_idx`,`hashhash`),
  UNIQUE KEY `uidkey` (`uid`,`hashkey`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `login_failures`
--

DROP TABLE IF EXISTS `login_failures`;
CREATE TABLE `login_failures` (
  `IP` varchar(15) NOT NULL default '1.1.1.1',
  `frozen` tinyint(3) unsigned NOT NULL default '0',
  `failcount` smallint(5) unsigned NOT NULL default '0',
  `failstamp` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`IP`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `loginmessage`
--

DROP TABLE IF EXISTS `loginmessage`;
CREATE TABLE `loginmessage` (
  `valid` tinyint(4) NOT NULL default '1',
  `message` tinytext NOT NULL,
  PRIMARY KEY  (`valid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `mailman_listnames`
--

DROP TABLE IF EXISTS `mailman_listnames`;
CREATE TABLE `mailman_listnames` (
  `listname` varchar(64) NOT NULL default '',
  `owner_uid` varchar(8) NOT NULL default '',
  `owner_idx` mediumint(8) unsigned NOT NULL default '0',
  `created` datetime default NULL,
  PRIMARY KEY  (`listname`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `mode_transitions`
--

DROP TABLE IF EXISTS `mode_transitions`;
CREATE TABLE `mode_transitions` (
  `op_mode1` varchar(20) NOT NULL default '',
  `state1` varchar(20) NOT NULL default '',
  `op_mode2` varchar(20) NOT NULL default '',
  `state2` varchar(20) NOT NULL default '',
  `label` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`op_mode1`,`state1`,`op_mode2`,`state2`),
  KEY `op_mode1` (`op_mode1`,`state1`),
  KEY `op_mode2` (`op_mode2`,`state2`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `motelogfiles`
--

DROP TABLE IF EXISTS `motelogfiles`;
CREATE TABLE `motelogfiles` (
  `logfileid` varchar(45) NOT NULL default '',
  `pid` varchar(12) NOT NULL default '',
  `gid` varchar(12) default NULL,
  `creator` varchar(8) NOT NULL default '',
  `created` datetime NOT NULL default '0000-00-00 00:00:00',
  `updated` datetime default NULL,
  `description` tinytext NOT NULL,
  `classfilepath` tinytext NOT NULL,
  `specfilepath` tinytext,
  `mote_type` varchar(30) default NULL,
  PRIMARY KEY  (`logfileid`,`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `new_interfaces`
--

DROP TABLE IF EXISTS `new_interfaces`;
CREATE TABLE `new_interfaces` (
  `new_node_id` int(11) NOT NULL default '0',
  `card` int(11) NOT NULL default '0',
  `mac` varchar(12) NOT NULL default '',
  `interface_type` varchar(15) default NULL,
  `switch_id` varchar(32) default NULL,
  `switch_card` tinyint(3) default NULL,
  `switch_port` tinyint(3) default NULL,
  `cable` smallint(6) default NULL,
  `len` tinyint(4) default NULL,
  `role` tinytext,
  PRIMARY KEY  (`new_node_id`,`card`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `new_nodes`
--

DROP TABLE IF EXISTS `new_nodes`;
CREATE TABLE `new_nodes` (
  `new_node_id` int(11) NOT NULL auto_increment,
  `node_id` varchar(32) NOT NULL default '',
  `type` varchar(30) default NULL,
  `IP` varchar(15) default NULL,
  `temporary_IP` varchar(15) default NULL,
  `dmesg` text,
  `created` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `identifier` varchar(255) default NULL,
  `floor` varchar(32) default NULL,
  `building` varchar(32) default NULL,
  `loc_x` int(10) unsigned NOT NULL default '0',
  `loc_y` int(10) unsigned NOT NULL default '0',
  `contact` tinytext,
  `phone` tinytext,
  `room` varchar(32) default NULL,
  `role` varchar(32) NOT NULL default 'testnode',
  PRIMARY KEY  (`new_node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `newdelays`
--

DROP TABLE IF EXISTS `newdelays`;
CREATE TABLE `newdelays` (
  `node_id` varchar(32) NOT NULL default '',
  `pipe0` smallint(5) unsigned NOT NULL default '0',
  `delay0` int(10) unsigned NOT NULL default '0',
  `bandwidth0` int(10) unsigned NOT NULL default '100',
  `lossrate0` float(10,3) NOT NULL default '0.000',
  `pipe1` smallint(5) unsigned NOT NULL default '0',
  `delay1` int(10) unsigned NOT NULL default '0',
  `bandwidth1` int(10) unsigned NOT NULL default '100',
  `lossrate1` float(10,3) NOT NULL default '0.000',
  `iface0` varchar(8) NOT NULL default '',
  `iface1` varchar(8) NOT NULL default '',
  `eid` varchar(32) default NULL,
  `pid` varchar(32) default NULL,
  `vname` varchar(32) default NULL,
  `card0` tinyint(3) unsigned default NULL,
  `card1` tinyint(3) unsigned default NULL,
  PRIMARY KEY  (`node_id`,`iface0`,`iface1`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `next_reserve`
--

DROP TABLE IF EXISTS `next_reserve`;
CREATE TABLE `next_reserve` (
  `node_id` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  PRIMARY KEY  (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `nextfreenode`
--

DROP TABLE IF EXISTS `nextfreenode`;
CREATE TABLE `nextfreenode` (
  `nodetype` varchar(30) NOT NULL default '',
  `nextid` int(10) unsigned NOT NULL default '1',
  `nextpri` int(10) unsigned NOT NULL default '1',
  PRIMARY KEY  (`nodetype`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `node_activity`
--

DROP TABLE IF EXISTS `node_activity`;
CREATE TABLE `node_activity` (
  `node_id` varchar(32) NOT NULL default '',
  `last_tty_act` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_net_act` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_cpu_act` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_ext_act` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_report` datetime NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `node_attributes`
--

DROP TABLE IF EXISTS `node_attributes`;
CREATE TABLE `node_attributes` (
  `node_id` varchar(32) NOT NULL default '',
  `attrkey` varchar(32) NOT NULL default '',
  `attrvalue` tinytext NOT NULL,
  PRIMARY KEY  (`node_id`,`attrkey`),
  KEY `node_id` (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `node_auxtypes`
--

DROP TABLE IF EXISTS `node_auxtypes`;
CREATE TABLE `node_auxtypes` (
  `node_id` varchar(32) NOT NULL default '',
  `type` varchar(30) NOT NULL default '',
  `count` int(11) default '1',
  PRIMARY KEY  (`node_id`,`type`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `node_bootlogs`
--

DROP TABLE IF EXISTS `node_bootlogs`;
CREATE TABLE `node_bootlogs` (
  `node_id` varchar(32) NOT NULL default '',
  `bootlog` text,
  `bootlog_timestamp` datetime default NULL,
  PRIMARY KEY  (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `node_features`
--

DROP TABLE IF EXISTS `node_features`;
CREATE TABLE `node_features` (
  `node_id` varchar(32) NOT NULL default '',
  `feature` varchar(30) NOT NULL default '',
  `weight` float NOT NULL default '0',
  PRIMARY KEY  (`node_id`,`feature`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `node_history`
--

DROP TABLE IF EXISTS `node_history`;
CREATE TABLE `node_history` (
  `history_id` int(10) unsigned NOT NULL auto_increment,
  `node_id` varchar(32) NOT NULL default '',
  `op` enum('alloc','free','move') NOT NULL default 'alloc',
  `uid` varchar(8) NOT NULL default '',
  `uid_idx` mediumint(8) unsigned NOT NULL default '0',
  `exptidx` int(10) unsigned default NULL,
  `stamp` int(10) unsigned default NULL,
  PRIMARY KEY  (`history_id`),
  KEY `node_id` (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `node_hostkeys`
--

DROP TABLE IF EXISTS `node_hostkeys`;
CREATE TABLE `node_hostkeys` (
  `node_id` varchar(32) NOT NULL default '',
  `sshrsa_v1` mediumtext,
  `sshrsa_v2` mediumtext,
  `sshdsa_v2` mediumtext,
  `sfshostid` varchar(128) default NULL,
  PRIMARY KEY  (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `node_idlestats`
--

DROP TABLE IF EXISTS `node_idlestats`;
CREATE TABLE `node_idlestats` (
  `node_id` varchar(32) NOT NULL default '',
  `tstamp` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_tty` datetime NOT NULL default '0000-00-00 00:00:00',
  `load_1min` float NOT NULL default '0',
  `load_5min` float NOT NULL default '0',
  `load_15min` float NOT NULL default '0',
  PRIMARY KEY  (`node_id`,`tstamp`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `node_rusage`
--

DROP TABLE IF EXISTS `node_rusage`;
CREATE TABLE `node_rusage` (
  `node_id` varchar(32) NOT NULL default '',
  `load_1min` float NOT NULL default '0',
  `load_5min` float NOT NULL default '0',
  `load_15min` float NOT NULL default '0',
  `disk_used` float NOT NULL default '0',
  `status_timestamp` datetime default NULL,
  PRIMARY KEY  (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `node_startloc`
--

DROP TABLE IF EXISTS `node_startloc`;
CREATE TABLE `node_startloc` (
  `node_id` varchar(32) NOT NULL default '',
  `building` varchar(32) NOT NULL default '',
  `floor` varchar(32) NOT NULL default '',
  `loc_x` float NOT NULL default '0',
  `loc_y` float NOT NULL default '0',
  `orientation` float NOT NULL default '0',
  PRIMARY KEY  (`node_id`,`building`,`floor`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `node_status`
--

DROP TABLE IF EXISTS `node_status`;
CREATE TABLE `node_status` (
  `node_id` varchar(32) NOT NULL default '',
  `status` enum('up','possibly down','down','unpingable') default NULL,
  `status_timestamp` datetime default NULL,
  PRIMARY KEY  (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `node_type_attributes`
--

DROP TABLE IF EXISTS `node_type_attributes`;
CREATE TABLE `node_type_attributes` (
  `type` varchar(30) NOT NULL default '',
  `attrkey` varchar(32) NOT NULL default '',
  `attrvalue` tinytext NOT NULL,
  `attrtype` enum('integer','float','boolean','string') default 'string',
  PRIMARY KEY  (`type`,`attrkey`),
  KEY `node_id` (`type`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `node_type_features`
--

DROP TABLE IF EXISTS `node_type_features`;
CREATE TABLE `node_type_features` (
  `type` varchar(30) NOT NULL default '',
  `feature` varchar(30) NOT NULL default '',
  `weight` float NOT NULL default '0',
  PRIMARY KEY  (`type`,`feature`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `node_types`
--

DROP TABLE IF EXISTS `node_types`;
CREATE TABLE `node_types` (
  `class` varchar(30) default NULL,
  `type` varchar(30) NOT NULL default '',
  `modelnetcore_osid` varchar(35) default NULL,
  `modelnetedge_osid` varchar(35) default NULL,
  `isvirtnode` tinyint(4) NOT NULL default '0',
  `ismodelnet` tinyint(1) NOT NULL default '0',
  `isjailed` tinyint(1) NOT NULL default '0',
  `isdynamic` tinyint(1) NOT NULL default '0',
  `isremotenode` tinyint(4) NOT NULL default '0',
  `issubnode` tinyint(4) NOT NULL default '0',
  `isplabdslice` tinyint(4) NOT NULL default '0',
  `isplabphysnode` tinyint(4) NOT NULL default '0',
  `issimnode` tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (`type`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `node_types_auxtypes`
--

DROP TABLE IF EXISTS `node_types_auxtypes`;
CREATE TABLE `node_types_auxtypes` (
  `auxtype` varchar(30) NOT NULL default '',
  `type` varchar(30) NOT NULL default '',
  PRIMARY KEY  (`auxtype`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `node_utilization`
--

DROP TABLE IF EXISTS `node_utilization`;
CREATE TABLE `node_utilization` (
  `node_id` varchar(32) NOT NULL default '',
  `allocated` int(10) unsigned NOT NULL default '0',
  `down` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `nodeipportnum`
--

DROP TABLE IF EXISTS `nodeipportnum`;
CREATE TABLE `nodeipportnum` (
  `node_id` varchar(32) NOT NULL default '',
  `port` smallint(5) unsigned NOT NULL default '11000',
  PRIMARY KEY  (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `nodelog`
--

DROP TABLE IF EXISTS `nodelog`;
CREATE TABLE `nodelog` (
  `node_id` varchar(32) NOT NULL default '',
  `log_id` smallint(5) unsigned NOT NULL auto_increment,
  `type` enum('misc') NOT NULL default 'misc',
  `reporting_uid` varchar(8) NOT NULL default '',
  `reporting_idx` mediumint(8) unsigned NOT NULL default '0',
  `entry` tinytext NOT NULL,
  `reported` datetime default NULL,
  PRIMARY KEY  (`node_id`,`log_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `nodes`
--

DROP TABLE IF EXISTS `nodes`;
CREATE TABLE `nodes` (
  `node_id` varchar(32) NOT NULL default '',
  `type` varchar(30) NOT NULL default '',
  `phys_nodeid` varchar(32) default NULL,
  `role` enum('testnode','virtnode','ctrlnode','testswitch','ctrlswitch','powerctrl','unused') NOT NULL default 'unused',
  `inception` datetime default NULL,
  `def_boot_osid` int(8) unsigned default NULL,
  `def_boot_path` text,
  `def_boot_cmd_line` text,
  `temp_boot_osid` int(8) unsigned default NULL,
  `next_boot_osid` int(8) unsigned default NULL,
  `next_boot_path` text,
  `next_boot_cmd_line` text,
  `pxe_boot_path` text,
  `rpms` text,
  `deltas` text,
  `tarballs` text,
  `startupcmd` tinytext,
  `startstatus` tinytext,
  `ready` tinyint(4) unsigned NOT NULL default '0',
  `priority` int(11) NOT NULL default '-1',
  `bootstatus` enum('okay','failed','unknown') default 'unknown',
  `status` enum('up','possibly down','down','unpingable') default NULL,
  `status_timestamp` datetime default NULL,
  `failureaction` enum('fatal','nonfatal','ignore') NOT NULL default 'fatal',
  `routertype` enum('none','ospf','static','manual','static-ddijk','static-old') NOT NULL default 'none',
  `eventstate` varchar(20) default NULL,
  `state_timestamp` int(10) unsigned default NULL,
  `op_mode` varchar(20) default NULL,
  `op_mode_timestamp` int(10) unsigned default NULL,
  `allocstate` varchar(20) default NULL,
  `allocstate_timestamp` int(10) unsigned default NULL,
  `update_accounts` smallint(6) default '0',
  `next_op_mode` varchar(20) NOT NULL default '',
  `ipodhash` varchar(64) default NULL,
  `osid` int(8) unsigned default NULL,
  `ntpdrift` float default NULL,
  `ipport_low` int(11) NOT NULL default '11000',
  `ipport_next` int(11) NOT NULL default '11000',
  `ipport_high` int(11) NOT NULL default '20000',
  `sshdport` int(11) NOT NULL default '11000',
  `jailflag` tinyint(3) unsigned NOT NULL default '0',
  `jailip` varchar(15) default NULL,
  `sfshostid` varchar(128) default NULL,
  `stated_tag` varchar(32) default NULL,
  `rtabid` smallint(5) unsigned NOT NULL default '0',
  `cd_version` varchar(32) default NULL,
  `battery_voltage` float default NULL,
  `battery_percentage` float default NULL,
  `battery_timestamp` int(10) unsigned default NULL,
  `boot_errno` int(11) NOT NULL default '0',
  `destination_x` float default NULL,
  `destination_y` float default NULL,
  `destination_orientation` float default NULL,
  `reserved_pid` varchar(12) default NULL,
  `uuid` varchar(40) NOT NULL default '',
  PRIMARY KEY  (`node_id`),
  KEY `phys_nodeid` (`phys_nodeid`),
  KEY `node_id` (`node_id`,`phys_nodeid`),
  KEY `role` (`role`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `nodetypeXpid_permissions`
--

DROP TABLE IF EXISTS `nodetypeXpid_permissions`;
CREATE TABLE `nodetypeXpid_permissions` (
  `type` varchar(30) NOT NULL default '',
  `pid` varchar(12) NOT NULL default '',
  `pid_idx` mediumint(8) unsigned NOT NULL default '0',
  PRIMARY KEY  (`type`,`pid_idx`),
  UNIQUE KEY `typepid` (`type`,`pid`),
  KEY `pid` (`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `nodeuidlastlogin`
--

DROP TABLE IF EXISTS `nodeuidlastlogin`;
CREATE TABLE `nodeuidlastlogin` (
  `node_id` varchar(32) NOT NULL default '',
  `uid` varchar(10) NOT NULL default '',
  `date` date default NULL,
  `time` time default NULL,
  PRIMARY KEY  (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `nologins`
--

DROP TABLE IF EXISTS `nologins`;
CREATE TABLE `nologins` (
  `nologins` tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (`nologins`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `nseconfigs`
--

DROP TABLE IF EXISTS `nseconfigs`;
CREATE TABLE `nseconfigs` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `vname` varchar(32) NOT NULL default '',
  `nseconfig` mediumtext,
  PRIMARY KEY  (`exptidx`,`vname`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`vname`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `nsfiles`
--

DROP TABLE IF EXISTS `nsfiles`;
CREATE TABLE `nsfiles` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `nsfile` mediumtext,
  PRIMARY KEY  (`exptidx`),
  UNIQUE KEY `pideid` (`pid`,`eid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `ntpinfo`
--

DROP TABLE IF EXISTS `ntpinfo`;
CREATE TABLE `ntpinfo` (
  `node_id` varchar(32) NOT NULL default '',
  `IP` varchar(64) NOT NULL default '',
  `type` enum('server','peer') NOT NULL default 'peer',
  PRIMARY KEY  (`node_id`,`IP`,`type`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `obstacles`
--

DROP TABLE IF EXISTS `obstacles`;
CREATE TABLE `obstacles` (
  `obstacle_id` int(11) unsigned NOT NULL auto_increment,
  `floor` varchar(32) default NULL,
  `building` varchar(32) default NULL,
  `x1` int(10) unsigned NOT NULL default '0',
  `y1` int(10) unsigned NOT NULL default '0',
  `z1` int(10) unsigned NOT NULL default '0',
  `x2` int(10) unsigned NOT NULL default '0',
  `y2` int(10) unsigned NOT NULL default '0',
  `z2` int(10) unsigned NOT NULL default '0',
  `description` tinytext,
  `label` tinytext,
  `draw` tinyint(1) NOT NULL default '0',
  `no_exclusion` tinyint(1) NOT NULL default '0',
  `no_tooltip` tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (`obstacle_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `os_boot_cmd`
--

DROP TABLE IF EXISTS `os_boot_cmd`;
CREATE TABLE `os_boot_cmd` (
  `OS` enum('Unknown','Linux','Fedora','FreeBSD','NetBSD','OSKit','Windows','TinyOS','Other') NOT NULL default 'Unknown',
  `version` varchar(12) NOT NULL default '',
  `role` enum('default','delay','linkdelay','vnodehost') NOT NULL default 'default',
  `boot_cmd_line` text,
  PRIMARY KEY  (`OS`,`version`,`role`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `os_info`
--

DROP TABLE IF EXISTS `os_info`;
CREATE TABLE `os_info` (
  `osname` varchar(20) NOT NULL default '',
  `pid` varchar(12) NOT NULL default '',
  `pid_idx` mediumint(8) unsigned NOT NULL default '0',
  `osid` int(8) unsigned NOT NULL default '0',
  `uuid` varchar(40) NOT NULL default '',
  `old_osid` varchar(35) NOT NULL default '',
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
  `nextosid` int(8) unsigned default NULL,
  `old_nextosid` varchar(35) NOT NULL default '',
  `max_concurrent` int(11) default NULL,
  `mfs` tinyint(4) NOT NULL default '0',
  `reboot_waittime` int(10) unsigned default NULL,
  PRIMARY KEY  (`osid`),
  UNIQUE KEY `pid` (`pid`,`osname`),
  KEY `OS` (`OS`),
  KEY `path` (`path`(255)),
  KEY `old_osid` (`old_osid`),
  KEY `uuid` (`uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `osid_map`
--

DROP TABLE IF EXISTS `osid_map`;
CREATE TABLE `osid_map` (
  `osid` int(8) unsigned NOT NULL default '0',
  `btime` datetime NOT NULL default '1000-01-01 00:00:00',
  `etime` datetime NOT NULL default '9999-12-31 23:59:59',
  `nextosid` int(8) unsigned NOT NULL default '0',
  PRIMARY KEY  (`osid`,`btime`,`etime`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `osidtoimageid`
--

DROP TABLE IF EXISTS `osidtoimageid`;
CREATE TABLE `osidtoimageid` (
  `osid` int(8) unsigned NOT NULL default '0',
  `type` varchar(30) NOT NULL default '',
  `imageid` int(8) unsigned NOT NULL default '0',
  PRIMARY KEY  (`osid`,`type`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `outlets`
--

DROP TABLE IF EXISTS `outlets`;
CREATE TABLE `outlets` (
  `node_id` varchar(32) NOT NULL default '',
  `power_id` varchar(32) NOT NULL default '',
  `outlet` tinyint(1) unsigned NOT NULL default '0',
  `last_power` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `outlets_remoteauth`
--

DROP TABLE IF EXISTS `outlets_remoteauth`;
CREATE TABLE `outlets_remoteauth` (
  `node_id` varchar(32) NOT NULL,
  `key_type` varchar(64) NOT NULL,
  `key_role` varchar(64) NOT NULL default '',
  `key_uid` varchar(64) NOT NULL default '',
  `mykey` text NOT NULL,
  PRIMARY KEY  (`node_id`,`key_type`,`key_role`,`key_uid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `partitions`
--

DROP TABLE IF EXISTS `partitions`;
CREATE TABLE `partitions` (
  `node_id` varchar(32) NOT NULL default '',
  `partition` tinyint(4) NOT NULL default '0',
  `osid` int(8) unsigned default NULL,
  `imageid` int(8) unsigned default NULL,
  `imagepid` varchar(12) NOT NULL default '',
  PRIMARY KEY  (`node_id`,`partition`),
  KEY `osid` (`osid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `plab_attributes`
--

DROP TABLE IF EXISTS `plab_attributes`;
CREATE TABLE `plab_attributes` (
  `attr_idx` int(11) unsigned NOT NULL auto_increment,
  `plc_idx` int(10) unsigned default NULL,
  `slicename` varchar(64) default NULL,
  `nodegroup_idx` int(10) unsigned default NULL,
  `node_id` varchar(32) default NULL,
  `attrkey` varchar(64) NOT NULL default '',
  `attrvalue` tinytext NOT NULL,
  PRIMARY KEY  (`attr_idx`),
  UNIQUE KEY `realattrkey` (`plc_idx`,`slicename`,`nodegroup_idx`,`node_id`,`attrkey`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `plab_comondata`
--

DROP TABLE IF EXISTS `plab_comondata`;
CREATE TABLE `plab_comondata` (
  `node_id` varchar(32) NOT NULL,
  `resptime` float default NULL,
  `uptime` float default NULL,
  `lastcotop` float default NULL,
  `date` double default NULL,
  `drift` float default NULL,
  `cpuspeed` float default NULL,
  `busycpu` float default NULL,
  `syscpu` float default NULL,
  `freecpu` float default NULL,
  `1minload` float default NULL,
  `5minload` float default NULL,
  `numslices` int(11) default NULL,
  `liveslices` int(11) default NULL,
  `connmax` float default NULL,
  `connavg` float default NULL,
  `timermax` float default NULL,
  `timeravg` float default NULL,
  `memsize` float default NULL,
  `memact` float default NULL,
  `freemem` float default NULL,
  `swapin` int(11) default NULL,
  `swapout` int(11) default NULL,
  `diskin` int(11) default NULL,
  `diskout` int(11) default NULL,
  `gbfree` float default NULL,
  `swapused` float default NULL,
  `bwlimit` float default NULL,
  `txrate` float default NULL,
  `rxrate` float default NULL,
  PRIMARY KEY  (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `plab_mapping`
--

DROP TABLE IF EXISTS `plab_mapping`;
CREATE TABLE `plab_mapping` (
  `node_id` varchar(32) NOT NULL default '',
  `plab_id` varchar(32) NOT NULL default '',
  `hostname` varchar(255) NOT NULL default '',
  `IP` varchar(15) NOT NULL default '',
  `mac` varchar(17) NOT NULL default '',
  `create_time` datetime default NULL,
  `deleted` tinyint(1) NOT NULL default '0',
  `plc_idx` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `plab_nodegroup_members`
--

DROP TABLE IF EXISTS `plab_nodegroup_members`;
CREATE TABLE `plab_nodegroup_members` (
  `plc_idx` int(10) unsigned NOT NULL default '0',
  `nodegroup_idx` int(10) unsigned NOT NULL default '0',
  `node_id` varchar(32) NOT NULL default '',
  PRIMARY KEY  (`plc_idx`,`nodegroup_idx`,`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `plab_nodegroups`
--

DROP TABLE IF EXISTS `plab_nodegroups`;
CREATE TABLE `plab_nodegroups` (
  `plc_idx` int(10) unsigned NOT NULL default '0',
  `nodegroup_idx` int(10) unsigned NOT NULL default '0',
  `name` varchar(64) NOT NULL default '',
  `description` text NOT NULL,
  PRIMARY KEY  (`plc_idx`,`nodegroup_idx`,`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `plab_nodehist`
--

DROP TABLE IF EXISTS `plab_nodehist`;
CREATE TABLE `plab_nodehist` (
  `idx` mediumint(10) unsigned NOT NULL auto_increment,
  `node_id` varchar(32) NOT NULL,
  `phys_node_id` varchar(32) NOT NULL,
  `timestamp` datetime NOT NULL,
  `component` varchar(64) NOT NULL,
  `operation` varchar(64) NOT NULL,
  `status` enum('success','failure','unknown') NOT NULL default 'unknown',
  `msg` text,
  PRIMARY KEY  (`idx`,`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `plab_nodehiststats`
--

DROP TABLE IF EXISTS `plab_nodehiststats`;
CREATE TABLE `plab_nodehiststats` (
  `node_id` varchar(32) NOT NULL,
  `unavail` float default NULL,
  `jitdeduct` float default NULL,
  `succtime` int(11) default NULL,
  `succnum` int(11) default NULL,
  `succjitnum` int(11) default NULL,
  `failtime` int(11) default NULL,
  `failnum` int(11) default NULL,
  `failjitnum` int(11) default NULL,
  PRIMARY KEY  (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `plab_objmap`
--

DROP TABLE IF EXISTS `plab_objmap`;
CREATE TABLE `plab_objmap` (
  `plc_idx` int(10) unsigned NOT NULL,
  `objtype` varchar(32) NOT NULL,
  `elab_id` varchar(64) NOT NULL,
  `plab_id` varchar(255) NOT NULL,
  `plab_name` tinytext NOT NULL,
  PRIMARY KEY  (`plc_idx`,`objtype`,`elab_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `plab_plc_attributes`
--

DROP TABLE IF EXISTS `plab_plc_attributes`;
CREATE TABLE `plab_plc_attributes` (
  `plc_idx` int(10) unsigned NOT NULL default '0',
  `attrkey` varchar(64) NOT NULL default '',
  `attrvalue` tinytext NOT NULL,
  PRIMARY KEY  (`plc_idx`,`attrkey`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `plab_plc_info`
--

DROP TABLE IF EXISTS `plab_plc_info`;
CREATE TABLE `plab_plc_info` (
  `plc_idx` int(10) unsigned NOT NULL auto_increment,
  `plc_name` varchar(64) NOT NULL default '',
  `api_url` varchar(255) NOT NULL default '',
  `def_slice_prefix` varchar(32) NOT NULL default '',
  `nodename_prefix` varchar(30) NOT NULL default '',
  `node_type` varchar(30) NOT NULL default '',
  `svc_slice_name` varchar(64) NOT NULL default '',
  PRIMARY KEY  (`plc_idx`),
  KEY `plc_name` (`plc_name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `plab_site_mapping`
--

DROP TABLE IF EXISTS `plab_site_mapping`;
CREATE TABLE `plab_site_mapping` (
  `site_name` varchar(255) NOT NULL default '',
  `site_idx` smallint(5) unsigned NOT NULL auto_increment,
  `node_id` varchar(32) NOT NULL default '',
  `node_idx` tinyint(3) unsigned NOT NULL default '0',
  `plc_idx` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`site_name`,`site_idx`,`node_idx`,`plc_idx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `plab_slice_attributes`
--

DROP TABLE IF EXISTS `plab_slice_attributes`;
CREATE TABLE `plab_slice_attributes` (
  `plc_idx` int(10) unsigned NOT NULL default '0',
  `slicename` varchar(64) NOT NULL default '',
  `attrkey` varchar(64) NOT NULL default '',
  `attrvalue` tinytext NOT NULL,
  PRIMARY KEY  (`plc_idx`,`slicename`,`attrkey`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `plab_slice_nodes`
--

DROP TABLE IF EXISTS `plab_slice_nodes`;
CREATE TABLE `plab_slice_nodes` (
  `slicename` varchar(64) NOT NULL default '',
  `node_id` varchar(32) NOT NULL default '',
  `leaseend` datetime default NULL,
  `nodemeta` text,
  `plc_idx` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`slicename`,`plc_idx`,`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `plab_slices`
--

DROP TABLE IF EXISTS `plab_slices`;
CREATE TABLE `plab_slices` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `slicename` varchar(64) NOT NULL default '',
  `slicemeta` text,
  `slicemeta_legacy` text,
  `leaseend` datetime default NULL,
  `admin` tinyint(1) default '0',
  `plc_idx` int(10) unsigned NOT NULL default '0',
  `is_created` tinyint(1) default '0',
  `is_configured` tinyint(1) default '0',
  `no_cleanup` tinyint(1) default '0',
  `no_destroy` tinyint(1) default '0',
  PRIMARY KEY  (`exptidx`,`slicename`,`plc_idx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `port_counters`
--

DROP TABLE IF EXISTS `port_counters`;
CREATE TABLE `port_counters` (
  `node_id` char(32) NOT NULL default '',
  `card` tinyint(3) unsigned NOT NULL default '0',
  `port` tinyint(3) unsigned NOT NULL default '0',
  `ifInOctets` int(10) unsigned NOT NULL default '0',
  `ifInUcastPkts` int(10) unsigned NOT NULL default '0',
  `ifInNUcastPkts` int(10) unsigned NOT NULL default '0',
  `ifInDiscards` int(10) unsigned NOT NULL default '0',
  `ifInErrors` int(10) unsigned NOT NULL default '0',
  `ifInUnknownProtos` int(10) unsigned NOT NULL default '0',
  `ifOutOctets` int(10) unsigned NOT NULL default '0',
  `ifOutUcastPkts` int(10) unsigned NOT NULL default '0',
  `ifOutNUcastPkts` int(10) unsigned NOT NULL default '0',
  `ifOutDiscards` int(10) unsigned NOT NULL default '0',
  `ifOutErrors` int(10) unsigned NOT NULL default '0',
  `ifOutQLen` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`node_id`,`card`,`port`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `port_registration`
--

DROP TABLE IF EXISTS `port_registration`;
CREATE TABLE `port_registration` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `service` varchar(64) NOT NULL default '',
  `node_id` varchar(32) NOT NULL default '',
  `port` int(11) unsigned NOT NULL default '0',
  PRIMARY KEY  (`exptidx`,`service`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`service`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `portmap`
--

DROP TABLE IF EXISTS `portmap`;
CREATE TABLE `portmap` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `vnode` varchar(32) NOT NULL default '',
  `vport` tinyint(4) NOT NULL default '0',
  `pport` varchar(32) NOT NULL default ''
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
-- Table structure for table `proj_memb`
--

DROP TABLE IF EXISTS `proj_memb`;
CREATE TABLE `proj_memb` (
  `uid` varchar(8) NOT NULL default '',
  `pid` varchar(12) NOT NULL default '',
  `trust` enum('none','user','local_root','group_root') default NULL,
  `date_applied` date default NULL,
  `date_approved` date default NULL,
  PRIMARY KEY  (`uid`,`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `project_stats`
--

DROP TABLE IF EXISTS `project_stats`;
CREATE TABLE `project_stats` (
  `pid` varchar(12) NOT NULL default '',
  `pid_idx` mediumint(8) unsigned NOT NULL default '0',
  `exptstart_count` int(11) unsigned default '0',
  `exptstart_last` datetime default NULL,
  `exptpreload_count` int(11) unsigned default '0',
  `exptpreload_last` datetime default NULL,
  `exptswapin_count` int(11) unsigned default '0',
  `exptswapin_last` datetime default NULL,
  `exptswapout_count` int(11) unsigned default '0',
  `exptswapout_last` datetime default NULL,
  `exptswapmod_count` int(11) unsigned default '0',
  `exptswapmod_last` datetime default NULL,
  `last_activity` datetime default NULL,
  `allexpt_duration` int(11) unsigned default '0',
  `allexpt_vnodes` int(11) unsigned default '0',
  `allexpt_vnode_duration` double(14,0) unsigned default '0',
  `allexpt_pnodes` int(11) unsigned default '0',
  `allexpt_pnode_duration` double(14,0) unsigned default '0',
  PRIMARY KEY  (`pid_idx`),
  UNIQUE KEY `pid` (`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `projects`
--

DROP TABLE IF EXISTS `projects`;
CREATE TABLE `projects` (
  `pid` varchar(12) NOT NULL default '',
  `pid_idx` mediumint(8) unsigned NOT NULL default '0',
  `created` datetime default NULL,
  `expires` date default NULL,
  `name` tinytext,
  `URL` tinytext,
  `funders` tinytext,
  `addr` tinytext,
  `head_uid` varchar(8) NOT NULL default '',
  `head_idx` mediumint(8) unsigned NOT NULL default '0',
  `num_members` int(11) default '0',
  `num_pcs` int(11) default '0',
  `num_sharks` int(11) default '0',
  `num_pcplab` int(11) default '0',
  `num_ron` int(11) default '0',
  `why` text,
  `control_node` varchar(10) default NULL,
  `unix_gid` smallint(5) unsigned NOT NULL auto_increment,
  `approved` tinyint(4) default '0',
  `inactive` tinyint(4) default '0',
  `date_inactive` datetime default NULL,
  `public` tinyint(4) NOT NULL default '0',
  `public_whynot` tinytext,
  `expt_count` mediumint(8) unsigned default '0',
  `expt_last` date default NULL,
  `pcremote_ok` set('pcplabphys','pcron','pcwa') default NULL,
  `default_user_interface` enum('emulab','plab') NOT NULL default 'emulab',
  `linked_to_us` tinyint(4) NOT NULL default '0',
  `cvsrepo_public` tinyint(1) NOT NULL default '0',
  `allow_workbench` tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (`pid_idx`),
  UNIQUE KEY `pid` (`pid`),
  KEY `unix_gid` (`unix_gid`),
  KEY `approved` (`approved`),
  KEY `approved_2` (`approved`),
  KEY `pcremote_ok` (`pcremote_ok`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `report_assign_violation`
--

DROP TABLE IF EXISTS `report_assign_violation`;
CREATE TABLE `report_assign_violation` (
  `seq` int(10) unsigned NOT NULL default '0',
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
  `seq` int(10) unsigned NOT NULL default '0',
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
  `seq` int(10) unsigned NOT NULL default '0',
  `stamp` int(10) unsigned NOT NULL default '0',
  `session` int(10) unsigned NOT NULL default '0',
  `invocation` int(10) unsigned NOT NULL default '0',
  `attempt` tinyint(1) NOT NULL default '0',
  `severity` smallint(3) NOT NULL default '0',
  `script` smallint(3) NOT NULL default '0',
  `error_type` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`seq`),
  KEY `session` (`session`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `reposition_status`
--

DROP TABLE IF EXISTS `reposition_status`;
CREATE TABLE `reposition_status` (
  `node_id` varchar(32) NOT NULL default '',
  `attempts` tinyint(4) NOT NULL default '0',
  `distance_remaining` float default NULL,
  PRIMARY KEY  (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `reserved`
--

DROP TABLE IF EXISTS `reserved`;
CREATE TABLE `reserved` (
  `node_id` varchar(32) NOT NULL default '',
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `rsrv_time` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `vname` varchar(32) default NULL,
  `erole` enum('node','virthost','delaynode','simhost','modelnet-core','modelnet-edge') NOT NULL default 'node',
  `simhost_violation` tinyint(3) unsigned NOT NULL default '0',
  `old_pid` varchar(12) NOT NULL default '',
  `old_eid` varchar(32) NOT NULL default '',
  `old_exptidx` int(11) NOT NULL default '0',
  `cnet_vlan` int(11) default NULL,
  `inner_elab_role` enum('boss','boss+router','router','ops','ops+fs','fs','node') default NULL,
  `inner_elab_boot` tinyint(1) default '0',
  `plab_role` enum('plc','node','none') NOT NULL default 'none',
  `plab_boot` tinyint(1) default '0',
  `mustwipe` tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (`node_id`),
  UNIQUE KEY `vname` (`pid`,`eid`,`vname`),
  UNIQUE KEY `vname2` (`exptidx`,`vname`),
  KEY `old_pid` (`old_pid`,`old_eid`),
  KEY `old_exptidx` (`old_exptidx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `scheduled_reloads`
--

DROP TABLE IF EXISTS `scheduled_reloads`;
CREATE TABLE `scheduled_reloads` (
  `node_id` varchar(32) NOT NULL default '',
  `image_id` int(8) unsigned NOT NULL default '0',
  `reload_type` enum('netdisk','frisbee') default NULL,
  PRIMARY KEY  (`node_id`)
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
-- Table structure for table `sitevariables`
--

DROP TABLE IF EXISTS `sitevariables`;
CREATE TABLE `sitevariables` (
  `name` varchar(255) NOT NULL default '',
  `value` text,
  `defaultvalue` text NOT NULL,
  `description` text,
  PRIMARY KEY  (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `state_timeouts`
--

DROP TABLE IF EXISTS `state_timeouts`;
CREATE TABLE `state_timeouts` (
  `op_mode` varchar(20) NOT NULL default '',
  `state` varchar(20) NOT NULL default '',
  `timeout` int(11) NOT NULL default '0',
  `action` mediumtext NOT NULL,
  PRIMARY KEY  (`op_mode`,`state`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `state_transitions`
--

DROP TABLE IF EXISTS `state_transitions`;
CREATE TABLE `state_transitions` (
  `op_mode` varchar(20) NOT NULL default '',
  `state1` varchar(20) NOT NULL default '',
  `state2` varchar(20) NOT NULL default '',
  `label` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`op_mode`,`state1`,`state2`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `state_triggers`
--

DROP TABLE IF EXISTS `state_triggers`;
CREATE TABLE `state_triggers` (
  `node_id` varchar(32) NOT NULL default '',
  `op_mode` varchar(20) NOT NULL default '',
  `state` varchar(20) NOT NULL default '',
  `trigger` tinytext NOT NULL,
  PRIMARY KEY  (`node_id`,`op_mode`,`state`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `switch_paths`
--

DROP TABLE IF EXISTS `switch_paths`;
CREATE TABLE `switch_paths` (
  `pid` varchar(12) default NULL,
  `eid` varchar(32) default NULL,
  `vname` varchar(32) default NULL,
  `node_id1` varchar(32) default NULL,
  `node_id2` varchar(32) default NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `switch_stack_types`
--

DROP TABLE IF EXISTS `switch_stack_types`;
CREATE TABLE `switch_stack_types` (
  `stack_id` varchar(32) NOT NULL default '',
  `stack_type` varchar(10) default NULL,
  `supports_private` tinyint(1) NOT NULL default '0',
  `single_domain` tinyint(1) NOT NULL default '1',
  `snmp_community` varchar(32) default NULL,
  `min_vlan` int(11) default NULL,
  `max_vlan` int(11) default NULL,
  `leader` varchar(32) default NULL,
  PRIMARY KEY  (`stack_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `switch_stacks`
--

DROP TABLE IF EXISTS `switch_stacks`;
CREATE TABLE `switch_stacks` (
  `node_id` varchar(32) NOT NULL default '',
  `stack_id` varchar(32) NOT NULL default '',
  `is_primary` tinyint(1) NOT NULL default '1',
  KEY `node_id` (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `table_regex`
--

DROP TABLE IF EXISTS `table_regex`;
CREATE TABLE `table_regex` (
  `table_name` varchar(64) NOT NULL default '',
  `column_name` varchar(64) NOT NULL default '',
  `column_type` enum('text','int','float') default NULL,
  `check_type` enum('regex','function','redirect') default NULL,
  `check` tinytext NOT NULL,
  `min` int(11) NOT NULL default '0',
  `max` int(11) NOT NULL default '0',
  `comment` tinytext,
  UNIQUE KEY `table_name` (`table_name`,`column_name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `testbed_stats`
--

DROP TABLE IF EXISTS `testbed_stats`;
CREATE TABLE `testbed_stats` (
  `idx` int(10) unsigned NOT NULL auto_increment,
  `start_time` datetime default NULL,
  `end_time` datetime default NULL,
  `exptidx` int(10) unsigned NOT NULL default '0',
  `rsrcidx` int(10) unsigned NOT NULL default '0',
  `action` varchar(16) NOT NULL default '',
  `exitcode` tinyint(3) default '0',
  `uid` varchar(8) NOT NULL default '',
  `uid_idx` mediumint(8) unsigned NOT NULL default '0',
  `log_session` int(10) unsigned default NULL,
  PRIMARY KEY  (`idx`),
  KEY `rsrcidx` (`rsrcidx`),
  KEY `exptidx` (`exptidx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `testsuite_preentables`
--

DROP TABLE IF EXISTS `testsuite_preentables`;
CREATE TABLE `testsuite_preentables` (
  `table_name` varchar(128) NOT NULL default '',
  `action` enum('drop','clean','prune') default 'drop',
  PRIMARY KEY  (`table_name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `tiplines`
--

DROP TABLE IF EXISTS `tiplines`;
CREATE TABLE `tiplines` (
  `tipname` varchar(32) NOT NULL default '',
  `node_id` varchar(32) NOT NULL default '',
  `server` varchar(64) NOT NULL default '',
  `portnum` int(11) NOT NULL default '0',
  `keylen` smallint(6) NOT NULL default '0',
  `keydata` text,
  PRIMARY KEY  (`tipname`),
  KEY `node_id` (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `tipservers`
--

DROP TABLE IF EXISTS `tipservers`;
CREATE TABLE `tipservers` (
  `server` varchar(64) NOT NULL default '',
  PRIMARY KEY  (`server`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `tmcd_redirect`
--

DROP TABLE IF EXISTS `tmcd_redirect`;
CREATE TABLE `tmcd_redirect` (
  `node_id` varchar(32) NOT NULL default '',
  `dbname` tinytext NOT NULL,
  PRIMARY KEY  (`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `traces`
--

DROP TABLE IF EXISTS `traces`;
CREATE TABLE `traces` (
  `node_id` varchar(32) NOT NULL default '',
  `idx` int(10) unsigned NOT NULL auto_increment,
  `iface0` varchar(8) NOT NULL default '',
  `iface1` varchar(8) NOT NULL default '',
  `pid` varchar(32) default NULL,
  `eid` varchar(32) default NULL,
  `exptidx` int(11) NOT NULL default '0',
  `linkvname` varchar(32) default NULL,
  `vnode` varchar(32) default NULL,
  `trace_type` tinytext,
  `trace_expr` tinytext,
  `trace_snaplen` int(11) NOT NULL default '0',
  `trace_db` tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (`node_id`,`idx`),
  KEY `pid` (`pid`,`eid`),
  KEY `exptidx` (`exptidx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `uidnodelastlogin`
--

DROP TABLE IF EXISTS `uidnodelastlogin`;
CREATE TABLE `uidnodelastlogin` (
  `uid` varchar(10) NOT NULL default '',
  `node_id` varchar(32) NOT NULL default '',
  `date` date default NULL,
  `time` time default NULL,
  PRIMARY KEY  (`uid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `unixgroup_membership`
--

DROP TABLE IF EXISTS `unixgroup_membership`;
CREATE TABLE `unixgroup_membership` (
  `uid` varchar(8) NOT NULL default '',
  `gid` varchar(16) NOT NULL default '',
  PRIMARY KEY  (`uid`,`gid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `user_policies`
--

DROP TABLE IF EXISTS `user_policies`;
CREATE TABLE `user_policies` (
  `uid` varchar(8) NOT NULL default '',
  `policy` varchar(32) NOT NULL default '',
  `auxdata` varchar(64) NOT NULL default '',
  `count` int(10) NOT NULL default '0',
  PRIMARY KEY  (`uid`,`policy`,`auxdata`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `user_pubkeys`
--

DROP TABLE IF EXISTS `user_pubkeys`;
CREATE TABLE `user_pubkeys` (
  `uid` varchar(8) NOT NULL default '',
  `uid_idx` mediumint(8) unsigned NOT NULL default '0',
  `idx` int(10) unsigned NOT NULL auto_increment,
  `pubkey` text,
  `stamp` datetime default NULL,
  `comment` varchar(128) NOT NULL default '',
  PRIMARY KEY  (`uid_idx`,`idx`),
  KEY `uid` (`uid`,`idx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `user_sfskeys`
--

DROP TABLE IF EXISTS `user_sfskeys`;
CREATE TABLE `user_sfskeys` (
  `uid` varchar(8) NOT NULL default '',
  `uid_idx` mediumint(8) unsigned NOT NULL default '0',
  `comment` varchar(128) NOT NULL default '',
  `pubkey` text,
  `stamp` datetime default NULL,
  PRIMARY KEY  (`uid_idx`,`comment`),
  KEY `uid` (`uid`,`comment`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `user_sslcerts`
--

DROP TABLE IF EXISTS `user_sslcerts`;
CREATE TABLE `user_sslcerts` (
  `uid` varchar(8) NOT NULL default '',
  `uid_idx` mediumint(8) unsigned NOT NULL default '0',
  `idx` int(10) unsigned NOT NULL default '0',
  `cert` text,
  `privkey` text,
  `created` datetime default NULL,
  `encrypted` tinyint(1) NOT NULL default '0',
  `status` enum('valid','revoked','expired') default 'valid',
  `orgunit` tinytext,
  `revoked` datetime default NULL,
  `password` tinytext,
  PRIMARY KEY  (`idx`),
  KEY `uid` (`uid`),
  KEY `uid_idx` (`uid_idx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `user_stats`
--

DROP TABLE IF EXISTS `user_stats`;
CREATE TABLE `user_stats` (
  `uid` varchar(8) NOT NULL default '',
  `uid_idx` mediumint(8) unsigned NOT NULL default '0',
  `uid_uuid` varchar(40) NOT NULL default '',
  `weblogin_count` int(11) unsigned default '0',
  `weblogin_last` datetime default NULL,
  `exptstart_count` int(11) unsigned default '0',
  `exptstart_last` datetime default NULL,
  `exptpreload_count` int(11) unsigned default '0',
  `exptpreload_last` datetime default NULL,
  `exptswapin_count` int(11) unsigned default '0',
  `exptswapin_last` datetime default NULL,
  `exptswapout_count` int(11) unsigned default '0',
  `exptswapout_last` datetime default NULL,
  `exptswapmod_count` int(11) unsigned default '0',
  `exptswapmod_last` datetime default NULL,
  `last_activity` datetime default NULL,
  `allexpt_duration` int(11) unsigned default '0',
  `allexpt_vnodes` int(11) unsigned default '0',
  `allexpt_vnode_duration` double(14,0) unsigned default '0',
  `allexpt_pnodes` int(11) unsigned default '0',
  `allexpt_pnode_duration` double(14,0) unsigned default '0',
  PRIMARY KEY  (`uid_idx`),
  KEY `uid_uuid` (`uid_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `users`
--

DROP TABLE IF EXISTS `users`;
CREATE TABLE `users` (
  `uid` varchar(8) NOT NULL default '',
  `uid_idx` mediumint(8) unsigned NOT NULL default '0',
  `uid_uuid` varchar(40) NOT NULL default '',
  `usr_created` datetime default NULL,
  `usr_expires` datetime default NULL,
  `usr_modified` datetime default NULL,
  `usr_name` tinytext,
  `usr_title` tinytext,
  `usr_affil` tinytext,
  `usr_affil_abbrev` varchar(16) default NULL,
  `usr_email` tinytext,
  `usr_URL` tinytext,
  `usr_addr` tinytext,
  `usr_addr2` tinytext,
  `usr_city` tinytext,
  `usr_state` tinytext,
  `usr_zip` tinytext,
  `usr_country` tinytext,
  `usr_phone` tinytext,
  `usr_shell` tinytext,
  `usr_pswd` tinytext NOT NULL,
  `usr_w_pswd` tinytext,
  `unix_uid` smallint(5) unsigned NOT NULL default '0',
  `status` enum('newuser','unapproved','unverified','active','frozen','archived','other') NOT NULL default 'newuser',
  `admin` tinyint(4) default '0',
  `foreign_admin` tinyint(4) default '0',
  `dbedit` tinyint(4) default '0',
  `stud` tinyint(4) default '0',
  `webonly` tinyint(4) default '0',
  `pswd_expires` date default NULL,
  `cvsweb` tinyint(4) NOT NULL default '0',
  `emulab_pubkey` text,
  `home_pubkey` text,
  `adminoff` tinyint(4) default '0',
  `verify_key` varchar(32) default NULL,
  `widearearoot` tinyint(4) default '0',
  `wideareajailroot` tinyint(4) default '0',
  `notes` text,
  `weblogin_frozen` tinyint(3) unsigned NOT NULL default '0',
  `weblogin_failcount` smallint(5) unsigned NOT NULL default '0',
  `weblogin_failstamp` int(10) unsigned NOT NULL default '0',
  `plab_user` tinyint(1) NOT NULL default '0',
  `user_interface` enum('emulab','plab') NOT NULL default 'emulab',
  `chpasswd_key` varchar(32) default NULL,
  `chpasswd_expires` int(10) unsigned NOT NULL default '0',
  `wikiname` tinytext,
  `wikionly` tinyint(1) default '0',
  `mailman_password` tinytext,
  PRIMARY KEY  (`uid_idx`),
  UNIQUE KEY `uid` (`uid`),
  KEY `unix_uid` (`unix_uid`),
  KEY `status` (`status`),
  KEY `uid_uuid` (`uid_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `userslastlogin`
--

DROP TABLE IF EXISTS `userslastlogin`;
CREATE TABLE `userslastlogin` (
  `uid` varchar(10) NOT NULL default '',
  `date` date default NULL,
  `time` time default NULL,
  PRIMARY KEY  (`uid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `usrp_orders`
--

DROP TABLE IF EXISTS `usrp_orders`;
CREATE TABLE `usrp_orders` (
  `order_id` varchar(32) NOT NULL default '',
  `email` tinytext,
  `name` tinytext,
  `phone` tinytext,
  `affiliation` tinytext,
  `num_mobos` int(11) default '0',
  `num_dboards` int(11) default '0',
  `intended_use` tinytext,
  `comments` tinytext,
  `order_date` datetime default NULL,
  `modify_date` datetime default NULL,
  PRIMARY KEY  (`order_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `v2pmap`
--

DROP TABLE IF EXISTS `v2pmap`;
CREATE TABLE `v2pmap` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `vname` varchar(32) NOT NULL default '',
  `node_id` varchar(32) NOT NULL default '',
  PRIMARY KEY  (`exptidx`,`vname`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`vname`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `veth_interfaces`
--

DROP TABLE IF EXISTS `veth_interfaces`;
CREATE TABLE `veth_interfaces` (
  `node_id` varchar(32) NOT NULL default '',
  `veth_id` int(10) unsigned NOT NULL auto_increment,
  `mac` varchar(12) NOT NULL default '000000000000',
  `IP` varchar(15) default NULL,
  `mask` varchar(15) default NULL,
  `iface` varchar(10) default NULL,
  `vnode_id` varchar(32) default NULL,
  `rtabid` smallint(5) unsigned NOT NULL default '0',
  PRIMARY KEY  (`node_id`,`veth_id`),
  KEY `IP` (`IP`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `vinterfaces`
--

DROP TABLE IF EXISTS `vinterfaces`;
CREATE TABLE `vinterfaces` (
  `node_id` varchar(32) NOT NULL default '',
  `unit` int(10) unsigned NOT NULL auto_increment,
  `mac` varchar(12) NOT NULL default '000000000000',
  `IP` varchar(15) default NULL,
  `mask` varchar(15) default NULL,
  `type` enum('alias','veth','veth-ne','vlan') NOT NULL default 'veth',
  `iface` varchar(10) default NULL,
  `rtabid` smallint(5) unsigned NOT NULL default '0',
  `vnode_id` varchar(32) default NULL,
  `exptidx` int(10) NOT NULL default '0',
  `virtlanidx` int(11) NOT NULL default '0',
  `vlanid` int(11) NOT NULL default '0',
  PRIMARY KEY  (`node_id`,`unit`),
  KEY `bynode` (`node_id`,`iface`),
  KEY `type` (`type`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_agents`
--

DROP TABLE IF EXISTS `virt_agents`;
CREATE TABLE `virt_agents` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `vname` varchar(64) NOT NULL default '',
  `vnode` varchar(32) NOT NULL default '',
  `objecttype` smallint(5) unsigned NOT NULL default '0',
  PRIMARY KEY  (`exptidx`,`vname`,`vnode`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`vname`,`vnode`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_firewalls`
--

DROP TABLE IF EXISTS `virt_firewalls`;
CREATE TABLE `virt_firewalls` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `fwname` varchar(32) NOT NULL default '',
  `type` enum('ipfw','ipfw2','ipchains','ipfw2-vlan') NOT NULL default 'ipfw',
  `style` enum('open','closed','basic','emulab') NOT NULL default 'basic',
  `log` tinytext NOT NULL,
  PRIMARY KEY  (`exptidx`,`fwname`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`fwname`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_lan_lans`
--

DROP TABLE IF EXISTS `virt_lan_lans`;
CREATE TABLE `virt_lan_lans` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `idx` int(11) NOT NULL auto_increment,
  `vname` varchar(32) NOT NULL default '',
  PRIMARY KEY  (`exptidx`,`idx`),
  UNIQUE KEY `vname` (`pid`,`eid`,`vname`),
  UNIQUE KEY `idx` (`pid`,`eid`,`idx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_lan_member_settings`
--

DROP TABLE IF EXISTS `virt_lan_member_settings`;
CREATE TABLE `virt_lan_member_settings` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `vname` varchar(32) NOT NULL default '',
  `member` varchar(32) NOT NULL default '',
  `capkey` varchar(32) NOT NULL default '',
  `capval` varchar(64) NOT NULL default '',
  PRIMARY KEY  (`exptidx`,`vname`,`member`,`capkey`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`vname`,`member`,`capkey`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_lan_settings`
--

DROP TABLE IF EXISTS `virt_lan_settings`;
CREATE TABLE `virt_lan_settings` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `vname` varchar(32) NOT NULL default '',
  `capkey` varchar(32) NOT NULL default '',
  `capval` varchar(64) NOT NULL default '',
  PRIMARY KEY  (`exptidx`,`vname`,`capkey`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`vname`,`capkey`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_lans`
--

DROP TABLE IF EXISTS `virt_lans`;
CREATE TABLE `virt_lans` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `vname` varchar(32) NOT NULL default '',
  `vnode` varchar(32) NOT NULL default '',
  `vport` tinyint(3) NOT NULL default '0',
  `ip` varchar(15) NOT NULL default '',
  `delay` float(10,2) default '0.00',
  `bandwidth` int(10) unsigned default NULL,
  `est_bandwidth` int(10) unsigned default NULL,
  `lossrate` float(10,5) default NULL,
  `q_limit` int(11) default '0',
  `q_maxthresh` int(11) default '0',
  `q_minthresh` int(11) default '0',
  `q_weight` float default '0',
  `q_linterm` int(11) default '0',
  `q_qinbytes` tinyint(4) default '0',
  `q_bytes` tinyint(4) default '0',
  `q_meanpsize` int(11) default '0',
  `q_wait` int(11) default '0',
  `q_setbit` int(11) default '0',
  `q_droptail` int(11) default '0',
  `q_red` tinyint(4) default '0',
  `q_gentle` tinyint(4) default '0',
  `member` text,
  `mask` varchar(15) default '255.255.255.0',
  `rdelay` float(10,2) default NULL,
  `rbandwidth` int(10) unsigned default NULL,
  `rest_bandwidth` int(10) unsigned default NULL,
  `rlossrate` float(10,5) default NULL,
  `cost` float NOT NULL default '1',
  `widearea` tinyint(4) default '0',
  `emulated` tinyint(4) default '0',
  `uselinkdelay` tinyint(4) default '0',
  `nobwshaping` tinyint(4) default '0',
  `mustdelay` tinyint(1) default '0',
  `usevethiface` tinyint(4) default '0',
  `encap_style` enum('alias','veth','veth-ne','vlan','vtun','egre','gre','default') NOT NULL default 'default',
  `trivial_ok` tinyint(4) default '1',
  `protocol` varchar(30) NOT NULL default 'ethernet',
  `is_accesspoint` tinyint(4) default '0',
  `traced` tinyint(1) default '0',
  `trace_type` enum('header','packet','monitor') NOT NULL default 'header',
  `trace_expr` tinytext,
  `trace_snaplen` int(11) NOT NULL default '0',
  `trace_endnode` tinyint(1) NOT NULL default '0',
  `trace_db` tinyint(1) NOT NULL default '0',
  `fixed_iface` varchar(16) default '',
  PRIMARY KEY  (`exptidx`,`vname`,`vnode`,`vport`),
  UNIQUE KEY `vport` (`pid`,`eid`,`vname`,`vnode`,`vport`),
  KEY `pid` (`pid`,`eid`,`vname`),
  KEY `vnode` (`pid`,`eid`,`vnode`),
  KEY `pideid` (`pid`,`eid`,`vname`,`vnode`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_node_desires`
--

DROP TABLE IF EXISTS `virt_node_desires`;
CREATE TABLE `virt_node_desires` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `vname` varchar(32) NOT NULL default '',
  `desire` varchar(30) NOT NULL default '',
  `weight` float default NULL,
  PRIMARY KEY  (`exptidx`,`vname`,`desire`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`vname`,`desire`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_node_motelog`
--

DROP TABLE IF EXISTS `virt_node_motelog`;
CREATE TABLE `virt_node_motelog` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `vname` varchar(32) NOT NULL default '',
  `logfileid` varchar(45) NOT NULL default '',
  PRIMARY KEY  (`pid`,`eid`,`vname`,`logfileid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_node_startloc`
--

DROP TABLE IF EXISTS `virt_node_startloc`;
CREATE TABLE `virt_node_startloc` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `vname` varchar(32) NOT NULL default '',
  `building` varchar(32) NOT NULL default '',
  `floor` varchar(32) NOT NULL default '',
  `loc_x` float NOT NULL default '0',
  `loc_y` float NOT NULL default '0',
  `orientation` float NOT NULL default '0',
  PRIMARY KEY  (`exptidx`,`vname`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`vname`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_nodes`
--

DROP TABLE IF EXISTS `virt_nodes`;
CREATE TABLE `virt_nodes` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `ips` text,
  `osname` varchar(20) default NULL,
  `cmd_line` text,
  `rpms` text,
  `deltas` text,
  `startupcmd` tinytext,
  `tarfiles` text,
  `vname` varchar(32) NOT NULL default '',
  `type` varchar(30) default NULL,
  `failureaction` enum('fatal','nonfatal','ignore') NOT NULL default 'fatal',
  `routertype` enum('none','ospf','static','manual','static-ddijk','static-old') NOT NULL default 'none',
  `fixed` text NOT NULL,
  `inner_elab_role` enum('boss','boss+router','router','ops','ops+fs','fs','node') default NULL,
  `plab_role` enum('plc','node','none') NOT NULL default 'none',
  `plab_plcnet` varchar(32) NOT NULL default 'none',
  `numeric_id` int(11) default NULL,
  PRIMARY KEY  (`exptidx`,`vname`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`vname`),
  KEY `pid` (`pid`,`eid`,`vname`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_parameters`
--

DROP TABLE IF EXISTS `virt_parameters`;
CREATE TABLE `virt_parameters` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `name` varchar(64) NOT NULL default '',
  `value` tinytext,
  `description` text,
  PRIMARY KEY  (`exptidx`,`name`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_programs`
--

DROP TABLE IF EXISTS `virt_programs`;
CREATE TABLE `virt_programs` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `vnode` varchar(32) NOT NULL default '',
  `vname` varchar(32) NOT NULL default '',
  `command` tinytext,
  `dir` tinytext,
  `timeout` int(10) unsigned default NULL,
  `expected_exit_code` tinyint(4) unsigned default NULL,
  PRIMARY KEY  (`exptidx`,`vnode`,`vname`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`vnode`,`vname`),
  KEY `vnode` (`vnode`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_routes`
--

DROP TABLE IF EXISTS `virt_routes`;
CREATE TABLE `virt_routes` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `vname` varchar(32) NOT NULL default '',
  `src` varchar(32) NOT NULL default '',
  `dst` varchar(32) NOT NULL default '',
  `dst_type` enum('host','net') NOT NULL default 'host',
  `dst_mask` varchar(15) default '255.255.255.0',
  `nexthop` varchar(32) NOT NULL default '',
  `cost` int(11) NOT NULL default '0',
  PRIMARY KEY  (`exptidx`,`vname`,`src`,`dst`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`vname`,`src`,`dst`),
  KEY `pid` (`pid`,`eid`,`vname`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_simnode_attributes`
--

DROP TABLE IF EXISTS `virt_simnode_attributes`;
CREATE TABLE `virt_simnode_attributes` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `vname` varchar(32) NOT NULL default '',
  `nodeweight` smallint(5) unsigned NOT NULL default '1',
  `eventrate` int(11) unsigned NOT NULL default '0',
  PRIMARY KEY  (`exptidx`,`vname`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`vname`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_tiptunnels`
--

DROP TABLE IF EXISTS `virt_tiptunnels`;
CREATE TABLE `virt_tiptunnels` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `host` varchar(32) NOT NULL default '',
  `vnode` varchar(32) NOT NULL default '',
  PRIMARY KEY  (`exptidx`,`host`,`vnode`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`host`,`vnode`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_trafgens`
--

DROP TABLE IF EXISTS `virt_trafgens`;
CREATE TABLE `virt_trafgens` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `vnode` varchar(32) NOT NULL default '',
  `vname` varchar(32) NOT NULL default '',
  `role` tinytext NOT NULL,
  `proto` tinytext NOT NULL,
  `port` int(11) NOT NULL default '0',
  `ip` varchar(15) NOT NULL default '',
  `target_vnode` varchar(32) NOT NULL default '',
  `target_vname` varchar(32) NOT NULL default '',
  `target_port` int(11) NOT NULL default '0',
  `target_ip` varchar(15) NOT NULL default '',
  `generator` tinytext NOT NULL,
  PRIMARY KEY  (`exptidx`,`vnode`,`vname`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`vnode`,`vname`),
  KEY `vnode` (`vnode`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_user_environment`
--

DROP TABLE IF EXISTS `virt_user_environment`;
CREATE TABLE `virt_user_environment` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `idx` int(10) unsigned NOT NULL auto_increment,
  `name` varchar(255) NOT NULL default '',
  `value` text,
  PRIMARY KEY  (`exptidx`,`idx`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`idx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `virt_vtypes`
--

DROP TABLE IF EXISTS `virt_vtypes`;
CREATE TABLE `virt_vtypes` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `name` varchar(32) NOT NULL default '',
  `weight` float(7,5) NOT NULL default '0.00000',
  `members` text,
  PRIMARY KEY  (`exptidx`,`name`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `vis_graphs`
--

DROP TABLE IF EXISTS `vis_graphs`;
CREATE TABLE `vis_graphs` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `zoom` decimal(8,3) NOT NULL default '0.000',
  `detail` tinyint(2) NOT NULL default '0',
  `image` mediumblob,
  PRIMARY KEY  (`exptidx`),
  UNIQUE KEY `pideid` (`pid`,`eid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `vis_nodes`
--

DROP TABLE IF EXISTS `vis_nodes`;
CREATE TABLE `vis_nodes` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `vname` varchar(32) NOT NULL default '',
  `vis_type` varchar(10) NOT NULL default '',
  `x` float NOT NULL default '0',
  `y` float NOT NULL default '0',
  PRIMARY KEY  (`exptidx`,`vname`),
  UNIQUE KEY `pideid` (`pid`,`eid`,`vname`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `vlans`
--

DROP TABLE IF EXISTS `vlans`;
CREATE TABLE `vlans` (
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `pid` varchar(12) NOT NULL default '',
  `virtual` varchar(64) default NULL,
  `members` text NOT NULL,
  `id` int(11) NOT NULL auto_increment,
  `tag` smallint(5) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `pid` (`pid`,`eid`,`virtual`),
  KEY `exptidx` (`exptidx`,`virtual`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `webcams`
--

DROP TABLE IF EXISTS `webcams`;
CREATE TABLE `webcams` (
  `id` int(11) unsigned NOT NULL default '0',
  `server` varchar(64) NOT NULL default '',
  `last_update` datetime default NULL,
  `URL` tinytext,
  `stillimage_URL` tinytext,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `webdb_table_permissions`
--

DROP TABLE IF EXISTS `webdb_table_permissions`;
CREATE TABLE `webdb_table_permissions` (
  `table_name` varchar(64) NOT NULL default '',
  `allow_read` tinyint(1) default '1',
  `allow_row_add_edit` tinyint(1) default '0',
  `allow_row_delete` tinyint(1) default '0',
  PRIMARY KEY  (`table_name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `webnews`
--

DROP TABLE IF EXISTS `webnews`;
CREATE TABLE `webnews` (
  `msgid` int(11) NOT NULL auto_increment,
  `subject` tinytext,
  `date` datetime default NULL,
  `author` varchar(32) default NULL,
  `body` text,
  `archived` tinyint(1) NOT NULL default '0',
  `archived_date` datetime default NULL,
  PRIMARY KEY  (`msgid`),
  KEY `date` (`date`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `widearea_accounts`
--

DROP TABLE IF EXISTS `widearea_accounts`;
CREATE TABLE `widearea_accounts` (
  `uid` varchar(8) NOT NULL default '',
  `uid_idx` mediumint(8) unsigned NOT NULL default '0',
  `node_id` varchar(32) NOT NULL default '',
  `trust` enum('none','user','local_root') default NULL,
  `date_applied` date default NULL,
  `date_approved` datetime default NULL,
  PRIMARY KEY  (`uid`,`node_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `widearea_delays`
--

DROP TABLE IF EXISTS `widearea_delays`;
CREATE TABLE `widearea_delays` (
  `time` double default NULL,
  `node_id1` varchar(32) NOT NULL default '',
  `iface1` varchar(10) NOT NULL default '',
  `node_id2` varchar(32) NOT NULL default '',
  `iface2` varchar(10) NOT NULL default '',
  `bandwidth` double default NULL,
  `time_stddev` float NOT NULL default '0',
  `lossrate` float NOT NULL default '0',
  `start_time` int(10) unsigned default NULL,
  `end_time` int(10) unsigned default NULL,
  PRIMARY KEY  (`node_id1`,`iface1`,`node_id2`,`iface2`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `widearea_nodeinfo`
--

DROP TABLE IF EXISTS `widearea_nodeinfo`;
CREATE TABLE `widearea_nodeinfo` (
  `node_id` varchar(32) NOT NULL default '',
  `machine_type` varchar(40) default NULL,
  `contact_uid` varchar(8) NOT NULL default '',
  `contact_idx` mediumint(8) unsigned NOT NULL default '0',
  `connect_type` varchar(20) default NULL,
  `city` tinytext,
  `state` tinytext,
  `country` tinytext,
  `zip` tinytext,
  `external_node_id` tinytext,
  `hostname` varchar(255) default NULL,
  `site` varchar(255) default NULL,
  `latitude` float default NULL,
  `longitude` float default NULL,
  `bwlimit` varchar(32) default NULL,
  `privkey` varchar(128) default NULL,
  `IP` varchar(15) default NULL,
  `gateway` varchar(15) NOT NULL default '',
  `dns` tinytext NOT NULL,
  `boot_method` enum('static','dhcp','') NOT NULL default '',
  PRIMARY KEY  (`node_id`),
  KEY `IP` (`IP`),
  KEY `privkey` (`privkey`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `widearea_privkeys`
--

DROP TABLE IF EXISTS `widearea_privkeys`;
CREATE TABLE `widearea_privkeys` (
  `privkey` varchar(64) NOT NULL default '',
  `IP` varchar(15) NOT NULL default '1.1.1.1',
  `user_name` tinytext NOT NULL,
  `user_email` tinytext NOT NULL,
  `cdkey` varchar(64) default NULL,
  `nextprivkey` varchar(64) default NULL,
  `rootkey` varchar(64) default NULL,
  `lockkey` varchar(64) default NULL,
  `requested` datetime NOT NULL default '0000-00-00 00:00:00',
  `updated` datetime NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (`privkey`,`IP`),
  KEY `IP` (`IP`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `widearea_recent`
--

DROP TABLE IF EXISTS `widearea_recent`;
CREATE TABLE `widearea_recent` (
  `time` double default NULL,
  `node_id1` varchar(32) NOT NULL default '',
  `iface1` varchar(10) NOT NULL default '',
  `node_id2` varchar(32) NOT NULL default '',
  `iface2` varchar(10) NOT NULL default '',
  `bandwidth` double default NULL,
  `time_stddev` float NOT NULL default '0',
  `lossrate` float NOT NULL default '0',
  `start_time` int(10) unsigned default NULL,
  `end_time` int(10) unsigned default NULL,
  PRIMARY KEY  (`node_id1`,`iface1`,`node_id2`,`iface2`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `widearea_updates`
--

DROP TABLE IF EXISTS `widearea_updates`;
CREATE TABLE `widearea_updates` (
  `IP` varchar(15) NOT NULL default '1.1.1.1',
  `roottag` tinytext NOT NULL,
  `update_requested` datetime NOT NULL default '0000-00-00 00:00:00',
  `update_started` datetime default NULL,
  `force` enum('yes','no') NOT NULL default 'no',
  PRIMARY KEY  (`IP`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `wireless_stats`
--

DROP TABLE IF EXISTS `wireless_stats`;
CREATE TABLE `wireless_stats` (
  `name` varchar(32) NOT NULL default '',
  `floor` varchar(32) NOT NULL default '',
  `building` varchar(32) NOT NULL default '',
  `data_eid` varchar(32) default NULL,
  `data_pid` varchar(32) default NULL,
  `type` varchar(32) default NULL,
  `altsrc` tinytext,
  PRIMARY KEY  (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `wires`
--

DROP TABLE IF EXISTS `wires`;
CREATE TABLE `wires` (
  `cable` smallint(3) unsigned default NULL,
  `len` tinyint(3) unsigned NOT NULL default '0',
  `type` enum('Node','Serial','Power','Dnard','Control','Trunk','OuterControl') NOT NULL default 'Node',
  `node_id1` char(32) NOT NULL default '',
  `card1` tinyint(3) unsigned NOT NULL default '0',
  `port1` tinyint(3) unsigned NOT NULL default '0',
  `node_id2` char(32) NOT NULL default '',
  `card2` tinyint(3) unsigned NOT NULL default '0',
  `port2` tinyint(3) unsigned NOT NULL default '0',
  PRIMARY KEY  (`node_id1`,`card1`,`port1`),
  KEY `node_id2` (`node_id2`,`card2`),
  KEY `dest` (`node_id2`,`card2`,`port2`),
  KEY `src` (`node_id1`,`card1`,`port1`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;


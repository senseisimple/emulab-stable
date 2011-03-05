DROP TABLE IF EXISTS `geni_users`;
CREATE TABLE `geni_users` (
  `hrn` varchar(256) NOT NULL default '',
  `uid` varchar(8) NOT NULL default '',
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `expires` datetime default NULL,
  `locked` datetime default NULL,
  `archived` datetime default NULL,
  `status` enum('active','archived','frozen') NOT NULL default 'frozen',
  `name` tinytext,
  `email` tinytext,
  `sa_uuid` varchar(40) NOT NULL default '',
  PRIMARY KEY  (`idx`),
  KEY `hrn` (`hrn`),
  UNIQUE KEY `uuid` (`uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `geni_components`;
CREATE TABLE `geni_components` (
  `hrn` varchar(256) NOT NULL default '',
  `uuid` varchar(40) NOT NULL default '',
  `manager_uuid` varchar(40) default NULL,
  `created` datetime default NULL,
  `expires` datetime default NULL,
  `url` tinytext,
  PRIMARY KEY  (`uuid`),
  UNIQUE KEY `hrn` (`hrn`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `geni_authorities`;
CREATE TABLE `geni_authorities` (
  `hrn` varchar(256) NOT NULL default '',
  `uuid` varchar(40) NOT NULL default '',
  `uuid_prefix` varchar(12) NOT NULL default '',
  `created` datetime default NULL,
  `expires` datetime default NULL,
  `type` enum('sa','ma','ch','cm','ses','am') NOT NULL default 'sa',
  `disabled` tinyint(1) NOT NULL default '0',
  `url` tinytext,
  `urn` tinytext,
  PRIMARY KEY  (`uuid`),
  UNIQUE KEY `urn` (`urn`(255))
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `geni_slices`;
CREATE TABLE `geni_slices` (
  `hrn` varchar(256) NOT NULL default '',
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `uuid` varchar(40) NOT NULL default '',
  `exptidx` int(11) default NULL,
  `created` datetime default NULL,
  `expires` datetime default NULL,
  `shutdown` datetime default NULL,
  `locked` datetime default NULL,
  `stitch_locked` datetime default NULL,
  `creator_uuid` varchar(40) NOT NULL default '',
  `creator_urn` tinytext,
  `name` tinytext,
  `sa_uuid` varchar(40) NOT NULL default '',
  `registered` tinyint(1) NOT NULL default '0',
  `needsfirewall` tinyint(1) NOT NULL default '0',
  `isplaceholder` tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `hrn` (`hrn`),
  UNIQUE KEY `uuid` (`uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `geni_slivers`;
CREATE TABLE `geni_slivers` (
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `uuid` varchar(40) NOT NULL default '',
  `hrn` varchar(256) NOT NULL default '',
  `nickname` varchar(256) default NULL,
  `slice_uuid` varchar(40) NOT NULL default '',
  `creator_uuid` varchar(40) NOT NULL default '',
  `resource_uuid` varchar(40) NOT NULL default '',
  `resource_type` varchar(40) NOT NULL default '',
  `resource_id` varchar(64) NOT NULL default '',
  `created` datetime default NULL,
  `expires` datetime default NULL,
  `locked` datetime default NULL,
  `credential_idx` int(10) unsigned default NULL,
  `component_uuid` varchar(40) default NULL,
  `aggregate_uuid` varchar(40) default NULL,
  `status` varchar(16) NOT NULL default 'created',
  `state` varchar(16) NOT NULL default 'stopped',
  `rspec_string` text,
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `uuid` (`uuid`),
  INDEX `slice_uuid` (`slice_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `geni_aggregates`;
CREATE TABLE `geni_aggregates` (
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `hrn` varchar(256) NOT NULL default '',
  `nickname` varchar(256) default NULL,
  `uuid` varchar(40) NOT NULL default '',
  `type` varchar(40) NOT NULL default '',
  `slice_uuid` varchar(40) NOT NULL default '',
  `creator_uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `expires` datetime default NULL,
  `locked` datetime default NULL,
  `registered` datetime default NULL,
  `credential_idx` int(10) unsigned default NULL,
  `component_idx` int(10) unsigned NOT NULL default '0',
  `aggregate_idx` int(10) unsigned default NULL,
  `status` varchar(16) NOT NULL default 'created',
  `state` varchar(16) NOT NULL default 'stopped',
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `uuid` (`uuid`),
  INDEX `slice_uuid` (`slice_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `geni_tickets`;
CREATE TABLE `geni_tickets` (
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `ticket_uuid` varchar(40) NOT NULL default '',
  `owner_uuid` varchar(40) NOT NULL default '',
  `slice_uuid` varchar(40) NOT NULL default '',
  `target_uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `redeem_before` datetime default NULL,
  `redeemed` datetime default NULL,
  `locked` datetime default NULL,
  `valid_until` datetime default NULL,
  `component_uuid` varchar(40) NOT NULL default '',
  `seqno` int(10) unsigned NOT NULL default '0',
  `ticket_string` text,
  PRIMARY KEY  (`idx`), 
  INDEX `owner_uuid` (`owner_uuid`),
  INDEX `slice_uuid` (`slice_uuid`),
  UNIQUE KEY `compseqno` (`component_uuid`, `seqno`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `geni_credentials`;
CREATE TABLE `geni_credentials` (
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `uuid` varchar(40) NOT NULL default '',
  `owner_uuid` varchar(40) NOT NULL default '',
  `this_uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `valid_until` datetime default NULL,
  `credential_string` text,
  PRIMARY KEY  (`idx`),
  INDEX `owner_uuid` (`owner_uuid`),
  INDEX `this_uuid` (`this_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `geni_certificates`;
CREATE TABLE `geni_certificates` (
  `uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `expires` datetime default NULL,
  `revoked` datetime default NULL,
  `serial` int(10) unsigned NOT NULL default '0',
  `cert` text,
  `DN` text,
  `privkey` text,
  `uri` text,
  `urn` text,
  PRIMARY KEY  (`uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `geni_crls`;
CREATE TABLE `geni_crls` (
  `uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `expires` datetime default NULL,
  `cert` text,
  `DN` text,
  PRIMARY KEY  (`uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE `geni_manifests` (
  `idx` int(10) unsigned NOT NULL auto_increment,
  `slice_uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `manifest` text,
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `slice_uuid` (`slice_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `geni_userkeys`;
CREATE TABLE `geni_userkeys` (
  `type` enum('ssh','password') NOT NULL default 'ssh',
  `uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `key` text,
  INDEX `uuid` (`uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `geni_resources`;
CREATE TABLE `geni_resources` (
  `pid` varchar(12) NOT NULL default '',
  `eid` varchar(32) NOT NULL default '',
  `exptidx` int(11) NOT NULL default '0',
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `manager_urn` tinytext,
  `created` datetime default NULL,
  `expires` datetime default NULL,
  `updated` datetime default NULL,
  `slice_idx` mediumint(8) unsigned NOT NULL default '0',
  `credential_idx` mediumint(8) unsigned NOT NULL default '0',
  `manifest_idx` mediumint(8) unsigned NOT NULL default '0',
  `ticket_idx` mediumint(8) unsigned NOT NULL default '0',
  `newticket_idx` mediumint(8) unsigned NOT NULL default '0',
  `rspec_idx` mediumint(8) unsigned default NULL,
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `manager` (`exptidx`,`manager_urn`(255))
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `geni_rspecs`;
CREATE TABLE `geni_rspecs` (
  `idx` int(10) unsigned NOT NULL auto_increment,
  `created` datetime default NULL,
  `rspec` text,
  PRIMARY KEY  (`idx`),
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `geni_bindings`;
CREATE TABLE `geni_bindings` (
  `slice_uuid` varchar(40) NOT NULL default '',
  `user_uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  PRIMARY KEY  (`slice_uuid`,`user_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `version_info`;
CREATE TABLE `version_info` (
  `name` varchar(32) NOT NULL default '',
  `value` tinytext NOT NULL,
  PRIMARY KEY  (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
REPLACE INTO `version_info` VALUES ('dbrev', '0');

DROP TABLE IF EXISTS `sliver_history`;
CREATE TABLE `sliver_history` (
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `uuid` varchar(40) NOT NULL default '',
  `hrn` varchar(256) NOT NULL default '',
  `slice_uuid` varchar(40) NOT NULL default '',
  `slice_hrn` varchar(256) NOT NULL default '',
  `creator_uuid` varchar(40) NOT NULL default '',
  `creator_hrn` varchar(256) NOT NULL default '',
  `resource_uuid` varchar(40) NOT NULL default '',
  `resource_type` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `destroyed` datetime default NULL,
  `component_uuid` varchar(40) default NULL,
  `component_hrn` varchar(256) default NULL,
  `aggregate_uuid` varchar(40) default NULL,
  `rspec_string` text,
  PRIMARY KEY  (`idx`),
  KEY `uuid` (`uuid`),
  INDEX `slice_uuid` (`slice_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `aggregate_history`;
CREATE TABLE `aggregate_history` (
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `uuid` varchar(40) NOT NULL default '',
  `hrn` varchar(256) NOT NULL default '',
  `type` varchar(40) NOT NULL default '',
  `slice_uuid` varchar(40) NOT NULL default '',
  `slice_hrn` varchar(256) NOT NULL default '',
  `creator_uuid` varchar(40) NOT NULL default '',
  `creator_hrn` varchar(256) NOT NULL default '',
  `created` datetime default NULL,
  `destroyed` datetime default NULL,
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `uuid` (`uuid`),
  INDEX `slice_uuid` (`slice_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE `manifest_history` (
  `idx` int(10) unsigned NOT NULL auto_increment,
  `aggregate_uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `manifest` text,
  PRIMARY KEY  (`idx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `ticket_history`;
CREATE TABLE `ticket_history` (
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `uuid` varchar(40) NOT NULL default '',
  `owner_uuid` varchar(40) NOT NULL default '',
  `owner_hrn` varchar(256) NOT NULL default '',
  `slice_uuid` varchar(40) NOT NULL default '',
  `slice_hrn` varchar(256) NOT NULL default '',
  `created` datetime default NULL,
  `redeemed` datetime default NULL,
  `expired` datetime default NULL,
  `released` datetime default NULL,
  `component_uuid` varchar(40) NOT NULL default '',
  `component_hrn` varchar(256) default NULL,
  `rspec_string` text,
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `uuid` (`uuid`),
  INDEX `slice_uuid` (`slice_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `client_slivers`;
CREATE TABLE `client_slivers` (
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `urn` varchar(256) NOT NULL default '',
  `slice_idx` mediumint(8) unsigned NOT NULL default '0',
  `manager_urn` varchar(256) NOT NULL default '',
  `creator_idx` mediumint(8) unsigned NOT NULL default '0',
  `created` datetime default NULL,
  `expires` datetime default NULL,
  `locked` datetime default NULL,
  `manifest` text,
  PRIMARY KEY  (`idx`),
  INDEX `slice_uuid` (`slice_idx`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;


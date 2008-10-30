#
# These are new tables to add to the Emulab DB. 
#

#
# Remote SAs register users at Emulab (we export GENI ClearingHouse
# APIs, whatever they are). Local users do not need to be in this table,
# we can get their data via the Emulab libraries. 
#
#  * This is a GENI ClearingHouse table; remote emulabs have just a
#    local users table. 
#  * Users in this table are users at other emulabs.
#  * uid_idx is for indexing into other tables, like user_sslcerts and
#    user_pubkeys. No need to have geni versions of those tables, as
#    long as we maintain uid_idx uniqueness. 
#  * sa_idx is an index into another table. SAs in in the prototype are
#    other Emulabs.
#  * The uid does not have to be unique, except for a given SA. 
#
DROP TABLE IF EXISTS `geni_users`;
CREATE TABLE `geni_users` (
  `hrn` varchar(256) NOT NULL default '',
  `uid` varchar(8) NOT NULL default '',
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `archived` datetime default NULL,
  `status` enum('active','archived','frozen') NOT NULL default 'frozen',
  `name` tinytext,
  `email` tinytext,
  `sa_uuid` varchar(40) NOT NULL default '',
  PRIMARY KEY  (`idx`),
  KEY `hrn` (`hrn`),
  UNIQUE KEY `uuid` (`uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

#
# All geni components have a GID (UUID->Key) and this
# table stores them all. In protogeni there are not that many components,
# and they mostly refer to other Emulabs.
#
#  * This is a GENI Clearinghouse table; Components are remote emulabs.
#  * The ID is a GENI dotted name.
#  * We store the pubkey for the component here. Remember that
#    components self generate their keys.
#  * Not worrying about Management Authorities at this point. I think
#    the prototype has a single MA, and its operated by hand with
#    direct mysql statements.
#
DROP TABLE IF EXISTS `geni_components`;
CREATE TABLE `geni_components` (
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `hrn` varchar(256) NOT NULL default '',
  `uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `url` tinytext,
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `hrn` (`hrn`),
  UNIQUE KEY `uuid` (`uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

#
# Geni Slice Authorities also have a GID. As with components, SAs are
# mostly other emulabs. GENI users are registered by SAs, as are slices.
#
#  * This is a GENI Clearinghouse table; SAs are remote emulabs.
#  * The ID is a GENI dotted name.
#  * We store the pubkey for the SA here. Remember that SAs self
#    generate their keys.
#  * The uuid_prefix is the topmost 8 bytes assigned to the SA; all
#    UUIDs generated (signed) by and registered must have these top
#    8 bytes. See wiki discussion.
#
DROP TABLE IF EXISTS `geni_authorities`;
CREATE TABLE `geni_authorities` (
  `hrn` varchar(256) NOT NULL default '',
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `uuid` varchar(40) NOT NULL default '',
  `uuid_prefix` varchar(12) NOT NULL default '',
  `created` datetime default NULL,
  `type` enum('sa','cm','ma','ch') NOT NULL default 'sa',
  `url` tinytext,
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `uuid` (`uuid`),
  UNIQUE KEY `uuid_prefix` (`uuid_prefix`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

#
# Geni Slices. Not to be confused with plab_slices ... a geni slice
# is an emulab experiment that spans the entire set of emulabs. SAs
# register geni slices at the Clearinghouse, in this table.
# 
# * Slices have UUIDs.
# * The sa_idx refers to the slice authority that created the slice.
# * The creator UUID should already be in the geni_users table, or in
#   the local users table.
# * The pubkey bound to the slice is that of the user creating the slice.
# 
DROP TABLE IF EXISTS `geni_slices`;
CREATE TABLE `geni_slices` (
  `hrn` varchar(256) NOT NULL default '',
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `uuid` varchar(40) NOT NULL default '',
  `exptidx` int(11) default NULL,
  `created` datetime default NULL,
  `creator_uuid` varchar(40) NOT NULL default '',
  `name` tinytext,
  `sa_uuid` varchar(40) NOT NULL default '',
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `hrn` (`hrn`),
  UNIQUE KEY `uuid` (`uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

#
# Geni Slivers. Created on components (by CMs of course). Locally, a sliver
# corresponds to an allocated node. 
#
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
  `created` datetime default NULL,
  `credential_idx` int(10) unsigned default NULL,
  `component_uuid` varchar(40) default NULL,
  `aggregate_uuid` varchar(40) default NULL,
  `status` enum('created','ready','broken') NOT NULL default 'created',
  `rspec_string` text,
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `uuid` (`uuid`),
  INDEX `slice_uuid` (`slice_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

#
# Geni Aggregates, which are a collection of resources (nodes, links, etc).
#
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
  `credential_idx` int(10) unsigned default NULL,
  `component_idx` int(10) unsigned NOT NULL default '0',
  `aggregate_idx` int(10) unsigned default NULL,
  `status` enum('created','ready','broken') NOT NULL default 'created',
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `uuid` (`uuid`),
  INDEX `slice_uuid` (`slice_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

#
# Table to remember tickets.
#
DROP TABLE IF EXISTS `geni_tickets`;
CREATE TABLE `geni_tickets` (
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `ticket_uuid` varchar(40) NOT NULL default '',
  `owner_uuid` varchar(40) NOT NULL default '',
  `slice_uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `redeem_before` datetime default NULL,
  `valid_until` datetime default NULL,
  `component_uuid` varchar(40) NOT NULL default '',
  `seqno` int(10) unsigned NOT NULL default '0',
  `ticket_string` text,
  PRIMARY KEY  (`idx`), 
  INDEX `owner_uuid` (`owner_uuid`),
  INDEX `slice_uuid` (`slice_uuid`),
  UNIQUE KEY `compseqno` (`component_uuid`, `seqno`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

#
# Table to remember credentials.
#
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

#
# Table to hold uuid<->certificate bindings, keeping them out of the other
# tables above. We use this on both the client and server side (storing the
# private key), say for a sliver.
#
DROP TABLE IF EXISTS `geni_certificates`;
CREATE TABLE `geni_certificates` (
  `uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `cert` text,
  `DN` text,
  `privkey` text,
  `revoked` datetime default NULL,
  PRIMARY KEY  (`uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

#
# A clearinghouse table to hold keys associated with geni users.
#
DROP TABLE IF EXISTS `geni_userkeys`;
CREATE TABLE `geni_userkeys` (
  `type` enum('ssh','password') NOT NULL default 'ssh',
  `uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `key` text,
  INDEX `uuid` (`uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

#
# This table maps between resources and their component manager. 
#
DROP TABLE IF EXISTS `geni_resources`;
CREATE TABLE `geni_resources` (
  `resource_uuid` varchar(40) NOT NULL default '',
  `resource_type` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `component_uuid` varchar(40) NOT NULL default '',
  PRIMARY KEY  (`resource_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

#
# Hold the users that are bound to slices.
#
DROP TABLE IF EXISTS `geni_bindings`;
CREATE TABLE `geni_bindings` (
  `slice_uuid` varchar(40) NOT NULL default '',
  `user_uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  PRIMARY KEY  (`slice_uuid`,`user_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;


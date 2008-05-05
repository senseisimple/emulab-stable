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
  `sa_idx` int(10) unsigned NOT NULL default '0',
  `cert` text,
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
  `hrn` varchar(256) NOT NULL default '',
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `name` tinytext,
  `url` tinytext,
  `cert` text,
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
DROP TABLE IF EXISTS `geni_sliceauthorities`;
CREATE TABLE `geni_sliceauthorities` (
  `id` varchar(8) NOT NULL default '',
  `id_idx` mediumint(8) unsigned NOT NULL default '0',
  `uuid` varchar(40) NOT NULL default '',
  `uuid_prefix` varchar(8) NOT NULL default '',
  `created` datetime default NULL,
  `name` tinytext,
  `url` tinytext,
  `pubkey` text,
  PRIMARY KEY  (`id_idx`),
  UNIQUE KEY `id` (`id`),
  UNIQUE KEY `uuid` (`uuid`)
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
  `created` datetime default NULL,
  `creator_uuid` varchar(40) NOT NULL default '',
  `name` tinytext,
  `sa_idx` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `hrn` (`hrn`),
  UNIQUE KEY `uuid` (`uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

#
# Geni Slivers. Created on components (by CMs of course).
#
DROP TABLE IF EXISTS `geni_slivers`;
CREATE TABLE `geni_slivers` (
  `idx` mediumint(8) unsigned NOT NULL default '0',
  `uuid` varchar(40) NOT NULL default '',
  `slice_uuid` varchar(40) NOT NULL default '',
  `creator_uuid` varchar(40) NOT NULL default '',
  `created` datetime default NULL,
  `cm_idx` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`idx`),
  UNIQUE KEY `uuid` (`uuid`),
  INDEX `slice_uuid` (`slice_uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;


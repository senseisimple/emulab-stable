# MySQL dump 8.13
#
# Host: localhost    Database: tbdb
#--------------------------------------------------------
# Server version	3.23.47-log

#
# Dumping data for table 'comments'
#

REPLACE INTO comments VALUES ('users',NULL,'testbed user accounts');
REPLACE INTO comments VALUES ('experiments',NULL,'user experiments');
REPLACE INTO comments VALUES ('images',NULL,'available disk images');
REPLACE INTO comments VALUES ('current_reloads',NULL,'currently pending disk reloads');
REPLACE INTO comments VALUES ('delays',NULL,'delay nodes');
REPLACE INTO comments VALUES ('loginmessage',NULL,'appears under login button in web interface');
REPLACE INTO comments VALUES ('nodes',NULL,'hardware, software, and status of testbed machines');
REPLACE INTO comments VALUES ('projects',NULL,'projects using the testbed');
REPLACE INTO comments VALUES ('partitions',NULL,'loaded operating systems on node partitions');
REPLACE INTO comments VALUES ('os_info',NULL,'available operating system features and information');
REPLACE INTO comments VALUES ('reserved',NULL,'node reservation');
REPLACE INTO comments VALUES ('wires',NULL,'physical wire types and connections');
REPLACE INTO comments VALUES ('nologins',NULL,'presence of a row will disallow non-admin web logins');
REPLACE INTO comments VALUES ('nsfiles',NULL,'NS simulator files used to configure experiments');
REPLACE INTO comments VALUES ('tiplines',NULL,'serial control \'TIP\' lines');
REPLACE INTO comments VALUES ('proj_memb',NULL,'project membership');
REPLACE INTO comments VALUES ('group_membership',NULL,'group membership');
REPLACE INTO comments VALUES ('node_types',NULL,'specifications regarding types of node hardware available');
REPLACE INTO comments VALUES ('last_reservation',NULL,'the last project to have reserved listed nodes');
REPLACE INTO comments VALUES ('groups',NULL,'groups information');
REPLACE INTO comments VALUES ('vlans',NULL,'configured router VLANs');
REPLACE INTO comments VALUES ('tipservers',NULL,'machines driving serial control \'TIP\' lines');
REPLACE INTO comments VALUES ('uidnodelastlogin',NULL,'last node logged into by users');
REPLACE INTO comments VALUES ('nodeuidlastlogin',NULL,'last user logged in to nodes');
REPLACE INTO comments VALUES ('scheduled_reloads',NULL,'pending disk reloads');
REPLACE INTO comments VALUES ('tmcd_redirect',NULL,'used to redirect node configuration client (TMCC) to \'fake\' database for testing purposes');
REPLACE INTO comments VALUES ('deltas',NULL,'user filesystem deltas');
REPLACE INTO comments VALUES ('delta_compat',NULL,'delta/OS compatibilities');
REPLACE INTO comments VALUES ('delta_inst',NULL,'nodes on which listed deltas are installed');
REPLACE INTO comments VALUES ('delta_proj',NULL,'projects which own listed deltas');
REPLACE INTO comments VALUES ('next_reserve',NULL,'scheduled reservations (e.g. by sched_reserve)');
REPLACE INTO comments VALUES ('outlets',NULL,'power controller and outlet connections for nodes');
REPLACE INTO comments VALUES ('exppid_access',NULL,'allows access to one project\'s experiment by another project');
REPLACE INTO comments VALUES ('lastlogin',NULL,'list of recently logged in web interface users');
REPLACE INTO comments VALUES ('switch_stacks',NULL,'switch stack membership');
REPLACE INTO comments VALUES ('switch_stack_types',NULL,'types of each switch stack');
REPLACE INTO comments VALUES ('nodelog',NULL,'log entries for nodes');
REPLACE INTO comments VALUES ('unixgroup_membership',NULL,'Unix group memberships for control (non-experiment) nodes');
REPLACE INTO comments VALUES ('interface_types',NULL,'network interface types');
REPLACE INTO comments VALUES ('foo','bar','baz');
REPLACE INTO comments VALUES ('login',NULL,'currently active web logins');
REPLACE INTO comments VALUES ('portmap',NULL,'provides consistency of ports across swaps');
REPLACE INTO comments VALUES ('webdb_table_permissions',NULL,'table access permissions for WebDB interface ');
REPLACE INTO comments VALUES ('comments',NULL,'database table and row descriptions (such as this)');
REPLACE INTO comments VALUES ('interfaces',NULL,'node network interfaces');
REPLACE INTO comments VALUES ('foreign_keys',NULL,'foreign key constraints for use by the dbcheck script');
REPLACE INTO comments VALUES ('nseconfigs',NULL,'Table for storing NSE configurations');
REPLACE INTO comments VALUES ('widearea_delays',NULL,'Delay and bandwidth metrics between WAN nodes');
REPLACE INTO comments VALUES ('virt_nodes',NULL,'Experiment virtual nodes');

#
# Dumping data for table 'event_eventtypes'
#

REPLACE INTO event_eventtypes VALUES (0,'REBOOT');
REPLACE INTO event_eventtypes VALUES (1,'START');
REPLACE INTO event_eventtypes VALUES (2,'STOP');
REPLACE INTO event_eventtypes VALUES (3,'UP');
REPLACE INTO event_eventtypes VALUES (4,'DOWN');
REPLACE INTO event_eventtypes VALUES (5,'UPDATE');
REPLACE INTO event_eventtypes VALUES (6,'MODIFY');
REPLACE INTO event_eventtypes VALUES (7,'SET');
REPLACE INTO event_eventtypes VALUES (8,'TIME');
REPLACE INTO event_eventtypes VALUES (9,'RESET');

#
# Dumping data for table 'event_objecttypes'
#

REPLACE INTO event_objecttypes VALUES (0,'TBCONTROL');
REPLACE INTO event_objecttypes VALUES (1,'LINK');
REPLACE INTO event_objecttypes VALUES (2,'TRAFGEN');
REPLACE INTO event_objecttypes VALUES (3,'TIME');
REPLACE INTO event_objecttypes VALUES (4,'PROGRAM');

#
# Dumping data for table 'exported_tables'
#

REPLACE INTO exported_tables VALUES ('comments');
REPLACE INTO exported_tables VALUES ('event_eventtypes');
REPLACE INTO exported_tables VALUES ('event_objecttypes');
REPLACE INTO exported_tables VALUES ('exported_tables');
REPLACE INTO exported_tables VALUES ('foreign_keys');
REPLACE INTO exported_tables VALUES ('state_timeouts');
REPLACE INTO exported_tables VALUES ('state_transitions');
REPLACE INTO exported_tables VALUES ('testsuite_preentables');
REPLACE INTO exported_tables VALUES ('webdb_table_permissions');

#
# Dumping data for table 'foreign_keys'
#

REPLACE INTO foreign_keys VALUES ('projects','head_uid','users','uid');
REPLACE INTO foreign_keys VALUES ('groups','pid','projects','pid');
REPLACE INTO foreign_keys VALUES ('groups','leader','users','uid');
REPLACE INTO foreign_keys VALUES ('experiments','expt_head_uid','users','uid');
REPLACE INTO foreign_keys VALUES ('experiments','pid','projects','pid');
REPLACE INTO foreign_keys VALUES ('experiments','pid,gid','groups','pid,gid');
REPLACE INTO foreign_keys VALUES ('os_info','pid','projects','pid');
REPLACE INTO foreign_keys VALUES ('node_types','osid','os_info','osid');
REPLACE INTO foreign_keys VALUES ('node_types','delay_osid','os_info','osid');
REPLACE INTO foreign_keys VALUES ('nodes','def_boot_osid','os_info','osid');
REPLACE INTO foreign_keys VALUES ('nodes','next_boot_osid','os_info','osid');
REPLACE INTO foreign_keys VALUES ('nodes','type','node_types','type');
REPLACE INTO foreign_keys VALUES ('images','pid','projects','pid');
REPLACE INTO foreign_keys VALUES ('images','part1_osid','os_info','osid');
REPLACE INTO foreign_keys VALUES ('images','part2_osid','os_info','osid');
REPLACE INTO foreign_keys VALUES ('images','part3_osid','os_info','osid');
REPLACE INTO foreign_keys VALUES ('images','part4_osid','os_info','osid');
REPLACE INTO foreign_keys VALUES ('images','default_osid','os_info','osid');
REPLACE INTO foreign_keys VALUES ('current_reloads','node_id','nodes','node_id');
REPLACE INTO foreign_keys VALUES ('current_reloads','image_id','images','imageid');
REPLACE INTO foreign_keys VALUES ('delays','node_id','nodes','node_id');
REPLACE INTO foreign_keys VALUES ('delays','eid,pid','experiments','eid,pid');
REPLACE INTO foreign_keys VALUES ('partitions','node_id','nodes','node_id');
REPLACE INTO foreign_keys VALUES ('partitions','osid','os_info','osid');
REPLACE INTO foreign_keys VALUES ('exppid_access','exp_pid,exp_eid','experiments','pid,eid');
REPLACE INTO foreign_keys VALUES ('group_membership','uid','users','uid');
REPLACE INTO foreign_keys VALUES ('group_membership','pid,gid','groups','pid,gid');
REPLACE INTO foreign_keys VALUES ('interfaces','interface_type','interface_types','type');
REPLACE INTO foreign_keys VALUES ('interfaces','node_id','nodes','node_id');
REPLACE INTO foreign_keys VALUES ('last_reservation','node_id','nodes','node_id');
REPLACE INTO foreign_keys VALUES ('last_reservation','pid','projects','pid');
REPLACE INTO foreign_keys VALUES ('lastlogin','uid','users','uid');
REPLACE INTO foreign_keys VALUES ('login','uid','users','uid');
REPLACE INTO foreign_keys VALUES ('newdelays','node_id','nodes','node_id');
REPLACE INTO foreign_keys VALUES ('newdelays','eid,pid','experiments','eid,pid');
REPLACE INTO foreign_keys VALUES ('next_reserve','eid,pid','experiments','eid,pid');
REPLACE INTO foreign_keys VALUES ('next_reserve','node_id','nodes','node_id');
REPLACE INTO foreign_keys VALUES ('nodeuidlastlogin','uid','users','uid');
REPLACE INTO foreign_keys VALUES ('nodeuidlastlogin','node_id','nodes','node_id');
REPLACE INTO foreign_keys VALUES ('nsfiles','eid,pid','experiments','eid,pid');
REPLACE INTO foreign_keys VALUES ('proj_memb','pid','projects','pid');
REPLACE INTO foreign_keys VALUES ('reserved','eid,pid','experiments','eid,pid');
REPLACE INTO foreign_keys VALUES ('reserved','node_id','nodes','node_id');
REPLACE INTO foreign_keys VALUES ('scheduled_reloads','node_id','nodes','node_id');
REPLACE INTO foreign_keys VALUES ('scheduled_reloads','image_id','images','imageid');
REPLACE INTO foreign_keys VALUES ('tiplines','node_id','nodes','node_id');
REPLACE INTO foreign_keys VALUES ('tmcd_redirect','node_id','nodes','node_id');
REPLACE INTO foreign_keys VALUES ('uidnodelastlogin','uid','users','uid');
REPLACE INTO foreign_keys VALUES ('uidnodelastlogin','node_id','nodes','node_id');
REPLACE INTO foreign_keys VALUES ('virt_lans','pid,eid','experiments','pid,eid');
REPLACE INTO foreign_keys VALUES ('virt_nodes','pid,eid','experiments','pid,eid');
REPLACE INTO foreign_keys VALUES ('virt_nodes','osname','os_info','osname');
REPLACE INTO foreign_keys VALUES ('vlans','pid,eid','experiments','pid,eid');
REPLACE INTO foreign_keys VALUES ('nseconfigs','eid,pid,vname','virt_nodes','eid,pid,vname');
REPLACE INTO foreign_keys VALUES ('nseconfigs','eid,pid','experiments','eid,pid');

#
# Dumping data for table 'state_timeouts'
#

REPLACE INTO state_timeouts VALUES ('NORMAL','REBOOTING',0,NULL);
REPLACE INTO state_timeouts VALUES ('NORMAL','REBOOTED',0,NULL);
REPLACE INTO state_timeouts VALUES ('NORMAL','ISUP',0,NULL);

#
# Dumping data for table 'state_transitions'
#

REPLACE INTO state_transitions VALUES ('NORMAL','ISUP','REBOOTING');
REPLACE INTO state_transitions VALUES ('NORMAL','REBBOTING','REBOOTING');
REPLACE INTO state_transitions VALUES ('NORMAL','REBOOTED','ISUP');
REPLACE INTO state_transitions VALUES ('NORMAL','REBOOTING','REBOOTED');

#
# Dumping data for table 'testsuite_preentables'
#

REPLACE INTO testsuite_preentables VALUES ('comments','drop');
REPLACE INTO testsuite_preentables VALUES ('iface_counters','drop');
REPLACE INTO testsuite_preentables VALUES ('lastlogin','drop');
REPLACE INTO testsuite_preentables VALUES ('login','drop');
REPLACE INTO testsuite_preentables VALUES ('loginmessage','drop');
REPLACE INTO testsuite_preentables VALUES ('node_idlestats','drop');
REPLACE INTO testsuite_preentables VALUES ('nodelog','drop');
REPLACE INTO testsuite_preentables VALUES ('nodeuidlastlogin','drop');
REPLACE INTO testsuite_preentables VALUES ('nologins','drop');
REPLACE INTO testsuite_preentables VALUES ('userslastlogin','drop');
REPLACE INTO testsuite_preentables VALUES ('uidnodelastlogin','drop');
REPLACE INTO testsuite_preentables VALUES ('next_reserve','clean');
REPLACE INTO testsuite_preentables VALUES ('last_reservation','clean');
REPLACE INTO testsuite_preentables VALUES ('current_reloads','clean');
REPLACE INTO testsuite_preentables VALUES ('scheduled_reloads','clean');
REPLACE INTO testsuite_preentables VALUES ('projects','prune');
REPLACE INTO testsuite_preentables VALUES ('group_membership','prune');
REPLACE INTO testsuite_preentables VALUES ('groups','prune');

#
# Dumping data for table 'webdb_table_permissions'
#

REPLACE INTO webdb_table_permissions VALUES ('comments',1,1,1);
REPLACE INTO webdb_table_permissions VALUES ('foreign_keys',1,1,1);
REPLACE INTO webdb_table_permissions VALUES ('interfaces',1,1,1);
REPLACE INTO webdb_table_permissions VALUES ('interface_types',1,1,1);
REPLACE INTO webdb_table_permissions VALUES ('lastlogin',1,1,1);
REPLACE INTO webdb_table_permissions VALUES ('nodes',1,1,0);
REPLACE INTO webdb_table_permissions VALUES ('node_types',1,1,0);
REPLACE INTO webdb_table_permissions VALUES ('tiplines',1,1,1);
REPLACE INTO webdb_table_permissions VALUES ('projects',1,1,0);


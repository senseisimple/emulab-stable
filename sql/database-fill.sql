-- MySQL dump 8.22
--
-- Host: localhost    Database: tbdb
---------------------------------------------------------
-- Server version	3.23.54-log

--
-- Dumping data for table 'comments'
--


REPLACE INTO comments VALUES ('users','','testbed user accounts');
REPLACE INTO comments VALUES ('experiments','','user experiments');
REPLACE INTO comments VALUES ('images','','available disk images');
REPLACE INTO comments VALUES ('current_reloads','','currently pending disk reloads');
REPLACE INTO comments VALUES ('delays','','delay nodes');
REPLACE INTO comments VALUES ('loginmessage','','appears under login button in web interface');
REPLACE INTO comments VALUES ('nodes','','hardware, software, and status of testbed machines');
REPLACE INTO comments VALUES ('projects','','projects using the testbed');
REPLACE INTO comments VALUES ('partitions','','loaded operating systems on node partitions');
REPLACE INTO comments VALUES ('os_info','','available operating system features and information');
REPLACE INTO comments VALUES ('reserved','','node reservation');
REPLACE INTO comments VALUES ('wires','','physical wire types and connections');
REPLACE INTO comments VALUES ('nologins','','presence of a row will disallow non-admin web logins');
REPLACE INTO comments VALUES ('nsfiles','','NS simulator files used to configure experiments');
REPLACE INTO comments VALUES ('tiplines','','serial control \'TIP\' lines');
REPLACE INTO comments VALUES ('proj_memb','','project membership');
REPLACE INTO comments VALUES ('group_membership','','group membership');
REPLACE INTO comments VALUES ('node_types','','specifications regarding types of node hardware available');
REPLACE INTO comments VALUES ('last_reservation','','the last project to have reserved listed nodes');
REPLACE INTO comments VALUES ('groups','','groups information');
REPLACE INTO comments VALUES ('vlans','','configured router VLANs');
REPLACE INTO comments VALUES ('tipservers','','machines driving serial control \'TIP\' lines');
REPLACE INTO comments VALUES ('uidnodelastlogin','','last node logged into by users');
REPLACE INTO comments VALUES ('nodeuidlastlogin','','last user logged in to nodes');
REPLACE INTO comments VALUES ('scheduled_reloads','','pending disk reloads');
REPLACE INTO comments VALUES ('tmcd_redirect','','used to redirect node configuration client (TMCC) to \'fake\' database for testing purposes');
REPLACE INTO comments VALUES ('deltas','','user filesystem deltas');
REPLACE INTO comments VALUES ('delta_compat','','delta/OS compatibilities');
REPLACE INTO comments VALUES ('delta_inst','','nodes on which listed deltas are installed');
REPLACE INTO comments VALUES ('delta_proj','','projects which own listed deltas');
REPLACE INTO comments VALUES ('next_reserve','','scheduled reservations (e.g. by sched_reserve)');
REPLACE INTO comments VALUES ('outlets','','power controller and outlet connections for nodes');
REPLACE INTO comments VALUES ('exppid_access','','allows access to one project\'s experiment by another project');
REPLACE INTO comments VALUES ('lastlogin','','list of recently logged in web interface users');
REPLACE INTO comments VALUES ('switch_stacks','','switch stack membership');
REPLACE INTO comments VALUES ('switch_stack_types','','types of each switch stack');
REPLACE INTO comments VALUES ('nodelog','','log entries for nodes');
REPLACE INTO comments VALUES ('unixgroup_membership','','Unix group memberships for control (non-experiment) nodes');
REPLACE INTO comments VALUES ('interface_types','','network interface types');
REPLACE INTO comments VALUES ('foo','bar','baz');
REPLACE INTO comments VALUES ('login','','currently active web logins');
REPLACE INTO comments VALUES ('portmap','','provides consistency of ports across swaps');
REPLACE INTO comments VALUES ('webdb_table_permissions','','table access permissions for WebDB interface ');
REPLACE INTO comments VALUES ('comments','','database table and row descriptions (such as this)');
REPLACE INTO comments VALUES ('interfaces','','node network interfaces');
REPLACE INTO comments VALUES ('foreign_keys','','foreign key constraints for use by the dbcheck script');
REPLACE INTO comments VALUES ('nseconfigs','','Table for storing NSE configurations');
REPLACE INTO comments VALUES ('widearea_delays','','Delay and bandwidth metrics between WAN nodes');
REPLACE INTO comments VALUES ('virt_nodes','','Experiment virtual nodes');

--
-- Dumping data for table 'event_eventtypes'
--


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
REPLACE INTO event_eventtypes VALUES (10,'KILL');
REPLACE INTO event_eventtypes VALUES (11,'HALT');
REPLACE INTO event_eventtypes VALUES (12,'SWAPOUT');

--
-- Dumping data for table 'event_objecttypes'
--


REPLACE INTO event_objecttypes VALUES (0,'TBCONTROL');
REPLACE INTO event_objecttypes VALUES (1,'LINK');
REPLACE INTO event_objecttypes VALUES (2,'TRAFGEN');
REPLACE INTO event_objecttypes VALUES (3,'TIME');
REPLACE INTO event_objecttypes VALUES (4,'PROGRAM');
REPLACE INTO event_objecttypes VALUES (5,'FRISBEE');
REPLACE INTO event_objecttypes VALUES (6,'SIMULATOR');

--
-- Dumping data for table 'exported_tables'
--


REPLACE INTO exported_tables VALUES ('comments');
REPLACE INTO exported_tables VALUES ('event_eventtypes');
REPLACE INTO exported_tables VALUES ('event_objecttypes');
REPLACE INTO exported_tables VALUES ('exported_tables');
REPLACE INTO exported_tables VALUES ('foreign_keys');
REPLACE INTO exported_tables VALUES ('mode_transitions');
REPLACE INTO exported_tables VALUES ('state_timeouts');
REPLACE INTO exported_tables VALUES ('state_transitions');
REPLACE INTO exported_tables VALUES ('state_triggers');
REPLACE INTO exported_tables VALUES ('testsuite_preentables');
REPLACE INTO exported_tables VALUES ('webdb_table_permissions');

--
-- Dumping data for table 'foreign_keys'
--


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

--
-- Dumping data for table 'mode_transitions'
--


REPLACE INTO mode_transitions VALUES ('MINIMAL','SHUTDOWN','NETBOOT','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('MINIMAL','SHUTDOWN','NORMAL','REBOOTING');
REPLACE INTO mode_transitions VALUES ('MINIMAL','SHUTDOWN','NORMALv1','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('MINIMAL','SHUTDOWN','RELOAD','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('NETBOOT','SHUTDOWN','MINIMAL','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('NETBOOT','SHUTDOWN','NORMAL','REBOOTING');
REPLACE INTO mode_transitions VALUES ('NETBOOT','SHUTDOWN','NORMALv1','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('NETBOOT','SHUTDOWN','RELOAD','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('NORMAL','REBOOTING','MINIMAL','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('NORMAL','REBOOTING','NETBOOT','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('NORMAL','REBOOTING','NORMALv1','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('NORMAL','REBOOTING','RELOAD','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('NORMAL','SHUTDOWN','MINIMAL','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('NORMAL','SHUTDOWN','NETBOOT','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('NORMAL','SHUTDOWN','NORMALv1','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('NORMAL','SHUTDOWN','RELOAD','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('NORMALv1','SHUTDOWN','MINIMAL','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('NORMALv1','SHUTDOWN','NETBOOT','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('NORMALv1','SHUTDOWN','NORMAL','REBOOTING');
REPLACE INTO mode_transitions VALUES ('NORMALv1','SHUTDOWN','RELOAD','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('RELOAD','RELOADDONE','MINIMAL','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('RELOAD','RELOADDONE','NETBOOT','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('RELOAD','RELOADDONE','NORMAL','REBOOTING');
REPLACE INTO mode_transitions VALUES ('RELOAD','RELOADDONE','NORMALv1','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('RELOAD','SHUTDOWN','MINIMAL','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('RELOAD','SHUTDOWN','NETBOOT','SHUTDOWN');
REPLACE INTO mode_transitions VALUES ('RELOAD','SHUTDOWN','NORMAL','REBOOTING');
REPLACE INTO mode_transitions VALUES ('RELOAD','SHUTDOWN','NORMALv1','SHUTDOWN');

--
-- Dumping data for table 'state_timeouts'
--


REPLACE INTO state_timeouts VALUES ('NORMAL','REBOOTING',120,'REBOOT');
REPLACE INTO state_timeouts VALUES ('NORMAL','REBOOTED',120,'REBOOT');
REPLACE INTO state_timeouts VALUES ('NORMAL','ISUP',0,'');
REPLACE INTO state_timeouts VALUES ('MINIMAL','SHUTDOWN',120,'REBOOT');
REPLACE INTO state_timeouts VALUES ('NORMALv1','SHUTDOWN',120,'REBOOT');
REPLACE INTO state_timeouts VALUES ('RELOAD','RELOADDONE',60,'NOTIFY');
REPLACE INTO state_timeouts VALUES ('EXPTSTATUS','ACTIVATING',0,'');
REPLACE INTO state_timeouts VALUES ('EXPTSTATUS','ACTIVE',0,'');
REPLACE INTO state_timeouts VALUES ('EXPTSTATUS','NEW',0,'');
REPLACE INTO state_timeouts VALUES ('EXPTSTATUS','PRERUN',0,'');
REPLACE INTO state_timeouts VALUES ('EXPTSTATUS','SWAPPED',0,'');
REPLACE INTO state_timeouts VALUES ('EXPTSTATUS','SWAPPING',0,'');
REPLACE INTO state_timeouts VALUES ('EXPTSTATUS','TERMINATING',0,'');
REPLACE INTO state_timeouts VALUES ('EXPTSTATUS','TESTING',0,'');
REPLACE INTO state_timeouts VALUES ('MINIMAL','BOOTING',180,'REBOOT');
REPLACE INTO state_timeouts VALUES ('MINIMAL','ISUP',0,'');
REPLACE INTO state_timeouts VALUES ('NODEALLOC','FREE_CLEAN',0,'');
REPLACE INTO state_timeouts VALUES ('NODEALLOC','FREE_DIRTY',0,'');
REPLACE INTO state_timeouts VALUES ('NODEALLOC','REBOOT',0,'');
REPLACE INTO state_timeouts VALUES ('NODEALLOC','RELOAD',0,'');
REPLACE INTO state_timeouts VALUES ('NODEALLOC','RESERVED',0,'');
REPLACE INTO state_timeouts VALUES ('NORMAL','BOOTING',180,'REBOOT');
REPLACE INTO state_timeouts VALUES ('NORMALv1','BOOTING',180,'REBOOT');
REPLACE INTO state_timeouts VALUES ('NORMALv1','ISUP',0,'');
REPLACE INTO state_timeouts VALUES ('NORMALv1','TBSETUP',15,'REBOOT');
REPLACE INTO state_timeouts VALUES ('RELOAD','BOOTING',180,'REBOOT');
REPLACE INTO state_timeouts VALUES ('RELOAD','RELOADING',600,'NOTIFY');
REPLACE INTO state_timeouts VALUES ('RELOAD','RELOADSETUP',60,'NOTIFY');
REPLACE INTO state_timeouts VALUES ('RELOAD','SHUTDOWN',120,'REBOOT');
REPLACE INTO state_timeouts VALUES ('USERSTATUS','ACTIVE',0,'');
REPLACE INTO state_timeouts VALUES ('USERSTATUS','FROZEN',0,'');
REPLACE INTO state_timeouts VALUES ('USERSTATUS','NEWUSER',0,'');
REPLACE INTO state_timeouts VALUES ('USERSTATUS','UNAPPROVED',0,'');
REPLACE INTO state_timeouts VALUES ('PCVM','BOOTING',180,'NOTIFY');
REPLACE INTO state_timeouts VALUES ('PCVM','SHUTDOWN',0,'');
REPLACE INTO state_timeouts VALUES ('PCVM','ISUP',0,'');
REPLACE INTO state_timeouts VALUES ('PCVM','TBSETUP',600,'NOTIFY');
REPLACE INTO state_timeouts VALUES ('TBCOMMAND','REBOOT',60,'CMDRETRY');
REPLACE INTO state_timeouts VALUES ('TBCOMMAND','POWEROFF',0,'CMDRETRY');
REPLACE INTO state_timeouts VALUES ('TBCOMMAND','POWERON',0,'CMDRETRY');
REPLACE INTO state_timeouts VALUES ('TBCOMMAND','POWERCYCLE',0,'CMDRETRY');

--
-- Dumping data for table 'state_transitions'
--


REPLACE INTO state_transitions VALUES ('ALWAYSUP','ISUP','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('ALWAYSUP','SHUTDOWN','ISUP');
REPLACE INTO state_transitions VALUES ('EXPTSTATUS','ACTIVATING','ACTIVE');
REPLACE INTO state_transitions VALUES ('EXPTSTATUS','ACTIVATING','SWAPPED');
REPLACE INTO state_transitions VALUES ('EXPTSTATUS','ACTIVATING','TESTING');
REPLACE INTO state_transitions VALUES ('EXPTSTATUS','ACTIVE','SWAPPING');
REPLACE INTO state_transitions VALUES ('EXPTSTATUS','NEW','PRERUN');
REPLACE INTO state_transitions VALUES ('EXPTSTATUS','PRERUN','SWAPPED');
REPLACE INTO state_transitions VALUES ('EXPTSTATUS','SWAPPED','ACTIVATING');
REPLACE INTO state_transitions VALUES ('EXPTSTATUS','SWAPPED','TERMINATING');
REPLACE INTO state_transitions VALUES ('EXPTSTATUS','SWAPPING','SWAPPED');
REPLACE INTO state_transitions VALUES ('EXPTSTATUS','TERMINATING','TERMINATED');
REPLACE INTO state_transitions VALUES ('EXPTSTATUS','TESTING','SWAPPING');
REPLACE INTO state_transitions VALUES ('MINIMAL','BOOTING','BOOTING');
REPLACE INTO state_transitions VALUES ('MINIMAL','BOOTING','ISUP');
REPLACE INTO state_transitions VALUES ('MINIMAL','BOOTING','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('MINIMAL','ISUP','BOOTING');
REPLACE INTO state_transitions VALUES ('MINIMAL','ISUP','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('MINIMAL','SHUTDOWN','BOOTING');
REPLACE INTO state_transitions VALUES ('MINIMAL','SHUTDOWN','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('NETBOOT','BOOTING','ISUP');
REPLACE INTO state_transitions VALUES ('NETBOOT','BOOTING','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('NETBOOT','ISUP','BOOTING');
REPLACE INTO state_transitions VALUES ('NETBOOT','ISUP','ISUP');
REPLACE INTO state_transitions VALUES ('NETBOOT','ISUP','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('NETBOOT','SHUTDOWN','BOOTING');
REPLACE INTO state_transitions VALUES ('NETBOOT','SHUTDOWN','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('NODEALLOC','FREE_CLEAN','RESERVED');
REPLACE INTO state_transitions VALUES ('NODEALLOC','FREE_DIRTY','RELOAD');
REPLACE INTO state_transitions VALUES ('NODEALLOC','FREE_DIRTY','RESERVED');
REPLACE INTO state_transitions VALUES ('NODEALLOC','REBOOT','FREE_DIRTY');
REPLACE INTO state_transitions VALUES ('NODEALLOC','RELOAD','FREE_CLEAN');
REPLACE INTO state_transitions VALUES ('NODEALLOC','RESERVED','REBOOT');
REPLACE INTO state_transitions VALUES ('NODEALLOC','RESERVED','RELOAD');
REPLACE INTO state_transitions VALUES ('NORMAL','BOOTING','BOOTING');
REPLACE INTO state_transitions VALUES ('NORMAL','BOOTING','REBOOTED');
REPLACE INTO state_transitions VALUES ('NORMAL','BOOTING','REBOOTING');
REPLACE INTO state_transitions VALUES ('NORMAL','BOOTING','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('NORMAL','ISUP','REBOOTING');
REPLACE INTO state_transitions VALUES ('NORMAL','ISUP','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('NORMAL','REBOOTED','BOOTING');
REPLACE INTO state_transitions VALUES ('NORMAL','REBOOTED','ISUP');
REPLACE INTO state_transitions VALUES ('NORMAL','REBOOTED','REBOOTING');
REPLACE INTO state_transitions VALUES ('NORMAL','REBOOTED','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('NORMAL','REBOOTING','BOOTING');
REPLACE INTO state_transitions VALUES ('NORMAL','REBOOTING','REBOOTING');
REPLACE INTO state_transitions VALUES ('NORMAL','REBOOTING','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('NORMAL','SHUTDOWN','BOOTING');
REPLACE INTO state_transitions VALUES ('NORMAL','SHUTDOWN','REBOOTING');
REPLACE INTO state_transitions VALUES ('NORMAL','SHUTDOWN','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('NORMALv1','BOOTING','BOOTING');
REPLACE INTO state_transitions VALUES ('NORMALv1','BOOTING','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('NORMALv1','BOOTING','TBSETUP');
REPLACE INTO state_transitions VALUES ('NORMALv1','ISUP','BOOTING');
REPLACE INTO state_transitions VALUES ('NORMALv1','ISUP','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('NORMALv1','SHUTDOWN','BOOTING');
REPLACE INTO state_transitions VALUES ('NORMALv1','SHUTDOWN','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('NORMALv1','TBSETUP','ISUP');
REPLACE INTO state_transitions VALUES ('NORMALv1','TBSETUP','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('PCVM','BOOTING','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('PCVM','BOOTING','TBSETUP');
REPLACE INTO state_transitions VALUES ('PCVM','ISUP','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('PCVM','SHUTDOWN','BOOTING');
REPLACE INTO state_transitions VALUES ('PCVM','TBSETUP','ISUP');
REPLACE INTO state_transitions VALUES ('PCVM','TBSETUP','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('RELOAD','BOOTING','BOOTING');
REPLACE INTO state_transitions VALUES ('RELOAD','BOOTING','RELOADSETUP');
REPLACE INTO state_transitions VALUES ('RELOAD','BOOTING','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('RELOAD','RELOADING','RELOADDONE');
REPLACE INTO state_transitions VALUES ('RELOAD','RELOADING','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('RELOAD','RELOADSETUP','RELOADING');
REPLACE INTO state_transitions VALUES ('RELOAD','RELOADSETUP','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('RELOAD','SHUTDOWN','BOOTING');
REPLACE INTO state_transitions VALUES ('RELOAD','SHUTDOWN','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('USERSTATUS','ACTIVE','FROZEN');
REPLACE INTO state_transitions VALUES ('USERSTATUS','FROZEN','ACTIVE');
REPLACE INTO state_transitions VALUES ('USERSTATUS','NEWUSER','UNAPPROVED');
REPLACE INTO state_transitions VALUES ('USERSTATUS','UNAPPROVED','ACTIVE');
REPLACE INTO state_transitions VALUES ('WIDEAREA','BOOTING','REBOOTED');
REPLACE INTO state_transitions VALUES ('WIDEAREA','BOOTING','REBOOTING');
REPLACE INTO state_transitions VALUES ('WIDEAREA','BOOTING','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('WIDEAREA','ISUP','REBOOTED');
REPLACE INTO state_transitions VALUES ('WIDEAREA','ISUP','REBOOTING');
REPLACE INTO state_transitions VALUES ('WIDEAREA','ISUP','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('WIDEAREA','REBOOTED','BOOTING');
REPLACE INTO state_transitions VALUES ('WIDEAREA','REBOOTED','ISUP');
REPLACE INTO state_transitions VALUES ('WIDEAREA','REBOOTED','REBOOTING');
REPLACE INTO state_transitions VALUES ('WIDEAREA','REBOOTED','SHUTDOWN');
REPLACE INTO state_transitions VALUES ('WIDEAREA','REBOOTING','BOOTING');
REPLACE INTO state_transitions VALUES ('WIDEAREA','REBOOTING','REBOOTED');
REPLACE INTO state_transitions VALUES ('WIDEAREA','REBOOTING','REBOOTING');
REPLACE INTO state_transitions VALUES ('WIDEAREA','SHUTDOWN','BOOTING');
REPLACE INTO state_transitions VALUES ('WIDEAREA','SHUTDOWN','REBOOTED');
REPLACE INTO state_transitions VALUES ('WIDEAREA','SHUTDOWN','SHUTDOWN');

--
-- Dumping data for table 'state_triggers'
--


REPLACE INTO state_triggers VALUES ('*','NORMAL','ISUP','RESET');
REPLACE INTO state_triggers VALUES ('*','NORMALv1','ISUP','RESET');
REPLACE INTO state_triggers VALUES ('*','MINIMAL','ISUP','RESET');
REPLACE INTO state_triggers VALUES ('*','RELOAD','RELOADDONE','RESET, RELOADDONE');
REPLACE INTO state_triggers VALUES ('*','ALWAYSUP','SHUTDOWN','ISUP');
REPLACE INTO state_triggers VALUES ('*','PCVM','ISUP','RESET');

--
-- Dumping data for table 'testsuite_preentables'
--


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
REPLACE INTO testsuite_preentables VALUES ('user_sfskeys','clean');
REPLACE INTO testsuite_preentables VALUES ('user_pubkeys','clean');
REPLACE INTO testsuite_preentables VALUES ('port_counters','drop');
REPLACE INTO testsuite_preentables VALUES ('images','prune');
REPLACE INTO testsuite_preentables VALUES ('os_info','prune');
REPLACE INTO testsuite_preentables VALUES ('node_activity','clean');
REPLACE INTO testsuite_preentables VALUES ('portmap','clean');
REPLACE INTO testsuite_preentables VALUES ('webnews','clean');
REPLACE INTO testsuite_preentables VALUES ('vis_nodes','clean');
REPLACE INTO testsuite_preentables VALUES ('vis_experiments','clean');
REPLACE INTO testsuite_preentables VALUES ('group_stats','clean');
REPLACE INTO testsuite_preentables VALUES ('project_stats','clean');
REPLACE INTO testsuite_preentables VALUES ('user_stats','clean');
REPLACE INTO testsuite_preentables VALUES ('experiment_stats','clean');
REPLACE INTO testsuite_preentables VALUES ('testbed_stats','clean');

--
-- Dumping data for table 'webdb_table_permissions'
--


REPLACE INTO webdb_table_permissions VALUES ('comments',1,1,1);
REPLACE INTO webdb_table_permissions VALUES ('foreign_keys',1,1,1);
REPLACE INTO webdb_table_permissions VALUES ('images',1,0,1);
REPLACE INTO webdb_table_permissions VALUES ('interfaces',1,1,1);
REPLACE INTO webdb_table_permissions VALUES ('interface_types',1,1,1);
REPLACE INTO webdb_table_permissions VALUES ('lastlogin',1,1,1);
REPLACE INTO webdb_table_permissions VALUES ('nodes',1,1,0);
REPLACE INTO webdb_table_permissions VALUES ('node_types',1,1,0);
REPLACE INTO webdb_table_permissions VALUES ('tiplines',1,1,1);
REPLACE INTO webdb_table_permissions VALUES ('os_info',1,1,1);
REPLACE INTO webdb_table_permissions VALUES ('projects',1,1,0);
REPLACE INTO webdb_table_permissions VALUES ('osidtoimageid',1,0,1);


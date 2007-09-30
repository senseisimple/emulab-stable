-- MySQL dump 8.23
--
-- Host: localhost    Database: tbdb
---------------------------------------------------------
-- Server version	3.23.59-nightly-20050301-log

--
-- Dumping data for table `causes`
--


REPLACE INTO causes VALUES ('temp','Temp Resource Shortage');
REPLACE INTO causes VALUES ('user','User Error');
REPLACE INTO causes VALUES ('internal','Internal Error');
REPLACE INTO causes VALUES ('software','Software Problem');
REPLACE INTO causes VALUES ('hardware','Hardware Problem');
REPLACE INTO causes VALUES ('unknown','Cause Unknown');
REPLACE INTO causes VALUES ('canceled','Canceled');

--
-- Dumping data for table `comments`
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
-- Dumping data for table `event_eventtypes`
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
REPLACE INTO event_eventtypes VALUES (13,'NSEEVENT');
REPLACE INTO event_eventtypes VALUES (14,'REPORT');
REPLACE INTO event_eventtypes VALUES (15,'ALERT');
REPLACE INTO event_eventtypes VALUES (16,'SETDEST');
REPLACE INTO event_eventtypes VALUES (17,'COMPLETE');
REPLACE INTO event_eventtypes VALUES (18,'MESSAGE');
REPLACE INTO event_eventtypes VALUES (19,'LOG');
REPLACE INTO event_eventtypes VALUES (20,'RUN');
REPLACE INTO event_eventtypes VALUES (21,'SNAPSHOT');
REPLACE INTO event_eventtypes VALUES (22,'RELOAD');
REPLACE INTO event_eventtypes VALUES (23,'CLEAR');
REPLACE INTO event_eventtypes VALUES (24,'CREATE');
REPLACE INTO event_eventtypes VALUES (25,'STOPRUN');

--
-- Dumping data for table `event_objecttypes`
--


REPLACE INTO event_objecttypes VALUES (0,'TBCONTROL');
REPLACE INTO event_objecttypes VALUES (1,'LINK');
REPLACE INTO event_objecttypes VALUES (2,'TRAFGEN');
REPLACE INTO event_objecttypes VALUES (3,'TIME');
REPLACE INTO event_objecttypes VALUES (4,'PROGRAM');
REPLACE INTO event_objecttypes VALUES (5,'FRISBEE');
REPLACE INTO event_objecttypes VALUES (6,'SIMULATOR');
REPLACE INTO event_objecttypes VALUES (7,'LINKTEST');
REPLACE INTO event_objecttypes VALUES (8,'NSE');
REPLACE INTO event_objecttypes VALUES (9,'SLOTHD');
REPLACE INTO event_objecttypes VALUES (10,'NODE');
REPLACE INTO event_objecttypes VALUES (11,'SEQUENCE');
REPLACE INTO event_objecttypes VALUES (12,'TIMELINE');
REPLACE INTO event_objecttypes VALUES (13,'CONSOLE');
REPLACE INTO event_objecttypes VALUES (14,'TOPOGRAPHY');
REPLACE INTO event_objecttypes VALUES (15,'LINKTRACE');
REPLACE INTO event_objecttypes VALUES (16,'EVPROXY');
REPLACE INTO event_objecttypes VALUES (17,'BGMON');

--
-- Dumping data for table `exported_tables`
--


REPLACE INTO exported_tables VALUES ('causes');
REPLACE INTO exported_tables VALUES ('comments');
REPLACE INTO exported_tables VALUES ('event_eventtypes');
REPLACE INTO exported_tables VALUES ('event_objecttypes');
REPLACE INTO exported_tables VALUES ('exported_tables');
REPLACE INTO exported_tables VALUES ('foreign_keys');
REPLACE INTO exported_tables VALUES ('mode_transitions');
REPLACE INTO exported_tables VALUES ('priorities');
REPLACE INTO exported_tables VALUES ('state_timeouts');
REPLACE INTO exported_tables VALUES ('state_transitions');
REPLACE INTO exported_tables VALUES ('state_triggers');
REPLACE INTO exported_tables VALUES ('table_regex');
REPLACE INTO exported_tables VALUES ('testsuite_preentables');
REPLACE INTO exported_tables VALUES ('webdb_table_permissions');

--
-- Dumping data for table `foreign_keys`
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
-- Dumping data for table `mode_transitions`
--


REPLACE INTO mode_transitions VALUES ('MINIMAL','SHUTDOWN','NETBOOT','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('MINIMAL','SHUTDOWN','NORMAL','REBOOTING','');
REPLACE INTO mode_transitions VALUES ('MINIMAL','SHUTDOWN','NORMALv1','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('MINIMAL','SHUTDOWN','RELOAD','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NETBOOT','SHUTDOWN','MINIMAL','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NETBOOT','SHUTDOWN','NORMAL','REBOOTING','');
REPLACE INTO mode_transitions VALUES ('NETBOOT','SHUTDOWN','NORMALv1','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NETBOOT','SHUTDOWN','RELOAD','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMAL','REBOOTING','MINIMAL','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMAL','REBOOTING','NETBOOT','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMAL','REBOOTING','NORMALv1','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMAL','REBOOTING','RELOAD','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMAL','SHUTDOWN','MINIMAL','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMAL','SHUTDOWN','NETBOOT','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMAL','SHUTDOWN','NORMALv1','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMAL','SHUTDOWN','RELOAD','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMALv1','SHUTDOWN','MINIMAL','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMALv1','SHUTDOWN','NETBOOT','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMALv1','SHUTDOWN','NORMAL','REBOOTING','');
REPLACE INTO mode_transitions VALUES ('NORMALv1','SHUTDOWN','RELOAD','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('RELOAD','RELOADDONE','MINIMAL','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('RELOAD','RELOADDONEV2','MINIMAL','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('RELOAD','RELOADDONE','NETBOOT','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('RELOAD','RELOADDONE','NORMAL','REBOOTING','');
REPLACE INTO mode_transitions VALUES ('RELOAD','RELOADDONEV2','NORMAL','REBOOTING','');
REPLACE INTO mode_transitions VALUES ('RELOAD','RELOADDONE','NORMALv1','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('RELOAD','RELOADDONEV2','NORMALv1','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('RELOAD','SHUTDOWN','MINIMAL','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('RELOAD','SHUTDOWN','NETBOOT','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('RELOAD','SHUTDOWN','NORMAL','REBOOTING','');
REPLACE INTO mode_transitions VALUES ('RELOAD','SHUTDOWN','NORMALv1','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('MINIMAL','SHUTDOWN','NORMALv2','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMALv2','SHUTDOWN','NORMALv1','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NETBOOT','SHUTDOWN','NORMALv2','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('PXEFBSD','SHUTDOWN','NORMAL','REBOOTING','');
REPLACE INTO mode_transitions VALUES ('PXEFBSD','SHUTDOWN','NORMALv1','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('PXEFBSD','SHUTDOWN','NORMAL','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('PXEFBSD','SHUTDOWN','MINIMAL','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('PXEFBSD','SHUTDOWN','NETBOOT','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('PXEFBSD','SHUTDOWN','NORMALv2','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('PXEFBSD','SHUTDOWN','RELOAD','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMALv2','SHUTDOWN','MINIMAL','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMALv2','SHUTDOWN','NETBOOT','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMALv2','SHUTDOWN','RELOAD','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('RELOAD','SHUTDOWN','PXEFBSD','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('RELOAD','RELOADDONE','NORMALv2','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('RELOAD','RELOADDONEV2','NORMALv2','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMALv2','SHUTDOWN','PXEFBSD','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('RELOAD','SHUTDOWN','NORMALv2','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMALv1','SHUTDOWN','PXEFBSD','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMAL','SHUTDOWN','PXEFBSD','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NETBOOT','SHUTDOWN','PXEFBSD','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('MINIMAL','SHUTDOWN','PXEFBSD','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMAL','REBOOTING','NORMALv2','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('NORMALv2','SHUTDOWN','NORMAL','REBOOTING','');
REPLACE INTO mode_transitions VALUES ('NORMALv1','SHUTDOWN','NORMALv2','SHUTDOWN','');
REPLACE INTO mode_transitions VALUES ('ALWAYSUP','SHUTDOWN','RELOAD-MOTE','SHUTDOWN','ReloadStart');
REPLACE INTO mode_transitions VALUES ('ALWAYSUP','ISUP','RELOAD-MOTE','SHUTDOWN','ReloadStart');
REPLACE INTO mode_transitions VALUES ('ALWAYSUP','ISUP','RELOAD-MOTE','ISUP','ReloadStart');
REPLACE INTO mode_transitions VALUES ('RELOAD-MOTE','SHUTDOWN','ALWAYSUP','ISUP','ReloadDone');

--
-- Dumping data for table `priorities`
--


REPLACE INTO priorities VALUES (0,'EMERG');
REPLACE INTO priorities VALUES (100,'ALERT');
REPLACE INTO priorities VALUES (200,'CRIT');
REPLACE INTO priorities VALUES (300,'ERR');
REPLACE INTO priorities VALUES (400,'WARNING');
REPLACE INTO priorities VALUES (500,'NOTICE');
REPLACE INTO priorities VALUES (600,'INFO');
REPLACE INTO priorities VALUES (700,'DEBUG');

--
-- Dumping data for table `state_timeouts`
--


REPLACE INTO state_timeouts VALUES ('NORMAL','REBOOTING',120,'REBOOT');
REPLACE INTO state_timeouts VALUES ('NORMAL','REBOOTED',60,'NOTIFY');
REPLACE INTO state_timeouts VALUES ('MINIMAL','SHUTDOWN',120,'REBOOT');
REPLACE INTO state_timeouts VALUES ('NORMALv1','TBSETUP',600,'NOTIFY');
REPLACE INTO state_timeouts VALUES ('RELOAD','RELOADDONE',60,'NOTIFY');
REPLACE INTO state_timeouts VALUES ('RELOAD','RELOADDONEV2',60,'NOTIFY');
REPLACE INTO state_timeouts VALUES ('EXPTSTATUS','ACTIVATING',0,'');
REPLACE INTO state_timeouts VALUES ('EXPTSTATUS','ACTIVE',0,'');
REPLACE INTO state_timeouts VALUES ('EXPTSTATUS','NEW',0,'');
REPLACE INTO state_timeouts VALUES ('EXPTSTATUS','PRERUN',0,'');
REPLACE INTO state_timeouts VALUES ('EXPTSTATUS','SWAPPED',0,'');
REPLACE INTO state_timeouts VALUES ('EXPTSTATUS','SWAPPING',0,'');
REPLACE INTO state_timeouts VALUES ('EXPTSTATUS','TERMINATING',0,'');
REPLACE INTO state_timeouts VALUES ('EXPTSTATUS','TESTING',0,'');
REPLACE INTO state_timeouts VALUES ('MINIMAL','BOOTING',180,'REBOOT');
REPLACE INTO state_timeouts VALUES ('NODEALLOC','FREE_CLEAN',0,'');
REPLACE INTO state_timeouts VALUES ('NODEALLOC','FREE_DIRTY',0,'');
REPLACE INTO state_timeouts VALUES ('NODEALLOC','REBOOT',0,'');
REPLACE INTO state_timeouts VALUES ('NODEALLOC','RELOAD',0,'');
REPLACE INTO state_timeouts VALUES ('NODEALLOC','RESERVED',0,'');
REPLACE INTO state_timeouts VALUES ('NORMAL','BOOTING',180,'REBOOT');
REPLACE INTO state_timeouts VALUES ('NORMALv1','BOOTING',180,'REBOOT');
REPLACE INTO state_timeouts VALUES ('RELOAD','BOOTING',180,'REBOOT');
REPLACE INTO state_timeouts VALUES ('RELOAD','RELOADING',600,'NOTIFY');
REPLACE INTO state_timeouts VALUES ('RELOAD','RELOADSETUP',60,'NOTIFY');
REPLACE INTO state_timeouts VALUES ('RELOAD','SHUTDOWN',120,'REBOOT');
REPLACE INTO state_timeouts VALUES ('USERSTATUS','ACTIVE',0,'');
REPLACE INTO state_timeouts VALUES ('USERSTATUS','FROZEN',0,'');
REPLACE INTO state_timeouts VALUES ('USERSTATUS','NEWUSER',0,'');
REPLACE INTO state_timeouts VALUES ('USERSTATUS','UNAPPROVED',0,'');
REPLACE INTO state_timeouts VALUES ('TBCOMMAND','REBOOT',15,'CMDRETRY');
REPLACE INTO state_timeouts VALUES ('TBCOMMAND','POWEROFF',0,'CMDRETRY');
REPLACE INTO state_timeouts VALUES ('TBCOMMAND','POWERON',0,'CMDRETRY');
REPLACE INTO state_timeouts VALUES ('TBCOMMAND','POWERCYCLE',0,'CMDRETRY');
REPLACE INTO state_timeouts VALUES ('PCVM','BOOTING',1200,'NOTIFY');
REPLACE INTO state_timeouts VALUES ('PCVM','SHUTDOWN',0,'');
REPLACE INTO state_timeouts VALUES ('PCVM','ISUP',0,'');
REPLACE INTO state_timeouts VALUES ('PCVM','TBSETUP',600,'NOTIFY');
REPLACE INTO state_timeouts VALUES ('PXEFBSD','REBOOTING',120,'REBOOT');
REPLACE INTO state_timeouts VALUES ('PXEFBSD','REBOOTED',60,'NOTIFY');
REPLACE INTO state_timeouts VALUES ('PXEFBSD','BOOTING',180,'REBOOT');
REPLACE INTO state_timeouts VALUES ('NORMALv2','TBSETUP',600,'NOTIFY');
REPLACE INTO state_timeouts VALUES ('NORMALv2','BOOTING',180,'REBOOT');
REPLACE INTO state_timeouts VALUES ('GARCIA-STARGATEv1','TBSETUP',600,'NOTIFY');
REPLACE INTO state_timeouts VALUES ('PXEKERNEL','PXEWAKEUP',20,'REBOOT');

--
-- Dumping data for table `state_transitions`
--


REPLACE INTO state_transitions VALUES ('ALWAYSUP','ISUP','SHUTDOWN','Reboot');
REPLACE INTO state_transitions VALUES ('ALWAYSUP','SHUTDOWN','ISUP','BootDone');
REPLACE INTO state_transitions VALUES ('PCVM','ISUP','BOOTING','Crash');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','TERMINATING','SWAPPED','Error');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','TERMINATING','ENDED','NoError');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','MODIFY_RESWAP','SWAPPING','Nonrecover Error');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','MODIFY_PARSE','ACTIVE','Error');
REPLACE INTO state_transitions VALUES ('NETBOOT','SHUTDOWN','PXEBOOTING','DHCP');
REPLACE INTO state_transitions VALUES ('RELOAD','PXEBOOTING','BOOTING','BootInfo');
REPLACE INTO state_transitions VALUES ('RELOAD','SHUTDOWN','PXEBOOTING','DHCP');
REPLACE INTO state_transitions VALUES ('NETBOOT','BOOTING','ISUP','BootDone');
REPLACE INTO state_transitions VALUES ('NETBOOT','BOOTING','SHUTDOWN','Error');
REPLACE INTO state_transitions VALUES ('NETBOOT','ISUP','BOOTING','KernelChange');
REPLACE INTO state_transitions VALUES ('NETBOOT','ISUP','ISUP','Retry');
REPLACE INTO state_transitions VALUES ('NETBOOT','ISUP','SHUTDOWN','Reboot');
REPLACE INTO state_transitions VALUES ('NETBOOT','SHUTDOWN','BOOTING','DHCP');
REPLACE INTO state_transitions VALUES ('NETBOOT','SHUTDOWN','SHUTDOWN','Retry');
REPLACE INTO state_transitions VALUES ('MINIMAL','BOOTING','ISUP','BootDone');
REPLACE INTO state_transitions VALUES ('MINIMAL','BOOTING','SHUTDOWN','Error');
REPLACE INTO state_transitions VALUES ('MINIMAL','ISUP','BOOTING','SilentReboot');
REPLACE INTO state_transitions VALUES ('MINIMAL','ISUP','SHUTDOWN','Reboot');
REPLACE INTO state_transitions VALUES ('MINIMAL','SHUTDOWN','BOOTING','DHCP');
REPLACE INTO state_transitions VALUES ('NORMAL','REBOOTING','REBOOTING','Retry');
REPLACE INTO state_transitions VALUES ('NORMALv1','TBSETUP','ISUP','BootDone');
REPLACE INTO state_transitions VALUES ('NORMALv1','SHUTDOWN','SHUTDOWN','Retry');
REPLACE INTO state_transitions VALUES ('NORMALv1','SHUTDOWN','PXEBOOTING','DHCP');
REPLACE INTO state_transitions VALUES ('NORMALv1','ISUP','SHUTDOWN','Reboot');
REPLACE INTO state_transitions VALUES ('NORMALv1','BOOTING','SHUTDOWN','Error');
REPLACE INTO state_transitions VALUES ('RELOAD','BOOTING','BOOTING','DHCPRetry');
REPLACE INTO state_transitions VALUES ('RELOAD','BOOTING','RELOADSETUP','BootOK');
REPLACE INTO state_transitions VALUES ('RELOAD','BOOTING','SHUTDOWN','Error');
REPLACE INTO state_transitions VALUES ('RELOAD','RELOADING','RELOADDONE','ReloadDone');
REPLACE INTO state_transitions VALUES ('RELOAD','RELOADING','RELOADDONEV2','ReloadDone');
REPLACE INTO state_transitions VALUES ('RELOAD','RELOADING','SHUTDOWN','Error');
REPLACE INTO state_transitions VALUES ('RELOAD','RELOADSETUP','RELOADING','ReloadReady');
REPLACE INTO state_transitions VALUES ('RELOAD','RELOADSETUP','SHUTDOWN','Error');
REPLACE INTO state_transitions VALUES ('RELOAD','SHUTDOWN','BOOTING','DHCP');
REPLACE INTO state_transitions VALUES ('RELOAD','SHUTDOWN','SHUTDOWN','Retry');
REPLACE INTO state_transitions VALUES ('USERSTATUS','ACTIVE','FROZEN','');
REPLACE INTO state_transitions VALUES ('USERSTATUS','FROZEN','ACTIVE','');
REPLACE INTO state_transitions VALUES ('USERSTATUS','NEWUSER','UNAPPROVED','');
REPLACE INTO state_transitions VALUES ('USERSTATUS','UNAPPROVED','ACTIVE','');
REPLACE INTO state_transitions VALUES ('BATCHSTATE','SWAPPED','ACTIVATING','SwapIn');
REPLACE INTO state_transitions VALUES ('WIDEAREA','ISUP','REBOOTED','SilentReboot');
REPLACE INTO state_transitions VALUES ('WIDEAREA','ISUP','SHUTDOWN','Reboot');
REPLACE INTO state_transitions VALUES ('WIDEAREA','REBOOTED','ISUP','BootDone');
REPLACE INTO state_transitions VALUES ('WIDEAREA','REBOOTED','SHUTDOWN','Error');
REPLACE INTO state_transitions VALUES ('BATCHSTATE','ACTIVATING','ACTIVE','SwapIn');
REPLACE INTO state_transitions VALUES ('NORMALv2','SHUTDOWN','SHUTDOWN','Retry');
REPLACE INTO state_transitions VALUES ('BATCHSTATE','ACTIVATING','POSTED','Batch');
REPLACE INTO state_transitions VALUES ('WIDEAREA','SHUTDOWN','REBOOTED','BootOK');
REPLACE INTO state_transitions VALUES ('WIDEAREA','SHUTDOWN','SHUTDOWN','Retry');
REPLACE INTO state_transitions VALUES ('PCVM','BOOTING','SHUTDOWN','Error');
REPLACE INTO state_transitions VALUES ('PCVM','BOOTING','TBSETUP','BootOK');
REPLACE INTO state_transitions VALUES ('PCVM','ISUP','SHUTDOWN','Reboot');
REPLACE INTO state_transitions VALUES ('PCVM','SHUTDOWN','BOOTING','StartBoot');
REPLACE INTO state_transitions VALUES ('PCVM','TBSETUP','ISUP','BootDone');
REPLACE INTO state_transitions VALUES ('PCVM','TBSETUP','SHUTDOWN','Error');
REPLACE INTO state_transitions VALUES ('PCVM','BOOTING','ISUP','BootDone');
REPLACE INTO state_transitions VALUES ('ALWAYSUP','ISUP','ISUP','Retry');
REPLACE INTO state_transitions VALUES ('PCVM','SHUTDOWN','SHUTDOWN','Retry');
REPLACE INTO state_transitions VALUES ('NORMAL','BOOTING','REBOOTING','Error');
REPLACE INTO state_transitions VALUES ('NORMALv1','BOOTING','TBSETUP','BootOK');
REPLACE INTO state_transitions VALUES ('NODEALLOC','RELOAD_TO_FREE','FREE_CLEAN','ReloadDone');
REPLACE INTO state_transitions VALUES ('NORMAL','SHUTDOWN','REBOOTING','Reboot');
REPLACE INTO state_transitions VALUES ('NORMAL','REBOOTED','ISUP','BootDone');
REPLACE INTO state_transitions VALUES ('NORMAL','BOOTING','SHUTDOWN','Error');
REPLACE INTO state_transitions VALUES ('NORMAL','PXEBOOTING','BOOTING','BootInfo');
REPLACE INTO state_transitions VALUES ('NORMALv1','PXEBOOTING','BOOTING','BootInfo');
REPLACE INTO state_transitions VALUES ('BATCHSTATE','POSTED','ACTIVATING','SwapIn');
REPLACE INTO state_transitions VALUES ('NORMAL','REBOOTING','PXEBOOTING','DHCP');
REPLACE INTO state_transitions VALUES ('NORMALv1','ISUP','PXEBOOTING','KernelChange');
REPLACE INTO state_transitions VALUES ('NODEALLOC','FREE_CLEAN','RES_INIT_CLEAN','Reserve');
REPLACE INTO state_transitions VALUES ('PXEKERNEL','PXEWAIT','PXEBOOTING','Retry');
REPLACE INTO state_transitions VALUES ('PXEKERNEL','PXEBOOTING','PXEWAIT','Free');
REPLACE INTO state_transitions VALUES ('BATCHSTATE','ACTIVATING','SWAPPED','NonBatch');
REPLACE INTO state_transitions VALUES ('NORMAL','ISUP','SHUTDOWN','Reboot');
REPLACE INTO state_transitions VALUES ('NORMAL','REBOOTING','SHUTDOWN','Reboot');
REPLACE INTO state_transitions VALUES ('NORMAL','REBOOTED','REBOOTING','Error');
REPLACE INTO state_transitions VALUES ('NORMAL','ISUP','REBOOTING','Reboot');
REPLACE INTO state_transitions VALUES ('NORMAL','BOOTING','REBOOTED','BootOK');
REPLACE INTO state_transitions VALUES ('NORMALv2','SHUTDOWN','PXEBOOTING','DHCP');
REPLACE INTO state_transitions VALUES ('NORMAL','SHUTDOWN','PXEBOOTING','DHCP');
REPLACE INTO state_transitions VALUES ('NORMAL','REBOOTED','SHUTDOWN','Error');
REPLACE INTO state_transitions VALUES ('EXAMPLE','NEW','VERIFIED','Verify');
REPLACE INTO state_transitions VALUES ('EXAMPLE','NEW','APPROVED','Approve');
REPLACE INTO state_transitions VALUES ('EXAMPLE','APPROVED','READY','Verify');
REPLACE INTO state_transitions VALUES ('EXAMPLE','VERIFIED','READY','Approve');
REPLACE INTO state_transitions VALUES ('EXAMPLE','FROZEN','READY','Thaw');
REPLACE INTO state_transitions VALUES ('EXAMPLE','READY','FROZEN','Freeze');
REPLACE INTO state_transitions VALUES ('EXAMPLE','LOCKED','READY','Unlock');
REPLACE INTO state_transitions VALUES ('EXAMPLE','READY','LOCKED','Lock');
REPLACE INTO state_transitions VALUES ('EXAMPLE','FROZEN','LOCKED','Lock');
REPLACE INTO state_transitions VALUES ('EXAMPLE','LOCKED','FROZEN','Freeze');
REPLACE INTO state_transitions VALUES ('EXAMPLE','READY','APPROVED','Un-Verify');
REPLACE INTO state_transitions VALUES ('EXAMPLE','READY','VERIFIED','Un-Approve');
REPLACE INTO state_transitions VALUES ('EXAMPLE','VERIFIED','NEW','Un-Verify');
REPLACE INTO state_transitions VALUES ('EXAMPLE','APPROVED','NEW','Un-Approve');
REPLACE INTO state_transitions VALUES ('BATCHSTATE','ACTIVE','TERMINATING','SwapOut');
REPLACE INTO state_transitions VALUES ('BATCHSTATE','TERMINATING','SWAPPED','SwapOut');
REPLACE INTO state_transitions VALUES ('BATCHSTATE','SWAPPED','POSTED','RePost');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','MODIFY_PRERUN','SWAPPED','(No)Error');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','SWAPPED','MODIFY_PRERUN','Modify');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','SWAPPED','TERMINATING','EndExp');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','SWAPPING','SWAPPED','(No)Error');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','ACTIVATING','SWAPPED','Error');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','ACTIVE','SWAPPING','SwapOut');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','ACTIVE','RESTARTING','Restart');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','RESTARTING','ACTIVE','(No)Error');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','ACTIVATING','ACTIVE','NoError');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','QUEUED','TERMINATING','Endexp');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','SWAPPED','QUEUED','Queue');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','QUEUED','ACTIVATING','BatchRun');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','QUEUED','SWAPPED','Dequeue');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','PRERUN','QUEUED','Batch');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','PRERUN','SWAPPED','Immediate');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','NEW','PRERUN','Create');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','NEW','ENDED','Endexp');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','SWAPPED','ACTIVATING','SwapIn');
REPLACE INTO state_transitions VALUES ('PXEKERNEL','PXEWAKEUP','PXEBOOTING','Wokeup');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','MODIFY_PARSE','MODIFY_RESWAP','NoError');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','ACTIVE','MODIFY_PARSE','Modify');
REPLACE INTO state_transitions VALUES ('EXPTSTATE','MODIFY_RESWAP','ACTIVE','(No)Error');
REPLACE INTO state_transitions VALUES ('PXEKERNEL','PXEWAIT','PXEWAKEUP','NodeAlloced');
REPLACE INTO state_transitions VALUES ('PXEKERNEL','SHUTDOWN','PXEBOOTING','BootInfo');
REPLACE INTO state_transitions VALUES ('PXEKERNEL','PXEBOOTING','PXEBOOTING','Retry');
REPLACE INTO state_transitions VALUES ('PXEKERNEL','PXEBOOTING','BOOTING','Not Free');
REPLACE INTO state_transitions VALUES ('NORMALv2','RECONFIG','SHUTDOWN','ReConfigFail');
REPLACE INTO state_transitions VALUES ('NODEALLOC','RELOAD_PENDING','RELOAD_TO_FREE','Reload');
REPLACE INTO state_transitions VALUES ('PXEKERNEL','PXEWAKEUP','PXEWAKEUP','Retry');
REPLACE INTO state_transitions VALUES ('NORMALv2','ISUP','SHUTDOWN','Reboot');
REPLACE INTO state_transitions VALUES ('NORMALv2','BOOTING','SHUTDOWN','Error');
REPLACE INTO state_transitions VALUES ('NORMALv2','SHUTDOWN','WEDGED','Error');
REPLACE INTO state_transitions VALUES ('NORMALv2','BOOTING','TBSETUP','BootOK');
REPLACE INTO state_transitions VALUES ('NORMALv2','PXEBOOTING','BOOTING','BootInfo');
REPLACE INTO state_transitions VALUES ('NORMALv2','ISUP','PXEBOOTING','KernelChange');
REPLACE INTO state_transitions VALUES ('NORMALv2','TBSETUP','SHUTDOWN','Error');
REPLACE INTO state_transitions VALUES ('NORMALv2','ISUP','RECONFIG','DoReConfig');
REPLACE INTO state_transitions VALUES ('NORMALv2','RECONFIG','TBSETUP','ReConfig');
REPLACE INTO state_transitions VALUES ('NORMALv2','TBSETUP','ISUP','BootDone');
REPLACE INTO state_transitions VALUES ('NORMALv1','TBSETUP','SHUTDOWN','Error');
REPLACE INTO state_transitions VALUES ('NORMAL','SHUTDOWN','SHUTDOWN','Retry');
REPLACE INTO state_transitions VALUES ('NETBOOT','PXEBOOTING','BOOTING','BootInfo');
REPLACE INTO state_transitions VALUES ('NODEALLOC','RES_INIT_CLEAN','RES_CLEAN_REBOOT','Reboot');
REPLACE INTO state_transitions VALUES ('NODEALLOC','RES_INIT_CLEAN','RELOAD_TO_DIRTY','Reload');
REPLACE INTO state_transitions VALUES ('NODEALLOC','RES_CLEAN_REBOOT','RES_WAIT_CLEAN','Rebooting');
REPLACE INTO state_transitions VALUES ('NODEALLOC','RELOAD_TO_DIRTY','RES_DIRTY_REBOOT','Reboot');
REPLACE INTO state_transitions VALUES ('NODEALLOC','RES_DIRTY_REBOOT','RES_WAIT_DIRTY','Rebooting');
REPLACE INTO state_transitions VALUES ('NODEALLOC','RES_WAIT_CLEAN','RES_READY','IsUp');
REPLACE INTO state_transitions VALUES ('NODEALLOC','RES_WAIT_DIRTY','RES_READY','IsUp');
REPLACE INTO state_transitions VALUES ('NODEALLOC','RES_READY','RELOAD_PENDING','Free');
REPLACE INTO state_transitions VALUES ('MINIMAL','BOOTING','BOOTING','DHCPRetry');
REPLACE INTO state_transitions VALUES ('MINIMAL','SHUTDOWN','SHUTDOWN','Retry');
REPLACE INTO state_transitions VALUES ('NORMALv2','TBSETUP','TBSETUP','LongSetup');
REPLACE INTO state_transitions VALUES ('OPSNODEBSD','ISUP','SHUTDOWN','Reboot');
REPLACE INTO state_transitions VALUES ('OPSNODEBSD','SHUTDOWN','TBSETUP','Booting');
REPLACE INTO state_transitions VALUES ('OPSNODEBSD','TBSETUP','ISUP','BootDone');
REPLACE INTO state_transitions VALUES ('OPSNODEBSD','ISUP','TBSETUP','Crash');
REPLACE INTO state_transitions VALUES ('RELOAD-MOTE','RELOADING','RELOADDONE','ReloadDone');
REPLACE INTO state_transitions VALUES ('RELOAD-MOTE','SHUTDOWN','RELOADING','Booting');
REPLACE INTO state_transitions VALUES ('NORMALv2','TBSETUP','TBFAILED','BootFail');
REPLACE INTO state_transitions VALUES ('NORMALv2','TBFAILED','SHUTDOWN','RebootAfterFail');
REPLACE INTO state_transitions VALUES ('PCVM','TBSETUP','TBFAILED','BootError');
REPLACE INTO state_transitions VALUES ('PCVM','TBFAILED','SHUTDOWN','Reboot');
REPLACE INTO state_transitions VALUES ('GARCIA-STARGATEv1','SHUTDOWN','SHUTDOWN','Retry');
REPLACE INTO state_transitions VALUES ('GARCIA-STARGATEv1','SHUTDOWN','TBSETUP','DHCP');
REPLACE INTO state_transitions VALUES ('GARCIA-STARGATEv1','SHUTDOWN','POWEROFF','PowerOff');
REPLACE INTO state_transitions VALUES ('GARCIA-STARGATEv1','POWEROFF','TBSETUP','PowerOn');
REPLACE INTO state_transitions VALUES ('GARCIA-STARGATEv1','RECONFIG','SHUTDOWN','ReConfigFail');
REPLACE INTO state_transitions VALUES ('GARCIA-STARGATEv1','ISUP','SHUTDOWN','Reboot');
REPLACE INTO state_transitions VALUES ('GARCIA-STARGATEv1','TBSETUP','SHUTDOWN','Error');
REPLACE INTO state_transitions VALUES ('GARCIA-STARGATEv1','ISUP','RECONFIG','DoReConfig');
REPLACE INTO state_transitions VALUES ('GARCIA-STARGATEv1','RECONFIG','TBSETUP','ReConfig');
REPLACE INTO state_transitions VALUES ('GARCIA-STARGATEv1','TBSETUP','ISUP','BootDone');
REPLACE INTO state_transitions VALUES ('GARCIA-STARGATEv1','TBSETUP','TBSETUP','LongSetup');
REPLACE INTO state_transitions VALUES ('GARCIA-STARGATEv1','TBSETUP','TBFAILED','BootFail');
REPLACE INTO state_transitions VALUES ('GARCIA-STARGATEv1','TBFAILED','SHUTDOWN','RebootAfterFail');

--
-- Dumping data for table `state_triggers`
--


REPLACE INTO state_triggers VALUES ('*','RELOAD','RELOADDONE','RESET, RELOADDONE');
REPLACE INTO state_triggers VALUES ('*','RELOAD','RELOADDONEV2','RESET, RELOADDONEV2');
REPLACE INTO state_triggers VALUES ('*','ALWAYSUP','SHUTDOWN','ISUP');
REPLACE INTO state_triggers VALUES ('*','*','ISUP','RESET, CHECKPORTREG');
REPLACE INTO state_triggers VALUES ('*','*','PXEBOOTING','PXEBOOT');
REPLACE INTO state_triggers VALUES ('*','*','BOOTING','BOOTING, CHECKGENISUP');
REPLACE INTO state_triggers VALUES ('*','MINIMAL','ISUP','RESET');
REPLACE INTO state_triggers VALUES ('*','RELOAD-MOTE','RELOADDONE','RELOADDONE');
REPLACE INTO state_triggers VALUES ('*','OPSNODEBSD','ISUP','SCRIPT:opsreboot');
REPLACE INTO state_triggers VALUES ('*','NORMALv2','WEDGED','POWERCYCLE');

--
-- Dumping data for table `table_regex`
--


REPLACE INTO table_regex VALUES ('eventlist','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('eventlist','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('eventlist','time','float','redirect','default:float',0,0,NULL);
REPLACE INTO table_regex VALUES ('eventlist','vnode','text','redirect','virt_agents:vnode',0,0,NULL);
REPLACE INTO table_regex VALUES ('eventlist','vname','text','regex','^[-\\w\\(\\)]+$',1,64,NULL);
REPLACE INTO table_regex VALUES ('eventlist','objecttype','int','redirect','default:tinyint',0,0,NULL);
REPLACE INTO table_regex VALUES ('eventlist','eventtype','int','redirect','default:tinyint',0,0,NULL);
REPLACE INTO table_regex VALUES ('eventlist','arguments','text','redirect','default:text',0,1024,NULL);
REPLACE INTO table_regex VALUES ('eventlist','atstring','text','redirect','default:text',0,1024,NULL);
REPLACE INTO table_regex VALUES ('experiments','eid','text','regex','^[a-zA-Z0-9][-a-zA-Z0-9]+$',2,19,'Must ensure not too long for the database. PID is 12, and the max is 32, so the user is not allowed to specify an EID more than 19, since other parts of the system may concatenate them together with a hyphen');
REPLACE INTO table_regex VALUES ('experiments','multiplex_factor','int','redirect','default:tinyint',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','forcelinkdelays','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','uselinkdelays','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','usewatunnels','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','uselatestwadata','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','wa_delay_solverweight','float','redirect','default:float',0,1024,NULL);
REPLACE INTO table_regex VALUES ('experiments','wa_bw_solverweight','float','redirect','default:float',0,1024,NULL);
REPLACE INTO table_regex VALUES ('experiments','wa_plr_solverweight','float','redirect','default:float',0,1024,NULL);
REPLACE INTO table_regex VALUES ('experiments','sync_server','text','redirect','virt_nodes:vname',0,0,NULL);

REPLACE INTO table_regex VALUES ('groups','project','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('groups','gid','text','regex','^[a-zA-Z][-\\w]+$',2,12,NULL);
REPLACE INTO table_regex VALUES ('groups','group_id','text','redirect','groups:gid',2,12,NULL);
REPLACE INTO table_regex VALUES ('groups','group_leader','text','redirect','users:uid',2,8,NULL);
REPLACE INTO table_regex VALUES ('groups','group_description','text','redirect','default:tinytext',0,256,NULL);

REPLACE INTO table_regex VALUES ('nodes','node_id','text','regex','^[-\\w]+$',1,12,NULL);
REPLACE INTO table_regex VALUES ('nseconfigs','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('nseconfigs','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('nseconfigs','vname','text','redirect','virt_nodes:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('nseconfigs','nseconfig','text','regex','^[\\040-\\176\\012\\011\\015]*$',0,16777215,NULL);
REPLACE INTO table_regex VALUES ('projects','newpid','text','regex','^[a-zA-Z][-a-zA-Z0-9]+$',2,12,NULL);
REPLACE INTO table_regex VALUES ('projects','head_uid','text','redirect','users:uid',0,0,NULL);
REPLACE INTO table_regex VALUES ('projects','name','text','redirect','default:tinytext',0,256,NULL);
REPLACE INTO table_regex VALUES ('projects','funders','text','redirect','default:tinytext',0,256,NULL);
REPLACE INTO table_regex VALUES ('projects','public','int','redirect','default:tinyint',0,1,NULL);
REPLACE INTO table_regex VALUES ('projects','linked_to_us','int','redirect','default:tinyint',0,1,NULL);
REPLACE INTO table_regex VALUES ('projects','public_whynot','text','redirect','default:tinytext',0,256,NULL);
REPLACE INTO table_regex VALUES ('projects','default_user_interface','text','regex','^(emulab|plab)$',2,12,NULL);
REPLACE INTO table_regex VALUES ('projects','pid','text','regex','^[-\\w]+$',2,12,NULL);
REPLACE INTO table_regex VALUES ('projects','URL','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('reserved','vname','text','redirect','virt_nodes:vname',1,32,NULL);

REPLACE INTO table_regex VALUES ('users','uid','text','regex','^[a-zA-Z][\\w]+$',2,8,NULL);
REPLACE INTO table_regex VALUES ('users','usr_phone','text','regex','^[-\\d\\(\\)\\+\\.x ]+$',7,64,NULL);
REPLACE INTO table_regex VALUES ('users','usr_name','text','regex','^[-\\w\\. ]+$',4,64,NULL);
REPLACE INTO table_regex VALUES ('users','usr_email','text','regex','^([-\\w\\+\\.]+)\\@([-\\w\\.]+)$',3,64,NULL);
REPLACE INTO table_regex VALUES ('users','usr_shell','text','regex','^(csh|sh|bash|tcsh)$',0,0,NULL);
REPLACE INTO table_regex VALUES ('users','usr_title','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('users','usr_affil','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('users','usr_addr','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('users','usr_addr2','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('users','usr_state','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('users','usr_city','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('users','usr_zip','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('users','usr_country','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('users','usr_URL','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('users','usr_pswd','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('users','password1','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('users','password2','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('users','w_password1','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('users','w_password2','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('users','user_interface','text','regex','^(emulab|plab)$',0,0,NULL);
REPLACE INTO table_regex VALUES ('users','notes','text','redirect','default:text',0,65535,NULL);

REPLACE INTO table_regex VALUES ('virt_agents','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_agents','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_agents','vname','text','redirect','eventlist:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_agents','vnode','text','regex','^([-\\w]+)|(\\*{1})$',1,32,NULL);
REPLACE INTO table_regex VALUES ('virt_agents','objecttype','int','redirect','default:tinyint',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','vname','text','redirect','virt_nodes:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','delay','float','redirect','default:float',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','bandwidth','int','redirect','default:int',0,2147483647,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','lossrate','float','redirect','default:float',0,1,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','q_limit','int','redirect','default:int',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','q_maxthresh','int','redirect','default:int',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','q_minthresh','int','redirect','default:int',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','q_weight','float','redirect','default:float',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','q_linterm','int','redirect','default:int',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','q_qinbytes','int','redirect','default:tinyint',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','q_bytes','int','redirect','default:tinyint',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','q_meanpsize','int','redirect','default:int',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','q_wait','int','redirect','default:int',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','q_setbit','int','redirect','default:int',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','q_droptail','int','redirect','default:int',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','q_red','int','redirect','default:tinyint',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','q_gentle','int','redirect','default:tinyint',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','member','text','regex','^[-\\w]+:[\\d]+$',0,128,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','mask','text','regex','^\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}$',0,15,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','rdelay','float','redirect','virt_lans:delay',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','rbandwidth','int','redirect','virt_lans:bandwidth',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','rlossrate','float','redirect','virt_lans:lossrate',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','cost','float','redirect','default:float',0,1,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','widearea','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','emulated','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','uselinkdelay','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','nobwshaping','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','usevethiface','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','encap_style','text','redirect','experiments:encap_style',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','trivial_ok','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','traced','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','trace_type','text','regex','^(header|packet|monitor)$',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','trace_expr','text','redirect','default:text',1,255,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','trace_snaplen','int','redirect','default:int',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','trace_endnode','int','redirect','default:tinyint',0,1,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','trace_db','int','redirect','default:tinyint',0,1,NULL);
REPLACE INTO table_regex VALUES ('virt_node_desires','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_node_desires','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_node_desires','vname','text','redirect','virt_nodes:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_node_desires','desire','text','regex','^[\\?\\*]?[\\+\\!\\&]?[-\\w?+]+$',1,30,NULL);
REPLACE INTO table_regex VALUES ('virt_node_desires','weight','int','redirect','default:float',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_nodes','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_nodes','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_nodes','ips','text','regex','^(\\d{1,2}:\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3} {0,1})*$',0,1024,NULL);
REPLACE INTO table_regex VALUES ('virt_nodes','osname','text','redirect','os_info:osname',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_nodes','cmd_line','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_nodes','rpms','text','regex','^([-\\w\\.\\/\\+:~]+;{0,1})*$',0,4096,NULL);
REPLACE INTO table_regex VALUES ('virt_nodes','deltas','text','regex','^([-\\w\\.\\/\\+]+:{0,1})*$',0,1024,NULL);
REPLACE INTO table_regex VALUES ('virt_nodes','startupcmd','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_nodes','tarfiles','text','regex','^([-\\w\\.\\/\\+]+\\s+[-\\w\\.\\/\\+:~]+;{0,1})*$',0,1024,NULL);
REPLACE INTO table_regex VALUES ('virt_nodes','vname','text','regex','^[-\\w]+$',1,32,NULL);
REPLACE INTO table_regex VALUES ('virt_nodes','type','text','regex','^[-\\w]*$',0,30,NULL);
REPLACE INTO table_regex VALUES ('virt_nodes','failureaction','text','regex','^(fatal|nonfatal|ignore)$',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_nodes','routertype','text','regex','^(none|ospf|static|manual|static-ddijk|static-old)$',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_nodes','fixed','text','regex','^[-\\w]*$',0,32,NULL);
REPLACE INTO table_regex VALUES ('virt_programs','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_programs','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_programs','vnode','text','redirect','virt_nodes:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_programs','vname','text','regex','^[-\\w\\(\\)]+$',1,32,NULL);
REPLACE INTO table_regex VALUES ('virt_programs','command','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_routes','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_routes','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_routes','vname','text','redirect','virt_nodes:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_routes','src','text','regex','^(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}){0,1}$',0,32,NULL);
REPLACE INTO table_regex VALUES ('virt_routes','dst','text','regex','^(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})$',0,32,NULL);
REPLACE INTO table_regex VALUES ('virt_routes','dst_type','text','regex','^(host|net)$',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_routes','dst_mask','text','regex','^(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})$',1,15,NULL);
REPLACE INTO table_regex VALUES ('virt_routes','nexthop','text','regex','^(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})$',0,32,NULL);
REPLACE INTO table_regex VALUES ('virt_routes','cost','float','redirect','default:float',0,100,NULL);
REPLACE INTO table_regex VALUES ('virt_trafgens','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_trafgens','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_trafgens','vnode','text','redirect','virt_nodes:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_trafgens','vname','text','regex','^[-\\w\\(\\)]+$',1,32,NULL);
REPLACE INTO table_regex VALUES ('virt_trafgens','role','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_trafgens','proto','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_trafgens','port','text','redirect','default:int',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_trafgens','ip','text','regex','^(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})$',0,15,NULL);
REPLACE INTO table_regex VALUES ('virt_trafgens','target_vnode','text','redirect','virt_nodes:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_trafgens','target_vname','text','regex','^[-\\w\\(\\)]+$',1,32,NULL);
REPLACE INTO table_regex VALUES ('virt_trafgens','target_port','text','redirect','virt_trafgens:port',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_trafgens','target_ip','text','redirect','virt_trafgens:ip',0,15,NULL);
REPLACE INTO table_regex VALUES ('virt_trafgens','generator','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_vtypes','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_vtypes','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_vtypes','name','text','regex','^[-\\w]+$',1,32,NULL);
REPLACE INTO table_regex VALUES ('virt_vtypes','weight','float','redirect','default:float',0,1,NULL);
REPLACE INTO table_regex VALUES ('virt_vtypes','members','text','regex','^([-\\w]+ ?)+$',0,1024,NULL);
REPLACE INTO table_regex VALUES ('default','tinytext','text','regex','^[\\040-\\176]*$',0,256,NULL);
REPLACE INTO table_regex VALUES ('default','text','text','regex','^[\\040-\\176]*$',0,65535,NULL);
REPLACE INTO table_regex VALUES ('projects','why','text','regex','^[\\040-\\176\\012\\015\\011]*$',0,4096,NULL);
REPLACE INTO table_regex VALUES ('default','tinyint','int','regex','^[\\d]+$',-128,127,'Default regex for tiny int fields. Allow any standard ascii integer, but no binary data');
REPLACE INTO table_regex VALUES ('default','boolean','int','regex','^(0|1)$',0,1,'Default regex for tiny int fields that are int booleans. Allow any 0 or 1');
REPLACE INTO table_regex VALUES ('default','tinyuint','int','regex','^[\\d]+$',0,255,'Default regex for tiny int fields. Allow any standard ascii integer, but no binary data');
REPLACE INTO table_regex VALUES ('default','int','int','regex','^[\\d]+$',-2147483648,2147483647,'Default regex for int fields. Allow any standard ascii integer, but no binary data');
REPLACE INTO table_regex VALUES ('default','float','float','regex','^[+-]?\\ *(\\d+(\\.\\d*)?|\\.\\d+)([eE][+-]?\\d+)?$',-2147483648,2147483647,'Default regex for float fields. Allow any digits and the decimal point');
REPLACE INTO table_regex VALUES ('default','default','text','regex','^[\\040-\\176]*$',0,256,'Default regex if one is not defined for a table/slot. Allow any standard ascii character, but no binary data');
REPLACE INTO table_regex VALUES ('projects','num_members','int','redirect','default:int',0,256,NULL);
REPLACE INTO table_regex VALUES ('projects','num_pcs','int','redirect','default:int',0,2048,NULL);
REPLACE INTO table_regex VALUES ('projects','num_pcplab','int','redirect','default:int',0,2048,NULL);
REPLACE INTO table_regex VALUES ('projects','num_ron','int','redirect','default:int',0,1024,NULL);

REPLACE INTO table_regex VALUES ('experiments','encap_style','text','regex','^(alias|veth|veth-ne|vlan|default)$',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','veth_encapsulate','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','allowfixnode','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','jail_osname','text','redirect','os_info:osname',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','delay_osname','text','redirect','os_info:osname',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','use_ipassign','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','ipassign_args','text','regex','^[\\w\\s-]*$',0,255,NULL);
REPLACE INTO table_regex VALUES ('experiments','expt_name','text','redirect','default:tinytext',1,255,NULL);
REPLACE INTO table_regex VALUES ('experiments','dpdb','int','redirect','default:tinyint',0,1,NULL);

REPLACE INTO table_regex VALUES ('experiments','description','text','regex','^[\\040-\\176\\012\\015\\011]*$',1,256,NULL);
REPLACE INTO table_regex VALUES ('experiments','idle_ignore','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','swappable','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','noswap_reason','text','redirect','default:tinytext',1,255,NULL);
REPLACE INTO table_regex VALUES ('experiments','idleswap','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','idleswap_timeout','int','redirect','default:int',1,2147483647,NULL);
REPLACE INTO table_regex VALUES ('experiments','noidleswap_reason','text','redirect','default:tinytext',1,255,NULL);
REPLACE INTO table_regex VALUES ('experiments','autoswap','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','autoswap_timeout','int','redirect','default:int',1,2147483647,NULL);
REPLACE INTO table_regex VALUES ('experiments','savedisk','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','cpu_usage','int','redirect','default:tinyint',0,5,NULL);
REPLACE INTO table_regex VALUES ('experiments','mem_usage','int','redirect','default:tinyint',0,5,NULL);
REPLACE INTO table_regex VALUES ('experiments','batchmode','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','linktest_level','int','redirect','default:tinyint',0,4,NULL);

REPLACE INTO table_regex VALUES ('virt_lans','protocol','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','is_accesspoint','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lan_settings','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lan_settings','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lan_settings','vname','text','redirect','virt_lans:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lan_settings','capkey','text','regex','^[-\\w]+$',1,32,NULL);
REPLACE INTO table_regex VALUES ('virt_lan_settings','capval','text','regex','^[-\\w\\.:+]+$',1,64,NULL);
REPLACE INTO table_regex VALUES ('virt_lan_member_settings','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lan_member_settings','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lan_member_settings','vname','text','redirect','virt_lan_settings:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lan_member_settings','member','text','redirect','virt_lans:member',1,32,NULL);
REPLACE INTO table_regex VALUES ('virt_lan_member_settings','capkey','text','redirect','virt_lan_settings:capkey',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lan_member_settings','capval','text','redirect','virt_lan_settings:capval',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','est_bandwidth','int','redirect','default:int',0,2147483647,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','rest_bandwidth','int','redirect','default:int',0,2147483647,NULL);
REPLACE INTO table_regex VALUES ('location_info','floor','text','regex','^[-\\w]+$',1,32,NULL);
REPLACE INTO table_regex VALUES ('location_info','building','text','regex','^[-\\w]+$',1,32,NULL);
REPLACE INTO table_regex VALUES ('location_info','loc_x','int','redirect','default:int',0,2048,NULL);
REPLACE INTO table_regex VALUES ('location_info','loc_y','int','redirect','default:int',0,2048,NULL);
REPLACE INTO table_regex VALUES ('location_info','contact','text','redirect','users:usr_name',0,64,NULL);
REPLACE INTO table_regex VALUES ('location_info','phone','text','regex','^[-\\d\\(\\)\\+\\.x ]+$',7,64,NULL);
REPLACE INTO table_regex VALUES ('location_info','room','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','vnode','text','redirect','virt_nodes:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','vport','int','redirect','default:tinyint',0,99,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','ip','text','regex','^(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})$',0,15,NULL);
REPLACE INTO table_regex VALUES ('experiments','usemodelnet','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','modelnet_cores','int','redirect','default:tinyint',0,5,NULL);
REPLACE INTO table_regex VALUES ('experiments','modelnet_edges','int','redirect','default:tinyint',0,5,NULL);
REPLACE INTO table_regex VALUES ('virt_lans','mustdelay','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('event_groups','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('event_groups','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('event_groups','group_name','text','redirect','eventlist:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('event_groups','agent_name','text','redirect','eventlist:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lan_lans','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lan_lans','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_lan_lans','vname','text','redirect','virt_nodes:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('firewall_rules','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('firewall_rules','eid','text','redirect','experimenets:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('firewall_rules','fwname','text','redirect','virt_nodes:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('firewall_rules','ruleno','int','redirect','default:int',0,50000,NULL);
REPLACE INTO table_regex VALUES ('firewall_rules','rule','text','regex','^\\w[-\\w \\t,/\\{\\}\\(\\)!:\\.]*$',0,1024,NULL);
REPLACE INTO table_regex VALUES ('virt_nodes','inner_elab_role','text','regex','^(boss|boss\\+router|router|ops|ops\\+fs|fs|node)$',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_nodes','plab_role','text','regex','^(plc|node|none)$',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','elab_in_elab','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','elabinelab_singlenet','int','redirect','default:boolean',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiments','elabinelab_cvstag','text','regex','^[-\\w\\@\\/\\.]+$',0,0,NULL);
REPLACE INTO table_regex VALUES ('images','imageid','text','regex','^[a-zA-Z0-9][-\\w\\.+]+$',0,45,NULL);
REPLACE INTO table_regex VALUES ('images','imagename','text','regex','^[a-zA-Z0-9][-\\w\\.+]+$',2,30,NULL);
REPLACE INTO table_regex VALUES ('experiments','security_level','int','redirect','default:tinyuint',0,4,NULL);
REPLACE INTO table_regex VALUES ('experiments','elabinelab_eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_node_startloc','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_node_startloc','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_node_startloc','vname','text','redirect','virt_nodes:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_node_startloc','building','text','regex','^[-\\w]+$',1,32,NULL);
REPLACE INTO table_regex VALUES ('virt_node_startloc','floor','text','regex','^[-\\w]+$',1,32,NULL);
REPLACE INTO table_regex VALUES ('virt_node_startloc','loc_x','float','redirect','default:float',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_node_startloc','loc_y','float','redirect','default:float',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_node_startloc','orientation','float','redirect','default:float',0,0,NULL);
REPLACE INTO table_regex VALUES ('eventlist','parent','text','regex','^[-\\w\\(\\)]+$',1,64,NULL);
REPLACE INTO table_regex VALUES ('experiments','delay_capacity','int','redirect','default:tinyint',1,10,NULL);
REPLACE INTO table_regex VALUES ('virt_user_environment','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_user_environment','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_user_environment','name','text','redirect','^[a-zA-Z][-\\w]+$',0,255,NULL);
REPLACE INTO table_regex VALUES ('virt_user_environment','value','text','redirect','default:text',1,512,NULL);
REPLACE INTO table_regex VALUES ('virt_programs','dir','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_programs','timeout','int','redirect','default:int',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_programs','expected_exit_code','int','redirect','default:tinyint',0,0,NULL);
REPLACE INTO table_regex VALUES ('users','wikiname','text','regex','^[A-Z]+[a-z]+[A-Z]+[A-Za-z0-9]*$',4,64,NULL);
REPLACE INTO table_regex VALUES ('virt_tiptunnels','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_tiptunnels','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_tiptunnels','host','text','redirect','virt_nodes:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_tiptunnels','vnode','text','redirect','virt_nodes:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_nodes','numeric_id','int','redirect','default:int',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_firewalls','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_firewalls','eid','text','redirect','experimenets:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_firewalls','fwname','text','redirect','virt_nodes:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_firewalls','type','text','regex','^(ipfw|ipfw2|ipchains|ipfw2-vlan)$',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_firewalls','style','text','regex','^(open|closed|basic|emulab)$',0,0,NULL);

REPLACE INTO table_regex VALUES ('mailman_lists','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('mailman_lists','password1','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('mailman_lists','password2','text','redirect','default:tinytext',0,0,NULL);
REPLACE INTO table_regex VALUES ('mailman_lists','fullname','text','redirect','users:usr_email',0,0,NULL);
REPLACE INTO table_regex VALUES ('mailman_lists','listname','text','redirect','mailman_listnames:listname',0,0,NULL);

REPLACE INTO table_regex VALUES ('mailman_listnames','listname','text','regex','^[-\\w\\.\\+]+$',3,64,NULL);

REPLACE INTO table_regex VALUES ('default','fulltext','text','regex','^[\\040-\\176\\012\\015\\011]*$',0,20000,NULL);
REPLACE INTO table_regex VALUES ('node_attributes','attrkey','text','regex','^[-\\w]+$',1,32,NULL);
REPLACE INTO table_regex VALUES ('node_attributes','attrvalue','text','regex','^[-\\w\\.+,\\s]+$',0,255,NULL);
REPLACE INTO table_regex VALUES ('archive_tags','description','text','redirect','projects:why',1,2048,NULL);
REPLACE INTO table_regex VALUES ('archive_tags','tag','text','regex','^[a-zA-Z][-\\w\\.\\+]+$',2,64,NULL);
REPLACE INTO table_regex VALUES ('experiment_templates','description','text','regex','^[\\040-\\176\\012\\015\\011]*$',1,4096,NULL);
REPLACE INTO table_regex VALUES ('experiment_templates','guid','text','regex','^[\\w]+$',1,32,NULL);
REPLACE INTO table_regex VALUES ('experiment_template_metadata','name','text','regex','^[\\040-\\176]*$',1,64,NULL);
REPLACE INTO table_regex VALUES ('experiment_template_metadata','value','text','regex','^[\\040-\\176\\012\\015\\011]*$',0,4096,NULL);
REPLACE INTO table_regex VALUES ('experiment_template_metadata','metadata_type','text','regex','^[\\w]*$',1,64,NULL);
REPLACE INTO table_regex VALUES ('virt_parameters','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_parameters','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_parameters','name','text','regex','^\\w[-\\w]+$',1,64,NULL);
REPLACE INTO table_regex VALUES ('virt_parameters','value','text','redirect','default:tinytext',0,256,NULL);
REPLACE INTO table_regex VALUES ('virt_parameters','description','text','redirect','experiment_templates:description',0,1024,NULL);
REPLACE INTO table_regex VALUES ('experiment_template_instance_bindings','name','text','regex','^\\w[-\\w]+$',1,64,NULL);
REPLACE INTO table_regex VALUES ('experiment_template_instance_bindings','value','text','redirect','default:tinytext',0,256,NULL);
REPLACE INTO table_regex VALUES ('experiment_runs','runid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO table_regex VALUES ('experiment_runs','description','text','regex','^[\\040-\\176\\012\\015\\011]*$',1,256,NULL);
REPLACE INTO table_regex VALUES ('experiment_run_bindings','name','text','regex','^\\w[-\\w]+$',1,64,NULL);
REPLACE INTO table_regex VALUES ('experiment_run_bindings','value','text','redirect','default:tinytext',0,256,NULL);
REPLACE INTO table_regex VALUES ('experiment_template_instances','description','text','regex','^[\\040-\\176\\012\\015\\011]*$',1,256,NULL);
REPLACE INTO table_regex VALUES ('virt_node_motelog','vname','text','redirect','virt_nodes:vname',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_node_motelog','logfileid','text','regex','^[-\\w\\.+]+$',2,45,NULL);
REPLACE INTO table_regex VALUES ('virt_node_motelog','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('virt_node_motelog','eid','text','redirect','experiments:eid',0,0,NULL);
REPLACE INTO `table_regex` VALUES ('virt_nodes','plab_plcnet','text','regex','^[\\w\\_\\d]+$',0,0,NULL);
REPLACE INTO table_regex VALUES ('os_info','osid','text','regex','^[-\\w\\.+]+$',2,35,NULL);
REPLACE INTO table_regex VALUES ('os_info','pid','text','redirect','projects:pid',0,0,NULL);
REPLACE INTO table_regex VALUES ('os_info','osname','text','regex','^[-\\w\\.+]+$',2,20,NULL);
REPLACE INTO table_regex VALUES ('os_info','description','text','regex','^[\\040-\\176\\012\\015\\011]*$',1,256,NULL);
REPLACE INTO table_regex VALUES ('os_info','OS','text','regex','^[-\\w]*$',1,32,NULL);
REPLACE INTO table_regex VALUES ('os_info','version','text','regex','^[-\\w\\.]*$',1,12,NULL);
REPLACE INTO table_regex VALUES ('os_info','path','text','regex','^[-\\w\\.\\/:]*$',1,256,NULL);
REPLACE INTO table_regex VALUES ('os_info','magic','text','redirect','default:tinytext',0,256,NULL);
REPLACE INTO table_regex VALUES ('os_info','shared','int','redirect','default:tinyint',0,1,NULL);
REPLACE INTO table_regex VALUES ('os_info','mustclean','int','redirect','default:tinyint',0,1,NULL);
REPLACE INTO table_regex VALUES ('os_info','osfeatures','text','regex','^[-\\w,]*$',1,128,NULL);
REPLACE INTO table_regex VALUES ('os_info','op_mode','text','regex','^[-\\w]*$',1,20,NULL);
REPLACE INTO table_regex VALUES ('os_info','nextosid','text','redirect','os_info:osid',0,0,NULL);
REPLACE INTO table_regex VALUES ('os_info','reboot_waittime','int','redirect','default:int',0,2000,NULL);

--
-- Dumping data for table `testsuite_preentables`
--


REPLACE INTO testsuite_preentables VALUES ('comments','drop');
REPLACE INTO testsuite_preentables VALUES ('iface_counters','drop');
REPLACE INTO testsuite_preentables VALUES ('experiment_resources','clean');
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
REPLACE INTO testsuite_preentables VALUES ('virt_routes','clean');
REPLACE INTO testsuite_preentables VALUES ('group_stats','clean');
REPLACE INTO testsuite_preentables VALUES ('project_stats','clean');
REPLACE INTO testsuite_preentables VALUES ('user_stats','clean');
REPLACE INTO testsuite_preentables VALUES ('experiment_stats','clean');
REPLACE INTO testsuite_preentables VALUES ('testbed_stats','clean');

--
-- Dumping data for table `webdb_table_permissions`
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
REPLACE INTO webdb_table_permissions VALUES ('table_regex',1,1,1);


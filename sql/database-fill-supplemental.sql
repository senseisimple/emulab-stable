-- 
-- database-fill-supplemental.sql - Various things that need to go into new
-- sites' databases, but don't really fit into database-fill.sql, which is
-- auto-generated. Also, unlike the contents of database-fill.sql, inserting
-- these is not idempotent, since a site may have changed them for some reason.
--

INSERT IGNORE INTO `node_types` VALUES ('pc','pc',NULL,NULL,0,0,0,0,0,0,0,0,0,0,0,0);
INSERT IGNORE INTO`node_type_attributes` VALUES ('pc','imageable','1','boolean');
INSERT IGNORE INTO `node_types` VALUES ('pcvm','pcvm',NULL,NULL,1,0,1,1,0,0,0,0,0,0,0,0);
INSERT IGNORE INTO `node_types` VALUES ('pcvwa','pcvwa',NULL,NULL,1,0,0,1,1,0,0,0,0,0,0,0);
INSERT IGNORE INTO `node_type_attributes` VALUES ('pcvwa','default_osid','0','integer');

INSERT IGNORE INTO os_boot_cmd VALUES ('FreeBSD','4.10','delay','/kernel.delay');
INSERT IGNORE INTO os_boot_cmd VALUES ('FreeBSD','4.10','vnodehost','/kernel.jail');
INSERT IGNORE INTO os_boot_cmd VALUES ('FreeBSD','4.10','linkdelay','/kernel.linkdelay');
INSERT IGNORE INTO os_boot_cmd VALUES ('FreeBSD','5.4','delay','/boot/kernel.delay/kernel');
INSERT IGNORE INTO os_boot_cmd VALUES ('FreeBSD','5.4','linkdelay','/boot/kernel.linkdelay/kernel');
INSERT IGNORE INTO os_boot_cmd VALUES ('FreeBSD','6.2','delay','/boot/kernel.poll/kernel HZ=10000');
INSERT IGNORE INTO os_boot_cmd VALUES ('FreeBSD','6.2','linkdelay','/boot/kernel/kernel HZ=1000');
INSERT IGNORE INTO os_boot_cmd VALUES ('FreeBSD','7.3','delay','/boot/kernel.poll/kernel HZ=10000');
INSERT IGNORE INTO os_boot_cmd VALUES ('FreeBSD','8.1','delay','/boot/kernel.poll/kernel HZ=10000');
INSERT IGNORE INTO os_boot_cmd VALUES ('Linux','9.0','linkdelay','linkdelay');

INSERT IGNORE INTO emulab_indicies (name,idx) VALUES ('cur_log_seq', 1);
INSERT IGNORE INTO emulab_indicies (name,idx) VALUES ('frisbee_index', 1);
INSERT IGNORE INTO emulab_indicies (name,idx) VALUES ('next_osid', 10000);
INSERT IGNORE INTO emulab_locks (name,value) VALUES ('pool_daemon', 0);

-- 
-- database-fill-supplemental.sql - Various things that need to go into new
-- sites' databases, but don't really fit into database-fill.sql, which is
-- auto-generated. Also, unlike the contents of database-fill.sql, inserting
-- these is not idempotent, since a site may have changed them for some reason.
--

INSERT IGNORE INTO os_info VALUES ('FREEBSD-MFS','emulab-ops','FREEBSD-MFS','root',NULL,'FreeBSD in an MFS','FreeBSD','4.5','boss:/tftpboot/freebsd',NULL,'','ping,ssh,ipod,isup',0,1,0,'PXEFBSD',NULL,NULL,1,150);
INSERT IGNORE INTO os_info VALUES ('FRISBEE-MFS','emulab-ops','FRISBEE-MFS','root',NULL,'Frisbee (FreeBSD) in an MFS','FreeBSD','4.5','boss:/tftpboot/frisbee',NULL,'','ping,ssh,ipod,isup',0,1,0,'RELOAD',NULL,NULL,1,150);
INSERT IGNORE INTO os_info VALUES ('NEWNODE-MFS','emulab-ops','NEWNODE-MFS','root',NULL,'NewNode (FreeBSD) in an MFS','FreeBSD','4.5','boss:/tftpboot/freebsd.newnode',NULL,'','ping,ssh,ipod,isup',0,1,0,'PXEFBSD',NULL,NULL,1,150);
INSERT IGNORE INTO os_info VALUES ('OPSNODE-BSD','emulab-ops','OPSNODE-BSD','root',NULL,'FreeBSD on the Operations Node','FreeBSD','4.X','',NULL,'','ping,ssh,ipod,isup',0,1,0,'OPSNODEBSD',NULL,NULL,1,150);
INSERT IGNORE INTO os_info VALUES ('FW-IPFW','emulab-ops','FW-IPFW','root',NULL,'IPFW Firewall','FreeBSD','',NULL,'FreeBSD','','ping,ssh,ipod,isup,veths,mlinks',0,1,1,'NORMAL','emulab-ops-FBSD47-STD',NULL,0,150);
INSERT IGNORE INTO node_types VALUES ('pcvm','pcvm','PIII',0,0,0.00,0,'emulab-ops-FBSD-JAIL',0,60,'',0,0,0,'eth0',NULL,0,'','',NULL,NULL,'',1,0,1,1,0,0,0,0,0,0,0,1,NULL,'FREEBSD-MFS','FRISBEE-MFS');

INSERT IGNORE INTO os_boot_cmd VALUES ('FreeBSD','4.10','delay','/kernel.delay');
INSERT IGNORE INTO os_boot_cmd VALUES ('FreeBSD','4.10','vnodehost','/kernel.jail');
INSERT IGNORE INTO os_boot_cmd VALUES ('FreeBSD','4.10','linkdelay','/kernel.linkdelay');
INSERT IGNORE INTO os_boot_cmd VALUES ('FreeBSD','5.4','delay','/boot/kernel.delay/kernel');
INSERT IGNORE INTO os_boot_cmd VALUES ('FreeBSD','5.4','linkdelay','/boot/kernel.linkdelay/kernel');
INSERT IGNORE INTO os_boot_cmd VALUES ('Linux','9.0','linkdelay','linkdelay');
INSERT IGNORE INTO os_boot_cmd VALUES ('FreeBSD','6.0','delay','/boot/kernel.delay/kernel');
INSERT IGNORE INTO os_boot_cmd VALUES ('FreeBSD','6.0','linkdelay','/boot/kernel.linkdelay/kernel');

-- 
-- database-create-supplemental.sql - Various things that need to go into new
-- sites' databases, but don't really fit into database-fill.sql, which is
-- auto-generated. Also, unlike the contents of database-fill.sql, inserting
-- these is not idempotent, since a site may have changed them for some reason.
--

INSERT INTO os_info VALUES ('PXEBOOT','emulab-ops','PXEBOOT','root',NULL,'Default pxeboot kernel for contacting bootinfo.','OSKit','','boss:/tftpboot/pxeboot',NULL,'','',0,1,0,'_BOOTWHAT_',NULL,NULL);
INSERT INTO os_info VALUES ('PXEFBSD','emulab-ops','PXEFBSD','root',NULL,'MFS FreeBSD over PXE','FreeBSD','4.5','boss:/tftpboot/pxeboot.freebsd',NULL,'','ping,ssh,ipod,isup',0,1,0,'NORMAL',NULL,NULL);
INSERT INTO os_info VALUES ('PXEFRISBEE','emulab-ops','PXEFRISBEE','root',NULL,'Frisbee MFS FreeBSD over PXE','FreeBSD','4.5','boss:/tftpboot/pxeboot.frisbee',NULL,'','ping,ssh,ipod,isup',0,1,0,'RELOAD',NULL,NULL);

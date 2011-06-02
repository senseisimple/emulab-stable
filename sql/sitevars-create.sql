-- MySQL dump 10.10
--
-- Host: localhost    Database: tbdb
-- ------------------------------------------------------
-- Server version	5.0.20-log
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Dumping data for table `sitevariables`
--

INSERT INTO sitevariables VALUES ('general/testvar',NULL,'43','A test variable',0);
INSERT INTO sitevariables VALUES ('web/nologins',NULL,'0','Non-zero value indicates that no user may log into the Web Interface; non-admin users are auto logged out.',0);
INSERT INTO sitevariables VALUES ('web/message',NULL,'','Message to place in large lettering under the login message on the Web Interface.',0);
INSERT INTO sitevariables VALUES ('idle/threshold','2','4','Number of hours of inactivity for a node/expt to be considered idle.',0);
INSERT INTO sitevariables VALUES ('idle/mailinterval',NULL,'4','Number of hours since sending a swap request before sending another one. (Timing of first one is determined by idle/threshold.)',0);
INSERT INTO sitevariables VALUES ('idle/cc_grp_ldrs',NULL,'3','Start CC\'ing group and project leaders on idle messages on the Nth message.',0);
INSERT INTO sitevariables VALUES ('batch/retry_wait','90','900','Number of seconds to wait before retrying a failed batch experiment.',0);
INSERT INTO sitevariables VALUES ('swap/idleswap_warn',NULL,'30','Number of minutes before an Idle-Swap to send a warning message. Set to 0 for no warning.',0);
INSERT INTO sitevariables VALUES ('swap/autoswap_warn',NULL,'60','Number of minutes before an Auto-Swap to send a warning message. Set to 0 for no warning.',0);
INSERT INTO sitevariables VALUES ('plab/stale_age',NULL,'60','Age in minutes at which to consider site data stale and thus node down (0==always use data)',0);
INSERT INTO sitevariables VALUES ('idle/batch_threshold',NULL,'30','Number of minutes of inactivity for a batch node/expt to be considered idle.',0);
INSERT INTO sitevariables VALUES ('general/recently_active','7','14','Number of days to be considered a recently active user of the testbed.',0);
INSERT INTO sitevariables VALUES ('plab/load_metric','load_five','load_fifteen','GMOND load metric to use (load_one, load_five, load_fifteen)',0);
INSERT INTO sitevariables VALUES ('plab/max_load','10','5.0','Load at which to stop admitting jobs (0==admit nothing, 1000==admit all)',0);
INSERT INTO sitevariables VALUES ('plab/min_disk',NULL,'10.0','Minimum disk space free at which to stop admitting jobs (0==admit all, 100==admit none)',0);
INSERT INTO sitevariables VALUES ('watchdog/interval','30','60','Interval in minutes between checks for changes in timeout values (0==never check)',0);
INSERT INTO sitevariables VALUES ('watchdog/ntpdrift',NULL,'240','Interval in minutes between reporting back NTP drift changes (0==never report)',0);
INSERT INTO sitevariables VALUES ('watchdog/cvsup',NULL,'720','Interval in minutes between remote node checks for software updates (0==never check)',0);
INSERT INTO sitevariables VALUES ('watchdog/isalive/local',NULL,'3','Interval in minutes between local node status reports (0==never report)',0);
INSERT INTO sitevariables VALUES ('watchdog/isalive/vnode',NULL,'10','Interval in minutes between virtual node status reports (0==never report)',0);
INSERT INTO sitevariables VALUES ('watchdog/isalive/plab',NULL,'10','Interval in minutes between planetlab node status reports (0==never report)',0);
INSERT INTO sitevariables VALUES ('watchdog/isalive/wa',NULL,'1','Interval in minutes between widearea node status reports (0==never report)',0);
INSERT INTO sitevariables VALUES ('watchdog/isalive/dead_time','10','120','Time, in minutes, after which to consider a node dead if it has not checked in via tha watchdog',0);
INSERT INTO sitevariables VALUES ('watchdog/dhcpdconf',NULL,'5','Time in minutes between DHCPD configuration updates (0==never update)',0);
INSERT INTO sitevariables VALUES ('plab/setup/vnode_batch_size',NULL,'40','Number of plab nodes to setup simultaneously',0);
INSERT INTO sitevariables VALUES ('plab/setup/vnode_wait_time','300','960','Number of seconds to wait for a plab node to setup',0);
INSERT INTO sitevariables VALUES ('watchdog/rusage','30','300','Interval in _seconds_ between node resource usage reports (0==never report)',0);
INSERT INTO sitevariables VALUES ('watchdog/hostkeys',NULL,'999999','Interval in minutes between host key reports (0=never report, 999999=once only)',0);
INSERT INTO sitevariables VALUES ('plab/message',NULL,'','Message to display at the top of the plab_ez page',0);
INSERT INTO sitevariables VALUES ('node/ssh_pubkey','ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAIEA5pIVUkDhVdgGUcsUTQgmI/N4AhJba05gGn7/Ja46OorcKH12sbn9uH4XImdXRF16VVPMTytcOUAqsMsQ20cUcGyvXHnmmNANrLO2htCzNUdrbPkx5X63FNujjp7mLgdlnwzh/Zuoxw65DVXeVp3T5+9Ad25O4u9ybYsHFc8RmBM= root@boss.emulab.net','','Boss SSH public key to install on nodes',0);
INSERT INTO sitevariables VALUES ('web/banner',NULL,'','Message to place in large lettering at top of home page (typically a special message)',0);
INSERT INTO sitevariables VALUES ('general/autoswap_threshold',NULL,'16','Number of hours before an experiment is forcibly swapped',0);
INSERT INTO sitevariables VALUES ('general/autoswap_mode','1','0','Control whether autoswap defaults to on or off in the Begin Experiment page',0);
INSERT INTO sitevariables VALUES ('webcam/anyone_can_view','1','0','Turn webcam viewing on/off for mere users; default is off',0);
INSERT INTO sitevariables VALUES ('webcam/admins_can_view',NULL,'1','Turn webcam viewing on/off for admin users; default is on',0);
INSERT INTO sitevariables VALUES ('swap/use_admission_control',NULL,'1','Use admission control when swapping in experiments',0);
INSERT INTO sitevariables VALUES ('robotlab/override','open','','Turn the Robot Lab on/off (open/close). This is an override over other settings',0);
INSERT INTO sitevariables VALUES ('robotlab/exclusive','0','1','Only one experiment at a time; do not turn this off!',0);
INSERT INTO sitevariables VALUES ('robotlab/opentime','08:00','07:00','Time the Robot lab opens for use.',0);
INSERT INTO sitevariables VALUES ('robotlab/closetime',NULL,'18:00','Time the Robot lab closes down for the night.',0);
INSERT INTO sitevariables VALUES ('robotlab/open','1','0','Turn the Robot Lab on/off for weekends and holidays. Overrides the open/close times.',0);
INSERT INTO sitevariables VALUES ('swap/admission_control_debug',NULL,'0','Turn on/off admission control debugging (lots of output!)',0);
INSERT INTO sitevariables VALUES ('elabinelab/boss_pkg',NULL,'emulab-boss-1.8','Name of boss node install package (DEPRECATED)',0);
INSERT INTO sitevariables VALUES ('elabinelab/boss_pkg_dir',NULL,'/share/freebsd/packages/FreeBSD-4.10-20041102','Path from which to fetch boss packages (DEPRECATED)',0);
INSERT INTO sitevariables VALUES ('elabinelab/ops_pkg',NULL,'emulab-ops-1.4','Name of ops node install package (DEPRECATED)',0);
INSERT INTO sitevariables VALUES ('elabinelab/ops_pkg_dir',NULL,'/share/freebsd/packages/FreeBSD-4.10-20041102','Path from which to fetch ops packages (DEPRECATED)',0);
INSERT INTO sitevariables VALUES ('elabinelab/windows','1','0','Turn on Windows support in inner Emulab',0);
INSERT INTO sitevariables VALUES ('elabinelab/singlenet',NULL,'0','Default control net config. 0==use inner cnet, 1==use real cnet',1);
INSERT INTO sitevariables VALUES ('elabinelab/boss_osid',NULL,'','Default (emulab-ops) OSID to boot on boss node. Empty string means use node_type default OSID',1);
INSERT INTO sitevariables VALUES ('elabinelab/ops_osid',NULL,'','Default (emulab-ops) OSID to boot on ops node. Empty string means use node_type default OSID',1);
INSERT INTO sitevariables VALUES ('elabinelab/fs_osid',NULL,'','Default (emulab-ops) OSID to boot on fs node. Empty string means use node_type default OSID',1);
INSERT INTO sitevariables VALUES ('general/firstinit/state',NULL,'Ready','Indicates that a new emulab is not setup yet. Moves through several states.',0);
INSERT INTO sitevariables VALUES ('general/firstinit/pid',NULL,'testbed','The Project Name of the first project.',0);
INSERT INTO sitevariables VALUES ('general/version/minor','168','','Source code minor revision number',0);
INSERT INTO sitevariables VALUES ('general/version/build','12/22/2009','','Build version number',0);
INSERT INTO sitevariables VALUES ('general/version/major','4','','Source code major revision number',0);
INSERT INTO sitevariables VALUES ('general/mailman/password','MessyBoy','','Admin password for Emulab generated lists',0);
INSERT INTO sitevariables VALUES ('swap/swapout_command_failaction',NULL,'warn','What to do if swapout command fails (warn == continue, fail == fail swapout).',0);
INSERT INTO sitevariables VALUES ('general/open_showexplist',NULL,'','Allow members of this project to view all running experiments on the experiment list page',0);
INSERT INTO sitevariables VALUES ('general/linux_endnodeshaping',NULL,'1','Use this sitevar to disable endnodeshaping on linux globally on your testbed',0);
INSERT INTO sitevariables VALUES ('swap/swapout_command','/usr/local/bin/create-swapimage -s','','Command to run in admin MFS on each node of an experiment at swapout time. Runs as swapout user.',0);
INSERT INTO sitevariables VALUES ('swap/swapout_command_timeout','360','120','Time (in seconds) to allow for command completion',0);
INSERT INTO sitevariables VALUES ('node/gw_mac','00:b0:8e:84:69:34','','MAC address of the control net router',0);
INSERT INTO sitevariables VALUES ('general/default_imagename','FBSD410+RHL90-STD','','Name of the default image for new nodes, assumed to be in the emulab-ops project.',0);
INSERT INTO sitevariables VALUES ('general/joinproject/admincheck','1','0','When set, a project may not have a mix of admin and non-admin users',0);
INSERT INTO sitevariables VALUES ('protogeni/allow_externalusers','1','1','When set, external users may allocate slivers on your testbed.',0);
INSERT INTO sitevariables VALUES ('protogeni/max_externalnodes',NULL,'1024','When set, external users may allocate slivers on your testbed.',0);
INSERT INTO sitevariables VALUES ('protogeni/cm_uuid','28a10955-aa00-11dd-ad1f-001143e453fe','','The UUID of the local Component Manager.',0);
INSERT INTO sitevariables VALUES ('protogeni/max_sliver_lifetime','90','90','The maximum sliver lifetime. When set limits the lifetime of a sliver on your CM. Also see protogeni/max_slice_lifetime.',0);
INSERT INTO sitevariables VALUES ('protogeni/max_slice_lifetime','90','90','The maximum slice credential lifetime. When set limits the lifetime of a slice credential. Also see protogeni/max_sliver_lifetime.',0);
INSERT INTO sitevariables VALUES ('protogeni/max_components','-1','-1','Maximum number of components that can be allocated. -1 indicates any number of components can be allocated.',0);
INSERT INTO sitevariables VALUES ('general/minpoolsize','3','1','The Minimum size of the shared pool',0);
INSERT INTO sitevariables VALUES ('general/maxpoolsize','5','1','The maximum size of the shared pool',0);
INSERT INTO sitevariables VALUES ('protogeni/sa_uuid','2b437faa-aa00-11dd-ad1f-001143e453fe','','The UUID of the local Slice Authority.',0);
INSERT INTO sitevariables VALUES ('general/poolnodetype','pc3000','','The preferred node type of the shared pool',0);
INSERT INTO sitevariables VALUES ('general/default_country','US','','The default country of your site',0);
INSERT INTO sitevariables VALUES ('general/default_latitude','40.768652','','The default latitude of your site',0);
INSERT INTO sitevariables VALUES ('general/default_longitude','-111.84581','','The default longitude of your site',0);
INSERT INTO sitevariables VALUES ('oml/default_osid',NULL,'','Default OSID to use for OML server',1);
INSERT INTO sitevariables VALUES ('oml/default_server_startcmd',NULL,'','Default command line to use to start OML server',1);
INSERT INTO sitevariables VALUES ('images/create/maxwait',NULL,'72','Max time (minutes) to allow for saving an image',0);
INSERT INTO sitevariables VALUES ('images/create/idlewait',NULL,'8','Max time (minutes) to allow between periods of progress (image file getting larger) when saving an image (should be <= maxwait)',0);
INSERT INTO sitevariables VALUES ('images/create/maxsize',NULL,'6','Max size (GB) of a created image',0);
INSERT INTO sitevariables VALUES ('general/testbed_shutdown',NULL,'0','Non-zero value indicates that the testbed is shutdown and scripts should not do anything when they run. DO NOT SET THIS BY HAND!',0);
INSERT INTO sitevariables VALUES ('images/frisbee/maxrate_std',NULL,'72000000','Max bandwidth (Bytes/sec) at which to distribute standard images from the /usr/testbed/images directory.',0);
INSERT INTO sitevariables VALUES ('images/frisbee/maxrate_usr',NULL,'54000000','Max bandwidth (Bytes/sec) at which to distribute user-defined images from the /proj/.../images directory.',0);

/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;


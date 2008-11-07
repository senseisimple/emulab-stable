-- MySQL dump 10.10
--
-- Host: localhost    Database: tbdb
-- ------------------------------------------------------
-- Server version	5.0.20-log

--
-- Dumping data for table `sitevariables`
--


INSERT IGNORE INTO sitevariables VALUES ('general/testvar',NULL,'43','A test variable');
INSERT IGNORE INTO sitevariables VALUES ('web/nologins',NULL,'0','Non-zero value indicates that no user may log into the Web Interface; non-admin users are auto logged out.');
INSERT IGNORE INTO sitevariables VALUES ('web/message',NULL,'','Message to place in large lettering under the login message on the Web Interface.');
INSERT IGNORE INTO sitevariables VALUES ('idle/threshold',NULL,'4','Number of hours of inactivity for a node/expt to be considered idle.');
INSERT IGNORE INTO sitevariables VALUES ('idle/mailinterval',NULL,'4','Number of hours since sending a swap request before sending another one. (Timing of first one is determined by idle/threshold.)');
INSERT IGNORE INTO sitevariables VALUES ('idle/cc_grp_ldrs',NULL,'3','Start CC\'ing group and project leaders on idle messages on the Nth message.');
INSERT IGNORE INTO sitevariables VALUES ('batch/retry_wait',NULL,'900','Number of seconds to wait before retrying a failed batch experiment.');
INSERT IGNORE INTO sitevariables VALUES ('swap/idleswap_warn',NULL,'30','Number of minutes before an Idle-Swap to send a warning message. Set to 0 for no warning.');
INSERT IGNORE INTO sitevariables VALUES ('swap/autoswap_warn',NULL,'60','Number of minutes before an Auto-Swap to send a warning message. Set to 0 for no warning.');
INSERT IGNORE INTO sitevariables VALUES ('plab/stale_age',NULL,'60','Age in minutes at which to consider site data stale and thus node down (0==always use data)');
INSERT IGNORE INTO sitevariables VALUES ('idle/batch_threshold',NULL,'30','Number of minutes of inactivity for a batch node/expt to be considered idle.');
INSERT IGNORE INTO sitevariables VALUES ('general/recently_active',NULL,'14','Number of days to be considered a recently active user of the testbed.');
INSERT IGNORE INTO sitevariables VALUES ('plab/load_metric',NULL,'load_fifteen','GMOND load metric to use (load_one, load_five, load_fifteen)');
INSERT IGNORE INTO sitevariables VALUES ('plab/max_load','10','5.0','Load at which to stop admitting jobs (0==admit nothing, 1000==admit all)');
INSERT IGNORE INTO sitevariables VALUES ('plab/min_disk',NULL,'10.0','Minimum disk space free at which to stop admitting jobs (0==admit all, 100==admit none)');
INSERT IGNORE INTO sitevariables VALUES ('watchdog/interval','30','60','Interval in minutes between checks for changes in timeout values (0==never check)');
INSERT IGNORE INTO sitevariables VALUES ('watchdog/ntpdrift',NULL,'240','Interval in minutes between reporting back NTP drift changes (0==never report)');
INSERT IGNORE INTO sitevariables VALUES ('watchdog/cvsup',NULL,'720','Interval in minutes between remote node checks for software updates (0==never check)');
INSERT IGNORE INTO sitevariables VALUES ('watchdog/isalive/local',NULL,'3','Interval in minutes between local node status reports (0==never report)');
INSERT IGNORE INTO sitevariables VALUES ('watchdog/isalive/vnode',NULL,'10','Interval in minutes between virtual node status reports (0==never report)');
INSERT IGNORE INTO sitevariables VALUES ('watchdog/isalive/plab',NULL,'10','Interval in minutes between planetlab node status reports (0==never report)');
INSERT IGNORE INTO sitevariables VALUES ('watchdog/isalive/wa',NULL,'1','Interval in minutes between widearea node status reports (0==never report)');
INSERT IGNORE INTO sitevariables VALUES ('watchdog/isalive/dead_time',NULL,'120','Time, in minutes, after which to consider a node dead if it has not checked in via tha watchdog');
INSERT IGNORE INTO sitevariables VALUES ('plab/setup/vnode_batch_size',NULL,'40','Number of plab nodes to setup simultaneously');
INSERT IGNORE INTO sitevariables VALUES ('plab/setup/vnode_wait_time',NULL,'960','Number of seconds to wait for a plab node to setup');
INSERT IGNORE INTO sitevariables VALUES ('watchdog/rusage',NULL,'300','Interval in _seconds_ between node resource usage reports (0==never report)');
INSERT IGNORE INTO sitevariables VALUES ('watchdog/hostkeys',NULL,'999999','Interval in minutes between host key reports (0=never report, 999999=once only)');
INSERT IGNORE INTO sitevariables VALUES ('plab/message',NULL,'','Message to display at the top of the plab_ez page');
INSERT IGNORE INTO sitevariables VALUES ('node/ssh_pubkey','ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAIEA5pIVUkDhVdgGUcsUTQgmI/N4AhJba05gGn7/Ja46OorcKH12sbn9uH4XImdXRF16VVPMTytcOUAqsMsQ20cUcGyvXHnmmNANrLO2htCzNUdrbPkx5X63FNujjp7mLgdlnwzh/Zuoxw65DVXeVp3T5+9Ad25O4u9ybYsHFc8RmBM= root@boss.emulab.net','','Boss SSH public key to install on nodes');
INSERT IGNORE INTO sitevariables VALUES ('web/banner',NULL,'','Message to place in large lettering at top of home page (typically a special message)');
INSERT IGNORE INTO sitevariables VALUES ('general/autoswap_threshold',NULL,'16','Number of hours before an experiment is forcibly swapped');
INSERT IGNORE INTO sitevariables VALUES ('general/autoswap_mode','1','0','Control whether autoswap defaults to on or off in the Begin Experiment page');
INSERT IGNORE INTO sitevariables VALUES ('webcam/anyone_can_view','1','0','Turn webcam viewing on/off for mere users; default is off');
INSERT IGNORE INTO sitevariables VALUES ('webcam/admins_can_view',NULL,'1','Turn webcam viewing on/off for admin users; default is on');
INSERT IGNORE INTO sitevariables VALUES ('swap/use_admission_control',NULL,'1','Use admission control when swapping in experiments');
INSERT IGNORE INTO sitevariables VALUES ('robotlab/override','','','Turn the Robot Lab on/off (open/close). This is an override over other settings');
INSERT IGNORE INTO sitevariables VALUES ('robotlab/exclusive',NULL,'1','Only one experiment at a time; do not turn this off!');
INSERT IGNORE INTO sitevariables VALUES ('robotlab/opentime','08:00','07:00','Time the Robot lab opens for use.');
INSERT IGNORE INTO sitevariables VALUES ('robotlab/closetime',NULL,'18:00','Time the Robot lab closes down for the night.');
INSERT IGNORE INTO sitevariables VALUES ('robotlab/open','1','0','Turn the Robot Lab on/off for weekends and holidays. Overrides the open/close times.');
INSERT IGNORE INTO sitevariables VALUES ('swap/admission_control_debug',NULL,'0','Turn on/off admission control debugging (lots of output!)');
INSERT IGNORE INTO sitevariables VALUES ('elabinelab/boss_pkg','emulab-boss-1.8','emulab-boss-1.8','Name of boss node install package');
INSERT IGNORE INTO sitevariables VALUES ('elabinelab/boss_pkg_dir','/share/freebsd/packages/FreeBSD-4.10-20041102','/share/freebsd/packages/FreeBSD-4.10-20041102','Path from which to fetch boss packages');
INSERT IGNORE INTO sitevariables VALUES ('elabinelab/ops_pkg','emulab-ops-1.4','emulab-ops-1.4','Name of ops node install package');
INSERT IGNORE INTO sitevariables VALUES ('elabinelab/ops_pkg_dir','/share/freebsd/packages/FreeBSD-4.10-20041102','/share/freebsd/packages/FreeBSD-4.10-20041102','Path from which to fetch ops packages');
INSERT IGNORE INTO sitevariables VALUES ('elabinelab/windows','1','0','Turn on Windows support in inner Emulab');
INSERT IGNORE INTO sitevariables VALUES ('general/firstinit/state',NULL,'Ready','Indicates that a new emulab is not setup yet. Moves through several states.');
INSERT IGNORE INTO sitevariables VALUES ('general/firstinit/pid',NULL,'testbed','The Project Name of the first project.');
INSERT IGNORE INTO sitevariables VALUES ('general/version/minor','159','','Source code minor revision number');
INSERT IGNORE INTO sitevariables VALUES ('general/version/build','07/18/2008','','Build version number');
INSERT IGNORE INTO sitevariables VALUES ('general/version/major','4','','Source code major revision number');
INSERT IGNORE INTO sitevariables VALUES ('general/mailman/password','','','Admin password for Emulab generated lists');
INSERT IGNORE INTO sitevariables VALUES ('general/linux_endnodeshaping','0','1','Use this sitevar to disable endnodeshaping on linux globally on your testbed');
INSERT IGNORE INTO sitevariables VALUES ('general/open_showexplist',NULL,'','Allow members of this experiment to view all running experiments on the experiment list page');
INSERT IGNORE INTO sitevariables VALUES ('swap/swapout_command',NULL,'','Command to run in admin MFS on each node of an experiment at swapout time. Runs as swapout user.');
INSERT IGNORE INTO sitevariables VALUES ('swap/swapout_command_failaction',NULL,'warn','What to do if swapout command fails (warn == continue, fail == fail swapout).');
INSERT IGNORE INTO sitevariables VALUES ('swap/swapout_command_timeout',NULL,'120','Time (in seconds) to allow for command completion');
INSERT IGNORE INTO sitevariables VALUES ('node/gw_mac','00:b0:8e:84:69:34','','MAC address of the control net router');
INSERT IGNORE INTO sitevariables VALUES ('general/default_imagename','FBSD410+RHL90-STD','','Name of the default image for new nodes, assumed to be in the emulab-ops project.');
INSERT IGNORE INTO sitevariables VALUES ('general/joinproject/admincheck','0','0','When set, a project may not have a mix of admin and non-admin users');
INSERT IGNORE INTO sitevariables VALUES ('protogeni/allow_externalusers','1','1','When set, external users may allocate slivers on your testbed.');
INSERT IGNORE INTO sitevariables VALUES ('protogeni/max_externalnodes',NULL,'1024','When set, maximum number of nodes that external users may allocate.');


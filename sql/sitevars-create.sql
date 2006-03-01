-- MySQL dump 8.23
--
-- Host: localhost    Database: tbdb
---------------------------------------------------------
-- Server version	3.23.58-log

--
-- Dumping data for table `sitevariables`
--


INSERT INTO sitevariables VALUES ('general/testvar',NULL,'43','A test variable');
INSERT INTO sitevariables VALUES ('web/nologins',NULL,'0','Non-zero value indicates that no user may log into the Web Interface; non-admin users are auto logged out.');
INSERT INTO sitevariables VALUES ('web/message',NULL,'','Message to place in large lettering under the login message on the Web Interface.');
INSERT INTO sitevariables VALUES ('idle/threshold',NULL,'4','Number of hours of inactivity for a node/expt to be considered idle.');
INSERT INTO sitevariables VALUES ('idle/mailinterval',NULL,'4','Number of hours since sending a swap request before sending another one. (Timing of first one is determined by idle/threshold.)');
INSERT INTO sitevariables VALUES ('idle/cc_grp_ldrs',NULL,'3','Start CC\'ing group and project leaders on idle messages on the Nth message.');
INSERT INTO sitevariables VALUES ('batch/retry_wait',NULL,'900','Number of seconds to wait before retrying a failed batch experiment.');
INSERT INTO sitevariables VALUES ('swap/idleswap_warn',NULL,'30','Number of minutes before an Idle-Swap to send a warning message. Set to 0 for no warning.');
INSERT INTO sitevariables VALUES ('swap/autoswap_warn',NULL,'60','Number of minutes before an Auto-Swap to send a warning message. Set to 0 for no warning.');
INSERT INTO sitevariables VALUES ('plab/stale_age',NULL,'60','Age in minutes at which to consider site data stale and thus node down (0==always use data)');
INSERT INTO sitevariables VALUES ('idle/batch_threshold',NULL,'30','Number of minutes of inactivity for a batch node/expt to be considered idle.');
INSERT INTO sitevariables VALUES ('general/recently_active',NULL,'14','Number of days to be considered a recently active user of the testbed.');
INSERT INTO sitevariables VALUES ('plab/load_metric',NULL,'load_fifteen','GMOND load metric to use (load_one, load_five, load_fifteen)');
INSERT INTO sitevariables VALUES ('plab/max_load','10','5.0','Load at which to stop admitting jobs (0==admit nothing, 1000==admit all)');
INSERT INTO sitevariables VALUES ('plab/min_disk',NULL,'10.0','Minimum disk space free at which to stop admitting jobs (0==admit all, 100==admit none)');
INSERT INTO sitevariables VALUES ('watchdog/interval','30','60','Interval in minutes between checks for changes in timeout values (0==never check)');
INSERT INTO sitevariables VALUES ('watchdog/ntpdrift',NULL,'240','Interval in minutes between reporting back NTP drift changes (0==never report)');
INSERT INTO sitevariables VALUES ('watchdog/cvsup',NULL,'720','Interval in minutes between remote node checks for software updates (0==never check)');
INSERT INTO sitevariables VALUES ('watchdog/isalive/local',NULL,'3','Interval in minutes between local node status reports (0==never report)');
INSERT INTO sitevariables VALUES ('watchdog/isalive/vnode',NULL,'10','Interval in minutes between virtual node status reports (0==never report)');
INSERT INTO sitevariables VALUES ('watchdog/isalive/plab',NULL,'10','Interval in minutes between planetlab node status reports (0==never report)');
INSERT INTO sitevariables VALUES ('watchdog/isalive/wa',NULL,'1','Interval in minutes between widearea node status reports (0==never report)');
INSERT INTO sitevariables VALUES ('watchdog/isalive/dead_time',NULL,'120','Time, in minutes, after which to consider a node dead if it has not checked in via tha watchdog');
INSERT INTO sitevariables VALUES ('plab/setup/vnode_batch_size',NULL,'40','Number of plab nodes to setup simultaneously');
INSERT INTO sitevariables VALUES ('plab/setup/vnode_wait_time',NULL,'960','Number of seconds to wait for a plab node to setup');
INSERT INTO sitevariables VALUES ('watchdog/rusage','300','60','Interval in _seconds_ between node resource usage reports (0==never report)');
INSERT INTO sitevariables VALUES ('watchdog/hostkeys',NULL,'999999','Interval in minutes between host key reports (0=never report, 999999=once only)');
INSERT INTO sitevariables VALUES ('plab/message','Planetlab support is currently broken due to API incompatibilities introduced into PLC.','','Message to display at the top of the plab_ez page');
INSERT INTO sitevariables VALUES ('node/ssh_pubkey','1024 37 168728947415883137658395816497236019932357443574364998989351516015013006429180411438552594116282442938932702706360430451154958992295988097967662214818020771421328881173382895214540694120581207714991274873698590147743427181599852480329442016838781882554809552882295931111276319070960396053057987057937216750401 root@paper.cs.utah.edu','','Boss SSH public key to install on nodes');
INSERT INTO sitevariables VALUES ('web/banner',NULL,'','Message to place in large lettering at top of home page (typically a special message)');
INSERT INTO sitevariables VALUES ('general/autoswap_threshold',NULL,'16','Number of hours before an experiment is forcibly swapped');
INSERT INTO sitevariables VALUES ('general/autoswap_mode','1','0','Control whether autoswap defaults to on or off in the Begin Experiment page');
INSERT INTO sitevariables VALUES ('webcam/anyone_can_view','1','0','Turn webcam viewing on/off for mere users; default is off');
INSERT INTO sitevariables VALUES ('webcam/admins_can_view',NULL,'1','Turn webcam viewing on/off for admin users; default is on');
INSERT INTO sitevariables VALUES ('swap/use_admission_control',NULL,'1','Use admission control when swapping in experiments');
INSERT INTO sitevariables VALUES ('robotlab/override','','','Turn the Robot Lab on/off (open/close). This is an override over other settings');
INSERT INTO sitevariables VALUES ('robotlab/exclusive',NULL,'1','Only one experiment at a time; do not turn this off!');
INSERT INTO sitevariables VALUES ('robotlab/opentime','08:00','07:00','Time the Robot lab opens for use.');
INSERT INTO sitevariables VALUES ('robotlab/closetime',NULL,'18:00','Time the Robot lab closes down for the night.');
INSERT INTO sitevariables VALUES ('robotlab/open','1','0','Turn the Robot Lab on/off for weekends and holidays. Overrides the open/close times.');
INSERT INTO sitevariables VALUES ('swap/admission_control_debug',NULL,'0','Turn on/off admission control debugging (lots of output!)');
INSERT INTO sitevariables VALUES ('elabinelab/boss_pkg','emulab-boss-1.8','emulab-boss-1.8','Name of boss node install package');
INSERT INTO sitevariables VALUES ('elabinelab/boss_pkg_dir','/share/freebsd/packages/FreeBSD-4.10-20041102','/share/freebsd/packages/FreeBSD-4.10-20041102','Path from which to fetch boss packages');
INSERT INTO sitevariables VALUES ('elabinelab/ops_pkg','emulab-ops-1.4','emulab-ops-1.4','Name of ops node install package');
INSERT INTO sitevariables VALUES ('elabinelab/ops_pkg_dir','/share/freebsd/packages/FreeBSD-4.10-20041102','/share/freebsd/packages/FreeBSD-4.10-20041102','Path from which to fetch ops packages');
INSERT INTO sitevariables VALUES ('elabinelab/windows','1','0','Turn on Windows support in inner Emulab');
INSERT INTO sitevariables VALUES ('general/firstinit/state',NULL,'Ready','Indicates that a new emulab is not setup yet. Moves through several states.');
INSERT INTO sitevariables VALUES ('general/firstinit/pid',NULL,'testbed','The Project Name of the first project.');
INSERT INTO sitevariables VALUES ('general/version/minor','456','','Source code minor revision number');
INSERT INTO sitevariables VALUES ('general/version/build','06/17/2005','','Build version number');
INSERT INTO sitevariables VALUES ('general/version/major','1','','Source code major revision number');
INSERT INTO sitevariables VALUES ('general/mailman/password','','','Admin password for Emulab generated lists');
INSERT INTO sitevariables VALUES ('general/linux_endnodeshaping','1','1','Use this sitevar to disable endnodeshaping on linux globally on your testbed');
INSERT INTO sitevariables VALUES ('general/open_showexplist',NULL,'','Allow members of this experiment to view all running experiments on the experiment list page');
INSERT INTO sitevariables VALUES ('swap/swapout_command',NULL,'','Command to run in admin MFS on each node of an experiment at swapout time. Runs as swapout user.');
INSERT INTO sitevariables VALUES ('swap/swapout_command_failaction',NULL,'warn','What to do if swapout command fails (warn == continue, fail == fail swapout).');
INSERT INTO sitevariables VALUES ('swap/swapout_command_timeout',NULL,'120','Time (in seconds) to allow for command completion');
INSERT INTO sitevariables VALUES ('node/gw_mac','00:b0:8e:84:69:34','','MAC address of the control net router');

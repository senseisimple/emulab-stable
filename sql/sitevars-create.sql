-- MySQL dump 8.22
--
-- Host: localhost    Database: tbdb
---------------------------------------------------------
-- Server version	3.23.54-log

--
-- Dumping data for table 'sitevariables'
--


INSERT INTO sitevariables VALUES ('general/testvar',NULL,'43','A test variable');
INSERT INTO sitevariables VALUES ('web/nologins',NULL,'0','Non-zero value indicates that no non-admin user may log into the Web Interface.');
INSERT INTO sitevariables VALUES ('web/message',NULL,'','Message to place in large lettering under the login message on the Web Interface.');
INSERT INTO sitevariables VALUES ('idle/threshold',NULL,'4','Number of hours of inactivity for a node/expt to be considered idle.');
INSERT INTO sitevariables VALUES ('idle/mailinterval',NULL,'4','Number of hours since sending a swap request before sending another one. (Timing of first one is determined by idle/threshold.)');
INSERT INTO sitevariables VALUES ('idle/cc_grp_ldrs',NULL,'3','Start CC\'ing group and project leaders on idle messages on the Nth message.');


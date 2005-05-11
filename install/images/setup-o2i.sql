-- MySQL dump 8.23
--
-- Host: localhost    Database: tbdb
---------------------------------------------------------
-- Server version	3.23.58-log

--
-- Table structure for table `osidtoimageid`
--

CREATE TABLE osidtoimageid (
  osid varchar(35) NOT NULL default '',
  type varchar(30) NOT NULL default '',
  imageid varchar(45) NOT NULL default '',
  PRIMARY KEY  (osid,type)
) TYPE=MyISAM;

--
-- Dumping data for table `osidtoimageid`
--
-- WHERE:  osid like '%-STD'

--
-- Change PCTYPE to an actual node_type in your database (e.g., "pc2800").
-- You will need a set of these lines for every node_type that can run
-- BSD or Linux.
--
INSERT INTO osidtoimageid VALUES ('emulab-ops-FBSD410-STD','PCTYPE','emulab-ops-FBSD410-STD');
INSERT INTO osidtoimageid VALUES ('emulab-ops-RHL90-STD','PCTYPE','emulab-ops-RHL90-STD');

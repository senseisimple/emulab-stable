-- MySQL dump 8.23
--
-- Host: localhost    Database: tbdb
---------------------------------------------------------
-- Server version	3.23.58-log

--
-- Dumping data for table `default_firewall_rules`
--


INSERT INTO default_firewall_rules VALUES ('ipfw','open',1,65534,'allow ip from any to any');
INSERT INTO default_firewall_rules VALUES ('ipfw','closed',1,55000,'allow ip from me to me');
INSERT INTO default_firewall_rules VALUES ('ipfw','closed',1,55100,'allow ip from me to 155.98.32.0/23');
INSERT INTO default_firewall_rules VALUES ('ipfw','closed',1,55101,'allow ip from 155.98.32.0/23 to me');
INSERT INTO default_firewall_rules VALUES ('ipfw','closed',1,65534,'deny ip from any to any');
INSERT INTO default_firewall_rules VALUES ('ipfw','basic',1,55000,'allow ip from me to me');
INSERT INTO default_firewall_rules VALUES ('ipfw','basic',1,55100,'allow ip from me to 155.98.32.0/23');
INSERT INTO default_firewall_rules VALUES ('ipfw','basic',1,55101,'allow ip from 155.98.32.0/23 to me');
INSERT INTO default_firewall_rules VALUES ('ipfw','basic',1,55200,'allow icmp from any to 155.98.36.0/22');
INSERT INTO default_firewall_rules VALUES ('ipfw','basic',1,55201,'allow icmp from 155.98.36.0/22 to any');
INSERT INTO default_firewall_rules VALUES ('ipfw','basic',1,55300,'allow tcp from any to 155.98.36.0/22 22');
INSERT INTO default_firewall_rules VALUES ('ipfw','basic',1,55301,'allow tcp from 155.98.36.0/22 22 to any');
INSERT INTO default_firewall_rules VALUES ('ipfw','basic',1,55400,'allow tcp from me 16534 to 155.98.36.0/22');
INSERT INTO default_firewall_rules VALUES ('ipfw','basic',1,55401,'allow tcp from 155.98.36.0/22 to me 16534');
INSERT INTO default_firewall_rules VALUES ('ipfw','basic',1,55500,'allow udp from 224.4.0.0/16 2917 to 224.4.0.0/16 2917');
INSERT INTO default_firewall_rules VALUES ('ipfw','basic',1,65534,'deny ip from any to any');


DELETE FROM default_firewall_rules;
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
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','open',1,65534,'allow all from any to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55501,'allow icmp from any to boss icmptypes 0');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55500,'allow icmp from boss to any icmptypes 6,8');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55402,'allow igmp from any to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55401,'allow udp from boss 3564-3820 to any 3564-3820');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55400,'allow udp from any to 234.5.6.0/24');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55340,'allow ip from any to boss 7777 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55330,'allow udp from any to boss 6969 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55322,'allow udp from any to fs 900 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55321,'allow udp from any not 0-700 to fs keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55320,'allow ip from any to fs 111 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55312,'allow udp from any not 0-1023 to 155.98.32.0/23 not 0-1023');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55311,'allow udp from any to 155.98.32.0/23 69');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55310,'allow udp from 155.98.32.0/23 not 0-1023 to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55300,'allow udp from any 67 to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55301,'allow udp from any to any 67');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55260,'allow ip from any to ops 2917 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55240,'allow udp from fs 2049 to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55241,'allow udp from any to fs 2049');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55250,'allow ip from any to boss 5999 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55230,'allow ip from any to ops 514 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55220,'allow ip from any to boss 123 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55210,'allow udp from any to boss 53 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55200,'allow tcp from boss to any 22 setup');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55130,'allow all from any to any frag');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55100,'allow mac-type arp');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55110,'check-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55120,'allow tcp from any to any established');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55001,'deny all from any to me via vlan0');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,55000,'allow all from me to me');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55321,'allow udp from any not 0-700 to fs keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55320,'allow ip from any to fs 111 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55312,'allow udp from any not 0-1023 to 155.98.32.0/23 not 0-1023');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55311,'allow udp from any to 155.98.32.0/23 69');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55310,'allow udp from 155.98.32.0/23 not 0-1023 to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55301,'allow udp from any to any 67');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55300,'allow udp from any 67 to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55260,'allow ip from any to ops 2917 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55250,'allow ip from any to boss 5999 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55241,'allow udp from any to fs 2049');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55240,'allow udp from fs 2049 to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55230,'allow ip from any to ops 514 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55220,'allow ip from any to boss 123 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55210,'allow udp from any to boss 53 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55130,'allow all from any to any frag');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55120,'allow tcp from any to any established');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55110,'check-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55100,'allow mac-type arp');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','closed',1,65534,'deny all from any to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55001,'deny all from any to me via vlan0');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55000,'allow all from me to me');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55322,'allow udp from any to fs 900 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55330,'allow udp from any to boss 6969 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55340,'allow ip from any to boss 7777 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55400,'allow udp from any to 234.5.6.0/24');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55401,'allow udp from boss 3564-3820 to any 3564-3820');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55402,'allow igmp from any to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55200,'allow tcp from any to any 22 setup');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,55500,'allow icmp from any to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','basic',1,65534,'deny all from any to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55000,'allow all from me to me');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55001,'deny all from any to me via vlan0');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55100,'allow mac-type arp');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55110,'check-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55120,'allow tcp from any to any established');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55130,'allow all from any to any frag');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55201,'allow tcp from any to any 80,443 setup keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55210,'allow udp from any to boss 53 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55220,'allow ip from any to boss 123 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55230,'allow ip from any to ops 514 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55240,'allow udp from fs 2049 to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55241,'allow udp from any to fs 2049');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55250,'allow ip from any to boss 5999 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55260,'allow ip from any to ops 2917 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55300,'allow udp from any 67 to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55301,'allow udp from any to any 67');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55310,'allow udp from 155.98.32.0/23 not 0-1023 to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55311,'allow udp from any to 155.98.32.0/23 69');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55312,'allow udp from any not 0-1023 to 155.98.32.0/23 not 0-1023');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55320,'allow ip from any to fs 111 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55321,'allow udp from any not 0-700 to fs keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55322,'allow udp from any to fs 900 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55330,'allow udp from any to boss 6969 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55340,'allow ip from any to boss 7777 keep-state');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55400,'allow udp from any to 234.5.6.0/24');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55401,'allow udp from boss 3564-3820 to any 3564-3820');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55402,'allow igmp from any to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55200,'allow tcp from any to any 22 setup');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55500,'allow icmp from any to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,65534,'deny all from any to any');
INSERT INTO default_firewall_rules VALUES ('ipfw2-vlan','emulab',1,55202,'allow tcp from any to any 3069 setup keep-state');


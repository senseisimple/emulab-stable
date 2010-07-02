drop table if exists emulab_pubs;
create table `emulab_pubs` (
  `idx`  int unsigned not null primary key auto_increment,
  `uuid` varchar(40) not null unique,
  `created` datetime not null,
  `owner` mediumint(8) unsigned not null,
  `submitted_by` mediumint(8) unsigned not null,
  `last_edit` datetime not null,
  `last_edit_by` mediumint(8) unsigned not null,
  `type` tinytext not null,
  `authors` tinytext not null,
  `affil` tinytext not null,
  `title` tinytext  not null,
  `conf` tinytext  not null,
  `conf_url` tinytext  not null,
  `where` tinytext  not null,
  `year` tinytext  not null,
  `month` float(3,1) not null,
  `volume` tinytext not null,
  `number` tinytext  not null,
  `pages` tinytext  not null,
  `url` tinytext  not null,
  `evaluated_on_emulab` tinytext  not null,
  `category` tinytext  not null,
  `project` tinytext  not null,
  `cite_osdi02` tinyint(1),
  `no_cite_why` tinytext not null,
  `notes` text not null,
  `visible` tinyint(1) default 1 not null,
  `deleted` tinyint(1) default 0 not null,
  `editable_owner` tinyint(1) default 1 not null,
  `editable_proj` tinyint(1) default 1 not null
);

drop table if exists emulab_pubs_month_map;
create table `emulab_pubs_month_map` (
  `display_order` int unsigned not null unique auto_increment,
  `month` float(3,1) not null primary key,
  `month_name` char(8) not null
);

insert into emulab_pubs_month_map (`month`, `month_name`) values
  (0, ''),
  (1, 'Jan'), 
  (2, 'Feb'), 
  (3, 'Mar'), 
  (4, 'Apr'), 
  (5, 'May'),
  (6, 'Jun'), 
  (7, 'Jul'), 
  (8, 'Aug'), 
  (9, 'Sep'), 
  (10, 'Oct'), 
  (11, 'Nov'), 
  (12, 'Dec'),
  (1.5, 'Jan-Feb'), 
  (2.5, 'Feb-Mar'), 
  (3.5, 'Mar-Apr'), 
  (4.5, 'Apr-May'), 
  (5.5, 'May-Jun'), 
  (6.5, 'Jun-Jul'), 
  (7.5, 'Jul-Aug'), 
  (8.5, 'Aug-Sep'), 
  (9.5, 'Sep-Oct'), 
  (10.5, 'Oct-Nov'), 
  (11.5, 'Nov-Dec'), 
  (12.5, 'Dec-Jan');


REPLACE INTO table_regex VALUES ('default','tinytext_utf8','text','regex','^(?:[\\x20-\\x7E]|[\\xC2-\\xDF][\\x80-\\xBF]|\\xE0[\\xA0-\\xBF][\\x80-\\xBF]|[\\xE1-\\xEC\\xEE\\xEF][\\x80-\\xBF]{2}|\\xED[\\x80-\\x9F][\\x80-\\xBF])*$',0,256,'adopted from http://www.w3.org/International/questions/qa-forms-utf-8.en.php');
REPLACE INTO table_regex VALUES ('default','text_utf8','text','regex','^(?:[\\x20-\\x7E]|[\\xC2-\\xDF][\\x80-\\xBF]|\\xE0[\\xA0-\\xBF][\\x80-\\xBF]|[\\xE1-\\xEC\\xEE\\xEF][\\x80-\\xBF]{2}|\\xED[\\x80-\\x9F][\\x80-\\xBF])*$',0,65535,'adopted from http://www.w3.org/International/questions/qa-forms-utf-8.en.php');
REPLACE INTO table_regex VALUES ('default','fulltext_utf8','text','regex','^(?:[\\x09\\x0A\\x0D\\x20-\\x7E]|[\\xC2-\\xDF][\\x80-\\xBF]|\\xE0[\\xA0-\\xBF][\\x80-\\xBF]|[\\xE1-\\xEC\\xEE\\xEF][\\x80-\\xBF]{2}|\\xED[\\x80-\\x9F][\\x80-\\xBF])*$',0,65535,'adopted from http://www.w3.org/International/questions/qa-forms-utf-8.en.php');


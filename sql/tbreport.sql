create table report_error (
	seq        integer unsigned not null primary key,
	stamp      integer unsigned not null,
	session    integer unsigned not null,
	attempt    tinyint(1) not null,
	severity   smallint(3) not null,
	script     smallint(3) not null,
	error_type varchar(255) not null,
	index      (session)
);

create table report_context (
	seq integer unsigned not null primary key,
	i0  integer,
	i1  integer,
	i2  integer,
	vc0 varchar(255) default null,
	vc1 varchar(255) default null,
	vc2 varchar(255) default null
);

create table report_assign_violation (
	seq         integer unsigned not null primary key,
	unassigned  integer,
	pnode_load  integer,
	no_connect  integer,
	link_users  integer,
	bandwidth   integer,
	desires     integer,
	vclass      integer,
	delay       integer,
	trivial_mix integer,
	subnodes    integer,
	max_types   integer,
	endpoints   integer
);

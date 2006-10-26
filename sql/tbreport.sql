create table report_error (
        seq        int(10) unsigned not null primary key,
        stamp      int(10) unsigned not null,
        session    int(10) unsigned not null,
        attempt    tinyint(1) not null,
        severity   smallint(3) not null,
        script     smallint(3) not null,
        error_type varchar(255) not null,
        index      (session)
);

create table report_context (
        seq int(10) unsigned not null primary key,
        i0  int(11),
        i1  int(11),
        i2  int(11),
        vc0 varchar(255) default null,
        vc1 varchar(255) default null,
        vc2 varchar(255) default null
);

create table report_assign_violation (
        seq         int(10) unsigned not null primary key,
        unassigned  int(11),
        pnode_load  int(11),
        no_connect  int(11),
        link_users  int(11),
        bandwidth   int(11),
        desires     int(11),
        vclass      int(11),
        delay       int(11),
        trivial_mix int(11),
        subnodes    int(11),
        max_types   int(11),
        endpoints   int(11)
);

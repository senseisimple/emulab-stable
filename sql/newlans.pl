#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2008 University of Utah and the Flux Group.
# All rights reserved.
#
use English;

use lib "/usr/testbed/lib";
use libdb;
use libtestbed;
use Lan;

#
# Untaint the path
# 
$ENV{'PATH'} = '/bin:/usr/bin:/usr/sbin';
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

exit(Lan->Initialize());


#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
use SemiModern::Perl;
use TBConfig;
use Data::Dumper;
use Test::More tests => 2;

#./tbts -p DPRJ -g DGRP t/tbts_cmdlineargs.t

ok($TBConfig::DEFAULT_PID eq "DPRJ", './tbts -p DPRJ');
ok($TBConfig::DEFAULT_GID eq "DGRP", './tbts -g DGRP');

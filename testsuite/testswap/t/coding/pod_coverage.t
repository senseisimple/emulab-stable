#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
use Test::More;
eval "use Test::Pod::Coverage 1.00";
if ( $@ ) {
  plan skip_all => "Test::Pod::Coverage 1.00 required for testing POD coverage";
}
all_pod_coverage_ok();

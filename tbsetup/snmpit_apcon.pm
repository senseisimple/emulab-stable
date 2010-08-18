#!/usr/bin/perl -W

#
# EMULAB-LGPL
# Copyright (c) 2000-2010 University of Utah and the Flux Group.
# Copyright (c) 2004-2009 Regents, University of California.
# All rights reserved.
#

#
# snmpit module for Apcon 2000 series layer 1 switch
#

package snmpit_apcon;
use strict;

$| = 1; # Turn off line buffering on output

use English;
use SNMP;
use snmpit_lib;

use libtestbed;


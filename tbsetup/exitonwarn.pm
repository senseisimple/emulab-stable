#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
$warnings = 0;

$SIG{__WARN__} = sub { print STDERR $_[0];$warnings++; };

END {
    if ($warnings > 0) {
	print STDERR "$warnings warnings.\n";
        # This actually causes perl to complain and exit with 255
	exit(1);		
    }
}

1;



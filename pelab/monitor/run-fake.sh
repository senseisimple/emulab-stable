#
# EMULAB-COPYRIGHT
# Copyright (c) 2007 University of Utah and the Flux Group.
# All rights reserved.
#

# Run in directory of monitor.
# Usage: run-fake.sh <path-to-logs>
#
# Example:
#
# run-fake.sh /proj/tbres/exp/pelab-generated/logs/elab-1

python monitor.py --mapping=$1/local/logs/ip-mapping.txt --experiment=foo/bar \
	--ip=127.0.0.1 --initial=$1/local/logs/initial-conditions.txt --fake \
	< $1/local/logs/libnetmon.out

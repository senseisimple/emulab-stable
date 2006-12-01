#! /bin/sh

#
# EMULAB-COPYRIGHT
# Copyright (c) 2004-2006 University of Utah and the Flux Group.
# All rights reserved.
#

## Variables

# The full path of the test case
test_file=$1
# The base name of the test case
test_file_base="test_simple_path.sh"
# The current test number for shell based tests.
test_num=0

## Helper functions

run_test() {
    echo "run_test: $*"
    $* > ${test_file_base}_${test_num}.tmp 2>&1
}

check_output() {
    diff -u - ${test_file_base}_${test_num}.tmp
    if test $? -ne 0; then
	echo $1
	exit 1
    fi
    test_num=`expr ${test_num} \+ 1`
}

##

run_test ./simple_path -x 3.5 -y 4.9 -u 6.5 -v 4.9

check_output "garcia1 didn't go over the top?" <<EOF
set waypoint(garcia1) 3.75 3.75
set waypoint(garcia1) 6.25 3.75
set waypoint(garcia1) NONE
-reverse-
set waypoint(garcia1) 6.25 3.75
set waypoint(garcia1) 3.75 3.75
set waypoint(garcia1) NONE
EOF

run_test ./simple_path -x 3.5 -y 5.1 -u 6.5 -v 5.1

check_output "garcia1 didn't go under the bottom?" <<EOF
set waypoint(garcia1) 3.75 6.25
set waypoint(garcia1) 6.25 6.25
set waypoint(garcia1) NONE
-reverse-
set waypoint(garcia1) 6.25 6.25
set waypoint(garcia1) 3.75 6.25
set waypoint(garcia1) NONE
EOF

run_test ./simple_path -x 4.9 -y 3.5 -u 4.9 -v 6.5

check_output "garcia1 didn't go around the left side?" <<EOF
set waypoint(garcia1) 3.75 3.75
set waypoint(garcia1) 3.75 6.25
set waypoint(garcia1) NONE
-reverse-
set waypoint(garcia1) 3.75 6.25
set waypoint(garcia1) 3.75 3.75
set waypoint(garcia1) NONE
EOF

run_test ./simple_path -x 5.1 -y 3.5 -u 5.1 -v 6.5

check_output "garcia1 didn't go around the right side?" <<EOF
set waypoint(garcia1) 6.25 3.75
set waypoint(garcia1) 6.25 6.25
set waypoint(garcia1) NONE
-reverse-
set waypoint(garcia1) 6.25 6.25
set waypoint(garcia1) 6.25 3.75
set waypoint(garcia1) NONE
EOF

run_test ./simple_path -x 3.5 -y 5.75 -u 5.0 -v 3.5

check_output "garcia1 didn't go to the top left?" <<EOF
set waypoint(garcia1) 3.75 3.75
set waypoint(garcia1) NONE
-reverse-
set waypoint(garcia1) 3.75 3.75
set waypoint(garcia1) NONE
EOF

run_test ./simple_path -x 3.5 -y 4.25 -u 5.0 -v 6.5

check_output "garcia1 didn't go to the bottom left?" <<EOF
set waypoint(garcia1) 3.75 6.25
set waypoint(garcia1) NONE
-reverse-
set waypoint(garcia1) 3.75 6.25
set waypoint(garcia1) NONE
EOF

run_test ./simple_path -x 4.25 -y 6.5 -u 6.5 -v 5.0

check_output "garcia1 didn't go to the bottom right?" <<EOF
set waypoint(garcia1) 6.25 6.25
set waypoint(garcia1) NONE
-reverse-
set waypoint(garcia1) 6.25 6.25
set waypoint(garcia1) NONE
EOF

run_test ./simple_path -x 4.25 -y 3.5 -u 6.5 -v 5.0

check_output "garcia1 didn't go to the top right?" <<EOF
set waypoint(garcia1) 6.25 3.75
set waypoint(garcia1) NONE
-reverse-
set waypoint(garcia1) 6.25 3.75
set waypoint(garcia1) NONE
EOF

run_test ./simple_path -x 3.8 -y 3.8 -u 9.0 -v 9.0

check_output "garcia1 didn't get out of obstacle?" <<EOF
set waypoint(garcia1) 6.25 3.75
set waypoint(garcia1) NONE
-reverse-
set waypoint(garcia1) GOAL_IN_OBSTACLE
EOF

#! /bin/sh

#
# EMULAB-COPYRIGHT
# Copyright (c) 2004 University of Utah and the Flux Group.
# All rights reserved.
#

## Variables

# The full path of the test case
test_file=$1
# The base name of the test case
test_file_base="syncd_test.sh"
# The current test number for shell based tests.
test_num=0

PORT_NUMBER=4545
SYNCD=./emulab-syncd
SYNC=./emulab-sync

$SYNCD -h > syncd-usage.out 2>&1
$SYNC -h > sync-usage.out 2>&1


## Helper functions

run_test() {
    echo "run_test: $*"
    $* > ${test_file_base}_${test_num}.tmp 2>&1
}

check_output() {
    cmp ${test_file_base}_${test_num}.tmp -
    if test $? -ne 0; then
	echo $1
	exit 1
    fi
    test_num=`expr ${test_num} \+ 1`
}


## Argument checking

run_test $SYNCD -p foo

check_output "syncd accepts port 'foo'?" <<EOF
Error: -p value is not a number: foo
`cat syncd-usage.out`

EOF

##    syncd

run_test $SYNCD -p -1

check_output "syncd accepts port -1?" <<EOF
Error: -p value is not between 0 and 65536: -1
`cat syncd-usage.out`

EOF

##

run_test $SYNCD -p 65536

check_output "syncd accepts port 65536?" <<EOF
Error: -p value is not between 0 and 65536: 65536
`cat syncd-usage.out`

EOF

##    sync

run_test $SYNC -p foo

check_output "sync accepts port 'foo'?" <<EOF
Error: -p value is not a number: foo
`cat sync-usage.out`

EOF

##

run_test $SYNC -p -1

check_output "sync accepts port -1?" <<EOF
Error: -p value is not between 0 and 65536: -1
`cat sync-usage.out`

EOF

##

run_test $SYNC -p 65536

check_output "sync accepts port 65536?" <<EOF
Error: -p value is not between 0 and 65536: 65536
`cat sync-usage.out`

EOF

##

run_test $SYNC -i foo

check_output "sync accepts string for -i?" <<EOF
Error: -i value is not a number: foo
`cat sync-usage.out`

EOF

##

run_test $SYNC -i -1

check_output "sync accepts negative -i value?" <<EOF
Error: -i value must be greater than zero: -1
`cat sync-usage.out`

EOF

##

run_test $SYNC -i 0

check_output "sync accepts zero -i value?" <<EOF
Error: -i value must be greater than zero: 0
`cat sync-usage.out`

EOF

##

run_test $SYNC -n 01234567890123456789012345678901234567890123456789012345678901234

check_output "sync accepts long barrier name?" <<EOF
Error: -n value is too long: 01234567890123456789012345678901234567890123456789012345678901234
`cat sync-usage.out`

EOF

##

run_test $SYNC -s nonexistentname

check_output "sync accepts bogus server name?" <<EOF
gethostbyname(nonexistentname) failed
EOF

##

run_test $SYNC -e -1

check_output "sync accepts negative error code?" <<EOF
Error: -e value must be zero or less than 240
`cat sync-usage.out`

EOF

##

run_test $SYNC -e 240

check_output "sync accepts negative error code?" <<EOF
Error: -e value must be zero or less than 240
`cat sync-usage.out`

EOF


## Real tests

$SYNCD -d -p $PORT_NUMBER > syncd.log 2>&1 &
SYNCD_PID="$!"

trap 'kill $SYNCD_PID' EXIT

sleep 1

SYNC="$SYNC -s localhost -p $PORT_NUMBER"

##

echo "Checking master and then slave..."

$SYNC -i 1 &
SYNC_1_PID="$!"

sleep 1

$SYNC &
SYNC_2_PID="$!"

if wait $SYNC_1_PID && wait $SYNC_2_PID; then
    :
else
    echo "Synchronization failed (1)..."
    exit 1
fi

##

echo "Checking slave and then master..."

$SYNC &
SYNC_1_PID="$!"

sleep 1

$SYNC -i 1 &
SYNC_2_PID="$!"

if wait $SYNC_1_PID && wait $SYNC_2_PID; then
    :
else
    echo "Synchronization failed (2)..."
    exit 1
fi

##

echo "Checking slave, master, and then slave..."

$SYNC &
SYNC_1_PID="$!"

sleep 1

$SYNC -i 2 &
SYNC_2_PID="$!"

sleep 1

$SYNC &
SYNC_3_PID="$!"

if wait $SYNC_1_PID && wait $SYNC_2_PID && wait $SYNC_3_PID; then
    :
else
    echo "Synchronization failed (3)..."
    exit 1
fi

##

echo "Checking slave, async master, and then slave..."

$SYNC &
SYNC_1_PID="$!"

sleep 1

$SYNC -a -i 3
$SYNC &
SYNC_2_PID="$!"

sleep 1

$SYNC &
SYNC_3_PID="$!"

if wait $SYNC_1_PID && wait $SYNC_2_PID && wait $SYNC_3_PID; then
    :
else
    echo "Synchronization failed (3)..."
    exit 1
fi

##

echo "Check HUP'ing the server (1)..."

$SYNC &
SYNC_1_PID="$!"

sleep 1

kill -HUP $SYNCD_PID

if wait $SYNC_1_PID; then
    echo "SIGHUP'ing the server failed (1)..."
    exit 1
elif test "$?" != 240; then
    echo "Wrong exit value (1)..."
    exit 1
fi

echo "Check synchronization after HUP (1)..."

$SYNC -i 1 &
SYNC_1_PID="$!"
$SYNC &
SYNC_2_PID="$!"

if wait $SYNC_1_PID && wait $SYNC_2_PID; then
    :
else
    echo "Synchronization failed after HUP (1)..."
    exit 1
fi

##

echo "Check HUP'ing the server (2)..."

$SYNC -i 2 &
SYNC_1_PID="$!"

sleep 1

kill -HUP $SYNCD_PID

if wait $SYNC_1_PID; then
    echo "SIGHUP'ing the server failed (2)..."
    exit 1
elif test "$?" != 240; then
    echo "Wrong exit value (2)..."
    exit 1
fi

echo "Check synchronization after HUP (1)..."

$SYNC -i 1 &
SYNC_1_PID="$!"
$SYNC &
SYNC_2_PID="$!"

if wait $SYNC_1_PID && wait $SYNC_2_PID; then
    :
else
    echo "Synchronization failed after HUP (1)..."
    exit 1
fi

##

echo "Checking non-default barrier..."

$SYNC -i 1 -n mybarrier &
SYNC_1_PID="$!"

$SYNC -n mybarrier &
SYNC_2_PID="$!"

if wait $SYNC_1_PID && wait $SYNC_2_PID; then
    :
else
    echo "Synchronization on mybarrier failed (1)..."
    exit 1
fi

##

echo "Checking error codes (1)..."

$SYNC -i 1 -e 10 &
SYNC_1_PID="$!"

$SYNC -e 20 &
SYNC_2_PID="$!"

wait $SYNC_1_PID
SYNC_1_RC="$?"
wait $SYNC_2_PID
SYNC_2_RC="$?"

if test "$SYNC_1_RC" = 20 && test "$SYNC_2_RC" = 20; then
    :
else
    echo "Error return failed (1)..."
    exit 1
fi

##

echo "Checking error codes (2)..."

$SYNC -i 1 &
SYNC_1_PID="$!"

$SYNC -e 30 &
SYNC_2_PID="$!"

wait $SYNC_1_PID
SYNC_1_RC="$?"
wait $SYNC_2_PID
SYNC_2_RC="$?"

if test "$SYNC_1_RC" = 30 && test "$SYNC_2_RC" = 30; then
    :
else
    echo "Error return failed (2)..."
    exit 1
fi

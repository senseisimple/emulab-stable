#! /bin/sh

#
# EMULAB-COPYRIGHT
# Copyright (c) 2005 University of Utah and the Flux Group.
# All rights reserved.
#

## Variables

# The full path of the test case
test_file=$1
# The base name of the test case
test_file_base="test_vmcd4.sh"
# The current test number for shell based tests.
test_num=0

# SRCDIR=@srcdir@
EMC_PORT=6565
VMC1_PORT=6566

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

# Start the daemons vmcd depends on:

../emc/emcd -l `pwd`/test_emcd4.log \
    -i `pwd`/test_emcd.pid \
    -p ${EMC_PORT} \
    -s ops \
    -c `realpath ${SRCDIR}/test_emcd2.config`

vmc-client -l `pwd`/test_vmc-client1.log \
    -i `pwd`/test_vmc-client1.pid \
    -p ${VMC1_PORT} \
    -f ${SRCDIR}/test_vmcd4.pos \
    foobar

# Start vmcd:

vmcd -l `pwd`/test_vmcd4.log \
    -i `pwd`/test_vmcd.pid \
    -e localhost \
    -p ${EMC_PORT} \
    -c localhost -P ${VMC1_PORT}

cleanup() {
    kill `cat test_vmcd.pid`
    kill `cat test_emcd.pid`
    kill `cat test_vmc-client1.pid`
}

trap 'cleanup' EXIT

sleep 2

newframe() {
    kill -s USR1 `cat test_vmc-client1.pid`
}

newframe

sleep 1

lpc=0
while test $lpc -lt 14; do
    newframe
    sleep 0.1
    lpc=`expr $lpc + 1`
done

run_test ../mtp/mtp_send -n localhost -P ${EMC_PORT} \
    -r emulab -i 1 -c 0 -m "empty" init -- \
    -w -i 1 request-position

check_output "no update?" <<EOF
Packet: version 2; role emc
 opcode:	update-position
  id:		1
  x:		6.000000
  y:		5.280000
  theta:	-1.190796
  status:	-1
  timestamp:	14.000000
EOF

run_test ../mtp/mtp_send -n localhost -P ${EMC_PORT} \
    -r emulab -i 1 -c 0 -m "empty" init -- \
    -w -i 2 request-position

check_output "no update?" <<EOF
Packet: version 2; role emc
 opcode:	update-position
  id:		2
  x:		20.000000
  y:		20.260000
  theta:	-1.570796
  status:	-1
  timestamp:	14.000000
EOF

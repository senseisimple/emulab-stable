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
test_file_base="test_mtp.sh"

PORT=6050

## Helper functions

check_output() {
    diff -u - ${test_file_base}.tmp
    if test $? -ne 0; then
	echo $1
	exit 1
    fi
}

##

./mtp_recv ${PORT} > ${test_file_base}.tmp 2>&1 &
MTP_RECV_PID=$!

trap 'kill $MTP_RECV_PID' EXIT

sleep 2

../mtp/mtp_send -n localhost -P ${PORT} \
    -r rmc -i 0 -c 0 -m "empty" init -- \
    -r rmc -i 1 request-position

kill $MTP_RECV_PID
trap '' EXIT

check_output "?" <<EOF
Listening for mtp packets on port: 6050
Packet: version 2; role rmc
 opcode:	init
  id:		0
  code:		0
  msg:		empty
Packet: version 2; role rmc
 opcode:	request-position
  id:		1
EOF

#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004 University of Utah and the Flux Group.
# All rights reserved.
#

TMPSUDOERS=/tmp/sudoers
ME=`whoami`

rm -f $TMPSUDOERS

cat > $TMPSUDOERS <<EOF
# sudoers file.
#
# This file MUST be edited with the 'visudo' command as root.
#
# See the sudoers man page for the details on how to write a sudoers file.
#

# Host alias specification

# User alias specification

# Cmnd alias specification

# Defaults specification

# User privilege specification
root    ALL=(ALL) ALL
$ME     ALL=(ALL) NOPASSWD: ALL
%root   ALL=(ALL) NOPASSWD: ALL
EOF

su -c "install -c -m 440 $TMPSUDOERS /etc/sudoers"

exit $?

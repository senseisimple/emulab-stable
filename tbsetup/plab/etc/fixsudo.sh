#!/bin/sh

{
cat >/tmp/edscript <<EOF
/^emulab_/s/ALL[ \t]*$/NOPASSWD: ALL/
w
q
EOF
su -c "ed /etc/sudoers < /tmp/edscript"
} > /dev/null 2>&1

exit $?

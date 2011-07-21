#!/bin/sh
#
# Called from /sbin/init to do the prepare and reboot.
#

. /etc/emulab/paths.sh

HOME=/
PATH=/sbin:/bin:/usr/sbin:/usr/bin
export HOME PATH

if ! mount -u -o rw /; then
	echo 'Mounting root filesystem rw failed, prepare aborting'
        exit 1
fi
/bin/rm -f /bootcmd

umount -a >/dev/null 2>&1
mount -a -t nonfs

case $? in
0)
        ;;
*)
        echo 'Mounting /etc/fstab filesystems failed, startup aborted'
        exit 1
        ;;
esac

$BINDIR/prepare 2>&1 | tee /prepare.log
$BINDIR/logboot /prepare.log
rm -f /prepare.log
echo "Rebooting for real ..."
sync
/sbin/reboot

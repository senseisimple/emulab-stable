#! /bin/sh

target_dir="$1"

if [ -z "$target_dir" ] || ! [ -d "$target_dir" ]; then
	echo "${0##*/}: invalid target directory \"$target_dir\""
	exit 1
fi

chmod 1777 "$target_dir/tmp"
chmod 1777 "$target_dir/var/tmp"
chmod 1777 "$target_dir/var/lock"
chmod 0700 "$target_dir/root/.ssh"
chmod 0600 "$target_dir/root/.ssh/authorized_keys"
chmod 0600 "$target_dir/etc/shadow"
chmod u+s "$target_dir/bin/busybox"
chmod u+s "$target_dir/usr/bin/sudo"
chmod 0440 "$target_dir/etc/sudoers"

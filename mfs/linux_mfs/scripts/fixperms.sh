#! /bin/sh

target_dir="$1"

if [ -z "$target_dir" ] || ! [ -d "$target_dir" ]; then
	echo "${0##*/}: invalid target directory \"$target_dir\""
	exit 1
fi

chmod 1777 "$target_dir/tmp"
chmod 1777 "$target_dir/var/tmp"

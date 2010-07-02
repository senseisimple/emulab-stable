#! /bin/sh

target_dir="$1"

if [ -z "$target_dir" ] || ! [ -d "$target_dir" ]; then
	echo "${0##*/}: invalid target directory \"$target_dir\""
	exit 1
fi

standard_dirs="dev tmp proc sys mnt home root etc dev/pts dev/shm boot
		var var/lock var/run var/log var/tmp"

for dir in $standard_dirs; do
	mkdir -p "$target_dir/$dir"
done
